#include "Data/LDGameData.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	using FObject = TSharedPtr<FJsonObject>;
	using FValue = TSharedPtr<FJsonValue>;

	struct FReader
	{
		FString Error;

		void Fail(const FString& Field)
		{
			if (Error.IsEmpty())
			{
				Error = TEXT("Invalid or missing P0 data: ") + Field;
			}
		}

		FValue ReadFile(const FString& Directory, const TCHAR* File)
		{
			FString Text;
			FValue Value;
			if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(Directory, File)) ||
			    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Value))
			{
				Fail(File);
			}
			return Value;
		}

		FObject Object(const FValue& Value)
		{
			if (!Value.IsValid() || Value->Type != EJson::Object)
			{
				Fail(TEXT("object"));
				return MakeShared<FJsonObject>();
			}
			return Value->AsObject();
		}

		FValue Field(const FObject& Object, const TCHAR* Key)
		{
			const FValue* Value = Object->Values.Find(Key);
			if (!Value)
			{
				Fail(Key);
				return nullptr;
			}
			return *Value;
		}

		TArray<FValue> Array(const FValue& Value)
		{
			if (!Value.IsValid() || Value->Type != EJson::Array)
			{
				Fail(TEXT("array"));
				return {};
			}
			return Value->AsArray();
		}

		double Number(const FValue& Value)
		{
			double Result = 0;
			if (!Value.IsValid() || !Value->TryGetNumber(Result) || !FMath::IsFinite(Result))
			{
				Fail(TEXT("finite number"));
			}
			return Result;
		}

		double Number(const FObject& Object, const TCHAR* Key)
		{
			return Number(Field(Object, Key));
		}

		int32 Integer(const FValue& Value)
		{
			const double Result = Number(Value);
			if (Result < MIN_int32 || Result > MAX_int32 || FMath::FloorToDouble(Result) != Result)
			{
				Fail(TEXT("int32"));
				return 0;
			}
			return static_cast<int32>(Result);
		}

		int32 Integer(const FObject& Object, const TCHAR* Key)
		{
			return Integer(Field(Object, Key));
		}

		FString String(const FObject& Object, const TCHAR* Key)
		{
			FString Result;
			if (!Object->TryGetStringField(Key, Result))
			{
				Fail(Key);
			}
			return Result;
		}

		FName Name(const FObject& Object, const TCHAR* Key)
		{
			const FString Value = String(Object, Key);
			if (Value.IsEmpty() || FTCHARToUTF8(*Value).Length() > 64)
			{
				Fail(Key);
				return NAME_None;
			}
			return FName(*Value);
		}

		bool Boolean(const FObject& Object, const TCHAR* Key)
		{
			bool Value = false;
			if (!Object->TryGetBoolField(Key, Value))
			{
				Fail(Key);
			}
			return Value;
		}
	};
} // namespace

bool ULDGameData::LoadP0(FString& OutError)
{
	return LoadP0FromDirectory(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("LD/Data")), OutError);
}

