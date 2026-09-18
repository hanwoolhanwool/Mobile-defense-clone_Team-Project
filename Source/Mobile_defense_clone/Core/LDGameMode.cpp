#include "Core/LDGameMode.h"

#include "Battle/LDCombatService.h"
#include "Battle/LDUnitActor.h"
#include "Board/LDBoardManager.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Data/LDGameData.h"
#include "Economy/LDEconomyService.h"
#include "Engine/World.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "Network/LDCommandProcessor.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDMatch, Log, All);

ALDGameMode::ALDGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	GameStateClass = ALDGameState::StaticClass();
	PlayerStateClass = ALDPlayerState::StaticClass();
	PlayerControllerClass = ALDPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	Participants.SetNum(2);
}

void ALDGameMode::InitGameState()
{
	if (!HasAuthority() || bEnding)
	{
		return;
	}
	Super::InitGameState();
	ALDGameState* State = GetGameState<ALDGameState>();
	if (!State)
	{
		return;
	}
	if (GameData)
	{
		RefreshReadiness();
		return;
	}
	GameData = NewObject<ULDGameData>(this);
	FString Error;
	if (!GameData->LoadP0(Error))
	{
		AbortMatch(Error);
		return;
	}
	FLDMatchContext Context;
	Context.MatchId = FGuid::NewGuid();
	Context.RulesVersion = GameData->GetRules().RulesVersion;
	if (!State->InitializeMatch(Context) || !State->SetPhase(ELDMatchPhase::Preparing))
	{
		AbortMatch(TEXT("GameState refused initial match context"));
		return;
	}
	CommandProcessor = NewObject<ULDCommandProcessor>(this);
	if (!CommandProcessor->Initialize(Context, GameData->GetRules()))
	{
		AbortMatch(TEXT("Command processor refused match contract"));
		return;
	}
	BoardManager = NewObject<ULDBoardManager>(this);
	EconomyService = NewObject<ULDEconomyService>(this);
	CombatService = NewObject<ULDCombatService>(this);
	int32 Seed = static_cast<int32>(Context.MatchId.A);
	FParse::Value(FCommandLine::Get(), TEXT("P0Seed="), Seed);
#if !UE_BUILD_SHIPPING
	FString Probe;
	FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe);
	bG1Probe = Probe.Equals(TEXT("G1"), ESearchCase::IgnoreCase);
#endif
	if (!BoardManager->Initialize(*GetWorld(), Context, *GameData) ||
	    !EconomyService->Initialize(Context, *GameData, Seed) ||
	    !CombatService->Initialize(Context, GameData->GetRules()) ||
	    !CommandProcessor->BindServices(*BoardManager, *EconomyService))
	{
		AbortMatch(TEXT("G2 board/economy/combat service initialization failed"));
		return;
	}
	BoardCommitHandle = BoardManager->OnBoardCommitted.AddUObject(this, &ALDGameMode::HandleBoardCommitted);
	EconomyChangedHandle = EconomyService->OnEconomyChanged.AddUObject(this, &ALDGameMode::HandleEconomyChanged);
	EnemyDeathHandle = CombatService->OnEnemyDeathCommitted.AddUObject(this, &ALDGameMode::HandleEnemyDeath);
	CommandProcessor->BeforeExternalCommand.BindUObject(this, &ALDGameMode::AdvanceBeforeExternalCommand);
	bServicesReady = true;
	UE_LOG(LogLDMatch, Display,
	       TEXT("G2 match %s rules=%s seed=%d units=%d; G1Probe=%d"), *Context.MatchId.ToString(),
	            *Context.RulesVersion.ToString(), Seed, GameData->GetUnits().Num(), bG1Probe);
	RefreshReadiness();
}

void ALDGameMode::BeginPlay()
{
	Super::BeginPlay();
	bPlayStarted = true;
	RefreshReadiness();
}

