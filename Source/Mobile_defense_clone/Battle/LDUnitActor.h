#pragma once

#include "CoreMinimal.h"
#include "Board/LDBoardTypes.h"
#include "Data/LDGameData.h"
#include "GameFramework/Actor.h"
#include "LDUnitActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

USTRUCT()
struct MOBILE_DEFENSE_CLONE_API FLDUnitAttackCue
{
	GENERATED_BODY()
	UPROPERTY()
	uint64 DamageEventId = 0;
	UPROPERTY()
	FVector TargetCanonical = FVector::ZeroVector;
	UPROPERTY()
	double ServerSeconds = 0;
};

// Placement is a copy of the board source. This actor owns neither money nor an attack cooldown.
UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDUnitActor : public AActor
{
	GENERATED_BODY()

public:
	ALDUnitActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool InitializePrepared(const FLDPlacedUnit& Unit, const FLDUnitRow& Row, const FTransform& Transform);
	void ApplyCommittedPlacement(const FLDPlacedUnit& Unit, const FTransform& Transform);
	void DeactivateCommitted();
	bool SetLocalViewPlayerIndex(int32 PlayerIndex);
	void SetPresentationSlot(int32 SlotIndex, double VisualMoveSeconds);
	void PresentCommittedAttack(uint64 DamageEventId, const FVector& TargetCanonical, double ServerSeconds);
	const FLDPlacedUnit& GetPlacement() const;
	const FLDUnitRow& GetUnitRow() const;
	bool IsCommitted() const;
	double GetRangeCm() const;
	FVector GetPresentationLocation() const;
	const FLDUnitAttackCue& GetLastAttackCue() const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Placement();
	void RefreshPresentation();
	double GetPresentationSeconds() const;

	UPROPERTY()
	TObjectPtr<USceneComponent> CanonicalRoot = nullptr;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> PresentationMesh = nullptr;
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ProjectileMesh = nullptr;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Material = nullptr;
	UPROPERTY(ReplicatedUsing = OnRep_Placement)
	FLDPlacedUnit Placement;
	UPROPERTY(ReplicatedUsing = OnRep_Placement)
	FVector CanonicalPosition = FVector::ZeroVector;
	UPROPERTY(ReplicatedUsing = OnRep_Placement)
	bool bCommitted = false;
	UPROPERTY(Replicated)
	FName AttackPresentation = NAME_None;
	UPROPERTY(Replicated)
	double RangeCm = 0;
	UPROPERTY(Replicated)
	FLinearColor UnitColor = FLinearColor::White;
	UPROPERTY(Replicated)
	FLDUnitAttackCue LastAttackCue;
	UPROPERTY(Replicated)
	int32 PresentationSlot = 0;
	UPROPERTY(Replicated)
	double MovePresentationSeconds = 0.15;
	FLDUnitRow UnitRow;
	FVector VisualCanonical = FVector::ZeroVector;
	FVector VisualMoveStart = FVector::ZeroVector;
	FVector VisualMoveTarget = FVector::ZeroVector;
	double VisualMoveStartedSeconds = 0;
	bool bVisualInitialized = false;
	int32 LocalPlayerIndex = INDEX_NONE;
	bool bPrepared = false;
	bool bEnding = false;
};
