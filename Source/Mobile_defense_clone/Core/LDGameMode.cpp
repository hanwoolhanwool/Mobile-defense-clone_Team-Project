#include "Core/LDGameMode.h"

#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Data/LDGameData.h"
#include "Network/LDCommandProcessor.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDMatchB, Log, All);

ALDGameMode::ALDGameMode()
{
	GameStateClass = ALDGameState::StaticClass();
	PlayerStateClass = ALDPlayerState::StaticClass();
	PlayerControllerClass = ALDPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void ALDGameMode::InitGameState()
{
	Super::InitGameState();
	ALDGameState* State = GetGameState<ALDGameState>();
	if (!State)
	{
		UE_LOG(LogLDMatchB, Error, TEXT("G0 requires ALDGameState"));
		return;
	}
	FLDMatchContext Context;
	Context.MatchId = FGuid::NewGuid();
	Context.RulesVersion = TEXT("0.3.0");
	State->InitializeMatch(Context);
	GameData = NewObject<ULDGameData>(this);
	FString Error;
	if (!GameData->LoadP0(Error))
	{
		State->SetPhase(ELDMatchPhase::Aborted);
		UE_LOG(LogLDMatchB, Error, TEXT("Match %s aborted: %s"), *Context.MatchId.ToString(), *Error);
		return;
	}
	CommandProcessor = NewObject<ULDCommandProcessor>(this);
	if (!CommandProcessor->Initialize(Context, GameData->GetRules()))
	{
		State->SetPhase(ELDMatchPhase::Aborted);
		UE_LOG(LogLDMatchB, Error, TEXT("Command contract initialization failed"));
		return;
	}
	State->SetPhase(ELDMatchPhase::Preparing);
	UE_LOG(LogLDMatchB, Display, TEXT("G0 B ready: 16 units, 10 waves; board/economy Stub keeps admission closed"));
}

void ALDGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	ALDPlayerController* Controller = Cast<ALDPlayerController>(NewPlayer);
	ALDGameState* State = GetGameState<ALDGameState>();
	ALDPlayerState* Player = Controller ? Controller->GetPlayerState<ALDPlayerState>() : nullptr;
	if (!CommandProcessor || !State || !Controller || !Player)
	{
		return;
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		if (Participants[Index].Get() == Controller)
		{
			return;
		}
		if (!Participants[Index].IsValid())
		{
			FLDParticipantContext Context;
			Context.MatchId = State->GetMatchContext().MatchId;
			Context.PlayerIndex = Index;
			Context.ConnectionEpoch = NextConnectionEpoch++;
			if (Player->InitializeParticipant(Context) && CommandProcessor->RegisterParticipant(Context))
			{
				Participants[Index] = Controller;
				Controller->InitializeServerSession(Context, *CommandProcessor);
			}
			return;
		}
	}
	UE_LOG(LogLDMatchB, Warning, TEXT("Participant admission refused: P0 has exactly two slots"));
}

void ALDGameMode::Logout(AController* Exiting)
{
	for (TWeakObjectPtr<ALDPlayerController>& Participant : Participants)
	{
		if (Participant.Get() == Exiting)
		{
			Participant->ShutdownServerSession();
			Participant.Reset();
		}
	}
	Super::Logout(Exiting);
}

void ALDGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CommandProcessor)
	{
		CommandProcessor->Close();
	}
	for (TWeakObjectPtr<ALDPlayerController>& Participant : Participants)
	{
		if (Participant.IsValid())
		{
			Participant->ShutdownServerSession();
		}
		Participant.Reset();
	}
	Super::EndPlay(EndPlayReason);
}
