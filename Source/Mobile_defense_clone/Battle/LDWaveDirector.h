#pragma once

#include "CoreMinimal.h"
#include "Battle/LDCombatEvents.h"
#include "Data/LDBattleTypes.h"
#include "UObject/Object.h"
#include "LDWaveDirector.generated.h"

class ALDEnemyActor;
class ALDGameState;
class ULDCombatService;
class ULDGameData;
struct FLDWaveRow;
using FLDEnemyActorFactory = TFunction<ALDEnemyActor*(UWorld&)>;
DECLARE_MULTICAST_DELEGATE_ThreeParams(FLDWaveTerminalRequested, ELDMatchResult, ELDResultReason, double);

// Owns schedule cursors and enemy membership. GameState owns the published match state, actors own HP.
// GameMode closes each timestamp: combat -> rewards -> this director -> terminal publication.
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDWaveDirector : public UObject
{
	GENERATED_BODY()
public:
	virtual UWorld* GetWorld() const override;
	bool Initialize(ULDGameData& Data, ALDGameState& State, ULDCombatService& Combat,
	                FLDEnemyActorFactory ActorFactory = {});
	bool StartAt(double ServerSeconds);
	double GetNextEventSeconds() const;
	void ProcessEventsAt(double ServerSeconds);
	bool HandleEnemyDeath(const FLDCombatDeath& Death);
	void RefreshCombatView();
	void EvaluateVictory(double ServerSeconds);
	void Stop();
	int32 GetTrackedEnemyCount() const;
	FLDWaveTerminalRequested OnTerminalRequested;

private:
#if !UE_BUILD_SHIPPING
	friend struct FLDG3BoundaryAccess;
#endif
#if WITH_DEV_AUTOMATION_TESTS
	friend class FLDWaveScheduleTest;
	friend class FLDWaveCapAndDeathTest;
	friend class FLDWaveBossBoundaryTest;
	friend class FLDWaveTerminalTruthTest;
	friend class FLDWaveFailureTest;
	friend class FLDWaveModeBoundaryTest;
	friend struct FLDWaveTestAccess;
#endif
	bool BeginWave(int32 WaveIndex, double ServerSeconds);
	bool SpawnEnemy(int32 RouteIndex, const FLDWaveRow& Wave, double ServerSeconds);
	void RequestTerminal(ELDMatchResult Result, ELDResultReason Reason, double ServerSeconds);
	UPROPERTY()
	TObjectPtr<ULDGameData> GameData;
	UPROPERTY()
	TObjectPtr<ALDGameState> GameState;
	UPROPERTY()
	TObjectPtr<ULDCombatService> CombatService;
	TMap<uint64, TWeakObjectPtr<ALDEnemyActor>> LivingEnemies;
	FLDEnemyActorFactory SpawnActor;
	TSet<uint64> NormalEnemies;
	double WaveStartSeconds = 0;
	double LastDeathServerSeconds = 0;
	int32 NextNormalOrdinal = 0;
	uint64 NextEnemyId = 1;
	bool bBossDeadlineProcessed = false;
	bool bInitialized = false;
	bool bStarted = false;
	bool bStopped = false;
};
