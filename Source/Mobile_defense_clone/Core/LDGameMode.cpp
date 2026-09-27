#include "Core/LDGameMode.h"

#include "Battle/LDCombatService.h"
#include "Battle/LDUnitActor.h"
#include "Battle/LDWaveDirector.h"
#include "Kismet/GameplayStatics.h"
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
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(LDP0);

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

void ALDGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	LoadingStartSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0;
	TravelSeed.Reset();
#if !UE_BUILD_SHIPPING
	const FString SeedText = UGameplayStatics::ParseOption(Options, TEXT("P0Seed"));
	int32 ParsedSeed = 0;
	if (!SeedText.IsEmpty() && LexTryParseString(ParsedSeed, *SeedText))
	{
		TravelSeed = ParsedSeed;
	}
#endif
	Super::InitGame(MapName, Options, ErrorMessage);
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
	if (!State->InitializeMatch(Context))
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
#if !UE_BUILD_SHIPPING
	FParse::Value(FCommandLine::Get(), TEXT("P0Seed="), Seed);
	if (TravelSeed.IsSet())
	{
		Seed = TravelSeed.GetValue();
	}
#endif
#if !UE_BUILD_SHIPPING
	FString Probe;
	FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe);
	bG1Probe = Probe.Equals(TEXT("G1"), ESearchCase::IgnoreCase);
	bG2Probe =
	    Probe.Equals(TEXT("G2"), ESearchCase::IgnoreCase) || Probe.Equals(TEXT("G3Load"), ESearchCase::IgnoreCase);
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
	CommandProcessor->AfterExternalCommandClock.BindUObject(this, &ALDGameMode::FinalizePendingTerminal);
	FLDBattleSnapshot Snapshot = State->GetBattleSnapshot();
	Snapshot.FinalWave = GameData->GetRules().FinalWave;
	Snapshot.MaxEnemyCount = GameData->GetRules().ActiveEnemyThreshold;
	Snapshot.LoadingDeadlineServerSeconds = LoadingStartSeconds + GameData->GetRules().LoadingTimeoutSeconds;
	State->UpdateBattle(Snapshot);
	if (!bG1Probe && !bG2Probe)
	{
		WaveDirector = NewObject<ULDWaveDirector>(this);
		if (!WaveDirector->Initialize(*GameData, *State, *CombatService))
		{
			AbortMatch(TEXT("Wave director initialization failed"));
			return;
		}
		WaveDirector->OnTerminalRequested.AddUObject(this, &ALDGameMode::RequestTerminal);
	}
	else
	{
		State->SetPhase(ELDMatchPhase::Preparing);
	}
	bServicesReady = true;
	UE_LOG(LogLDMatch, Display,
	       TEXT("P0 match %s rules=%s seed=%d units=%d; G1Probe=%d"), *Context.MatchId.ToString(),
	            *Context.RulesVersion.ToString(), Seed, GameData->GetUnits().Num(), bG1Probe);
	RefreshReadiness();
}

void ALDGameMode::BeginPlay()
{
	Super::BeginPlay();
	bPlayStarted = true;
	if (GameData && !bEnding)
	{
		GetWorldTimerManager().SetTimer(LogicTimer, this, &ALDGameMode::AdvanceLogic,
		                                1.0f / GameData->GetRules().LogicHz, true);
	}
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
	    (GetGameState<ALDGameState>()->GetPhase() == ELDMatchPhase::Running ||
	     GetGameState<ALDGameState>()->GetPhase() == ELDMatchPhase::Preparing))
	{
		RequestTerminal(ELDMatchResult::Aborted, ELDResultReason::ParticipantDisconnected,
		                GetWorld()->GetTimeSeconds());
		FinalizePendingTerminal();
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
	return HasAuthority() && !bEnding && PendingResult == ELDMatchResult::None && !bG1Probe && bServicesReady &&
	       State && (State->GetPhase() == ELDMatchPhase::Preparing || State->GetPhase() == ELDMatchPhase::Running);
}

