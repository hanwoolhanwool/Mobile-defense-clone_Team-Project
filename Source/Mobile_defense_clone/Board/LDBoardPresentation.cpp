#include "Board/LDBoardPresentation.h"

#include "Board/LDViewTransform.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor CellColor = FLinearColor::FromSRGBColor(FColor(222, 207, 174));
	const FLinearColor SelectedColor = FLinearColor::FromSRGBColor(FColor(93, 193, 170));
	const FLinearColor BorderColor = FLinearColor::FromSRGBColor(FColor(151, 134, 108));
	const FLinearColor PathColor = FLinearColor::FromSRGBColor(FColor(43, 48, 59));
} // namespace

ALDBoardPresentation::ALDBoardPresentation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CanonicalOrigin"));
	SetRootComponent(SceneRoot);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("LocalBoardCamera"));
	Camera->SetupAttachment(SceneRoot);
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->SetRelativeRotation(FRotator(-90, 90, 0));
	Camera->SetRelativeLocation(FVector(0, 0, 2400));
	Camera->bConstrainAspectRatio = false;
	Camera->bOverrideAspectRatioAxisConstraint = true;
	Camera->AspectRatioAxisConstraint = AspectRatio_MaintainXFOV;
	Camera->bAutoCalculateOrthoPlanes = false;
	Camera->bUseCameraHeightAsViewTarget = false;
	Camera->OrthoNearClipPlane = 1;
	Camera->OrthoFarClipPlane = 10000;
	Camera->PostProcessBlendWeight = 0;
	bFindCameraComponentWhenViewTarget = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/LD/Materials/M_P0Flat.M_P0Flat"));
	CubeMesh = Cube.Object;
	FlatMaterial = Material.Object;
}

UStaticMeshComponent* ALDBoardPresentation::AddCube(const FVector& Location, const FVector& Size,
                                                    const FLinearColor& Color)
{
	UStaticMeshComponent* Visual = NewObject<UStaticMeshComponent>(this);
	Visual->SetupAttachment(SceneRoot);
	Visual->SetStaticMesh(CubeMesh);
	Visual->SetRelativeLocation(Location);
	Visual->SetRelativeScale3D(Size / 100.0);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetCastShadow(false);
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(FlatMaterial, Visual);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Visual->SetMaterial(0, Material);
	Visual->RegisterComponent();
	Visuals.Add(Visual);
	return Visual;
}

bool ALDBoardPresentation::Initialize(const FLDBoardGeometry& InGeometry, int32 InLocalPlayerIndex, FString& OutError)
{
	OutError.Reset();
	if (bInitialized || !InGeometry.IsReady() || InLocalPlayerIndex < 0 || InLocalPlayerIndex > 1 || !CubeMesh ||
	    !FlatMaterial)
	{
		OutError = TEXT("Local board geometry or required primitive/material is unavailable");
		return false;
	}
	Geometry = InGeometry;
	LocalPlayerIndex = InLocalPlayerIndex;
	const double Size = Geometry.GetCellSizeCm();
	const FVector2D Field = Geometry.GetFieldSizeCm();
	AddCube(FVector(0, 0, -35), FVector(20000, 20000, 10), FLinearColor::FromSRGBColor(FColor(23, 29, 40)));
	AddCube(FVector(0, 0, -8), FVector(Field.X, Field.Y, 8), PathColor);
	for (int32 Cell = 0; Cell < 36; ++Cell)
	{
		FVector Canonical;
		Geometry.TryGetCellCenter(Cell, Canonical);
		const FVector Presented = FLDViewTransform::ToPresentation(Canonical, LocalPlayerIndex);
		AddCube(Presented, FVector(Size, Size, 6), BorderColor);
		UStaticMeshComponent* Inner = AddCube(Presented + FVector(0, 0, 4), FVector(Size - 3, Size - 3, 6), CellColor);
		CellVisuals.Add(Inner);
		CellMaterials.Add(Cast<UMaterialInstanceDynamic>(Inner->GetMaterial(0)));
	}
	// Subtle chevrons mark the shared canonical -X movement; their direction is the same in both views.
	for (double X : {280.0, 0.0, -280.0})
	{
		for (double Sign : {-1.0, 1.0})
		{
			UStaticMeshComponent* Arrow = AddCube(FVector(X + 12, Sign * 10, 4), FVector(32, 6, 5),
			                                      FLinearColor::FromSRGBColor(FColor(112, 119, 132)));
			Arrow->SetRelativeRotation(FRotator(0, Sign * 40, 0));
		}
	}
	for (int32 Gate = 0; Gate < 2; ++Gate)
	{
		const FVector Start(3.5 * Size, Gate == 0 ? -4 * Size : 4 * Size, 4);
		AddCube(FLDViewTransform::ToPresentation(Start, LocalPlayerIndex), FVector(Size * 0.62, Size * 0.62, 8),
		        Gate == LocalPlayerIndex ? SelectedColor : FLinearColor::FromSRGBColor(FColor(222, 153, 90)));
	}
	bInitialized = true;
	return true;
}

void ALDBoardPresentation::ApplyViewportLayout(const FLDBoardViewportLayout& Layout)
{
	if (Layout.OrthoWidthCm <= 0 || Layout.ViewportSizePixels.Y <= 0)
	{
		return;
	}
	Camera->SetRelativeLocation(Layout.CameraLocation);
	Camera->SetOrthoWidth(static_cast<float>(Layout.OrthoWidthCm));
	Camera->SetAspectRatio(static_cast<float>(Layout.ViewportSizePixels.X / Layout.ViewportSizePixels.Y));
	const double Size = Geometry.GetCellSizeCm();
	if (Geometry.IsReady() && FMath::IsFinite(Layout.PixelsPerCm) && Layout.PixelsPerCm > 0 && Size > 0)
	{
		// A shared cell edge needs a visible pixel gap; keep tiny viewports from consuming the cell interior.
		const double GapCm = FMath::Min(Size * 0.2, FMath::Max(3.0, 1.5 / Layout.PixelsPerCm));
		const double InnerScale = (Size - GapCm) / 100.0;
		for (UStaticMeshComponent* CellVisual : CellVisuals)
		{
			if (IsValid(CellVisual))
			{
				FVector Scale = CellVisual->GetRelativeScale3D();
				Scale.X = InnerScale;
				Scale.Y = InnerScale;
				CellVisual->SetRelativeScale3D(Scale);
			}
		}
	}
}

void ALDBoardPresentation::SetSelectedCell(int32 CellId)
{
	if (CellMaterials.IsValidIndex(SelectedCellId))
	{
		CellMaterials[SelectedCellId]->SetVectorParameterValue(TEXT("Color"), CellColor);
	}
	SelectedCellId =
	    Geometry.ValidateSelection(LocalPlayerIndex, CellId) == ELDCellInputResult::Selected ? CellId : INDEX_NONE;
	if (CellMaterials.IsValidIndex(SelectedCellId))
	{
		CellMaterials[SelectedCellId]->SetVectorParameterValue(TEXT("Color"), SelectedColor);
	}
}

const FLDBoardGeometry& ALDBoardPresentation::GetGeometry() const
{
	return Geometry;
}