bool ULDGameData::LoadP0FromDirectory(const FString& Directory, FString& OutError)
{
	FReader Reader;
	FLDGameRules Next;
	TMap<FName, FLDUnitRow> NextUnits;
	TMap<FName, FLDEnemyRow> NextEnemies;
	TArray<FLDWaveRow> NextWaves;
	const FObject Root = Reader.Object(Reader.ReadFile(Directory, TEXT("GameRules.json")));
	Next.SchemaVersion = Reader.Integer(Root, TEXT("SchemaVersion"));
	Next.RulesVersion = Reader.Name(Root, TEXT("RulesVersion"));
	if (Next.SchemaVersion != 2 || Next.RulesVersion != FName(TEXT("0.3.0")))
	{
		Reader.Fail(TEXT("SchemaVersion=2 and RulesVersion=0.3.0 required"));
	}
	const FObject Session = Reader.Object(Reader.Field(Root, TEXT("Session")));
	Next.HumanPlayerCount = Reader.Integer(Session, TEXT("HumanPlayerCount"));
	Next.GateCount = Reader.Integer(Session, TEXT("GateCount"));
	Next.LogicHz = Reader.Integer(Session, TEXT("LogicHz"));
	Next.PreparationSeconds = Reader.Number(Session, TEXT("PreparationSeconds"));
	Next.LoadingTimeoutSeconds = Reader.Number(Session, TEXT("LoadingTimeoutSeconds"));
	const FObject Overrides = Reader.Object(Reader.Field(Root, TEXT("P0Overrides")));
	Next.FinalWave = Reader.Integer(Overrides, TEXT("FinalWave"));
	for (const TCHAR* Disabled :
	     {TEXT("EnableSkills"), TEXT("EnableRoulette"), TEXT("EnableUpgrades"),
	                                                         TEXT("EnableCraft"),
	                                                              TEXT("EnableExchange"),
	                                                                   TEXT("EnableHunt"),
	                                                                        TEXT("EnableDungeon"),
	                                                                             TEXT("EnableMissions"),
	                                                                                  TEXT("EnableMeta"),
	                                                                                       TEXT("EnableAccountXP")})
	{
		if (Reader.Boolean(Overrides, Disabled))
		{
			Reader.Fail(Disabled);
		}
	}
	const FObject Board = Reader.Object(Reader.Field(Root, TEXT("Board")));
	Next.Columns = Reader.Integer(Board, TEXT("Columns"));
	Next.Rows = Reader.Integer(Board, TEXT("Rows"));
	Next.CellsPerPlayer = Reader.Integer(Board, TEXT("CellsPerPlayer"));
	Next.MaxUnitsPerPlayer = Reader.Integer(Board, TEXT("MaxUnitsPerPlayer"));
	Next.MaxStack = Reader.Integer(Board, TEXT("MaxStack"));
	Next.CellSizeCm = Reader.Number(Board, TEXT("CellSizeCm"));
	Next.MoveLockSeconds = Reader.Number(Board, TEXT("MoveLockSeconds"));
	Next.VisualMoveSeconds = Reader.Number(Board, TEXT("VisualMoveSeconds"));
	Next.InitialAttackDelaySeconds = Reader.Number(Board, TEXT("InitialAttackDelaySeconds"));
	for (const FValue& X : Reader.Array(Reader.Field(Board, TEXT("XCentersCm"))))
	{
		Next.XCentersCm.Add(Reader.Number(X));
	}
	for (const FValue& Player : Reader.Array(Reader.Field(Board, TEXT("YCentersByPlayer"))))
	{
		TArray<double> Values;
		for (const FValue& Y : Reader.Array(Player))
		{
			Values.Add(Reader.Number(Y));
		}
		Next.YCentersByPlayer.Add(MoveTemp(Values));
	}
	for (const FValue& Player : Reader.Array(Reader.Field(Board, TEXT("PlacementOrderByPlayer"))))
	{
		TArray<int32> Values;
		for (const FValue& Cell : Reader.Array(Player))
		{
			Values.Add(Reader.Integer(Cell));
		}
		Next.PlacementOrderByPlayer.Add(MoveTemp(Values));
	}
	const FObject Paths = Reader.Object(Reader.Field(Root, TEXT("Paths")));
	Next.LengthPerGateCm = Reader.Number(Paths, TEXT("LengthPerGateCm"));
	for (const FValue& Gate : Reader.Array(Reader.Field(Paths, TEXT("PointsByGateCm"))))
	{
		TArray<FVector> Points;
		for (const FValue& Point : Reader.Array(Gate))
		{
			const TArray<FValue> XYZ = Reader.Array(Point);
			if (XYZ.Num() != 3)
			{
				Reader.Fail(TEXT("path XYZ"));
				continue;
			}
			Points.Emplace(Reader.Number(XYZ[0]), Reader.Number(XYZ[1]), Reader.Number(XYZ[2]));
		}
		Next.PointsByGateCm.Add(MoveTemp(Points));
	}
	const FObject Defeat = Reader.Object(Reader.Field(Root, TEXT("Defeat")));
	Next.ActiveEnemyThreshold = Reader.Integer(Defeat, TEXT("ActiveEnemyThreshold"));
	Next.BossDeadlineSeconds = Reader.Number(Defeat, TEXT("BossDeadlineSeconds"));
	const FObject Economy = Reader.Object(Reader.Field(Root, TEXT("Economy")));
	Next.StartingGold = Reader.Integer(Economy, TEXT("StartingGold"));
	Next.StartingStars = Reader.Integer(Economy, TEXT("StartingStars"));
	Next.SummonBaseGold = Reader.Integer(Economy, TEXT("SummonBaseGold"));
	Next.SummonIncrementGold = Reader.Integer(Economy, TEXT("SummonIncrementGold"));
	Next.CommonSaleFractionOfNextPaidPrice = Reader.Number(Economy, TEXT("CommonSaleFractionOfNextPaidPrice"));
	Next.NormalKillGoldPerPlayer = Reader.Integer(Economy, TEXT("NormalKillGoldPerPlayer"));
	Next.BossKillGoldPerPlayer = Reader.Integer(Economy, TEXT("BossKillGoldPerPlayer"));
	Next.BossKillStarsPerPlayer = Reader.Integer(Economy, TEXT("BossKillStarsPerPlayer"));
	Next.FastBossKillSeconds = Reader.Number(Economy, TEXT("FastBossKillSeconds"));
	Next.FastBossBonusStarsPerPlayer = Reader.Integer(Economy, TEXT("FastBossBonusStarsPerPlayer"));
	const FObject Connectivity = Reader.Object(Reader.Field(Root, TEXT("Connectivity")));
	Next.CommandRatePerSecond = Reader.Integer(Connectivity, TEXT("CommandRatePerSecond"));
	Next.CommandBurst = Reader.Integer(Connectivity, TEXT("CommandBurst"));
	Next.RequestResultCacheCount = Reader.Integer(Connectivity, TEXT("RequestResultCacheCount"));
	for (const FValue& Value : Reader.Array(Reader.ReadFile(Directory, TEXT("DT_SummonProfiles.json"))))
	{
		const FObject Profile = Reader.Object(Value);
		if (Reader.Name(Profile, TEXT("Name")) == FName(TEXT("Gold_Default")))
		{
			for (const TCHAR* Grade : {TEXT("Common"), TEXT("Rare"), TEXT("Epic"), TEXT("Legendary")})
			{
				Next.GoldGradeWeights.Add(FName(Grade), Reader.Integer(Profile, *(FString(Grade) + TEXT("Weight"))));
			}
		}
	}
	if (Next.HumanPlayerCount != 2 || Next.GateCount != 2 || Next.FinalWave != 10 || Next.Columns != 6 ||
	    Next.Rows != 3 || Next.CellsPerPlayer != 18 || Next.MaxStack != 3 || Next.MaxUnitsPerPlayer != 20 ||
	    Next.CellSizeCm <= 0 || Next.LogicHz != 20 || Next.CommandRatePerSecond != 8 || Next.CommandBurst != 12 ||
	    Next.RequestResultCacheCount != 256 || Next.XCentersCm.Num() != 6 || Next.YCentersByPlayer.Num() != 2 ||
	    Next.PlacementOrderByPlayer.Num() != 2 || Next.PointsByGateCm.Num() != 2 || Next.GoldGradeWeights.Num() != 4)
	{
		Reader.Fail(TEXT("P0 dimensions, population, session or command limits"));
	}
	for (int32 Player = 0; Player < Next.PlacementOrderByPlayer.Num(); ++Player)
	{
		const TArray<int32>& Order = Next.PlacementOrderByPlayer[Player];
		TSet<int32> Seen;
		for (int32 Cell : Order)
		{
			if (Cell < Player * 18 || Cell >= (Player + 1) * 18 || Seen.Contains(Cell))
			{
				Reader.Fail(TEXT("placement ownership/uniqueness"));
			}
			Seen.Add(Cell);
		}
		if (Order.Num() != 18 || !Next.YCentersByPlayer.IsValidIndex(Player) ||
		    Next.YCentersByPlayer[Player].Num() != 3)
		{
			Reader.Fail(TEXT("placement/Y shape"));
		}
	}
	for (const FValue& Value : Reader.Array(Reader.ReadFile(Directory, TEXT("DT_Units.json"))))
	{
		const FObject Object = Reader.Object(Value);
		if (!Reader.Boolean(Object, TEXT("EnabledInP0")))
		{
			continue;
		}
		FLDUnitRow Unit;
		Unit.UnitId = Reader.Name(Object, TEXT("Name"));
		Unit.DisplayName = Reader.String(Object, TEXT("DisplayName"));
		Unit.Grade = Reader.Name(Object, TEXT("Grade"));
		Unit.DamageType = Reader.Name(Object, TEXT("DamageType"));
		Unit.AttackPresentation = Reader.Name(Object, TEXT("AttackPresentation"));
		Unit.SaleCurrency = Reader.Name(Object, TEXT("SaleCurrency"));
		Unit.SalePolicy = Reader.Name(Object, TEXT("SalePolicy"));
		Unit.SkillId = Reader.Name(Object, TEXT("SkillId"));
		Unit.VisualId = Reader.Name(Object, TEXT("VisualId"));
		Unit.BaseAttack = Reader.Number(Object, TEXT("BaseAttack"));
		Unit.AttackIntervalSeconds = Reader.Number(Object, TEXT("AttackIntervalSeconds"));
		Unit.RangeCm = Reader.Number(Object, TEXT("RangeCm"));
		Unit.MaxStack = Reader.Integer(Object, TEXT("MaxStack"));
		Unit.SaleAmount = Reader.Integer(Object, TEXT("SaleAmount"));
		Unit.bEnabledInP0 = true;
		if (Unit.UnitId.IsNone() || NextUnits.Contains(Unit.UnitId) || !Next.GoldGradeWeights.Contains(Unit.Grade) ||
		    !Unit.SkillId.IsNone() || Unit.BaseAttack <= 0 || Unit.RangeCm <= 0 || Unit.AttackIntervalSeconds <= 0 ||
		    Unit.MaxStack != 3 || Unit.SaleAmount < 0)
		{
			Reader.Fail(TEXT("unit row invariants"));
		}
		NextUnits.Add(Unit.UnitId, Unit);
	}
	for (const FValue& Value : Reader.Array(Reader.ReadFile(Directory, TEXT("DT_EnemyTypes.json"))))
	{
		const FObject Object = Reader.Object(Value);
		const FName Id = Reader.Name(Object, TEXT("Name"));
		if (Id != FName(TEXT("N01")) && Id != FName(TEXT("B01")))
		{
			continue;
		}
		FLDEnemyRow Enemy;
		Enemy.EnemyTypeId = Id;
		Enemy.Kind = Reader.Name(Object, TEXT("Kind"));
		Enemy.HPScale = Reader.Number(Object, TEXT("HPScale"));
		Enemy.FixedHP = Reader.Number(Object, TEXT("FixedHP"));
		Enemy.SpeedCmPerSec = Reader.Number(Object, TEXT("SpeedCmPerSec"));
		Enemy.Armor = Reader.Number(Object, TEXT("Armor"));
		Enemy.MagicResistance = Reader.Number(Object, TEXT("MagicResistance"));
		if (NextEnemies.Contains(Id) || Enemy.SpeedCmPerSec <= 0 || Enemy.MagicResistance < 0 ||
		    Enemy.MagicResistance >= 1 || Enemy.HPScale + Enemy.FixedHP <= 0)
		{
			Reader.Fail(TEXT("enemy row invariants"));
		}
		NextEnemies.Add(Id, Enemy);
	}
	for (const FValue& Value : Reader.Array(Reader.ReadFile(Directory, TEXT("DT_Waves.json"))))
	{
		const FObject Object = Reader.Object(Value);
		const int32 Index = Reader.Integer(Object, TEXT("WaveIndex"));
		if (Index > Next.FinalWave)
		{
			continue;
		}
		FLDWaveRow Wave;
		Wave.WaveIndex = Index;
		Wave.DurationSeconds = Reader.Number(Object, TEXT("DurationSeconds"));
		Wave.SpawnWindowSeconds = Reader.Number(Object, TEXT("SpawnWindowSeconds"));
		Wave.SpawnIntervalSeconds = Reader.Number(Object, TEXT("SpawnIntervalSeconds"));
		Wave.FirstSpawnOffsetSeconds = Reader.Number(Object, TEXT("FirstSpawnOffsetSeconds"));
		Wave.NormalCountPerGate = Reader.Integer(Object, TEXT("NormalCountPerGate"));
		Wave.NormalBaseHP = Reader.Number(Object, TEXT("NormalBaseHP"));
		Wave.SpawnProfileId = Reader.Name(Object, TEXT("SpawnProfileId"));
		Wave.BossId = Reader.Name(Object, TEXT("BossId"));
		Wave.BossCountPerGate = Reader.Integer(Object, TEXT("BossCountPerGate"));
		Wave.BossDeadlineSeconds = Reader.Number(Object, TEXT("BossDeadlineSeconds"));
		Wave.PostBossDelaySeconds = Reader.Number(Object, TEXT("PostBossDelaySeconds"));
		if (Index != NextWaves.Num() + 1 || Wave.DurationSeconds <= 0 || Wave.SpawnIntervalSeconds < 0 ||
		    (Wave.NormalCountPerGate > 0 && Wave.SpawnIntervalSeconds == 0) || Wave.NormalCountPerGate < 0 ||
		    Wave.NormalBaseHP <= 0 || (!Wave.BossId.IsNone() && !NextEnemies.Contains(Wave.BossId)))
		{
			Reader.Fail(TEXT("ordered waves and references"));
		}
		NextWaves.Add(Wave);
	}
	if (NextUnits.Num() != 16 || NextEnemies.Num() != 2 || NextWaves.Num() != 10)
	{
		Reader.Fail(TEXT("16 P0 units, 2 enemy types and 10 waves"));
	}
	OutError = Reader.Error;
	if (!OutError.IsEmpty())
	{
		return false;
	}
	Rules = MoveTemp(Next);
	Units = MoveTemp(NextUnits);
	Enemies = MoveTemp(NextEnemies);
	Waves = MoveTemp(NextWaves);
	LoadedDirectory = Directory;
	bLoaded = true;
	return true;
}

bool ULDGameData::IsLoaded() const
{
	return bLoaded;
}

const FLDGameRules& ULDGameData::GetRules() const
{
	return Rules;
}

const TArray<FLDWaveRow>& ULDGameData::GetWaves() const
{
	return Waves;
}

const TMap<FName, FLDUnitRow>& ULDGameData::GetUnits() const
{
	return Units;
}

bool ULDGameData::TryGetUnitRow(FName UnitId, FLDUnitRow& OutRow) const
{
	if (const FLDUnitRow* Row = Units.Find(UnitId))
	{
		OutRow = *Row;
		return true;
	}
	return false;
}

bool ULDGameData::TryGetEnemyRow(FName EnemyTypeId, FLDEnemyRow& OutRow) const
{
	if (const FLDEnemyRow* Row = Enemies.Find(EnemyTypeId))
	{
		OutRow = *Row;
		return true;
	}
	return false;
}
