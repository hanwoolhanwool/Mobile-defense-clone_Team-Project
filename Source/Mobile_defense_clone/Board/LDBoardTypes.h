#pragma once

#include "CoreMinimal.h"
#include "LDBoardTypes.generated.h"

/** Authoritative placement is owned by BoardManager; actors and clients receive value copies. */
USTRUCT()
struct MOBILE_DEFENSE_CLONE_API FLDPlacedUnit
{
	GENERATED_BODY()
	UPROPERTY()
	uint64 InstanceId = 0;
	UPROPERTY()
	FName UnitId = NAME_None;
	UPROPERTY()
	int32 PlayerIndex = INDEX_NONE;
	UPROPERTY()
	int32 CellId = INDEX_NONE;
	UPROPERTY()
	double MoveBlockedUntilServerSeconds = 0;
};

UENUM()
enum class ELDBoardChangeReason : uint8
{
	Summon,
	Move,
	Merge,
	Sell
};

struct MOBILE_DEFENSE_CLONE_API FLDBoardCommit
{
	FGuid MatchId;
	int32 PlayerIndex = INDEX_NONE;
	int32 BoardRevision = 0;
	double CommitServerSeconds = 0;
	ELDBoardChangeReason ChangeReason = ELDBoardChangeReason::Summon;
	TArray<FLDPlacedUnit> AddedOrUpdatedUnits;
	TArray<uint64> RemovedInstanceIds;
};

USTRUCT()
struct MOBILE_DEFENSE_CLONE_API FLDBoardSnapshot
{
	GENERATED_BODY()
	UPROPERTY()
	FGuid MatchId;
	UPROPERTY()
	int32 PlayerIndex = INDEX_NONE;
	UPROPERTY()
	int32 BoardRevision = 0;
	UPROPERTY()
	int32 Population = 0;
	UPROPERTY()
	TArray<FLDPlacedUnit> Units;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FLDBoardCommitted, const FLDBoardCommit&);
