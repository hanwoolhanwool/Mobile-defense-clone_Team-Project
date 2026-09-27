#pragma once

#include "CoreMinimal.h"
#include "Data/LDMatchTypes.h"
#include "LDBattleTypes.generated.h"

UENUM(BlueprintType)
enum class ELDMatchResult : uint8
{
	None,
	Victory,
	Defeat,
	Aborted
};

UENUM(BlueprintType)
enum class ELDResultReason : uint8
{
	None,
	EnemyLimit,
	BossTimeout,
	LoadingTimeout,
	ParticipantDisconnected,
	InitializationFailure
};

USTRUCT(BlueprintType)
struct MOBILE_DEFENSE_CLONE_API FLDBossSnapshot
{
	GENERATED_BODY()
	UPROPERTY()
	uint64 EnemyId = 0;
	UPROPERTY()
	int32 RouteIndex = INDEX_NONE;
	UPROPERTY()
	double HP = 0;
	UPROPERTY()
	double MaxHP = 0;
	UPROPERTY()
	bool bAlive = false;
};

// The GameState owns this single replicated match view; widgets never derive gameplay outcomes.
USTRUCT(BlueprintType)
struct MOBILE_DEFENSE_CLONE_API FLDBattleSnapshot
{
	GENERATED_BODY()
	UPROPERTY()
	FGuid MatchId;
	UPROPERTY()
	int32 Revision = 0;
	UPROPERTY()
	ELDMatchPhase Phase = ELDMatchPhase::Loading;
	UPROPERTY()
	int32 WaveIndex = 0;
	UPROPERTY()
	int32 FinalWave = 10;
	UPROPERTY()
	double LoadingDeadlineServerSeconds = 0;
	UPROPERTY()
	double PreparationEndServerSeconds = 0;
	UPROPERTY()
	double WaveEndServerSeconds = 0;
	UPROPERTY()
	double BossDeadlineServerSeconds = 0;
	UPROPERTY()
	int32 ActiveEnemyCount = 0;
	UPROPERTY()
	int32 MaxEnemyCount = 100;
	UPROPERTY()
	bool bFinalSpawnsComplete = false;
	UPROPERTY()
	ELDMatchResult Result = ELDMatchResult::None;
	UPROPERTY()
	ELDResultReason ResultReason = ELDResultReason::None;
	UPROPERTY()
	double ResultServerSeconds = 0;
	UPROPERTY()
	TArray<FLDBossSnapshot> Bosses;

	bool IsTerminal() const
	{
		return Phase == ELDMatchPhase::Result || Phase == ELDMatchPhase::Aborted;
	}
};
