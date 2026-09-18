#pragma once

#include "CoreMinimal.h"

/** Local presentation only. Server locations and logical identifiers never use this transform. */
struct MOBILE_DEFENSE_CLONE_API FLDViewTransform
{
	static FVector ToPresentation(const FVector& Canonical, int32 LocalPlayerIndex)
	{
		return FVector(Canonical.X, LocalPlayerIndex == 1 ? -Canonical.Y : Canonical.Y, Canonical.Z);
	}

	static FVector ToCanonical(const FVector& Presented, int32 LocalPlayerIndex)
	{
		return ToPresentation(Presented, LocalPlayerIndex);
	}
};
