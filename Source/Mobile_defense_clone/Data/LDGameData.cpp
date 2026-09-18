#include "Data/LDGameData.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	class FCheckedObject
	{
	public:
		FCheckedObject(TSharedPtr<FJsonObject> InObject, FString InPath, FString& InError)
		    : Object(MoveTemp(InObject)), Path(MoveTemp(InPath)), Error(InError)
		{
		}

		bool Fail(const FString& Field, const FString& Reason) const
		{
			if (Error.IsEmpty())
			{
				Error = Path + TEXT(".") + Field + TEXT(": ") + Reason;
			}
			return false;
		}

		bool Number(const TCHAR* Field, double& OutValue, double Minimum = 0, double Maximum = 1.e9) const
		{
			if (!Object.IsValid() || !Object->TryGetNumberField(Field, OutValue) || !FMath::IsFinite(OutValue) ||
			    OutValue < Minimum || OutValue > Maximum)
			{
				return Fail(Field, TEXT("required finite number outside allowed range"));
			}
			return true;
		}

		bool Integer(const TCHAR* Field, int32& OutValue, int32 Minimum = 0, int32 Maximum = 1000000) const
		{
			double Value = 0;
			if (!Number(Field, Value, Minimum, Maximum))
			{
				return false;
			}
			if (Value != FMath::FloorToDouble(Value))
			{
				return Fail(Field, TEXT("required integer"));
			}
			OutValue = static_cast<int32>(Value);
			return true;
		}

		bool String(const TCHAR* Field, FString& OutValue) const
		{
			return (Object.IsValid() && Object->TryGetStringField(Field, OutValue) && !OutValue.IsEmpty()) ||
			       Fail(Field, TEXT("required nonempty string"));
		}

		bool Name(const TCHAR* Field, FName& OutValue) const
		{
			FString Value;
			if (!String(Field, Value))
			{
				return false;
			}
			OutValue = FName(*Value);
			return true;
		}

		bool Boolean(const TCHAR* Field, bool& OutValue) const
		{
			return (Object.IsValid() && Object->TryGetBoolField(Field, OutValue)) ||
			       Fail(Field, TEXT("required boolean"));
		}

		bool RequireBool(const TCHAR* Field, bool Expected) const
		{
			bool Actual = false;
			return Boolean(Field, Actual) && (Actual == Expected || Fail(Field, TEXT("unsupported P0 policy")));
		}

		bool RequireString(const TCHAR* Field, const TCHAR* Expected) const
		{
			FString Actual;
			return String(Field, Actual) && (Actual == Expected || Fail(Field, TEXT("unsupported P0 policy")));
		}

		bool RequireStrings(const TCHAR* Field, const TArray<FString>& Expected) const
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!Array(Field, Values) || Values->Num() != Expected.Num())
			{
				return Fail(Field, TEXT("unsupported P0 list"));
			}
			for (int32 Index = 0; Index < Expected.Num(); ++Index)
			{
				FString Value;
				if (!(*Values)[Index]->TryGetString(Value) || Value != Expected[Index])
				{
					return Fail(Field, TEXT("unsupported P0 list entry"));
				}
			}
			return true;
		}

		bool RequireInteger(const TCHAR* Field, int32 Expected) const
		{
			int32 Value = 0;
			return Integer(Field, Value, Expected, Expected);
		}

		bool Array(const TCHAR* Field, const TArray<TSharedPtr<FJsonValue>>*& OutValues) const
		{
			return (Object.IsValid() && Object->TryGetArrayField(Field, OutValues)) ||
			       Fail(Field, TEXT("required array"));
		}

		FCheckedObject Child(const TCHAR* Field) const
		{
			const TSharedPtr<FJsonObject>* ChildObject = nullptr;
			if (!Object.IsValid() || !Object->TryGetObjectField(Field, ChildObject))
			{
				Fail(Field, TEXT("required object"));
				return FCheckedObject(nullptr, Path + TEXT(".") + Field, Error);
			}
			return FCheckedObject(*ChildObject, Path + TEXT(".") + Field, Error);
		}

		bool IsNull(const TCHAR* Field) const
		{
			const TSharedPtr<FJsonValue> Value = Object.IsValid() ? Object->TryGetField(Field) : nullptr;
			return (Value.IsValid() && Value->IsNull()) || Fail(Field, TEXT("P0 requires null (no cap)"));
		}

	private:
		TSharedPtr<FJsonObject> Object;
		FString Path;
		FString& Error;
	};

	bool LoadObject(const FString& File, TSharedPtr<FJsonObject>& OutObject, FString& OutError)
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *File))
		{
			OutError = File + TEXT(": cannot read required runtime JSON");
			return false;
		}
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, OutObject) || !OutObject.IsValid())
		{
			OutError = File + TEXT(": invalid JSON object: ") + Reader->GetErrorMessage();
			return false;
		}
		return true;
	}

	bool LoadRows(const FString& File, TArray<TSharedPtr<FJsonObject>>& OutRows, FString& OutError)
	{
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *File))
		{
			OutError = File + TEXT(": cannot read required runtime JSON");
			return false;
		}
		TArray<TSharedPtr<FJsonValue>> Values;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Values))
		{
			OutError = File + TEXT(": invalid JSON row array: ") + Reader->GetErrorMessage();
			return false;
		}
		TSet<FName> SeenNames;
		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			const TSharedPtr<FJsonObject>* Row = nullptr;
			if (!Values[Index].IsValid() || !Values[Index]->TryGetObject(Row))
			{
				OutError = FString::Printf(TEXT("%s[%d]: required row object"), *File, Index);
				return false;
			}
			FCheckedObject Checked(*Row, FString::Printf(TEXT("%s[%d]"), *File, Index), OutError);
			FName Name;
			if (!Checked.Name(TEXT("Name"), Name) || Name.IsNone() || SeenNames.Contains(Name))
			{
				return Checked.Fail(TEXT("Name"), TEXT("empty or duplicate row identifier"));
			}
			SeenNames.Add(Name);
			OutRows.Add(*Row);
		}
		return true;
	}

	bool ReadNumberArray(const FCheckedObject& Object, const TCHAR* Field, int32 Count, TArray<double>& OutValues)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object.Array(Field, Values) || Values->Num() != Count)
		{
			return Object.Fail(Field, TEXT("incorrect coordinate count"));
		}
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			double Number = 0;
			if (!Value.IsValid() || !Value->TryGetNumber(Number) || !FMath::IsFinite(Number))
			{
				return Object.Fail(Field, TEXT("required finite coordinate"));
			}
			OutValues.Add(Number);
		}
		return true;
	}

	bool ParseLayout(const FCheckedObject& Root, FLDGameRules& Rules)
	{
		const FCheckedObject Board = Root.Child(TEXT("Board"));
		if (!Board.Integer(
		        TEXT("Columns"), Rules.Columns, 6, 6) ||
		        !Board.Integer(
		            TEXT("Rows"), Rules.Rows, 3, 3) ||
		            !Board.Integer(
		                TEXT("CellsPerPlayer"), Rules.CellsPerPlayer, 18, 18) ||
		                !Board.Integer(
		                    TEXT("MaxUnitsPerPlayer"), Rules.MaxUnitsPerPlayer, 20, 20) ||
		                    !Board.Integer(
		                        TEXT("MaxStack"), Rules.MaxStack, 3, 3) ||
		                        !Board.Number(TEXT("CellSizeCm"), Rules.CellSizeCm, 1) ||
		                                      !Board.Number(
		                                          TEXT("MoveLockSeconds"), Rules.MoveLockSeconds) ||
		                                          !Board.Number(
		                                              TEXT("VisualMoveSeconds"),
		                                                   Rules.VisualMoveSeconds) ||
		                                              !Board.Number(
		                                                  TEXT("InitialAttackDelaySeconds"),
		                                                       Rules.InitialAttackDelaySeconds) ||
		                                                  !ReadNumberArray(
		                                                      Board, TEXT("XCentersCm"),
		                                                                  Rules.Columns,
		                                                                  Rules.XCentersCm) ||
		                                                      !Board.RequireBool(
		                                                          TEXT("TransferPreservesInstanceAndTimers"), true) ||
		                                                          !Board.RequireBool(
		                                                              TEXT("ManualIndividualSplit"), false) ||
		                                                              !Board.RequireBool(
		                                                                  TEXT("MergeUsesSelectedCell"), false) ||
		                                                                  !Board.RequireInteger(
		                                                                      TEXT("MaxPartialStacksPerUnitPerArea"),
		                                                                           1) ||
		                                                                      !Board.RequireString(TEXT("MoveMode"),
		                                                                          TEXT("WholeStackDragSwap")) ||
		                                                                          !Board.RequireString(TEXT("PlacementPolicy"),
		                                                                              TEXT("SameUnitPartialThenLocalColumnTopDown")) ||
		                                                                              !Board.RequireString(TEXT("SaleCompaction"),
		                                                                                  TEXT("TransferFromExistingPartialToSoldStack")))
		{
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* YArrays = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Orders = nullptr;
		if (!Board.Array(TEXT("YCentersByPlayer"), YArrays) || YArrays->Num() != 2 ||
		                 !Board.Array(TEXT("PlacementOrderByPlayer"), Orders) || Orders->Num() != 2)
		{
			return Board.Fail(TEXT("YCentersByPlayer/PlacementOrderByPlayer"), TEXT("requires two boards"));
		}
		for (int32 Player = 0; Player < 2; ++Player)
		{
			const TArray<TSharedPtr<FJsonValue>>* Coordinates = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* Cells = nullptr;
			if (!(*YArrays)[Player]->TryGetArray(Coordinates) || Coordinates->Num() != Rules.Rows ||
			    !(*Orders)[Player]->TryGetArray(Cells) || Cells->Num() != Rules.CellsPerPlayer)
			{
				return Board.Fail(TEXT("YCentersByPlayer/PlacementOrderByPlayer"), TEXT("invalid row shape"));
			}
			TArray<double> PlayerY;
			for (const TSharedPtr<FJsonValue>& Value : *Coordinates)
			{
				double Coordinate = 0;
				if (!Value->TryGetNumber(Coordinate) || !FMath::IsFinite(Coordinate))
				{
					return Board.Fail(TEXT("YCentersByPlayer"), TEXT("invalid coordinate"));
				}
				PlayerY.Add(Coordinate);
			}
			Rules.YCentersByPlayer.Add(MoveTemp(PlayerY));
			TArray<int32> PlayerOrder;
			for (const TSharedPtr<FJsonValue>& Value : *Cells)
			{
				double Cell = 0;
				if (!Value->TryGetNumber(Cell) || !FMath::IsFinite(Cell) || Cell != FMath::FloorToDouble(Cell) ||
				    Cell < Player * 18 || Cell >= (Player + 1) * 18 || PlayerOrder.Contains(static_cast<int32>(Cell)))
				{
					return Board.Fail(TEXT("PlacementOrderByPlayer"), TEXT("must be a permutation of owned CellIds"));
				}
				PlayerOrder.Add(static_cast<int32>(Cell));
			}
			Rules.PlacementOrderByPlayer.Add(MoveTemp(PlayerOrder));
		}
		for (int32 Column = 0; Column < Rules.Columns; ++Column)
		{
			const double Expected = (Column - (Rules.Columns - 1) * 0.5) * Rules.CellSizeCm;
			if (!FMath::IsNearlyEqual(Rules.XCentersCm[Column], Expected, 0.01))
			{
				return Board.Fail(TEXT("XCentersCm"), TEXT("columns must be centered and one CellSizeCm apart"));
			}
		}
		for (int32 Row = 0; Row < Rules.Rows; ++Row)
		{
			if (!FMath::IsNearlyEqual(Rules.YCentersByPlayer[0][Row], (Row - Rules.Rows) * Rules.CellSizeCm, 0.01) ||
			    !FMath::IsNearlyEqual(Rules.YCentersByPlayer[1][Row], (Row + 1) * Rules.CellSizeCm, 0.01))
			{
				return Board.Fail(TEXT("YCentersByPlayer"),
				                       TEXT("boards must be separated by their shared central path"));
			}
		}
		const FCheckedObject Paths = Root.Child(TEXT("Paths"));
		const TArray<TSharedPtr<FJsonValue>>* Routes = nullptr;
		if (!Paths.RequireBool(TEXT("Closed"), true) ||
		                       !Paths.RequireBool(TEXT("LinearSegments"), true) ||
		                                          !Paths.Number(TEXT("LengthPerGateCm"), Rules.LengthPerGateCm, 1) ||
		                                                        !Paths.Array(TEXT("PointsByGateCm"), Routes) ||
		                                                                     Routes->Num() != 2)
		{
			return Paths.Fail(TEXT("PointsByGateCm"), TEXT("requires two closed linear routes"));
		}
		for (const TSharedPtr<FJsonValue>& Route : *Routes)
		{
			const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
			if (!Route->TryGetArray(Points) || Points->Num() != 4)
			{
				return Paths.Fail(TEXT("PointsByGateCm"), TEXT("requires four corners per route"));
			}
			TArray<FVector> RoutePoints;
			for (const TSharedPtr<FJsonValue>& Point : *Points)
			{
				const TArray<TSharedPtr<FJsonValue>>* XYZ = nullptr;
				double Values[3] = {0, 0, 0};
				if (!Point->TryGetArray(XYZ) || XYZ->Num() != 3)
				{
					return Paths.Fail(TEXT("PointsByGateCm"), TEXT("requires XYZ"));
				}
				for (int32 Axis = 0; Axis < 3; ++Axis)
				{
					if (!(*XYZ)[Axis]->TryGetNumber(Values[Axis]) || !FMath::IsFinite(Values[Axis]))
					{
						return Paths.Fail(TEXT("PointsByGateCm"), TEXT("invalid route coordinate"));
					}
				}
				RoutePoints.Add(FVector(Values[0], Values[1], Values[2]));
			}
			double Length = 0;
			for (int32 Index = 0; Index < RoutePoints.Num(); ++Index)
			{
				const double Segment = FVector::Distance(RoutePoints[Index], RoutePoints[(Index + 1) % 4]);
				if (Segment <= UE_SMALL_NUMBER)
				{
					return Paths.Fail(TEXT("PointsByGateCm"), TEXT("zero length segment"));
				}
				Length += Segment;
			}
			if (!FMath::IsNearlyEqual(Length, Rules.LengthPerGateCm, 0.01))
			{
				return Paths.Fail(TEXT("LengthPerGateCm"), TEXT("does not match closed route length"));
			}
			Rules.PointsByGateCm.Add(MoveTemp(RoutePoints));
		}
		if (!Rules.PointsByGateCm[0][1].Equals(Rules.PointsByGateCm[1][1]) ||
		    !Rules.PointsByGateCm[0][2].Equals(Rules.PointsByGateCm[1][2]))
		{
			return Paths.Fail(TEXT("PointsByGateCm"), TEXT("central segment must have identical direction"));
		}
		const double OuterX = (Rules.Columns + 1) * Rules.CellSizeCm * 0.5;
		const double OuterY = (Rules.Rows + 1) * Rules.CellSizeCm;
		for (int32 Route = 0; Route < 2; ++Route)
		{
			const double Y = Route == 0 ? -OuterY : OuterY;
			const FVector Expected[4] = {FVector(OuterX, Y, 0), FVector(OuterX, 0, 0), FVector(-OuterX, 0, 0),
			                             FVector(-OuterX, Y, 0)};
			for (int32 Point = 0; Point < 4; ++Point)
			{
				if (!Rules.PointsByGateCm[Route][Point].Equals(Expected[Point], 0.01))
				{
					return Paths.Fail(TEXT("PointsByGateCm"), TEXT("route must surround its own board"));
				}
			}
		}
		return true;
	}

	bool ParseRules(const FCheckedObject& Root, FLDGameRules& Rules)
	{
		if (!Root.Integer(TEXT("SchemaVersion"), Rules.SchemaVersion, 2, 2) ||
		                  !Root.Name(TEXT("RulesVersion"), Rules.RulesVersion) ||
		                             Rules.RulesVersion != FName(TEXT("0.3.0")))
		{
			return Root.Fail(TEXT("RulesVersion"), TEXT("requires SchemaVersion 2 / RulesVersion 0.3.0"));
		}
		const FCheckedObject Session = Root.Child(TEXT("Session"));
		const FCheckedObject P0 = Root.Child(TEXT("P0Overrides"));
		if (!Session.Integer(
		        TEXT("HumanPlayerCount"), Rules.HumanPlayerCount, 2, 2) ||
		        !Session.Integer(
		            TEXT("GateCount"), Rules.GateCount, 2, 2) ||
		            !Session.Integer(
		                TEXT("LogicHz"), Rules.LogicHz, 20, 20) ||
		                !Session.Number(
		                    TEXT("PreparationSeconds"), Rules.PreparationSeconds) ||
		                    !Session.Number(
		                        TEXT("LoadingTimeoutSeconds"), Rules.LoadingTimeoutSeconds, 1) ||
		                        !P0.Integer(TEXT("FinalWave"), Rules.FinalWave, 10, 10) ||
		                                    !P0.RequireBool(
		                                        TEXT("EnableStars"), true) ||
		                                        !P0.RequireStrings(TEXT("AllowedGrades"),
		                                            {TEXT("Common"), TEXT("Rare"), TEXT("Epic"), TEXT("Legendary")}) ||
		                                                  !P0.RequireStrings(TEXT("MergeInputGrades"),
		                                                      {TEXT("Common"), TEXT("Rare"), TEXT("Epic")}) ||
		                                                       !P0.RequireStrings(TEXT("AllowedNormalEnemyIds"), {TEXT("N01")}) ||
		                                                           !P0.RequireString(TEXT("GoldProfile"), TEXT("Gold_Default")) ||
		                                                               !P0.RequireString(TEXT("NormalSpawnProfileId"),
		                                                                                      TEXT("Early")))
		{
			return false;
		}
		const TCHAR*
		    DisabledFeatures[] =
		        {
		            TEXT("EnableSkills"),   TEXT("EnableRoulette"),  TEXT("EnableUpgrades"), TEXT("EnableCraft"),
		                                                                                          TEXT("EnableExchange"), TEXT("EnableAccountXP"), TEXT("EnableHunt"),     TEXT("EnableDungeon"),
		                                                                                                                                             TEXT("EnableMissions"), TEXT("EnableMeta")};
		for (const TCHAR* Feature : DisabledFeatures)
		{
			if (!P0.RequireBool(Feature, false))
			{
				return false;
			}
		}
		const FCheckedObject Defeat = Root.Child(TEXT("Defeat"));
		const FCheckedObject Victory = Root.Child(TEXT("Victory"));
		const FCheckedObject Timing = Root.Child(TEXT("Timing"));
		const FCheckedObject Economy = Root.Child(TEXT("Economy"));
		const FCheckedObject Connectivity = Root.Child(TEXT("Connectivity"));
		if (!Defeat.Integer(
		        TEXT("ActiveEnemyThreshold"), Rules.ActiveEnemyThreshold, 100, 100) ||
		        !Defeat.RequireInteger(
		            TEXT("ContinuousSeconds"), 0) ||
		            !Defeat.RequireStrings(TEXT("CountedEnemyKinds"), {TEXT("Normal")}) ||
		                !Defeat.Number(
		                    TEXT("BossDeadlineSeconds"), Rules.BossDeadlineSeconds, 60, 60) ||
		                    !Defeat.RequireBool(
		                        TEXT("RequireBothWaveBossesDead"), true) ||
		                        !Victory.RequireBool(
		                            TEXT("RequiresFinalSpawnsComplete"), true) ||
		                            !Victory.RequireBool(
		                                TEXT("RequiresNoNormalEnemies"),
		                                     true) ||
		                                !Victory.RequireBool(
		                                    TEXT("RequiresBothFinalBossesDead"),
		                                         true) ||
		                                    !Timing.RequireBool(
		                                        TEXT("DamageAtDeadlineCounts"),
		                                             true) ||
		                                        !Timing.RequireBool(
		                                            TEXT("CommandsSerial"),
		                                                 true) ||
		                                            !Timing.RequireBool(
		                                                TEXT("RejectedRequestConsumesRng"), false) ||
		                                                !Timing.RequireString(TEXT("Order"),
		                                                    TEXT("CommandsThenDueDamageThenDeathsAndRewardsThenSpawnsAndCountDefeatThenDeadlinesThenVictoryThenNextWave")) ||
		                                                    !Economy.Integer(
		                                                        TEXT("StartingGold"), Rules.StartingGold, 100, 100) ||
		                                                        !Economy.Integer(
		                                                            TEXT("StartingStars"), Rules.StartingStars, 0, 0) ||
		                                                            !Economy.Integer(
		                                                                TEXT("SummonBaseGold"), Rules.SummonBaseGold,
		                                                                     20, 20) ||
		                                                                !Economy.Integer(
		                                                                    TEXT("SummonIncrementGold"),
		                                                                         Rules.SummonIncrementGold, 2, 2) ||
		                                                                    !Economy.IsNull(
		                                                                        TEXT("SummonMaxGold")) ||
		                                                                        !Economy.Number(
		                                                                            TEXT("CommonSaleFractionOfNextPaidPrice"),
		                                                                                Rules
		                                                                                    .CommonSaleFractionOfNextPaidPrice,
		                                                                                0.5, 0.5) ||
		                                                                            !Economy.Integer(
		                                                                                TEXT("NormalKillGoldPerPlayer"),
		                                                                                    Rules
		                                                                                        .NormalKillGoldPerPlayer,
		                                                                                    1, 1) ||
		                                                                                !Economy.Integer(
		                                                                                    TEXT("BossKillGoldPerPlayer"),
		                                                                                        Rules
		                                                                                            .BossKillGoldPerPlayer,
		                                                                                        100, 100) ||
		                                                                                    !Economy.Integer(
		                                                                                        TEXT("BossKillStarsPerPlayer"),
		                                                                                            Rules
		                                                                                                .BossKillStarsPerPlayer,
		                                                                                            2, 2) ||
		                                                                                        !Economy.Number(
		                                                                                            TEXT("FastBossKillSeconds"),
		                                                                                                Rules
		                                                                                                    .FastBossKillSeconds,
		                                                                                                30, 30) ||
		                                                                                            !Economy.Integer(
		                                                                                                TEXT("FastBossBonusStarsPerPlayer"),
		                                                                                                    Rules
		                                                                                                        .FastBossBonusStarsPerPlayer,
		                                                                                                    1, 1) ||
		                                                                                                !Economy.RequireBool(
		                                                                                                    TEXT("RewardsToBothPlayers"),
		                                                                                                        true) ||
		                                                                                                    !Economy.RequireBool(
		                                                                                                        TEXT("WaveBaseRewardsEnabled"),
		                                                                                                            false) ||
		                                                                                                        !Economy.RequireBool(
		                                                                                                            TEXT("FreeSummonIncrementsPaidCounter"),
		                                                                                                                false) ||
		                                                                                                            !Economy
		                                                                                                                 .RequireString(TEXT("CommonSaleRounding"),
		                                                                                                                     TEXT("Floor")) ||
		                                                                                                                     !Economy
		                                                                                                                          .RequireString(TEXT("SummonAdmission"),
		                                                                                                                              TEXT("BeforeRngRequirePopulationAndCapacityForEveryPossibleUnit")) ||
		                                                                                                                              !Connectivity
		                                                                                                                                   .Integer(
		                                                                                                                                       TEXT("CommandRatePerSecond"),
		                                                                                                                                           Rules
		                                                                                                                                               .CommandRatePerSecond,
		                                                                                                                                           1) ||
		                                                                                                                                       !Connectivity
		                                                                                                                                            .Integer(
		                                                                                                                                                TEXT("CommandBurst"),
		                                                                                                                                                    Rules
		                                                                                                                                                        .CommandBurst,
		                                                                                                                                                    1) ||
		                                                                                                                                                !Connectivity
		                                                                                                                                                     .Integer(TEXT("RequestResultCacheCount"),
		                                                                                                                                                         Rules
		                                                                                                                                                             .RequestResultCacheCount,
		                                                                                                                                                         256,
		                                                                                                                                                         256))
		{
			return false;
		}
		return ParseLayout(Root, Rules);
	}

	bool ParseUnits(const TArray<TSharedPtr<FJsonObject>>& Rows, const FLDGameRules& Rules,
	                TMap<FName, FLDUnitRow>& OutUnits, FString& Error)
	{
		TMap<FName, int32> GradeCounts;
		for (const TSharedPtr<FJsonObject>& Object : Rows)
		{
			FCheckedObject Checked(Object, TEXT("DT_Units.json") + TEXT("[") + Object->GetStringField(TEXT("Name")) +
			                                                                                          TEXT("]"), Error);
			FLDUnitRow Row;
			if (!Checked.Boolean(TEXT("EnabledInP0"), Row.bEnabledInP0))
			{
				return false;
			}
			if (!Row.bEnabledInP0)
			{
				continue;
			}
			double RangeCells = 0;
			if (!Checked.Name(
			        TEXT("Name"), Row.UnitId) ||
			        !Checked.String(
			            TEXT("DisplayName"), Row.DisplayName) ||
			            !Checked.Name(
			                TEXT("Grade"), Row.Grade) ||
			                !Checked.Name(
			                    TEXT("DamageType"), Row.DamageType) ||
			                    !Checked.Name(
			                        TEXT("AttackPresentation"), Row.AttackPresentation) ||
			                        !Checked.Name(
			                            TEXT("SaleCurrency"), Row.SaleCurrency) ||
			                            !Checked.Name(
			                                TEXT("SalePolicy"), Row.SalePolicy) ||
			                                !Checked.Name(TEXT("SkillId"), Row.SkillId) ||
			                                              !Checked.Name(
			                                                  TEXT("VisualId"), Row.VisualId) ||
			                                                  !Checked.Number(
			                                                      TEXT("BaseAttack"), Row.BaseAttack,
			                                                           0.001) ||
			                                                      !Checked.Number(
			                                                          TEXT("AttackIntervalSeconds"),
			                                                               Row.AttackIntervalSeconds, 0.125) ||
			                                                          !Checked.Number(
			                                                              TEXT("RangeCm"),
			                                                                   Row.RangeCm, 0.001) ||
			                                                              !Checked.Number(
			                                                                  TEXT("RangeCells"), RangeCells, 0.001) ||
			                                                                  !Checked.Integer(
			                                                                      TEXT("MaxStack"),
			                                                                           Row.MaxStack, 3, 3) ||
			                                                                      !Checked.Integer(TEXT("SaleAmount"),
			                                                                                            Row.SaleAmount))
			{
				return false;
			}
			if (Row.Grade != TEXT("Common") && Row.Grade != TEXT("Rare") &&
			                                                     Row.Grade != TEXT("Epic") &&
			                                                                       Row.Grade != TEXT("Legendary"))
			{
				return Checked.Fail(TEXT("Grade"), TEXT("grade excluded from P0"));
			}
			if (!Row.SkillId.IsNone() || Row.VisualId.IsNone() ||
			    (Row.DamageType != TEXT("Physical") && Row.DamageType != TEXT("Magic")) ||
			     (Row.AttackPresentation != TEXT("Melee") && Row.AttackPresentation != TEXT("Projectile")) ||
			      !FMath::IsNearlyEqual(Row.RangeCm, RangeCells * Rules.CellSizeCm, 0.01))
			{
				return Checked.Fail(TEXT("SkillId/VisualId/DamageType/AttackPresentation/RangeCm"),
				                         TEXT("invalid P0 row"));
			}
			const int32 SaleStars = Row.Grade == TEXT("Rare") ? 1 : (Row.Grade == TEXT("Epic") ? 2 : 4);
			if (Row.Grade == TEXT("Common"))
			{
				if (Row.SaleCurrency != TEXT("Gold") || Row.SalePolicy != TEXT("HalfNextPaidSummonFloor") ||
				                                                               Row.SaleAmount != 0)
				{
					return Checked.Fail(TEXT("SalePolicy"), TEXT("Common uses half next paid summon price"));
				}
			}
			else if (Row.SaleCurrency != TEXT("Stars") || Row.SalePolicy != TEXT("Fixed") ||
			                                                                     Row.SaleAmount != SaleStars)
			{
				return Checked.Fail(TEXT("SaleAmount"), TEXT("Rare/Epic/Legendary require 1/2/4 Stars"));
			}
			++GradeCounts.FindOrAdd(Row.Grade);
			OutUnits.Add(Row.UnitId, MoveTemp(Row));
		}
		if (OutUnits.Num() != 16 || GradeCounts.Num() != 4)
		{
			Error = TEXT("DT_Units.json: P0 requires exactly 16 active units across four grades");
			return false;
		}
		for (const TPair<FName, int32>& Pair : GradeCounts)
		{
			if (Pair.Value != 4)
			{
				Error = TEXT("DT_Units.json.Grade: P0 requires four units per grade");
				return false;
			}
		}
		return true;
	}

	bool ParseEnemies(const TArray<TSharedPtr<FJsonObject>>& Rows, TMap<FName, FLDEnemyRow>& OutEnemies, FString& Error)
	{
		for (const TSharedPtr<FJsonObject>& Object : Rows)
		{
			FCheckedObject Checked(Object, TEXT("DT_EnemyTypes.json[") + Object->GetStringField(TEXT("Name")) +
			                                                                                    TEXT("]"), Error);
			FLDEnemyRow Row;
			if (!Checked.Name(TEXT("Name"), Row.EnemyTypeId))
			{
				return false;
			}
			if (Row.EnemyTypeId != TEXT("N01") && Row.EnemyTypeId != TEXT("B01"))
			{
				continue;
			}
			if (!Checked.Name(TEXT("Kind"), Row.Kind) ||
			                  !Checked.Number(
			                      TEXT("HPScale"), Row.HPScale) ||
			                      !Checked.Number(
			                          TEXT("FixedHP"), Row.FixedHP) ||
			                          !Checked.Number(
			                              TEXT("SpeedCmPerSec"), Row.SpeedCmPerSec, 0.001) ||
			                              !Checked.Number(
			                                  TEXT("Armor"), Row.Armor, -50) ||
			                                  !Checked.Number(TEXT("MagicResistance"), Row.MagicResistance, 0, 0.75) ||
			                                                  !Checked.RequireString(TEXT("AbilityId"), TEXT("None")))
			{
				return false;
			}
			if ((Row.EnemyTypeId == TEXT("N01") && (Row.Kind != TEXT("Normal") || Row.HPScale <= 0)) ||
			     (Row.EnemyTypeId == TEXT("B01") && (Row.Kind != TEXT("Boss") || Row.FixedHP <= 0)))
			{
				return Checked.Fail(TEXT("Kind/HP"), TEXT("invalid active enemy"));
			}
			OutEnemies.Add(Row.EnemyTypeId, Row);
		}
		if (OutEnemies.Num() != 2)
		{
			Error = TEXT("DT_EnemyTypes.json: required N01 or B01 missing");
			return false;
		}
		return true;
	}

	bool ParseWaves(const TArray<TSharedPtr<FJsonObject>>& Rows, TArray<FLDWaveRow>& OutWaves, FString& Error)
	{
		TSet<int32> SeenWaves;
		for (const TSharedPtr<FJsonObject>& Object : Rows)
		{
			FCheckedObject Checked(Object, TEXT("DT_Waves.json[") + Object->GetStringField(TEXT("Name")) + TEXT("]"),
			                                                                                                    Error);
			FLDWaveRow Row;
			if (!Checked.Integer(TEXT("WaveIndex"), Row.WaveIndex, 1, 80) || SeenWaves.Contains(Row.WaveIndex))
			{
				return Checked.Fail(TEXT("WaveIndex"), TEXT("invalid or duplicate wave index"));
			}
			SeenWaves.Add(Row.WaveIndex);
			if (Row.WaveIndex > 10)
			{
				continue;
			}
			int32 GoldReward = 0;
			int32 StarReward = 0;
			if (!Checked.Number(
			        TEXT("DurationSeconds"), Row.DurationSeconds, 0.001) ||
			        !Checked.Number(
			            TEXT("SpawnWindowSeconds"), Row.SpawnWindowSeconds) ||
			            !Checked.Number(
			                TEXT("SpawnIntervalSeconds"), Row.SpawnIntervalSeconds) ||
			                !Checked.Number(
			                    TEXT("FirstSpawnOffsetSeconds"), Row.FirstSpawnOffsetSeconds) ||
			                    !Checked.Integer(
			                        TEXT("NormalCountPerGate"), Row.NormalCountPerGate) ||
			                        !Checked.Number(
			                            TEXT("NormalBaseHP"), Row.NormalBaseHP) ||
			                            !Checked.Name(
			                                TEXT("SpawnProfileId"), Row.SpawnProfileId) ||
			                                !Checked.Name(TEXT("BossId"), Row.BossId) ||
			                                              !Checked.Integer(
			                                                  TEXT("BossCountPerGate"), Row.BossCountPerGate) ||
			                                                  !Checked.Number(
			                                                      TEXT("BossDeadlineSeconds"),
			                                                           Row.BossDeadlineSeconds) ||
			                                                      !Checked.Number(
			                                                          TEXT("PostBossDelaySeconds"),
			                                                               Row.PostBossDelaySeconds) ||
			                                                          !Checked.Integer(
			                                                              TEXT("GoldRewardPerPlayer"), GoldReward, 0,
			                                                                   0) ||
			                                                              !Checked.Integer(TEXT("StarRewardPerPlayer"),
			                                                                                    StarReward, 0, 0))
			{
				return false;
			}
			if (Row.WaveIndex == 10)
			{
				if (Row.BossId != TEXT("B01") || Row.BossCountPerGate != 1 || Row.NormalCountPerGate != 0 ||
				                       Row.BossDeadlineSeconds != 60 || Row.SpawnWindowSeconds != 0 ||
				                       Row.SpawnIntervalSeconds != 0)
				{
					return Checked.Fail(TEXT("BossId/Count/Deadline"),
					                         TEXT("wave 10 requires one B01 per gate, 60 seconds"));
				}
			}
			else if (!Row.BossId.IsNone() || Row.BossCountPerGate != 0 || Row.NormalCountPerGate <= 0 ||
			         Row.NormalBaseHP <= 0 || Row.SpawnIntervalSeconds <= 0 ||
			         Row.SpawnProfileId !=
			             TEXT("Early") || Row.SpawnWindowSeconds > Row.DurationSeconds ||
			                  Row.FirstSpawnOffsetSeconds + (Row.NormalCountPerGate - 1) * Row.SpawnIntervalSeconds >=
			                      Row.SpawnWindowSeconds)
			{
				return Checked.Fail(TEXT("Spawns"),
				                         TEXT("normal spawns must fit before window end and reference Early"));
			}
			OutWaves.Add(Row);
		}
		OutWaves.Sort([](const FLDWaveRow& A, const FLDWaveRow& B) { return A.WaveIndex < B.WaveIndex; });
		if (OutWaves.Num() != 10)
		{
			Error = TEXT("DT_Waves.json: missing P0 wave in 1..10");
			return false;
		}
		return true;
	}

	bool ParseProfiles(const TArray<TSharedPtr<FJsonObject>>& SummonRows,
	                   const TArray<TSharedPtr<FJsonObject>>& SpawnRows, FLDGameRules& Rules, FString& Error)
	{
		bool bHasGold = false;
		bool bHasEarly = false;
		const TCHAR* Grades[] = {TEXT("Common"), TEXT("Rare"), TEXT("Epic"), TEXT("Legendary")};
		for (const TSharedPtr<FJsonObject>& Object : SummonRows)
		{
			if (Object->GetStringField(TEXT("Name")) != TEXT("Gold_Default"))
			{
				continue;
			}
			FCheckedObject Checked(Object, TEXT("DT_SummonProfiles.json[Gold_Default]"), Error);
			int32 Unused = 0;
			if (!Checked.RequireString(
			        TEXT("Source"), TEXT("Gold")) ||
			             !Checked.Integer(TEXT("Level"), Unused, 0, 0) ||
			                              !Checked.Integer(TEXT("FailWeight"), Unused, 0, 0) ||
			                                               !Checked.Integer(TEXT("MythicWeight"), Unused, 0, 0) ||
			                                                                !Checked.Integer(TEXT("PityThreshold"),
			                                                                                      Unused, 0, 0))
			{
				return false;
			}
			int32 Sum = 0;
			for (const TCHAR* Grade : Grades)
			{
				int32 Weight = 0;
				if (!Checked.Integer(*(FString(Grade) + TEXT("Weight")), Weight, 0, 10000))
				{
					return false;
				}
				Rules.GoldGradeWeights.Add(FName(Grade), Weight);
				Sum += Weight;
			}
			if (Sum != 10000)
			{
				return Checked.Fail(TEXT("Weights"), TEXT("sum must equal 10000"));
			}
			bHasGold = true;
		}
		for (const TSharedPtr<FJsonObject>& Object : SpawnRows)
		{
			if (Object->GetStringField(TEXT("Name")) != TEXT("Early"))
			{
				continue;
			}
			FCheckedObject Checked(Object, TEXT("DT_SpawnProfiles.json[Early]"), Error);
			int32 Weight = 0;
			if (!Checked.Integer(TEXT("N01Weight"), Weight, 10000, 10000) ||
			                     !Checked.Integer(TEXT("N02Weight"), Weight, 0, 0) ||
			                                      !Checked.Integer(TEXT("N03Weight"), Weight, 0, 0) ||
			                                                       !Checked.Integer(TEXT("N04Weight"), Weight, 0, 0))
			{
				return false;
			}
			bHasEarly = true;
		}
		if (!bHasGold || !bHasEarly)
		{
			Error = TEXT("DT_SummonProfiles.json/DT_SpawnProfiles.json: required Gold_Default or Early missing");
			return false;
		}
		return true;
	}
} // namespace

