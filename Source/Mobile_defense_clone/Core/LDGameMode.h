#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "LDGameMode.generated.h"

class ULDGameData;
class ULDCommandProcessor;
class ALDPlayerController;
class ULDBoardManager;
class ULDEconomyService;
class ULDCombatService;
struct FLDBoardCommit;
struct FLDEconomySnapshot;
struct FLDCombatDeath;

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
	ULDBoardManager* GetBoardManager() const;
	ULDEconomyService* GetEconomyService() const;
	ULDCombatService* GetCombatService() const;
	ULDCommandProcessor* GetCommandProcessor() const;

protected:
	virtual void BeginPlay() override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FLDP0OpenFrameBoundaryTest;
#endif
	void RefreshReadiness();
	void RegisterPendingParticipants();
	bool RegisterParticipant(ALDPlayerController& Controller);
	void StopMatchServices();
	void ReleasePlayerSessions();
	void AdvanceLogic();
	void AdvanceBeforeExternalCommand(double ServerSeconds);
	void HandleBoardCommitted(const FLDBoardCommit& Commit);
	void HandleEconomyChanged(const FLDEconomySnapshot& Snapshot);
	void HandleEnemyDeath(const FLDCombatDeath& Death);
	void PublishPlayerSnapshots(int32 PlayerIndex);

	UPROPERTY()
	TObjectPtr<ULDGameData> GameData = nullptr;

	UPROPERTY()
	TObjectPtr<ULDCommandProcessor> CommandProcessor = nullptr;
	UPROPERTY()
	TObjectPtr<ULDBoardManager> BoardManager = nullptr;
	UPROPERTY()
	TObjectPtr<ULDEconomyService> EconomyService = nullptr;
	UPROPERTY()
	TObjectPtr<ULDCombatService> CombatService = nullptr;
	FDelegateHandle BoardCommitHandle;
	FDelegateHandle EconomyChangedHandle;
	FDelegateHandle EnemyDeathHandle;
	FTimerHandle LogicTimer;
	double LogicOriginSeconds = 0;
	uint64 LogicStep = 0;
	int32 LastBoardRevisions[2] = {0, 0};
	bool bServicesReady = false;
	bool bPlayStarted = false;
	bool bG1Probe = false;

	TArray<TWeakObjectPtr<APlayerController>> Participants;
	TArray<TWeakObjectPtr<ALDPlayerController>> PendingParticipants;
	uint64 NextConnectionEpoch = 1;
	bool bEnding = false;
};
