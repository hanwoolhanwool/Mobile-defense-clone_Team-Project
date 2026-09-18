#pragma once

#include "CoreMinimal.h"
#include "LDCommandTypes.generated.h"

UENUM()
enum class ELDCommandType : uint8
{
	Summon,
	Merge,
	Sell,
	Move,
	Upgrade,
	DungeonEnter,
	DungeonReturnCell,
	DungeonReturnAll,
	Craft,
	Swap,
	Lock,
	Target
};

UENUM()
enum class ELDCommandResultCode : uint8
{
	Success,
	NoChange,
	Pending,
	RequestIdConflict,
	RequestExpired,
	InvalidEpoch,
	InvalidPayload,
	InvalidData,
	InvalidCell,
	PhaseNotAllowed,
	FeatureDisabled,
	StaleBoard,
	MissingInstance,
	NotOwner,
	Locked,
	Busy,
	NoSpace,
	InsufficientResource,
	LimitReached,
	RateLimited
};

/** Fixed-size P0 wire payload. No client identity, prices, RNG or allocation-sized input. */
USTRUCT()
struct MOBILE_DEFENSE_CLONE_API FLDCommand
{
	GENERATED_BODY()

	UPROPERTY()
	uint64 ConnectionEpoch = 0;
	UPROPERTY()
	uint32 RequestId = 0;
	UPROPERTY()
	ELDCommandType CommandType = ELDCommandType::Summon;
	UPROPERTY()
	int32 ExpectedBoardRevision = 0;
	// Summon: 0 = Gold, 1 = Star. P0 rejects Star as FeatureDisabled.
	UPROPERTY()
	uint8 Source = 0;
	UPROPERTY()
	uint8 RouletteGrade = 0;
	// Selected merge instance, sold instance, or source stack member for Move.
	UPROPERTY()
	uint64 InstanceId = 0;
	UPROPERTY()
	uint64 ConsumedInstanceId0 = 0;
	UPROPERTY()
	uint64 ConsumedInstanceId1 = 0;
	UPROPERTY()
	uint64 ConsumedInstanceId2 = 0;
	UPROPERTY()
	int32 DestinationCellId = INDEX_NONE;
	// 0 = Stack. A Single mode is deliberately not represented in P0.
	UPROPERTY()
	uint8 MoveMode = 0;

	bool IsValidPayload() const;
	FLDCommand Normalized() const;
	bool HasSameContent(const FLDCommand& Other) const;
};

USTRUCT()
struct MOBILE_DEFENSE_CLONE_API FLDCommandResult
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid MatchId;
	UPROPERTY()
	uint64 ConnectionEpoch = 0;
	UPROPERTY()
	uint32 RequestId = 0;
	UPROPERTY()
	ELDCommandResultCode ResultCode = ELDCommandResultCode::InvalidPayload;
	UPROPERTY()
	int32 NewBoardRevision = 0;
	UPROPERTY()
	int32 EconomyRevision = 0;
	UPROPERTY()
	TArray<uint64> CreatedInstanceIds;
	UPROPERTY()
	TArray<uint64> MovedInstanceIds;
	UPROPERTY()
	TArray<uint64> RemovedInstanceIds;
	UPROPERTY()
	uint64 EventId = 0;
};
