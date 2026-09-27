#include "Core/LDGameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Board/LDBoardManager.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Data/LDGameData.h"
#include "Economy/LDEconomyService.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Network/LDCommandProcessor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0EntryAddressTest, "LD.P0.G3.Entry.NumericAddressBoundary",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0EntryAddressTest::RunTest(const FString& Parameters)
{
	FString Address;
	TestTrue(TEXT("IPv4 defaults to game port"), ULDGameInstance::NormalizeJoinAddress(TEXT("127.0.0.1"), Address));
	TestEqual(TEXT("Canonical default"), Address, FString(TEXT("127.0.0.1:7777")));
	TestTrue(TEXT("Explicit port and edge spaces"),
	              ULDGameInstance::NormalizeJoinAddress(TEXT(" 192.168.001.010:65535 "), Address));
	TestEqual(TEXT("Canonical dotted decimal"), Address, FString(TEXT("192.168.1.10:65535")));
	for (const TCHAR* Invalid :
	     {TEXT(""),
	           TEXT("localhost"),
	                TEXT("127.1"),
	                     TEXT("127..0.1"),
	                          TEXT("256.0.0.1"),
	                               TEXT("0.0.0.0"),
	                                    TEXT("224.1.1.1"),
	                                         TEXT("255.255.255.255"),
	                                              TEXT("127.0.0.1:0"),
	                                                   TEXT("127.0.0.1:65536"),
	                                                        TEXT("127.0.0.1:"),
	                                                             TEXT("127.0.0.1:7:8"),
	                                                                  TEXT("127.0.0.1?listen"),
	                                                                       TEXT("127.0.0.1/P0"),
	                                                                            TEXT("127.0.0.1 -log"),
	                                                                                 TEXT("127.0.0.1;quit"),
	                                                                                      TEXT("127.0.0.1\nquit"),
	                                                                                           TEXT("127.0.0.1:+7777")})
	{
		Address = TEXT("previous valid address");
		TestFalse(FString::Printf(TEXT("Reject non-endpoint [%s]"), Invalid),
		                          ULDGameInstance::NormalizeJoinAddress(Invalid, Address));
		TestTrue(TEXT("Failure cannot retain a previous destination"), Address.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0EntryTravelGateTest, "LD.P0.G3.Entry.TravelIntentAndReturnLifetime",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0EntryTravelGateTest::RunTest(const FString& Parameters)
{
	ULDGameInstance* Instance = NewObject<ULDGameInstance>();
	Instance->NotifyEntryReady();
	TestTrue(TEXT("First host/join travel intent admitted"), Instance->TryBeginMatchTravel());
	TestFalse(TEXT("Second click before travel cannot start another travel"), Instance->TryBeginMatchTravel());
	const FText Reason = FText::FromString(TEXT("connection lost fixture"));
	TestTrue(TEXT("Failure schedules one return even during pending join"), Instance->RequestEntryReturn(Reason));
	TestFalse(TEXT("Duplicate failure/result cannot schedule another return"),
	               Instance->RequestEntryReturn(FText::FromString(TEXT("duplicate"))));
	TestEqual(TEXT("Original failure reason retained"), Instance->GetEntryMessage().ToString(), Reason.ToString());
	// This unit test verifies admission and cleanup only; actual OpenLevel/rejoin is the packaged pair gate.
	Instance->NotifyEntryReady();
	TestFalse(TEXT("Entry lifetime cancels its old deferred ticker and reopens input"), Instance->IsTravelPending());
	TestTrue(TEXT("Second match intent works in the same GameInstance"), Instance->TryBeginMatchTravel());
	TestTrue(TEXT("A new match clears old errors"), Instance->GetEntryMessage().IsEmpty());
	Instance->NotifyEntryReady();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0ClockResultRewardTest, "LD.P0.G3.Commands.RewardBeforeTerminalClose",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0ClockResultRewardTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	ULDGameData* Data = NewObject<ULDGameData>(World);
	FString Error;
	FLDMatchContext Match;
	Match.MatchId = FGuid::NewGuid();
	Match.RulesVersion = TEXT("0.3.0");
	ULDBoardManager* Board = NewObject<ULDBoardManager>(World);
	ULDEconomyService* Economy = NewObject<ULDEconomyService>(World);
	ULDCommandProcessor* Processor = NewObject<ULDCommandProcessor>(World);
	const bool bReady = Data->LoadP0(Error) && Board->Initialize(*World, Match, *Data) &&
	                    Economy->Initialize(Match, *Data, 1776) && Processor->Initialize(Match, Data->GetRules()) &&
	                    Processor->BindServices(*Board, *Economy);
	if (TestTrue(TEXT("Actual board/economy/processor initialize"), bReady))
	{
		FLDParticipantContext Participant;
		Participant.MatchId = Match.MatchId;
		Participant.PlayerIndex = 0;
		Participant.ConnectionEpoch = 1;
		Processor->RegisterParticipant(Participant);
		Processor->SetAcceptingCommands(true);
		int32 FinalizerCalls = 0;
		Processor->BeforeExternalCommand.BindLambda(
		    [Processor, Match](double Now)
		    {
			    FLDCombatDeath Death;
			    Death.MatchId = Match.MatchId;
			    Death.DeathEventId = 1;
			    Death.EnemyId = 1;
			    Death.SpawnSerial = 1;
			    Death.EnemyTypeId = TEXT("B01");
			    Death.SpawnWaveIndex = 10;
			    Death.SpawnedServerSeconds = 0;
			    Death.DeathServerSeconds = Now;
			    Processor->EnqueueCombatReward(Death);
			    Processor->SetAcceptingCommands(false);
		    });
		Processor->AfterExternalCommandClock.BindLambda(
		    [this, Processor, Economy, &FinalizerCalls]()
		    {
			    ++FinalizerCalls;
			    TestEqual(TEXT("Final boss gold exists before publishing/closing result"), Economy->GetSnapshot(0).Gold,
			                   200);
			    TestEqual(TEXT("Fast boss stars exist before closing result"), Economy->GetSnapshot(0).Stars, 3);
			    Processor->Close();
		    });
		FLDCommand Command;
		Command.ConnectionEpoch = 1;
		Command.RequestId = 1;
		const FLDCommandResult First = Processor->SubmitAtTime(Participant, Command, 10);
		TestEqual(TEXT("The later command cannot purchase after terminal time"), First.ResultCode,
		               ELDCommandResultCode::PhaseNotAllowed);
		TestEqual(TEXT("Exactly one finalizer before result"), FinalizerCalls, 1);
		TestEqual(TEXT("Terminal rejection duplicate returns same outcome"),
		               Processor->SubmitAtTime(Participant, Command, 11).ResultCode, First.ResultCode);
		TestEqual(TEXT("Repeated closed requests cannot refinalize"), FinalizerCalls, 1);
		TestEqual(TEXT("Final reward remains committed"), Economy->GetSnapshot(0).Gold, 200);
		TestEqual(TEXT("Rejected purchase keeps population zero"), Board->GetSnapshot(0).Population, 0);
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return bReady;
}

#endif
