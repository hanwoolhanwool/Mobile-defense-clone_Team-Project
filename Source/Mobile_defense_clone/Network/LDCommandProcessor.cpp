#include "Network/LDCommandProcessor.h"

#include "Data/LDGameData.h"
#include "Board/LDBoardManager.h"
#include "Economy/LDEconomyService.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

bool ULDCommandProcessor::Initialize(const FLDMatchContext& Context, const FLDGameRules& Rules)
{
	if (bInitialized || !Context.IsValid() || Rules.CommandRatePerSecond != 8 || Rules.CommandBurst != 12 ||
	    Rules.RequestResultCacheCount != 256)
	{
		return false;
	}
	MatchContext = Context;
	RatePerSecond = Rules.CommandRatePerSecond;
	Burst = Rules.CommandBurst;
	CacheCapacity = Rules.RequestResultCacheCount;
	bInitialized = true;
	return true;
}

bool ULDCommandProcessor::RegisterParticipant(const FLDParticipantContext& Context)
{
	if (!bInitialized || bClosed || !Context.IsValid() || Context.MatchId != MatchContext.MatchId)
	{
		return false;
	}
	if (const FSession* Existing = Sessions.Find(Context.PlayerIndex))
	{
		// Same epoch registration must never erase its execution history.
		if (Context.ConnectionEpoch <= Existing->Context.ConnectionEpoch)
		{
			return Context.ConnectionEpoch == Existing->Context.ConnectionEpoch;
		}
	}
	FSession Session;
	Session.Context = Context;
	Session.Tokens = Burst;
	Session.ResponseTokens = Burst;
	Sessions.Add(Context.PlayerIndex, MoveTemp(Session));
	return true;
}

void ULDCommandProcessor::SetAcceptingCommands(bool bAccept)
{
	bAcceptingCommands = bInitialized && !bClosed && bAccept;
}

void ULDCommandProcessor::Close()
{
	// Retain deduplication history through Result/EndPlay. New admissions cannot execute.
	bAcceptingCommands = false;
	bClosed = true;
	BeforeExternalCommand.Unbind();
	RewardQueue.Reset();
	QueuedDeathIds.Reset();
	if (BoardManager)
	{
		BoardManager->Close();
		EconomyService->Close();
	}
}

ULDCommandProcessor::FSession* ULDCommandProcessor::FindSession(const FLDParticipantContext& Context)
{
	FSession* Session = Sessions.Find(Context.PlayerIndex);
	return Session && Context.IsValid() && Context.MatchId == MatchContext.MatchId &&
	               Session->Context.ConnectionEpoch == Context.ConnectionEpoch
	           ? Session
	           : nullptr;
}

bool ULDCommandProcessor::ConsumeToken(double& Tokens, double& LastSeconds, double NowSeconds) const
{
	if (!FMath::IsFinite(NowSeconds) || NowSeconds < LastSeconds)
	{
		return false;
	}
	Tokens = FMath::Min(Burst, Tokens + (NowSeconds - LastSeconds) * RatePerSecond);
	LastSeconds = NowSeconds;
	if (Tokens < 1.0)
	{
		return false;
	}
	Tokens -= 1.0;
	return true;
}

bool ULDCommandProcessor::CanSendResponse(const FLDParticipantContext& Context, double NowSeconds)
{
	FSession* Session = FindSession(Context);
	return Session && ConsumeToken(Session->ResponseTokens, Session->LastResponseSeconds, NowSeconds);
}

FLDCommandResult ULDCommandProcessor::Submit(const FLDParticipantContext& Context, const FLDCommand& Command)
{
	const UWorld* World = GetOuter() ? GetOuter()->GetWorld() : nullptr;
	return SubmitAtTime(Context, Command, World ? World->GetTimeSeconds() : FPlatformTime::Seconds());
}

void ULDCommandProcessor::CacheResult(FSession& Session, const FLDCommand& Command, const FLDCommandResult& Result)
{
	if (Session.Order.Num() == CacheCapacity)
	{
		Session.Results.Remove(Session.Order[0]);
		Session.Order.RemoveAt(0);
	}
	Session.Order.Add(Command.RequestId);
	Session.Results.Add(Command.RequestId, {Command, Result});
}

