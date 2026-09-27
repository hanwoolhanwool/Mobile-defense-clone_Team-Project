#pragma once

#include "CoreMinimal.h"
#include "Data/LDBattleTypes.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "LDGameMode.generated.h"

class ULDGameData;
class ULDCommandProcessor;
class ALDPlayerController;
class ULDBoardManager;
class ULDEconomyService;
class ULDCombatService;
class ULDWaveDirector;
struct FLDBoardCommit;
struct FLDEconomySnapshot;
struct FLDCombatDeath;

UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALDGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
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
	ULDWaveDirector* GetWaveDirector() const;

protected:
	virtual void BeginPlay() override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FLDP0OpenFrameBoundaryTest;
	friend class FLDWaveModeBoundaryTest;
	friend class FLDWaveReadinessTest;
#endif
	void RefreshReadiness();
	void RegisterPendingParticipants();
	bool RegisterParticipant(ALDPlayerController& Controller);
	void StopMatchServices();
	void ReleasePlayerSessions();
	void AdvanceLogic();
	void AdvanceBeforeExternalCommand(double ServerSeconds);
	void AdvanceTimelineBefore(double ServerSeconds);
	void RequestTerminal(ELDMatchResult Result, ELDResultReason Reason, double ServerSeconds);
	void FinalizePendingTerminal();
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
	UPROPERTY()
	TObjectPtr<ULDWaveDirector> WaveDirector = nullptr;
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
	bool bG2Probe = false;
	double LoadingStartSeconds = 0;
	TOptional<int32> TravelSeed;
	ELDMatchResult PendingResult = ELDMatchResult::None;
	ELDResultReason PendingReason = ELDResultReason::None;
	double PendingResultSeconds = 0;
	bool bAdvancingTimeline = false;

	TArray<TWeakObjectPtr<APlayerController>> Participants;
	TArray<TWeakObjectPtr<ALDPlayerController>> PendingParticipants;
	uint64 NextConnectionEpoch = 1;
	bool bEnding = false;
};