void ALDGameMode::PostLogin(APlayerController* NewPlayer)
{
	ALDPlayerController* Controller = Cast<ALDPlayerController>(NewPlayer);
	if (!HasAuthority() || !Controller || bEnding)
	{
		return;
	}
	for (const TWeakObjectPtr<APlayerController>& Participant : Participants)
	{
		if (Participant.Get() == NewPlayer)
		{
			return;
		}
	}
	if (PendingParticipants.Contains(Controller))
	{
		return;
	}
	Super::PostLogin(NewPlayer);
	// Login and service readiness are independent events. Retain identity until both have happened.
	PendingParticipants.Add(Controller);
	RefreshReadiness();
}

void ALDGameMode::RegisterPendingParticipants()
{
	ALDGameState* State = GetGameState<ALDGameState>();
	if (!HasAuthority() || bEnding || !GameData || !GameData->IsLoaded() || !CommandProcessor || !State ||
	    !State->GetMatchContext().IsValid())
	{
		return;
	}
	// A rejected extra participant can synchronously Logout through GameSession. Iterate a detached snapshot.
	TArray<TWeakObjectPtr<ALDPlayerController>> Waiting = MoveTemp(PendingParticipants);
	PendingParticipants.Reset();
	for (const TWeakObjectPtr<ALDPlayerController>& Pending : Waiting)
	{
		ALDPlayerController* Controller = Pending.Get();
		if (Controller && !RegisterParticipant(*Controller))
		{
			PendingParticipants.AddUnique(Controller);
		}
		if (bEnding)
		{
			return;
		}
	}
}

bool ALDGameMode::RegisterParticipant(ALDPlayerController& Controller)
{
	ALDGameState* State = GetGameState<ALDGameState>();
	ALDPlayerState* Player = Controller.GetPlayerState<ALDPlayerState>();
	if (!State || !Player)
	{
		return false;
	}
	for (const TWeakObjectPtr<APlayerController>& Participant : Participants)
	{
		if (Participant.Get() == &Controller)
		{
			return true;
		}
	}
	int32 PlayerIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Participants.Num(); ++Index)
	{
		if (!Participants[Index].IsValid())
		{
			PlayerIndex = Index;
			break;
		}
	}
	if (PlayerIndex == INDEX_NONE)
	{
		if (GameSession)
		{
			GameSession->KickPlayer(&Controller, NSLOCTEXT("LD", "P0SessionFull", "P0 requires exactly two players."));
		}
		return true;
	}
	FLDParticipantContext Context;
	Context.MatchId = State->GetMatchContext().MatchId;
	Context.PlayerIndex = PlayerIndex;
	Context.ConnectionEpoch = NextConnectionEpoch++;
	if (!Player->InitializeParticipant(Context) || !CommandProcessor->RegisterParticipant(Context))
	{
		AbortMatch(TEXT("PlayerState refused server participant context"));
		return false;
	}
	Participants[PlayerIndex] = &Controller;
	Controller.InitializeServerSession(Context, *CommandProcessor);
	PublishPlayerSnapshots(PlayerIndex);
	UE_LOG(LogLDMatch, Display,
	       TEXT("Participant index=%d epoch=%llu registered"), PlayerIndex, Context.ConnectionEpoch);
	return true;
}

void ALDGameMode::Logout(AController* Exiting)
{
	bool bLostParticipant = false;
	PendingParticipants.RemoveAll([Exiting](const TWeakObjectPtr<ALDPlayerController>& Pending)
	                              { return !Pending.IsValid() || Pending.Get() == Exiting; });
	if (ALDPlayerController* Controller = Cast<ALDPlayerController>(Exiting))
	{
		Controller->ShutdownServerSession();
	}
	for (TWeakObjectPtr<APlayerController>& Participant : Participants)
	{
		if (Participant.Get() == Exiting)
		{
			bLostParticipant = true;
			Participant.Reset();
		}
	}
	Super::Logout(Exiting);
	if (bLostParticipant && !bEnding && GetGameState<ALDGameState>() &&
	    GetGameState<ALDGameState>()->GetPhase() == ELDMatchPhase::Running)
	{
		AbortMatch(TEXT("Participant disconnected during active match"));
	}
	RefreshReadiness();
}

void ALDGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopMatchServices();
	ReleasePlayerSessions();
	Super::EndPlay(EndPlayReason);
}

