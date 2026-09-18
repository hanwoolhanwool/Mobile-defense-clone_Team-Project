#pragma once

#include "CoreMinimal.h"
#include "LDMatchTypes.generated.h"

UENUM(BlueprintType)
enum class ELDMatchPhase : uint8
{
	Loading,
	Preparing,
	Running,
	Result,
	Aborted
};

USTRUCT(BlueprintType)
struct MOBILE_DEFENSE_CLONE_API FLDMatchContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LD|Match")
	FGuid MatchId;
	UPROPERTY(BlueprintReadOnly, Category = "LD|Match")
	FName RulesVersion = NAME_None;

	bool IsValid() const
	{
		return MatchId.IsValid() && !RulesVersion.IsNone();
	}
};

USTRUCT()
struct MOBILE_DEFENSE_CLONE_API FLDParticipantContext
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid MatchId;
	UPROPERTY()
	int32 PlayerIndex = INDEX_NONE;
	UPROPERTY()
	uint64 ConnectionEpoch = 0;

	bool IsValid() const
	{
		return MatchId.IsValid() && PlayerIndex >= 0 && PlayerIndex < 2 && ConnectionEpoch != 0;
	}
};
