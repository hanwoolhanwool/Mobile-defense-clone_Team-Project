// Formatting sample only; not part of the Unreal build.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LDStyleExample.generated.h"

USTRUCT(BlueprintType)
struct FLDStyleExample : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LD|Example")
	FName UnitId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LD|Example", meta = (ClampMin = "0.0", Units = "s"))
	float DurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LD|Example")
	bool bEnabled = false;

	bool IsConfigured() const
	{
		return bEnabled && UnitId != NAME_None;
	}
};
