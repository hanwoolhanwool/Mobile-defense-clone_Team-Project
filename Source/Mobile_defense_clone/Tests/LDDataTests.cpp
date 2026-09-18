#include "Data/LDGameData.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Data/LDMatchTypes.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	FString MakeFixture(FAutomationTestBase& Test)
	{
		const FString Directory = FPaths::Combine(
		    FPaths::ProjectSavedDir(), TEXT("Automation/LD/P0/G0"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
		IFileManager::Get().MakeDirectory(*Directory, true);
		const TCHAR* Files[] = {
		    TEXT("GameRules.json"), TEXT("DT_Units.json"),          TEXT("DT_EnemyTypes.json"),
		                                                                 TEXT("DT_Waves.json"),  TEXT("DT_SummonProfiles.json"), TEXT("DT_SpawnProfiles.json")};
		for (const TCHAR* File : Files)
		{
			FString Contents;
			const FString Source = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("LD/Data"), File);
			if (!FFileHelper::LoadFileToString(Contents, *Source) ||
			    !FFileHelper::SaveStringToFile(Contents, *FPaths::Combine(Directory, File)))
			{
				Test.AddError(TEXT("Cannot prepare runtime-data fixture from ") + Source);
				return FString();
			}
		}
		// Preserved under Saved for failure reproduction. No tracked input is modified or deleted.
		return Directory;
	}

	bool MutateRules(const FString& Directory, TFunctionRef<void(FJsonObject&)> Mutation)
	{
		const FString File = FPaths::Combine(Directory, TEXT("GameRules.json"));
		FString Contents;
		TSharedPtr<FJsonObject> Root;
		if (!FFileHelper::LoadFileToString(Contents, *File) ||
		    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Contents), Root) || !Root.IsValid())
		{
			return false;
		}
		Mutation(*Root);
		Contents.Reset();
		if (!FJsonSerializer::Serialize(Root.ToSharedRef(), TJsonWriterFactory<>::Create(&Contents)))
		{
			return false;
		}
		return FFileHelper::SaveStringToFile(Contents, *File);
	}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0DataValidTest, "LD.P0.G0.Data.ValidP0Snapshot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0DataValidTest::RunTest(const FString& Parameters)
{
	ULDGameData* Data = NewObject<ULDGameData>();
	FString Error;
	if (!TestTrue(TEXT("Packaged content data loads"), Data->LoadP0(Error)))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("Exactly 16 active units"), Data->GetUnits().Num(), 16);
	TestEqual(TEXT("Only P0 waves retained"), Data->GetWaves().Num(), 10);
	TestEqual(TEXT("Schema 2 required"), Data->GetRules().SchemaVersion, 2);
	TestEqual(TEXT("P0 overrides base FinalWave 80"), Data->GetRules().FinalWave, 10);
	TestEqual(TEXT("Personal cells independent from population"), Data->GetRules().CellsPerPlayer, 18);
	TestEqual(TEXT("Personal population baseline"), Data->GetRules().MaxUnitsPerPlayer, 20);
	TestEqual(TEXT("Gold_Default Common weight"), Data->GetRules().GoldGradeWeights.FindRef(TEXT("Common")), 9743);
	TestEqual(TEXT("Gold_Default Rare weight"), Data->GetRules().GoldGradeWeights.FindRef(TEXT("Rare")), 198);
	TestEqual(TEXT("Gold_Default Epic weight"), Data->GetRules().GoldGradeWeights.FindRef(TEXT("Epic")), 49);
	TestEqual(TEXT("Gold_Default Legendary weight"), Data->GetRules().GoldGradeWeights.FindRef(TEXT("Legendary")), 10);
	FLDUnitRow Unit;
	TestTrue(TEXT("C01 exists"), Data->TryGetUnitRow(TEXT("C01"), Unit));
	TestEqual(TEXT("C01 attack fixture"), Unit.BaseAttack, 15.0);
	TestEqual(TEXT("C01 range fixture"), Unit.RangeCm, 175.0);
	TestTrue(TEXT("P0 skill has no executor reference"), Unit.SkillId.IsNone());
	TestFalse(TEXT("P1 mythic excluded"), Data->TryGetUnitRow(TEXT("M01"), Unit));
	TestFalse(TEXT("Unknown row rejected"), Data->TryGetUnitRow(TEXT("NotAUnit"), Unit));
	TestEqual(TEXT("Failed query preserves caller output"), Unit.UnitId, FName(TEXT("C01")));
	TestEqual(TEXT("Final bosses per gate"), Data->GetWaves().Last().BossCountPerGate, 1);
	TestEqual(TEXT("Final deadline"), Data->GetWaves().Last().BossDeadlineSeconds, 60.0);
	TestTrue(TEXT("Same match snapshot repeated initialization is idempotent"), Data->LoadP0(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0DataInvalidPolicyTest, "LD.P0.G0.Data.RejectChangedContractsAtomically",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0DataInvalidPolicyTest::RunTest(const FString& Parameters)
{
	struct FMutation
	{
		FString Name;
		TFunction<void(FJsonObject&)> Apply;
	};
	TArray<FMutation> Mutations;
	Mutations.Add({TEXT("SchemaVersion"), [](FJsonObject& Root) { Root.SetNumberField(TEXT("SchemaVersion"), 1); }});
	Mutations.Add(
	    {TEXT("RulesVersion"), [](FJsonObject& Root) { Root.SetStringField(TEXT("RulesVersion"), TEXT("0.2.0")); }});
	Mutations.Add({TEXT("EnableSkills"), [](FJsonObject& Root)
	                    { Root.GetObjectField(TEXT("P0Overrides"))->SetBoolField(TEXT("EnableSkills"), true); }});
	Mutations.Add({TEXT("ContinuousSeconds"), [](FJsonObject& Root)
	                    { Root.GetObjectField(TEXT("Defeat"))->SetNumberField(TEXT("ContinuousSeconds"), 3); }});
	Mutations.Add(
	    {TEXT("Timing.Order"), [](FJsonObject& Root)
	          { Root.GetObjectField(TEXT("Timing"))->SetStringField(TEXT("Order"), TEXT("DeadlinesBeforeDamage")); }});
	Mutations.Add({TEXT("MergeUsesSelectedCell"), [](FJsonObject& Root)
	                    { Root.GetObjectField(TEXT("Board"))->SetBoolField(TEXT("MergeUsesSelectedCell"), true); }});
	Mutations.Add(
	    {TEXT("MaxPartialStacksPerUnitPerArea"), [](FJsonObject& Root)
	          { Root.GetObjectField(TEXT("Board"))->SetNumberField(TEXT("MaxPartialStacksPerUnitPerArea"), 2); }});
	for (const FString Field :
	     {FString(TEXT("AllowedGrades")), FString(TEXT("MergeInputGrades")), FString(TEXT("AllowedNormalEnemyIds"))})
	{
		Mutations.Add({Field, [Field](FJsonObject& Root)
		               { Root.GetObjectField(TEXT("P0Overrides"))->SetArrayField(Field, {}); }});
	}
	Mutations.Add({TEXT("CountedEnemyKinds"), [](FJsonObject& Root)
	                    { Root.GetObjectField(TEXT("Defeat"))->SetArrayField(TEXT("CountedEnemyKinds"), {}); }});
	Mutations.Add({TEXT("XCentersCm duplicate"),
	                    [](FJsonObject& Root)
	                    {
		                    TArray<TSharedPtr<FJsonValue>> Values =
		                        Root.GetObjectField(TEXT("Board"))->GetArrayField(TEXT("XCentersCm"));
		                    Values[1] = Values[0];
		                    Root.GetObjectField(TEXT("Board"))->SetArrayField(TEXT("XCentersCm"), Values);
	                    }});
	Mutations.Add({TEXT("YCentersByPlayer overlap"),
	                    [](FJsonObject& Root)
	                    {
		                    TArray<TSharedPtr<FJsonValue>> Values =
		                        Root.GetObjectField(TEXT("Board"))->GetArrayField(TEXT("YCentersByPlayer"));
		                    Values[1] = Values[0];
		                    Root.GetObjectField(TEXT("Board"))->SetArrayField(TEXT("YCentersByPlayer"), Values);
	                    }});
	Mutations.Add({TEXT("Missing required Economy"), [](FJsonObject& Root) { Root.RemoveField(TEXT("Economy")); }});
	for (const FMutation& Mutation : Mutations)
	{
		const FString Directory = MakeFixture(*this);
		if (Directory.IsEmpty() ||
		    !TestTrue(Mutation.Name + TEXT(" fixture saved"), MutateRules(Directory, Mutation.Apply)))
		{
			return false;
		}
		ULDGameData* Data = NewObject<ULDGameData>();
		FString Error;
		TestFalse(Mutation.Name + TEXT(" rejected"), Data->LoadP0FromDirectory(Directory, Error));
		TestFalse(Mutation.Name + TEXT(" never marked ready"), Data->IsLoaded());
		TestEqual(Mutation.Name + TEXT(" no partial units published"), Data->GetUnits().Num(), 0);
		TestFalse(Mutation.Name + TEXT(" diagnostic includes cause"), Error.IsEmpty());
		AddInfo(Mutation.Name + TEXT(" => ") + Error + TEXT("; fixture=") + Directory);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0DataBadReferenceTest, "LD.P0.G0.Data.InvalidRowsAndMissingFiles",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0DataBadReferenceTest::RunTest(const FString& Parameters)
{
	const FString Directory = MakeFixture(*this);
	if (Directory.IsEmpty())
	{
		return false;
	}
	FString Contents;
	const FString WavesFile = FPaths::Combine(Directory, TEXT("DT_Waves.json"));
	if (!FFileHelper::LoadFileToString(Contents, *WavesFile))
	{
		return false;
	}
	Contents.ReplaceInline(TEXT("\"B01\""), TEXT("\"MISSING_BOSS\""));
	TestTrue(TEXT("Write missing boss fixture"), FFileHelper::SaveStringToFile(Contents, *WavesFile));
	ULDGameData* Data = NewObject<ULDGameData>();
	FString Error;
	TestFalse(TEXT("Invalid boss reference rejected"), Data->LoadP0FromDirectory(Directory, Error));
	TestEqual(TEXT("Late failure publishes no unit snapshot"), Data->GetUnits().Num(), 0);
	TestEqual(TEXT("Late failure publishes no wave snapshot"), Data->GetWaves().Num(), 0);
	TestTrue(TEXT("Failure is diagnostic"), Error.Contains(TEXT("BossId")));
	TestTrue(TEXT("Failed load can retry corrected production data"), Data->LoadP0(Error));
	ULDGameData* Missing = NewObject<ULDGameData>();
	TestFalse(TEXT("Missing directory fails explicitly"),
	               Missing->LoadP0FromDirectory(FPaths::Combine(Directory, TEXT("MissingRequiredFiles")), Error));
	TestTrue(TEXT("Missing filename reported"), Error.Contains(TEXT("GameRules.json")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0ContextTest, "LD.P0.G0.Data.ParticipantIdentity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0ContextTest::RunTest(const FString& Parameters)
{
	FLDParticipantContext Context;
	TestFalse(TEXT("Empty context rejected"), Context.IsValid());
	Context.MatchId = FGuid::NewGuid();
	Context.PlayerIndex = 0;
	Context.ConnectionEpoch = 1;
	TestTrue(TEXT("Server participant zero accepted"), Context.IsValid());
	Context.PlayerIndex = 1;
	TestTrue(TEXT("Server participant one accepted"), Context.IsValid());
	Context.PlayerIndex = 2;
	TestFalse(TEXT("Third board rejected"), Context.IsValid());
	Context.PlayerIndex = -1;
	TestFalse(TEXT("Negative board rejected"), Context.IsValid());
	Context.PlayerIndex = 0;
	Context.ConnectionEpoch = 0;
	TestFalse(TEXT("Unissued epoch rejected"), Context.IsValid());
	return true;
}

#endif
