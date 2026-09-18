#pragma once

#include "CoreMinimal.h"
#include "Battle/LDRouteModel.h"
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

// G1 path participant only. Combat, HP, damage and death rewards are deliberately absent at this gate.
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

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_RoutePoints();

	UFUNCTION()
	void OnRep_RouteSnapshot();

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

	FLDRouteModel RouteModel;
	double InitialServerSeconds = 0;
	int32 LocalViewPlayerIndex = INDEX_NONE;
	bool bRouteStopped = false;
	bool bEnding = false;
	static constexpr double PresentationHeightCm = 35;
};
