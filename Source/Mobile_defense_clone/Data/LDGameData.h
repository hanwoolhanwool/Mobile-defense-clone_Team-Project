#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LDGameData.generated.h"

// Plain value snapshots: JSON Name becomes UnitId, not a second reflected DataTable row key.
struct MOBILE_DEFENSE_CLONE_API FLDUnitRow
{
	FName UnitId = NAME_None;
	FString DisplayName;
	FName Grade = NAME_None;
	FName DamageType = NAME_None;
	FName AttackPresentation = NAME_None;
	FName SaleCurrency = NAME_None;
	FName SalePolicy = NAME_None;
	FName SkillId = NAME_None;
	FName VisualId = NAME_None;
	double BaseAttack = 0;
	double AttackIntervalSeconds = 0;
	double RangeCm = 0;
	int32 MaxStack = 0;
	int32 SaleAmount = 0;
	bool bEnabledInP0 = false;
};

struct MOBILE_DEFENSE_CLONE_API FLDEnemyRow
{
	FName EnemyTypeId = NAME_None;
	FName Kind = NAME_None;
	double HPScale = 0;
	double FixedHP = 0;
	double SpeedCmPerSec = 0;
	double Armor = 0;
	double MagicResistance = 0;
};

struct MOBILE_DEFENSE_CLONE_API FLDWaveRow
{
	int32 WaveIndex = 0;
	double DurationSeconds = 0;
	double SpawnWindowSeconds = 0;
	double SpawnIntervalSeconds = 0;
	double FirstSpawnOffsetSeconds = 0;
	int32 NormalCountPerGate = 0;
	double NormalBaseHP = 0;
	FName SpawnProfileId = NAME_None;
	FName BossId = NAME_None;
	int32 BossCountPerGate = 0;
	double BossDeadlineSeconds = 0;
	double PostBossDelaySeconds = 0;
};

struct MOBILE_DEFENSE_CLONE_API FLDGameRules
{
	int32 SchemaVersion = 0;
	FName RulesVersion = NAME_None;
	int32 HumanPlayerCount = 0;
	int32 GateCount = 0;
	int32 FinalWave = 0;
	int32 LogicHz = 0;
	double PreparationSeconds = 0;
	double LoadingTimeoutSeconds = 0;
	int32 Columns = 0;
	int32 Rows = 0;
	int32 CellsPerPlayer = 0;
	int32 MaxUnitsPerPlayer = 0;
	int32 MaxStack = 0;
	double CellSizeCm = 0;
	double MoveLockSeconds = 0;
	double VisualMoveSeconds = 0;
	double InitialAttackDelaySeconds = 0;
	TArray<double> XCentersCm;
	TArray<TArray<double>> YCentersByPlayer;
	TArray<TArray<int32>> PlacementOrderByPlayer;
	TArray<TArray<FVector>> PointsByGateCm;
	double LengthPerGateCm = 0;
	int32 ActiveEnemyThreshold = 0;
	double BossDeadlineSeconds = 0;
	int32 StartingGold = 0;
	int32 StartingStars = 0;
	int32 SummonBaseGold = 0;
	int32 SummonIncrementGold = 0;
	double CommonSaleFractionOfNextPaidPrice = 0;
	int32 NormalKillGoldPerPlayer = 0;
	int32 BossKillGoldPerPlayer = 0;
	int32 BossKillStarsPerPlayer = 0;
	double FastBossKillSeconds = 0;
	int32 FastBossBonusStarsPerPlayer = 0;
	int32 CommandRatePerSecond = 0;
	int32 CommandBurst = 0;
	int32 RequestResultCacheCount = 0;
	TMap<FName, int32> GoldGradeWeights;
};

UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDGameData : public UObject
{
	GENERATED_BODY()

public:
	bool LoadP0(FString& OutError);
	bool LoadP0FromDirectory(const FString& Directory, FString& OutError);
	bool IsLoaded() const;
	const FLDGameRules& GetRules() const;
	const TArray<FLDWaveRow>& GetWaves() const;
	const TMap<FName, FLDUnitRow>& GetUnits() const;
	bool TryGetUnitRow(FName UnitId, FLDUnitRow& OutRow) const;
	bool TryGetEnemyRow(FName EnemyTypeId, FLDEnemyRow& OutRow) const;

private:
	// Replaced together only after all required files and references pass validation.
	FLDGameRules Rules;
	TMap<FName, FLDUnitRow> Units;
	TMap<FName, FLDEnemyRow> Enemies;
	TArray<FLDWaveRow> Waves;
	FString LoadedDirectory;
	bool bLoaded = false;
};
