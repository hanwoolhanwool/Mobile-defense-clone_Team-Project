#include "Board/LDBoardGeometry.h"

#include "Data/LDGameData.h"

bool FLDBoardGeometry::Initialize(const FLDGameRules& Rules, FString& OutError)
{
	OutError.Reset();
	if (Rules.Columns != 6 || Rules.Rows != 3 || Rules.CellsPerPlayer != 18 || Rules.XCentersCm.Num() != 6 ||
	    Rules.YCentersByPlayer.Num() != 2 || !FMath::IsFinite(Rules.CellSizeCm) || Rules.CellSizeCm <= 0)
	{
		OutError = TEXT("P0 geometry requires two 6x3 boards and a positive cell size");
		return false;
	}
	TArray<FVector> NextCenters;
	for (int32 Player = 0; Player < 2; ++Player)
	{
		if (Rules.YCentersByPlayer[Player].Num() != 3)
		{
			OutError = TEXT("P0 geometry requires three row centers per board");
			return false;
		}
		for (int32 Row = 0; Row < 3; ++Row)
		{
			for (int32 Column = 0; Column < 6; ++Column)
			{
				const double X = Rules.XCentersCm[Column];
				const double Y = Rules.YCentersByPlayer[Player][Row];
				if (!FMath::IsFinite(X) || !FMath::IsFinite(Y))
				{
					OutError = TEXT("Cell centers must be finite");
					return false;
				}
				NextCenters.Emplace(X, Y, 0);
			}
		}
	}
	Centers = MoveTemp(NextCenters);
	CellSizeCm = Rules.CellSizeCm;
	FieldSizeCm = FVector2D(8 * CellSizeCm, 9 * CellSizeCm);
	return true;
}

bool FLDBoardGeometry::IsReady() const
{
	return Centers.Num() == 36;
}

bool FLDBoardGeometry::TryGetCellCenter(int32 CellId, FVector& OutCanonical) const
{
	if (!Centers.IsValidIndex(CellId))
	{
		return false;
	}
	OutCanonical = Centers[CellId];
	return true;
}

bool FLDBoardGeometry::TryGetCellAtCanonicalPosition(const FVector& Canonical, int32& OutCellId) const
{
	OutCellId = INDEX_NONE;
	if (!IsReady() || Canonical.ContainsNaN())
	{
		return false;
	}
	const double Half = CellSizeCm / 2;
	for (int32 Cell = 0; Cell < Centers.Num(); ++Cell)
	{
		const FVector Delta = Canonical - Centers[Cell];
		// Half-open edges assign shared boundaries to only one cell, without a hidden hit-test gap.
		if (Delta.X >= -Half && Delta.X < Half && Delta.Y >= -Half && Delta.Y < Half)
		{
			OutCellId = Cell;
			return true;
		}
	}
	return false;
}

int32 FLDBoardGeometry::GetCellOwner(int32 CellId) const
{
	return Centers.IsValidIndex(CellId) ? CellId / 18 : INDEX_NONE;
}

ELDCellInputResult FLDBoardGeometry::ValidateSelection(int32 PlayerIndex, int32 CellId) const
{
	if (!IsReady() || PlayerIndex < 0 || PlayerIndex > 1)
	{
		return ELDCellInputResult::NotReady;
	}
	const int32 Owner = GetCellOwner(CellId);
	if (Owner == INDEX_NONE)
	{
		return ELDCellInputResult::OutsideBoard;
	}
	return Owner == PlayerIndex ? ELDCellInputResult::Selected : ELDCellInputResult::NotOwner;
}

double FLDBoardGeometry::GetCellSizeCm() const
{
	return CellSizeCm;
}

FVector2D FLDBoardGeometry::GetFieldSizeCm() const
{
	return FieldSizeCm;
}

bool FLDBoardViewportLayout::Initialize(const FVector2D& ViewportPixels, const FBox2D& SafePixels,
                                        const FVector2D& FieldSizeCm)
{
	if (ViewportPixels.ContainsNaN() || ViewportPixels.X <= 0 || ViewportPixels.Y <= 0 || !SafePixels.bIsValid ||
	    SafePixels.Min.ContainsNaN() || SafePixels.Max.ContainsNaN() || SafePixels.Min.X < 0 || SafePixels.Min.Y < 0 ||
	    SafePixels.Max.X > ViewportPixels.X || SafePixels.Max.Y > ViewportPixels.Y || SafePixels.GetSize().X <= 0 ||
	    SafePixels.GetSize().Y <= 0 || FieldSizeCm.ContainsNaN() || FieldSizeCm.X <= 0 || FieldSizeCm.Y <= 0 ||
	    !FMath::IsNearlyEqual(FieldSizeCm.X / FieldSizeCm.Y, 8.0 / 9.0, 0.0001))
	{
		return false;
	}
	const FVector2D SafeSize = SafePixels.GetSize();
	const double Scale = FMath::Min(SafeSize.X / 1080.0, SafeSize.Y / 2340.0);
	const FVector2D FieldPixels(960 * Scale, 1080 * Scale);
	const FVector2D Center = SafePixels.Min + FVector2D(SafeSize.X * 0.5, SafeSize.Y * (1060.0 / 2340.0));
	ViewportSizePixels = ViewportPixels;
	SafeRectPixels = SafePixels;
	FieldRectPixels = FBox2D(Center - FieldPixels / 2, Center + FieldPixels / 2);
	PixelsPerCm = FieldPixels.X / FieldSizeCm.X;
	OrthoWidthCm = ViewportPixels.X / PixelsPerCm;
	// Pitch -90, yaw 90: camera right=-WorldX, camera up=+WorldY.
	const FVector2D Offset = (Center - ViewportPixels / 2) / PixelsPerCm;
	CameraLocation = FVector(Offset.X, Offset.Y, 2400);
	return true;
}

bool FLDBoardViewportLayout::IsInsideField(const FVector2D& ScreenPixels) const
{
	return FieldRectPixels.bIsValid && !ScreenPixels.ContainsNaN() && ScreenPixels.X >= FieldRectPixels.Min.X &&
	       ScreenPixels.Y >= FieldRectPixels.Min.Y && ScreenPixels.X < FieldRectPixels.Max.X &&
	       ScreenPixels.Y < FieldRectPixels.Max.Y;
}