void ALDGameMode::RefreshReadiness()
{
	RegisterPendingParticipants();
	ALDGameState* State = GetGameState<ALDGameState>();
	if (bEnding || PendingResult != ELDMatchResult::None || !State || State->GetBattleSnapshot().IsTerminal())
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
	const double Now = GetWorld()->GetTimeSeconds();
	if (!bG2Probe && State->GetPhase() == ELDMatchPhase::Loading &&
	    Now > State->GetBattleSnapshot().LoadingDeadlineServerSeconds)
	{
		RequestTerminal(ELDMatchResult::Aborted, ELDResultReason::LoadingTimeout,
		                State->GetBattleSnapshot().LoadingDeadlineServerSeconds);
		FinalizePendingTerminal();
		return;
	}
	if (ConnectedCount != 2 || !bServicesReady || !bPlayStarted)
	{
		State->SetReadinessReason(
		    FString::Printf(TEXT("Loading: %d/2 participants; services=%d"), ConnectedCount, bServicesReady));
		return;
	}
	if (bG2Probe && State->GetPhase() == ELDMatchPhase::Preparing)
	{
		State->SetPhase(ELDMatchPhase::Running);
		State->SetReadinessReason(TEXT("G2 explicit combat fixture; normal waves disabled"));
		LogicOriginSeconds = Now;
		LogicStep = 0;
		CommandProcessor->SetAcceptingCommands(true);
	}
	else if (State->GetPhase() == ELDMatchPhase::Loading)
	{
		FLDBattleSnapshot Snapshot = State->GetBattleSnapshot();
		Snapshot.PreparationEndServerSeconds = Now + GameData->GetRules().PreparationSeconds;
		State->UpdateBattle(Snapshot);
		State->SetPhase(ELDMatchPhase::Preparing);
		State->SetReadinessReason(TEXT("Preparing: 2/2 participants; P0 board commands enabled"));
		CommandProcessor->SetAcceptingCommands(true);
	}
}
void ALDGameMode::AbortMatch(const FString& Reason)
{
	if (!HasAuthority() || bEnding || PendingResult != ELDMatchResult::None)
	{
		return;
	}
	if (ALDGameState* State = GetGameState<ALDGameState>())
	{
		State->SetReadinessReason(Reason);
	}
	RequestTerminal(ELDMatchResult::Aborted, ELDResultReason::InitializationFailure, GetWorld()->GetTimeSeconds());
	if (!bAdvancingTimeline)
	{
		FinalizePendingTerminal();
	}
	UE_LOG(LogLDMatch, Error, TEXT("Match aborted: %s"), *Reason);
}

void ALDGameMode::RequestTerminal(ELDMatchResult Result, ELDResultReason Reason, double ServerSeconds)
{
	if (bEnding || PendingResult != ELDMatchResult::None)
	{
		return;
	}
	PendingResult = Result;
	PendingReason = Reason;
	PendingResultSeconds = ServerSeconds;
	if (CommandProcessor)
	{
		CommandProcessor->SetAcceptingCommands(false);
	}
	// A terminal request can arrive inside a damage observer. Interrupt the active attack loop now,
	// while keeping the Processor alive to drain the already committed death before publishing Result.
	if (CombatService)
	{
		CombatService->Stop();
	}
}