FLDCommandResult ULDCommandProcessor::SubmitAtTime(const FLDParticipantContext& IncomingContext,
                                                   const FLDCommand& IncomingCommand, double NowSeconds)
{
	const FLDParticipantContext Context = IncomingContext;
	const FLDCommand Command = IncomingCommand;
	FLDCommandResult Result;
	Result.MatchId = MatchContext.MatchId;
	Result.ConnectionEpoch = Command.ConnectionEpoch;
	Result.RequestId = Command.RequestId;
	FSession* Session = FindSession(Context);
	if (!Session || Command.ConnectionEpoch != Context.ConnectionEpoch)
	{
		Result.ResultCode = ELDCommandResultCode::InvalidEpoch;
		return Result;
	}
	if (!Command.IsValidPayload())
	{
		return Result;
	}
	const FLDCommand Normalized = Command.Normalized();
	if (const FCachedCommand* Cached = Session->Results.Find(Command.RequestId))
	{
		if (Cached->Command.HasSameContent(Normalized))
		{
			return Cached->Result;
		}
		Result.ResultCode = ELDCommandResultCode::RequestIdConflict;
		return Result;
	}
	if (bProcessing)
	{
		const bool bSameKey = ExecutingPlayer == Context.PlayerIndex && ExecutingCommand.IsSet() &&
		                      ExecutingCommand->ConnectionEpoch == Command.ConnectionEpoch &&
		                      ExecutingCommand->RequestId == Command.RequestId;
		Result.ResultCode =
		    bSameKey ? (ExecutingCommand->HasSameContent(Normalized) ? ELDCommandResultCode::Pending
		                                                             : ELDCommandResultCode::RequestIdConflict)
		             : ELDCommandResultCode::Busy;
		return Result;
	}
	if (Command.RequestId <= Session->HighestAdmittedRequestId)
	{
		Result.ResultCode = ELDCommandResultCode::RequestExpired;
		return Result;
	}
	if (bAcceptingCommands && FMath::IsFinite(NowSeconds))
	{
		TGuardValue<bool> Processing(bProcessing, true);
		BeforeExternalCommand.ExecuteIfBound(NowSeconds);
	}
	// The clock hook queues earlier deaths while guarded; drain them before this command reads either source.
	DrainCombatRewards();
	// Reward publication may replace or rehash Sessions. Never keep a pointer across an external callback.
	Session = FindSession(Context);
	if (!Session)
	{
		Result.ResultCode = ELDCommandResultCode::InvalidEpoch;
		return Result;
	}
	Session->HighestAdmittedRequestId = Command.RequestId;
	if (!ConsumeToken(Session->Tokens, Session->LastSeconds, NowSeconds))
	{
		Result.ResultCode = ELDCommandResultCode::RateLimited;
	}
	else if (!bAcceptingCommands)
	{
		Result.ResultCode = ELDCommandResultCode::PhaseNotAllowed;
	}
	else
	{
		TGuardValue<bool> Processing(bProcessing, true);
		ExecutingPlayer = Context.PlayerIndex;
		ExecutingCommand = Normalized;
		ExecuteCommand(Context, Normalized, NowSeconds, Result);
		ExecutingCommand.Reset();
		ExecutingPlayer = INDEX_NONE;
	}
	Session = FindSession(Context);
	if (Session && !Session->Results.Contains(Command.RequestId))
	{
		CacheResult(*Session, Normalized, Result);
	}
	return Result;
}

bool ULDCommandProcessor::BindServices(ULDBoardManager& Board, ULDEconomyService& Economy)
{
	if (!bInitialized || bClosed || BoardManager || Board.GetSnapshot(0).MatchId != MatchContext.MatchId ||
	    Economy.GetSnapshot(0).MatchId != MatchContext.MatchId)
	{
		return false;
	}
	BoardManager = &Board;
	EconomyService = &Economy;
	return true;
}

