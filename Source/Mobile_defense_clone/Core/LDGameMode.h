#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LDGameMode.generated.h"

class ULDGameData;
class ULDCommandProcessor;
class ALDPlayerController;

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
	const ULDGameData* GetGameData() const;
	bool CanAcceptCommands() const;
	// Service failures end admission, while the connected session can still replay finalized results.
	void AbortMatch(const FString& Reason);

protected:
	virtual void BeginPlay() override;

private:
	void RefreshReadiness();
	void RegisterPendingParticipants();
	bool RegisterParticipant(ALDPlayerController& Controller);
	void StopMatchServices();
	void ReleasePlayerSessions();

	UPROPERTY()
	TObjectPtr<ULDGameData> GameData = nullptr;

	UPROPERTY()
	TObjectPtr<ULDCommandProcessor> CommandProcessor = nullptr;

	TArray<TWeakObjectPtr<APlayerController>> Participants;
	TArray<TWeakObjectPtr<ALDPlayerController>> PendingParticipants;
	uint64 NextConnectionEpoch = 1;
	bool bEnding = false;
};
