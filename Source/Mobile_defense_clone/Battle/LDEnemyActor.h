#pragma once

#include "CoreMinimal.h"
#include "Battle/LDCombatEvents.h"
#include "Battle/LDRouteModel.h"
#include "Data/LDGameData.h"
#include "GameFramework/Actor.h"
#include "LDEnemyActor.generated.h"

class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

USTRUCT()
struct MOBILE_DEFENSE_CLONE_API FLDEnemyRouteSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid MatchId;

	UPROPERTY()
	uint64 EnemyId = 0;

	UPROPERTY()
	int32 RouteIndex = INDEX_NONE;

	UPROPERTY()
	double TotalDistanceCm = 0;

	UPROPERTY()
	double SampleServerSeconds = 0;

	UPROPERTY()
	double SpeedCmPerSecond = 0;

	UPROPERTY()
	bool bActive = false;
};

USTRUCT()
struct MOBILE_DEFENSE_CLONE_API FLDEnemyCombatSnapshot
{
	GENERATED_BODY()
	UPROPERTY()
	FName EnemyTypeId = NAME_None;
	UPROPERTY()
	uint64 SpawnSerial = 0;
	UPROPERTY()
	int32 SpawnWaveIndex = 0;
	UPROPERTY()
	double SpawnedServerSeconds = 0;
	UPROPERTY()
	double DeathServerSeconds = 0;
	UPROPERTY()
	double MaxHP = 0;
	UPROPERTY()
	double HP = 0;
	UPROPERTY()
	bool bAlive = false;
};

// Route identity survives laps. Combat initialization is separate so the explicit G1 fixture remains unchanged.
UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDEnemyActor : public AActor
{
	GENERATED_BODY()

public:
	ALDEnemyActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool InitializeRoute(const FGuid& MatchId, uint64 EnemyId, int32 RouteIndex, const TArray<FVector>& Points,
	                     double SpeedCmPerSecond, double StartServerSeconds);
	bool AdvanceRouteTo(double ServerSeconds);
	void StopRoute();
	bool SetLocalViewPlayerIndex(int32 PlayerIndex);
	bool RefreshPresentation(double ViewServerSeconds);
	const FLDEnemyRouteSnapshot& GetRouteSnapshot() const;
	FVector GetPresentationLocation() const;
	bool IsPresentationVisible() const;
	bool InitializeCombat(const FLDEnemyRow& Row, double MaxHP, uint64 SpawnSerial, int32 SpawnWaveIndex,
	                      double SpawnedServerSeconds);
	ELDDamageResult TryApplyDamage(const FLDDamageEvent& Event, FLDCombatDeath& OutDeath);
	void StopCombat();
	bool IsCombatAlive() const;
	const FLDEnemyCombatSnapshot& GetCombatSnapshot() const;
	const FLDEnemyRow& GetEnemyRow() const;
	bool TryGetCanonicalPositionAt(double ServerSeconds, FVector& OutPosition) const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_RoutePoints();

	UFUNCTION()
	void OnRep_RouteSnapshot();
	UFUNCTION()
	void OnRep_CombatSnapshot();

	void ApplyCanonicalSnapshot();
	void RefreshColor();
	double GetPresentationServerSeconds() const;

	UPROPERTY(VisibleAnywhere, Category = "LD|Route")
	TObjectPtr<USceneComponent> CanonicalRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "LD|Route")
	TObjectPtr<UStaticMeshComponent> PresentationMesh = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PresentationMaterial = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_RoutePoints)
	TArray<FVector> RoutePoints;

	UPROPERTY(ReplicatedUsing = OnRep_RouteSnapshot)
	FLDEnemyRouteSnapshot RouteSnapshot;
	UPROPERTY(ReplicatedUsing = OnRep_CombatSnapshot)
	FLDEnemyCombatSnapshot CombatSnapshot;
	FLDEnemyRow EnemyRow;
	TSet<uint64> AppliedDamageEvents;
	bool bCombatClosed = false;

	FLDRouteModel RouteModel;
	double InitialServerSeconds = 0;
	int32 LocalViewPlayerIndex = INDEX_NONE;
	bool bRouteStopped = false;
	bool bEnding = false;
	static constexpr double PresentationHeightCm = 35;
};
