#include "Core/LDGameMode.h"

#include "Core/LDGameState.h"
#include "Core/LDPlayerState.h"
#include "Data/LDGameData.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDMatch, Log, All);

ALDGameMode::ALDGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	GameStateClass = ALDGameState::StaticClass();
	PlayerStateClass = ALDPlayerState::StaticClass();
	DefaultPawnClass = nullptr;
	Participants.SetNum(2);
}

void ALDGameMode::InitGameState()
{
	Super::InitGameState();
	ALDGameState* State = GetGameState<ALDGameState>();
	if (!State || GameData)
	{
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
	Super::PostLogin(NewPlayer);
	ALDGameState* State = GetGameState<ALDGameState>();
	ALDPlayerState* Player = NewPlayer ? NewPlayer->GetPlayerState<ALDPlayerState>() : nullptr;
	if (!State || !Player || bEnding || State->GetPhase() == ELDMatchPhase::Aborted)
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
			GameSession->KickPlayer(NewPlayer, NSLOCTEXT("LD", "P0SessionFull", "P0 requires exactly two players."));
		}
		return;
	}
	FLDParticipantContext Context;
	Context.MatchId = State->GetMatchContext().MatchId;
	Context.PlayerIndex = PlayerIndex;
	Context.ConnectionEpoch = NextConnectionEpoch++;
	if (!Player->InitializeParticipant(Context))
	{
		AbortMatch(TEXT("PlayerState refused server participant context"));
		return;
	}
	Participants[PlayerIndex] = NewPlayer;
	UE_LOG(LogLDMatch, Display,
	       TEXT("Participant index=%d epoch=%llu registered; B session Stub remains closed"), PlayerIndex,
	            Context.ConnectionEpoch);
	RefreshReadiness();
}

void ALDGameMode::Logout(AController* Exiting)
{
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
	State->SetReadinessReason(FString::Printf(TEXT("Preparing: %d/2 participants; Stub: Board/Economy/Command/Route services not connected"), ConnectedCount));
}

void ALDGameMode::AbortMatch(const FString& Reason)
{
	if (ALDGameState* State = GetGameState<ALDGameState>())
	{
		if (State->GetPhase() == ELDMatchPhase::Aborted || State->GetPhase() == ELDMatchPhase::Result)
		{
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
	Participants.Reset();
	// No B service, combat subscription or spawn reservation exists at this G0 boundary.
}
