#pragma once

#include "CoreMinimal.h"
#include "LDEconomyTypes.generated.h"

/** Owner-only display snapshot. RNG and source state never come from the client. */
USTRUCT()
struct MOBILE_DEFENSE_CLONE_API FLDEconomySnapshot
{
	GENERATED_BODY()
	UPROPERTY()
	FGuid MatchId;
	UPROPERTY()
	int32 PlayerIndex = INDEX_NONE;
	UPROPERTY()
	int32 EconomyRevision = 0;
	UPROPERTY()
	int32 Gold = 0;
	UPROPERTY()
	int32 Stars = 0;
	UPROPERTY()
	int32 PaidSummonCount = 0;
	UPROPERTY()
	int32 NextSummonGold = 0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FLDEconomyChanged, const FLDEconomySnapshot&);
