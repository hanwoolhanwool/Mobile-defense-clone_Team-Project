#pragma once

#include "CoreMinimal.h"

struct FLDUnitRow;

struct MOBILE_DEFENSE_CLONE_API FLDTargetCandidate
{
	uint64 EnemyId = 0;
	uint64 SpawnSerial = 0;
	FVector CanonicalPosition = FVector::ZeroVector;
	bool bAlive = false;
};

// Pure rules: no World, networking, board mutations or reward dependencies.
struct MOBILE_DEFENSE_CLONE_API FLDCombatRules
{
	static bool TryCalculateDamage(const FLDUnitRow& Unit, double Armor, double MagicResistance, int32& OutDamage);
	static int32 SelectTarget(const FVector& CanonicalOrigin, double RangeCm,
	                          const TArray<FLDTargetCandidate>& Candidates);
};
