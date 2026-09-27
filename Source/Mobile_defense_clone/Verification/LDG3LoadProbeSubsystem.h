#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "Battle/LDCombatEvents.h"
#include "Data/LDMatchTypes.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "LDG3LoadProbeSubsystem.generated.h"

class ALDEnemyActor;
class ALDGameMode;
class ALDPlayerController;
class ALDUnitActor;

/** Development-only fixture coordination. The only RPC reports observation, never a gameplay command. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDG3LoadProbeState : public AActor
{
	GENERATED_BODY()
public:
	ALDG3LoadProbeState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated)
	int32 Phase = 1;
	UPROPERTY(Replicated)
	int32 Batch = -1;
	UPROPERTY(Replicated)
	uint64 BatchFirstEnemyId = 0;
	UPROPERTY(Replicated)
	double SustainStartServerSeconds = 0;
	UPROPERTY(Replicated)
	double SustainDurationSeconds = 1200;
	UPROPERTY(Replicated)
	bool bServerPassed = false;
	UFUNCTION(Server, Reliable)
	void ServerReportObservation(int32 ObservedPhase, int32 ObservedBatch, bool bPass);
	bool bClientSustainReady = false;
	int32 ClientObservedBatch = -1;
	bool bClientFinished = false;
	bool bClientPassed = true;
};

/** Explicit -P0Probe=G3Load: workload/lifetime fixture, never a normal-wave victory or balance test. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDG3LoadProbeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	bool PrepareLoad(ALDGameMode& Mode);
	bool PrepareUnits(ALDGameMode& Mode);
	ALDEnemyActor* SpawnEnemy(ALDGameMode& Mode, bool bBoss, double HP, int32 Ordinal);
	void TickAuthority(ALDGameMode& Mode, double Now);
	void TickLocal(ALDPlayerController& Controller, double Now);
	void BeginBatch(ALDGameMode& Mode);
	void DestroyEnemies(ALDGameMode& Mode);
	void BeginStop(ALDGameMode& Mode);
	void OnDamage(const FLDDamageEvent& Event, int32 PlayerIndex, int32 EffectiveDamage);
	void OnDeath(const FLDCombatDeath& Death);
	void Check(const FString& Name, bool bPass, const FString& Detail = TEXT(""));
	void Sample(const FString& Label, double Now);
	void WriteResult(bool bHandshakeConfirmed);
	void BeginProfileCapture();
	void EndProfileCapture();
	bool PollProfileWrite();
	void FailAndExit(const FString& Reason);
	int32 CountUncollected(const TArray<TWeakObjectPtr<ALDEnemyActor>>& Actors) const;

	FString OutputDirectory;
	TWeakObjectPtr<ALDG3LoadProbeState> State;
	FLDParticipantContext Participants[2];
	TArray<TWeakObjectPtr<ALDEnemyActor>> Enemies;
	TArray<TWeakObjectPtr<ALDEnemyActor>> AllEnemies;
	TArray<TWeakObjectPtr<ALDUnitActor>> AllUnits;
	TArray<TWeakObjectPtr<ALDEnemyActor>> BossActors;
	TSet<uint64> AuthoredUnitIds;
	TSet<uint64> ObservedEnemyIds;
	TSet<uint64> ObservedUnitIds;
	TSet<uint64> DeathIds;
	TSet<uint64> AttackedEnemyIds;
	TSet<FName> AttackingUnitTypes;
	FDelegateHandle DamageHandle;
	FDelegateHandle DeathHandle;
	uint64 NextEnemyId = 100000;
	uint64 DamageEvents = 0;
	uint64 DamageEventsAtStop = 0;
	uint64 FixtureDamageId = 1000000000;
	int32 NaturalDeaths = 0;
	int32 FixtureDeaths = 0;
	int32 LocalPlayerIndex = INDEX_NONE;
	int32 LocalPhase = -1;
	int32 LocalBatch = -2;
	int32 CompletedBatches = 0;
	int32 IntegritySamples = 0;
	int32 MinimumUnits = MAX_int32;
	int32 MinimumNormals = MAX_int32;
	int32 MinimumBosses = MAX_int32;
	int32 MinimumOwnerPopulation = MAX_int32;
	double LastIntegrityAt = 0;
	bool bIntegrityFailed = false;
	int32 BaselineGold[2] = {0, 0};
	int32 BaselineStars[2] = {0, 0};
	int32 BaselineEconomyRevision[2] = {0, 0};
	int32 BaselineRng[2] = {0, 0};
	uint64 BaselineRss = 0;
	uint64 PeakRss = 0;
	uint64 FinalRss = 0;
	double CreatedAt = 0;
	double LoadSeconds = 1200;
	double SustainStartedAt = 0;
	double PhaseStartedAt = 0;
	double LastSampleAt = 0;
	double LastTickAt = 0;
	double ExitAt = 0;
	double LastRouteSum = 0;
	double MeasuredSustainSeconds = 0;
	bool bSustainObserved = false;
	bool bRouteMovementObserved = false;
	bool bResultWritten = false;
	bool bFinished = false;
	bool bFailed = false;
	bool bFailingExit = false;
	bool bProfileStarted = false;
	bool bProfileEndRequested = false;
	bool bProfileWritten = false;
	bool bHandshakeComplete = false;
	bool bScreenshotRequested = false;
	FString ProfilePath;
	TSharedFuture<FString> ProfileWrite;
	TArray<float> SustainedFrameMs;
	TArray<float> SampleFrameMs;
	TArray<TSharedPtr<class FJsonValue>> Checks;
	TArray<TSharedPtr<class FJsonValue>> MemoryCheckpoints;
};
