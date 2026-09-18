#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LDGameMode.generated.h"

class ALDPlayerController;
class ULDGameData;
class ULDCommandProcessor;

UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALDGameMode();
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TObjectPtr<ULDGameData> GameData;
	UPROPERTY()
	TObjectPtr<ULDCommandProcessor> CommandProcessor;
	TWeakObjectPtr<ALDPlayerController> Participants[2];
	uint64 NextConnectionEpoch = 1;
};
