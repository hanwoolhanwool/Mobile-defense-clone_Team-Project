#pragma once

#include "CoreMinimal.h"

// Facts emitted by the server. Reward amounts and recipients belong to EconomyService.
struct MOBILE_DEFENSE_CLONE_API FLDCombatDeath
{
	FGuid MatchId;
	uint64 DeathEventId = 0;
	uint64 EnemyId = 0;
	uint64 SpawnSerial = 0;
	FName EnemyTypeId = NAME_None;
	int32 SpawnWaveIndex = 0;
	double SpawnedServerSeconds = 0;
	double DeathServerSeconds = 0;
};

struct MOBILE_DEFENSE_CLONE_API FLDDamageEvent
{
	FGuid MatchId;
	uint64 DamageEventId = 0;
	uint64 SourceInstanceId = 0;
	uint64 EnemyId = 0;
	int32 Amount = 0;
	double AttackServerSeconds = 0;
};

enum class ELDDamageResult : uint8
{
	Rejected,
	Duplicate,
	Applied,
	Killed
};