void ALDGameMode::FinalizePendingTerminal()
{
	if (bEnding || PendingResult == ELDMatchResult::None)
	{
		return;
	}
	// Called after Processor releases its command-clock guard and drains accepted deaths.
	if (CommandProcessor)
	{
		CommandProcessor->DrainCombatRewards();
	}
	// Combat has stopped, but an observer may have requested Abort immediately after a nonlethal hit.
	// Publish that last committed actor HP before Result without advancing combat or evaluating victory.
	if (WaveDirector)
	{
		WaveDirector->RefreshCombatView();
	}
	if (ALDGameState* State = GetGameState<ALDGameState>())
	{
		State->FinalizeResult(PendingResult, PendingReason, PendingResultSeconds);
	}
	UE_LOG(LogLDMatch, Display,
	       TEXT("P0 terminal result=%d reason=%d time=%.6f"), static_cast<int32>(PendingResult),
	            static_cast<int32>(PendingReason), PendingResultSeconds);
	StopMatchServices();
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
	if (WaveDirector)
	{
		WaveDirector->Stop();
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
	if (bEnding || bG1Probe || !bServicesReady)
	{
		return;
	}
	AdvanceTimelineBefore(GetWorld()->GetTimeSeconds());
	CommandProcessor->DrainCombatRewards();
	FinalizePendingTerminal();
}

void ALDGameMode::AdvanceBeforeExternalCommand(double ServerSeconds)
{
	if (CanAcceptCommands())
	{
		AdvanceTimelineBefore(ServerSeconds);
	}
}

void ALDGameMode::AdvanceTimelineBefore(double ServerSeconds)
{
	CSV_SCOPED_TIMING_STAT(LDP0, Timeline);
	if (bEnding || bAdvancingTimeline || !FMath::IsFinite(ServerSeconds) || ServerSeconds < 0 ||
	    PendingResult != ELDMatchResult::None)
	{
		return;
	}
	TGuardValue<bool> Advancing(bAdvancingTimeline, true);
	ALDGameState* State = GetGameState<ALDGameState>();
	if (!State)
	{
		return;
	}
	if (State->GetPhase() == ELDMatchPhase::Loading)
	{
		if (State->GetBattleSnapshot().LoadingDeadlineServerSeconds < ServerSeconds)
		{
			RequestTerminal(ELDMatchResult::Aborted, ELDResultReason::LoadingTimeout,
			                State->GetBattleSnapshot().LoadingDeadlineServerSeconds);
		}
		return;
	}
	if (State->GetPhase() == ELDMatchPhase::Preparing)
	{
		const double StartSeconds = State->GetBattleSnapshot().PreparationEndServerSeconds;
		if (StartSeconds >= ServerSeconds)
		{
			return;
		}
		State->SetPhase(ELDMatchPhase::Running);
		State->SetReadinessReason(TEXT("Running: two participants; 10 P0 waves"));
		if (bEnding || PendingResult != ELDMatchResult::None)
		{
			return;
		}
		LogicOriginSeconds = StartSeconds;
		LogicStep = 0;
		if (!WaveDirector || !WaveDirector->StartAt(StartSeconds))
		{
			if (PendingResult == ELDMatchResult::None)
			{
				RequestTerminal(ELDMatchResult::Aborted, ELDResultReason::InitializationFailure, StartSeconds);
			}
			return;
		}
	}
	if (State->GetPhase() != ELDMatchPhase::Running)
	{
		return;
	}
	const double Interval = 1.0 / GameData->GetRules().LogicHz;
	while (!bEnding && PendingResult == ELDMatchResult::None)
	{
		const double NextStep = LogicOriginSeconds + (LogicStep + 1) * Interval;
		const double NextEvent = WaveDirector ? WaveDirector->GetNextEventSeconds() : NextStep;
		const double At = FMath::Min(NextStep, NextEvent);
		if (At >= ServerSeconds)
		{
			break;
		}
		// The current world time remains open for commands. Older exact event times close in stage order.
		CombatService->AdvanceCombatTo(At);
		CommandProcessor->DrainCombatRewards();
		if (bEnding || PendingResult != ELDMatchResult::None)
		{
			break;
		}
		if (WaveDirector)
		{
			WaveDirector->RefreshCombatView();
			if (bEnding || PendingResult != ELDMatchResult::None)
			{
				break;
			}
			WaveDirector->ProcessEventsAt(At);
		}
		if (At == NextStep)
		{
			++LogicStep;
		}
	}
	if (PendingResult == ELDMatchResult::None)
	{
		// Close intervening due hits, but not ServerSeconds itself; all prior deadlines/spawns are now closed.
		CombatService->AdvanceCombatBefore(ServerSeconds);
		if (WaveDirector)
		{
			WaveDirector->RefreshCombatView();
			WaveDirector->EvaluateVictory(ServerSeconds);
		}
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
	if (!bEnding && PendingResult == ELDMatchResult::None && CommandProcessor &&
	    (!WaveDirector || WaveDirector->HandleEnemyDeath(Death)))
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

ULDWaveDirector* ALDGameMode::GetWaveDirector() const
{
	return HasAuthority() ? WaveDirector.Get() : nullptr;
}

bool ALDGameMode::IsLogicTimerActive() const
{
	return GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(LogicTimer);
}
