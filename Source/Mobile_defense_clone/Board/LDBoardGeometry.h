#pragma once

#include "CoreMinimal.h"
#include "LDBoardGeometry.generated.h"

struct FLDGameRules;

UENUM()
enum class ELDCellInputResult : uint8
{
	None,
	Selected,
	NotReady,
	OutsideBoard,
	NotOwner,
	InvalidProjection
};

/** Immutable canonical board geometry shared by rendering and hit testing. No unit/economy state. */
struct MOBILE_DEFENSE_CLONE_API FLDBoardGeometry
{
	bool Initialize(const FLDGameRules& Rules, FString& OutError);
	bool IsReady() const;
	bool TryGetCellCenter(int32 CellId, FVector& OutCanonical) const;
	bool TryGetCellAtCanonicalPosition(const FVector& Canonical, int32& OutCellId) const;
	int32 GetCellOwner(int32 CellId) const;
	ELDCellInputResult ValidateSelection(int32 PlayerIndex, int32 CellId) const;
	double GetCellSizeCm() const;
	FVector2D GetFieldSizeCm() const;

private:
	TArray<FVector> Centers;
	double CellSizeCm = 0;
	FVector2D FieldSizeCm = FVector2D::ZeroVector;
};

/** Physical viewport pixels, before UMG DPI scaling. The field always uses one uniform scale. */
struct MOBILE_DEFENSE_CLONE_API FLDBoardViewportLayout
{
	FVector2D ViewportSizePixels = FVector2D::ZeroVector;
	FBox2D SafeRectPixels = FBox2D(ForceInit);
	FBox2D FieldRectPixels = FBox2D(ForceInit);
	double PixelsPerCm = 0;
	double OrthoWidthCm = 0;
	FVector CameraLocation = FVector::ZeroVector;

	bool Initialize(const FVector2D& ViewportPixels, const FBox2D& SafePixels, const FVector2D& FieldSizeCm);
	bool IsInsideField(const FVector2D& ScreenPixels) const;
};
