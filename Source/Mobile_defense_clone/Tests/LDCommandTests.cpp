#include "Network/LDCommandProcessor.h"

#include "Data/LDGameData.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FLDGameRules MakeCommandRules()
	{
		FLDGameRules Rules;
		Rules.CommandRatePerSecond = 8;
		Rules.CommandBurst = 12;
		Rules.RequestResultCacheCount = 256;
		return Rules;
	}

	FLDParticipantContext MakeParticipant(const FLDMatchContext& Match, int32 Index)
	{
		FLDParticipantContext Participant;
		Participant.MatchId = Match.MatchId;
		Participant.PlayerIndex = Index;
		Participant.ConnectionEpoch = 1;
		return Participant;
	}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCommandAdmissionTest, "LD.P0.G0.Commands.AdmissionAndReplay",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDCommandAdmissionTest::RunTest(const FString& Parameters)
{
	FLDMatchContext Match;
	Match.MatchId = FGuid::NewGuid();
	Match.RulesVersion = TEXT("0.3.0");
	ULDCommandProcessor* Processor = NewObject<ULDCommandProcessor>();
	TestTrue(TEXT("Valid current contract initializes"), Processor->Initialize(Match, MakeCommandRules()));
	const FLDParticipantContext Player = MakeParticipant(Match, 0);
	TestTrue(TEXT("Server identity registered"), Processor->RegisterParticipant(Player));
	FLDCommand Command;
	Command.ConnectionEpoch = 1;
	Command.RequestId = 1;
	TestEqual(TEXT("Before service readiness, no command executes"),
	               Processor->SubmitAtTime(Player, Command, 0).ResultCode, ELDCommandResultCode::PhaseNotAllowed);
	Processor->SetAcceptingCommands(true);
	TestEqual(TEXT("Readiness does not replace an already finalized rejection"),
	               Processor->SubmitAtTime(Player, Command, 0).ResultCode, ELDCommandResultCode::PhaseNotAllowed);
	Command.ExpectedBoardRevision = 1;
	TestEqual(TEXT("Same key changed content cannot replace first result"),
	               Processor->SubmitAtTime(Player, Command, 0).ResultCode, ELDCommandResultCode::RequestIdConflict);
	Command.ExpectedBoardRevision = 0;
	Command.RequestId = 2;
	TestEqual(TEXT("Absent Board/Economy is explicit Stub"), Processor->SubmitAtTime(Player, Command, 0).ResultCode,
	               ELDCommandResultCode::FeatureDisabled);
	Processor->Close();
	Processor->SetAcceptingCommands(true);
	TestEqual(TEXT("Cached result survives match closure"), Processor->SubmitAtTime(Player, Command, 0).ResultCode,
	               ELDCommandResultCode::FeatureDisabled);
	Command.RequestId = 3;
	const FLDCommandResult Rejected = Processor->SubmitAtTime(Player, Command, 0);
	TestEqual(TEXT("Response remains bound to the server match"), Rejected.MatchId, Match.MatchId);
	TestEqual(TEXT("Close is terminal for new admissions"), Rejected.ResultCode, ELDCommandResultCode::PhaseNotAllowed);
	TestEqual(TEXT("Rejection cannot publish a created unit"), Rejected.CreatedInstanceIds.Num(), 0);
	TestEqual(TEXT("Rejection cannot publish an event"), Rejected.EventId, uint64(0));
	TestEqual(TEXT("Rejection preserves board revision"), Rejected.NewBoardRevision, 0);
	TestEqual(TEXT("Rejection preserves economy revision"), Rejected.EconomyRevision, 0);
	FLDParticipantContext Impostor = Player;
	Impostor.MatchId = FGuid::NewGuid();
	TestEqual(TEXT("Other match identity is rejected before cache lookup"),
	               Processor->SubmitAtTime(Impostor, Command, 0).ResultCode, ELDCommandResultCode::InvalidEpoch);
	Command.ConnectionEpoch = 2;
	TestEqual(TEXT("Payload cannot issue a new epoch"), Processor->SubmitAtTime(Player, Command, 0).ResultCode,
	               ELDCommandResultCode::InvalidEpoch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCommandLimitsTest, "LD.P0.G0.Commands.LimitsAndExpiry",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDCommandLimitsTest::RunTest(const FString& Parameters)
{
	FLDMatchContext Match;
	Match.MatchId = FGuid::NewGuid();
	Match.RulesVersion = TEXT("0.3.0");
	ULDCommandProcessor* Processor = NewObject<ULDCommandProcessor>();
	Processor->Initialize(Match, MakeCommandRules());
	FLDParticipantContext Player = MakeParticipant(Match, 0);
	Processor->RegisterParticipant(Player);
	FLDCommand Command;
	Command.ConnectionEpoch = 1;
	for (uint32 Request = 1; Request <= 12; ++Request)
	{
		Command.RequestId = Request;
		TestEqual(TEXT("Twelve immediate requests fit the documented burst"),
		               Processor->SubmitAtTime(Player, Command, 0).ResultCode, ELDCommandResultCode::PhaseNotAllowed);
	}
	Command.RequestId = 13;
	TestEqual(TEXT("The thirteenth immediate request is rate limited"),
	               Processor->SubmitAtTime(Player, Command, 0).ResultCode, ELDCommandResultCode::RateLimited);
	TestEqual(TEXT("A rejected request is not re-executed after refill"),
	               Processor->SubmitAtTime(Player, Command, 1).ResultCode, ELDCommandResultCode::RateLimited);
	Command.RequestId = 14;
	TestEqual(TEXT("Exactly 0.125 seconds restores one command"),
	               Processor->SubmitAtTime(Player, Command, 0.125).ResultCode, ELDCommandResultCode::PhaseNotAllowed);
	for (uint32 Request = 15; Request <= 257; ++Request)
	{
		Command.RequestId = Request;
		Processor->SubmitAtTime(Player, Command, double(Request));
	}
	TestEqual(TEXT("Finalized history is bounded at 256"), Processor->GetCachedResultCount(0), 256);
	Command.RequestId = 1;
	TestEqual(TEXT("An evicted request must never execute again"),
	               Processor->SubmitAtTime(Player, Command, 300).ResultCode, ELDCommandResultCode::RequestExpired);
	TestTrue(TEXT("Re-registering same epoch preserves history"), Processor->RegisterParticipant(Player));
	TestEqual(TEXT("History survives duplicate initialization"), Processor->GetCachedResultCount(0), 256);
	Player.ConnectionEpoch = 2;
	TestTrue(TEXT("Server may advance the epoch"), Processor->RegisterParticipant(Player));
	TestEqual(TEXT("Old wire epoch cannot access new history"),
	               Processor->SubmitAtTime(Player, Command, 300).ResultCode, ELDCommandResultCode::InvalidEpoch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCommandPayloadTest, "LD.P0.G0.Commands.PayloadNormalization",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDCommandPayloadTest::RunTest(const FString& Parameters)
{
	FLDCommand Merge;
	Merge.CommandType = ELDCommandType::Merge;
	Merge.ConnectionEpoch = 7;
	Merge.RequestId = 4;
	Merge.InstanceId = 11;
	Merge.ConsumedInstanceId0 = 12;
	Merge.ConsumedInstanceId1 = 11;
	Merge.ConsumedInstanceId2 = 10;
	TestTrue(TEXT("Three distinct selected-cell candidates are structurally valid"), Merge.IsValidPayload());
	FLDCommand Reordered = Merge;
	Swap(Reordered.ConsumedInstanceId0, Reordered.ConsumedInstanceId2);
	TestTrue(TEXT("Material order has no request meaning"), Merge.Normalized().HasSameContent(Reordered.Normalized()));
	Reordered.InstanceId = 10;
	TestFalse(TEXT("Selected source is part of request meaning"),
	               Merge.Normalized().HasSameContent(Reordered.Normalized()));
	Reordered.ConsumedInstanceId1 = Reordered.ConsumedInstanceId0;
	TestFalse(TEXT("Duplicate material IDs are rejected, not deduplicated"), Reordered.IsValidPayload());
	Merge.Source = 1;
	TestFalse(TEXT("Unrelated summon fields cannot ride a merge payload"), Merge.IsValidPayload());
	TestTrue(TEXT("P0 command has no variable-size wire fields"), sizeof(FLDCommand) < 1024);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDBDataContractTest, "LD.P0.G0.Commands.BIndependentLoader",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDBDataContractTest::RunTest(const FString& Parameters)
{
	ULDGameData* Data = NewObject<ULDGameData>();
	FString Error;
	if (!TestTrue(TEXT("Staged P0 dataset loads"), Data->LoadP0(Error)))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("P0 active roster count"), Data->GetUnits().Num(), 16);
	TestEqual(TEXT("P0 cap overrides 80-wave source"), Data->GetWaves().Num(), 10);
	TestEqual(TEXT("Boss-only final wave has no normal spawning"), Data->GetWaves().Last().NormalCountPerGate, 0);
	TestEqual(TEXT("No normal spawning permits a zero normal interval"), Data->GetWaves().Last().SpawnIntervalSeconds,
	               0.0);
	TestEqual(TEXT("Personal board contains 18 cells"), Data->GetRules().CellsPerPlayer, 18);
	TestEqual(TEXT("Population is independent of 18 x 3 capacity"), Data->GetRules().MaxUnitsPerPlayer, 20);
	TestFalse(
	    TEXT("Missing replacement cannot silently load"),
	         Data->LoadP0FromDirectory(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("G0DoesNotExist")), Error));
	TestEqual(TEXT("Failed replacement keeps previously validated snapshot"), Data->GetUnits().Num(), 16);
	return true;
}

#endif