const ULDGameData* ALDGameMode::GetGameData() const
{
	return GameData;
}

bool ALDGameMode::CanAcceptCommands() const
{
	const ALDGameState* State = GetGameState<ALDGameState>();
	return HasAuthority() && !bEnding && !bG1Probe && bServicesReady && State &&
	       State->GetPhase() == ELDMatchPhase::Running;
}

void ALDGameMode::RefreshReadiness()
{
	RegisterPendingParticipants();
	ALDGameState* State = GetGameState<ALDGameState>();
	if (bEnding || !State || State->GetPhase() != ELDMatchPhase::Preparing)
	{
		return;
	}
	int32 ConnectedCount = 0;
	for (const TWeakObjectPtr<APlayerController>& Participant : Participants)
	{
		ConnectedCount += Participant.IsValid() ? 1 : 0;
	}
	if (bG1Probe)
	{
		State->SetReadinessReason(
		    FString::Printf(TEXT("G1 fixture: %d/2 participants; combat and commands closed"), ConnectedCount));
		return;
	}
	if (ConnectedCount != 2 || !bServicesReady || !bPlayStarted)
	{
		State->SetReadinessReason(
		    FString::Printf(TEXT("Preparing: %d/2 participants; services=%d"), ConnectedCount, bServicesReady));
		return;
	}
	if (!State->SetPhase(ELDMatchPhase::Running))
	{
		AbortMatch(TEXT("GameState refused ready match transition"));
		return;
	}
	State->SetReadinessReason(TEXT("G2 running: board/economy/combat ready; waves are a later gate"));
	LogicOriginSeconds = GetWorld()->GetTimeSeconds();
	LogicStep = 0;
	CommandProcessor->SetAcceptingCommands(true);
	GetWorldTimerManager().SetTimer(LogicTimer, this, &ALDGameMode::AdvanceLogic, 1.0f / GameData->GetRules().LogicHz,
	                                true);
}

void ALDGameMode::AbortMatch(const FString& Reason)
{
	if (!HasAuthority() || bEnding)
	{
		return;
	}
	if (ALDGameState* State = GetGameState<ALDGameState>())
	{
		if (State->GetPhase() == ELDMatchPhase::Aborted || State->GetPhase() == ELDMatchPhase::Result)
		{
			StopMatchServices();
			return;
		}
		State->SetReadinessReason(Reason);
		State->SetPhase(ELDMatchPhase::Aborted);
	}
	StopMatchServices();
	UE_LOG(LogLDMatch, Error, TEXT("Match aborted: %s"), *Reason);
}

void ALDGameMode::StopMatchServices()
{
	if (bEnding)
	{
		return;
	}
	bEnding = true;
	bServicesReady = false;
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (CommandProcessor)
	{
		CommandProcessor->SetAcceptingCommands(false);
		CommandProcessor->DrainCombatRewards();
		CommandProcessor->Close();
	}
	if (CombatService)
	{
		CombatService->OnEnemyDeathCommitted.Remove(EnemyDeathHandle);
		CombatService->Stop();
	}
	if (BoardManager)
	{
		BoardManager->OnBoardCommitted.Remove(BoardCommitHandle);
		BoardManager->Close();
	}
	if (EconomyService)
	{
		EconomyService->OnEconomyChanged.Remove(EconomyChangedHandle);
		EconomyService->Close();
	}
	// Terminal phase is not a disconnected session: keep Controller -> Processor and finalized result history.
}

void ALDGameMode::ReleasePlayerSessions()
{
	for (const TWeakObjectPtr<APlayerController>& Participant : Participants)
	{
		if (ALDPlayerController* Controller = Cast<ALDPlayerController>(Participant.Get()))
		{
			Controller->ShutdownServerSession();
		}
	}
	for (const TWeakObjectPtr<ALDPlayerController>& Pending : PendingParticipants)
	{
		if (Pending.IsValid())
		{
			Pending->ShutdownServerSession();
		}
	}
	Participants.Empty();
	PendingParticipants.Empty();
}