bool ULDGameData::LoadP0(FString& OutError)
{
	return LoadP0FromDirectory(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("LD/Data")), OutError);
}

bool ULDGameData::LoadP0FromDirectory(const FString& Directory, FString& OutError)
{
	OutError.Reset();
	FString AbsoluteDirectory = FPaths::ConvertRelativePathToFull(Directory);
	FPaths::NormalizeDirectoryName(AbsoluteDirectory);
	if (bLoaded)
	{
		if (LoadedDirectory == AbsoluteDirectory)
		{
			return true;
		}
		OutError = TEXT("GameData is immutable after load; create a new match snapshot for a different directory");
		return false;
	}
	TSharedPtr<FJsonObject> Root;
	const FString RulesFile = FPaths::Combine(AbsoluteDirectory, TEXT("GameRules.json"));
	FLDGameRules CandidateRules;
	TMap<FName, FLDUnitRow> CandidateUnits;
	TMap<FName, FLDEnemyRow> CandidateEnemies;
	TArray<FLDWaveRow> CandidateWaves;
	TArray<TSharedPtr<FJsonObject>> UnitRows, EnemyRows, WaveRows, SummonRows, SpawnRows;
	if (!LoadObject(RulesFile, Root, OutError) ||
	    !ParseRules(FCheckedObject(Root, RulesFile, OutError), CandidateRules) ||
	    !LoadRows(
	        FPaths::Combine(AbsoluteDirectory, TEXT("DT_Units.json")), UnitRows, OutError) ||
	        !ParseUnits(UnitRows, CandidateRules, CandidateUnits, OutError) ||
	        !LoadRows(FPaths::Combine(AbsoluteDirectory, TEXT("DT_EnemyTypes.json")), EnemyRows, OutError) ||
	                  !ParseEnemies(EnemyRows, CandidateEnemies, OutError) ||
	                  !LoadRows(FPaths::Combine(AbsoluteDirectory, TEXT("DT_Waves.json")), WaveRows, OutError) ||
	                            !ParseWaves(WaveRows, CandidateWaves, OutError) ||
	                            !LoadRows(FPaths::Combine(AbsoluteDirectory, TEXT("DT_SummonProfiles.json")),
	                                                      SummonRows, OutError) ||
	                                      !LoadRows(FPaths::Combine(AbsoluteDirectory, TEXT("DT_SpawnProfiles.json")),
	                                                                SpawnRows, OutError) ||
	                                                !ParseProfiles(SummonRows, SpawnRows, CandidateRules, OutError))
	{
		return false;
	}
	Rules = MoveTemp(CandidateRules);
	Units = MoveTemp(CandidateUnits);
	Enemies = MoveTemp(CandidateEnemies);
	Waves = MoveTemp(CandidateWaves);
	LoadedDirectory = MoveTemp(AbsoluteDirectory);
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
	const FLDUnitRow* Row = bLoaded ? Units.Find(UnitId) : nullptr;
	if (!Row)
	{
		return false;
	}
	OutRow = *Row;
	return true;
}

bool ULDGameData::TryGetEnemyRow(FName EnemyTypeId, FLDEnemyRow& OutRow) const
{
	const FLDEnemyRow* Row = bLoaded ? Enemies.Find(EnemyTypeId) : nullptr;
	if (!Row)
	{
		return false;
	}
	OutRow = *Row;
	return true;
}