void ULDCommandProcessor::ExecuteCommand(const FLDParticipantContext& Context, const FLDCommand& Command,
                                         double ServerSeconds, FLDCommandResult& Result)
{
	if (!BoardManager || !EconomyService)
	{
		// Explicit G0/isolated-role fixture boundary before gameplay services are bound.
		Result.ResultCode = ELDCommandResultCode::FeatureDisabled;
		return;
	}
	Result.NewBoardRevision = BoardManager->GetSnapshot(Context.PlayerIndex).BoardRevision;
	Result.EconomyRevision = EconomyService->GetSnapshot(Context.PlayerIndex).EconomyRevision;
	Result.ResultCode = BoardManager->ValidateCommand(Context, Command, ServerSeconds);
	if (Result.ResultCode != ELDCommandResultCode::Success)
	{
		return;
	}
	FLDPlacedUnit Source;
	BoardManager->TryGetUnit(Command.InstanceId, Source);
	FLDEconomyPlan EconomyPlan;
	Result.ResultCode = EconomyService->TryPrepare(Context.PlayerIndex, Command, Source.UnitId, EconomyPlan);
	if (Result.ResultCode != ELDCommandResultCode::Success)
	{
		return;
	}
	FLDBoardPlan BoardPlan;
	Result.ResultCode = BoardManager->TryPrepare(Context, Command, EconomyPlan.ResultUnitId, ServerSeconds, BoardPlan);
	if (Result.ResultCode != ELDCommandResultCode::Success || !BoardManager->ValidatePrepared(BoardPlan) ||
	    !EconomyService->ValidatePrepared(EconomyPlan) || !FindSession(Context) || bClosed || !bAcceptingCommands)
	{
		if (Result.ResultCode == ELDCommandResultCode::Success)
		{
			Result.ResultCode =
			    !FindSession(Context) ? ELDCommandResultCode::InvalidEpoch : ELDCommandResultCode::StaleBoard;
		}
		BoardManager->CancelPrepared(BoardPlan);
		return;
	}
	// Both plans are valid. No fallible external work or callbacks may occur until both sources are committed.
	BoardManager->CommitPrepared(BoardPlan);
	EconomyService->CommitPrepared(EconomyPlan);
	Result.NewBoardRevision = BoardPlan.After.BoardRevision;
	Result.EconomyRevision = EconomyPlan.After.EconomyRevision;
	Result.EventId = NextEventId++;
	Result.RemovedInstanceIds = BoardPlan.Commit.RemovedInstanceIds;
	for (const FLDPlacedUnit& Unit : BoardPlan.Commit.AddedOrUpdatedUnits)
	{
		if (Unit.InstanceId >= BoardPlan.ExpectedNextInstanceId)
		{
			Result.CreatedInstanceIds.Add(Unit.InstanceId);
		}
		else
		{
			Result.MovedInstanceIds.Add(Unit.InstanceId);
		}
	}
	// Store the old session's terminal outcome before publication can log it out or replace its epoch.
	if (FSession* CommittedSession = FindSession(Context))
	{
		CacheResult(*CommittedSession, Command, Result);
	}
	BoardManager->PublishPrepared(BoardPlan);
	EconomyService->PublishPrepared(EconomyPlan);
}

void ULDCommandProcessor::EnqueueCombatReward(const FLDCombatDeath& Death)
{
	if (!bClosed && bInitialized && Death.MatchId == MatchContext.MatchId && Death.DeathEventId != 0 &&
	    !QueuedDeathIds.Contains(Death.DeathEventId))
	{
		QueuedDeathIds.Add(Death.DeathEventId);
		RewardQueue.Add(Death);
	}
}

void ULDCommandProcessor::DrainCombatRewards()
{
	if (bProcessing || bClosed || !EconomyService || RewardQueue.IsEmpty())
	{
		return;
	}
	TGuardValue<bool> Processing(bProcessing, true);
	TArray<FLDCombatDeath> Batch = MoveTemp(RewardQueue);
	RewardQueue.Reset();
	QueuedDeathIds.Reset();
	for (const FLDCombatDeath& Death : Batch)
	{
		EconomyService->ApplyCombatReward(Death);
	}
}

int32 ULDCommandProcessor::GetCachedResultCount(int32 PlayerIndex) const
{
	const FSession* Session = Sessions.Find(PlayerIndex);
	return Session ? Session->Results.Num() : 0;
}