void ALDGameMode::AdvanceLogic()
{
	if (!CanAcceptCommands())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const double StepSeconds = 1.0 / GameData->GetRules().LogicHz;
	// Slate input and next-frame network dispatch can still submit this WorldTime. Close only older times.
	// Integer step index avoids accumulating interval drift and retains missed logical steps.
	while (LogicOriginSeconds + (LogicStep + 1) * StepSeconds < Now && CanAcceptCommands())
	{
		++LogicStep;
		CombatService->AdvanceCombatTo(LogicOriginSeconds + LogicStep * StepSeconds);
		CommandProcessor->DrainCombatRewards();
	}
}

void ALDGameMode::AdvanceBeforeExternalCommand(double ServerSeconds)
{
	if (CanAcceptCommands())
	{
		// The processor drains queued deaths after releasing its reentrancy guard, before reading money/board.
		CombatService->AdvanceCombatBefore(ServerSeconds);
	}
}

void ALDGameMode::HandleBoardCommitted(const FLDBoardCommit& Commit)
{
	const ALDGameState* State = GetGameState<ALDGameState>();
	if (bEnding || !State || !BoardManager || !CombatService || Commit.MatchId != State->GetMatchContext().MatchId ||
	    Commit.PlayerIndex < 0 || Commit.PlayerIndex > 1 ||
	    Commit.BoardRevision <= LastBoardRevisions[Commit.PlayerIndex] ||
	    BoardManager->GetSnapshot(Commit.PlayerIndex).BoardRevision != Commit.BoardRevision)
	{
		return;
	}
	LastBoardRevisions[Commit.PlayerIndex] = Commit.BoardRevision;
	for (uint64 Removed : Commit.RemovedInstanceIds)
	{
		CombatService->UnregisterUnit(Removed);
	}
	for (const FLDPlacedUnit& Added : Commit.AddedOrUpdatedUnits)
	{
		ALDUnitActor* Unit = nullptr;
		if (BoardManager->TryGetCommittedUnitActor(Added.InstanceId, Unit) && Unit &&
		    Unit->GetPlacement().PlayerIndex == Commit.PlayerIndex && Unit->GetPlacement().UnitId == Added.UnitId &&
		    Unit->GetPlacement().CellId == Added.CellId)
		{
			CombatService->RegisterCommittedUnit(*Unit, Commit.CommitServerSeconds);
		}
	}
	PublishPlayerSnapshots(Commit.PlayerIndex);
}

void ALDGameMode::HandleEconomyChanged(const FLDEconomySnapshot& Snapshot)
{
	const ALDGameState* State = GetGameState<ALDGameState>();
	if (State && Snapshot.MatchId == State->GetMatchContext().MatchId)
	{
		PublishPlayerSnapshots(Snapshot.PlayerIndex);
	}
}

void ALDGameMode::HandleEnemyDeath(const FLDCombatDeath& Death)
{
	if (!bEnding && CommandProcessor)
	{
		CommandProcessor->EnqueueCombatReward(Death);
	}
}

void ALDGameMode::PublishPlayerSnapshots(int32 PlayerIndex)
{
	if (!BoardManager || !EconomyService || !Participants.IsValidIndex(PlayerIndex))
	{
		return;
	}
	if (ALDPlayerController* Controller = Cast<ALDPlayerController>(Participants[PlayerIndex].Get()))
	{
		Controller->PublishSnapshots(BoardManager->GetSnapshot(PlayerIndex), EconomyService->GetSnapshot(PlayerIndex));
	}
}

ULDBoardManager* ALDGameMode::GetBoardManager() const
{
	return HasAuthority() ? BoardManager.Get() : nullptr;
}
ULDEconomyService* ALDGameMode::GetEconomyService() const
{
	return HasAuthority() ? EconomyService.Get() : nullptr;
}
ULDCombatService* ALDGameMode::GetCombatService() const
{
	return HasAuthority() ? CombatService.Get() : nullptr;
}
ULDCommandProcessor* ALDGameMode::GetCommandProcessor() const
{
	return HasAuthority() ? CommandProcessor.Get() : nullptr;
}
