#include "Network/LDCommandProcessor.h"

#include "Data/LDGameData.h"
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
	return SubmitAtTime(Context, Command, FPlatformTime::Seconds());
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

FLDCommandResult ULDCommandProcessor::SubmitAtTime(const FLDParticipantContext& Context, const FLDCommand& Command,
                                                   double NowSeconds)
{
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
	if (Command.RequestId <= Session->HighestAdmittedRequestId)
	{
		Result.ResultCode = ELDCommandResultCode::RequestExpired;
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
		// G0 Stub: Board and Economy do not exist yet. Never synthesize a success.
		Result.ResultCode = ELDCommandResultCode::FeatureDisabled;
	}
	CacheResult(*Session, Normalized, Result);
	return Result;
}

int32 ULDCommandProcessor::GetCachedResultCount(int32 PlayerIndex) const
{
	const FSession* Session = Sessions.Find(PlayerIndex);
	return Session ? Session->Results.Num() : 0;
}
