#pragma once

#include "CoreMinimal.h"
#include "Board/LDBoardGeometry.h"
#include "GameFramework/Actor.h"
#include "LDBoardPresentation.generated.h"

class UCameraComponent;
class USceneComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

/** Non-replicated local board visuals and camera. Canonical gameplay objects are never moved here. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDBoardPresentation : public AActor
{
	GENERATED_BODY()

public:
	ALDBoardPresentation();
	bool Initialize(const FLDBoardGeometry& InGeometry, int32 InLocalPlayerIndex, FString& OutError);
	void ApplyViewportLayout(const FLDBoardViewportLayout& Layout);
	void SetSelectedCell(int32 CellId);
	void SetRangePresentation(const FVector& CanonicalCenter, double RadiusCm);
	const FLDBoardGeometry& GetGeometry() const;

private:
	UStaticMeshComponent* AddCube(const FVector& Location, const FVector& Size, const FLinearColor& Color);
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> FlatMaterial;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Visuals;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> CellVisuals;
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CellMaterials;
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> RangeVisuals;
	FVector RangeCenter = FVector::ZeroVector;
	double RangeRadiusCm = -1;
	double PixelsPerCm = 1;
	FLDBoardGeometry Geometry;
	int32 LocalPlayerIndex = INDEX_NONE;
	int32 SelectedCellId = INDEX_NONE;
	bool bInitialized = false;
};
