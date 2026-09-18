#include "Core/LDGameMode.h"

#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Data/LDGameData.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "Network/LDCommandProcessor.h"
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
	UE_LOG(LogLDMatch, Display,
	       TEXT("G0 match %s rules=%s units=%d waves=%d; role services are explicit Stub"), *Context.MatchId.ToString(),
	            *Context.RulesVersion.ToString(), GameData->GetUnits().Num(), GameData->GetWaves().Num());
	RefreshReadiness();
}

void ALDGameMode::BeginPlay()
{
	Super::BeginPlay();
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
	UE_LOG(LogLDMatch, Display,
	       TEXT("Participant index=%d epoch=%llu registered; board/economy Stub keeps admission closed"), PlayerIndex,
	            Context.ConnectionEpoch);
	return true;
}

void ALDGameMode::Logout(AController* Exiting)
{
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
			Participant.Reset();
		}
	}
	Super::Logout(Exiting);
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
	// G0 deliberately has no successful service substitute. G1/G2 integration replaces this boundary.
	return false;
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
	State->SetReadinessReason(FString::Printf(
	    TEXT("Preparing: %d/2 participants; Stub: Board/Economy/Route services not connected"), ConnectedCount));
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
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (CommandProcessor)
	{
		CommandProcessor->Close();
	}
	// Terminal phase is not a disconnected session: keep Controller -> Processor and finalized result history.
	// G0 has no combat subscription or spawn reservation yet.
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
