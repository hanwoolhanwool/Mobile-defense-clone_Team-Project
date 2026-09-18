#include "Core/LDGameState.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/LDPlayerState.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0MatchLifecycleTest, "LD.P0.G0.Integration.MatchLifecycle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0MatchLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient test world"), World))
	{
		return false;
	}
	ALDGameState* State = World->SpawnActor<ALDGameState>();
	ALDPlayerState* Player = World->SpawnActor<ALDPlayerState>();
	if (!TestNotNull(TEXT("Authoritative state"), State) || !TestNotNull(TEXT("Participant state"), Player))
	{
		World->DestroyWorld(false);
		return false;
	}
	TestFalse(TEXT("No running state before data/context"), State->SetPhase(ELDMatchPhase::Running));
	FLDMatchContext Match;
	Match.MatchId = FGuid::NewGuid();
	Match.RulesVersion = TEXT("0.3.0");
	TestTrue(TEXT("Initial match context"), State->InitializeMatch(Match));
	TestTrue(TEXT("Repeated initialization is idempotent"), State->InitializeMatch(Match));
	FLDMatchContext DifferentMatch = Match;
	DifferentMatch.MatchId = FGuid::NewGuid();
	TestFalse(TEXT("Live match cannot be overwritten"), State->InitializeMatch(DifferentMatch));
	TestTrue(TEXT("Loading to preparing"), State->SetPhase(ELDMatchPhase::Preparing));
	TestFalse(TEXT("Preparing cannot publish a premature result"), State->SetPhase(ELDMatchPhase::Result));
	TestTrue(TEXT("Preparing to running"), State->SetPhase(ELDMatchPhase::Running));
	TestFalse(TEXT("Running never rewinds to preparing"), State->SetPhase(ELDMatchPhase::Preparing));
	TestTrue(TEXT("One terminal result"), State->SetPhase(ELDMatchPhase::Result));
	TestTrue(TEXT("Repeated result is harmless"), State->SetPhase(ELDMatchPhase::Result));
	TestFalse(TEXT("Late abort cannot replace a result"), State->SetPhase(ELDMatchPhase::Aborted));
	TestFalse(TEXT("Terminal match never restarts"), State->SetPhase(ELDMatchPhase::Running));
	FLDParticipantContext Participant;
	Participant.MatchId = Match.MatchId;
	Participant.PlayerIndex = 1;
	Participant.ConnectionEpoch = 42;
	TestTrue(TEXT("Server participant assignment"), Player->InitializeParticipant(Participant));
	TestTrue(TEXT("Duplicate assignment does not reset state"), Player->InitializeParticipant(Participant));
	Participant.PlayerIndex = 0;
	TestFalse(TEXT("Existing participant cannot be reassigned"), Player->InitializeParticipant(Participant));
	TestEqual(TEXT("Public identity remains stable"), Player->GetPlayerIndex(), 1);
	ALDGameState* NextState = World->SpawnActor<ALDGameState>();
	if (TestNotNull(TEXT("New match state"), NextState))
	{
		TestEqual(TEXT("New match starts Loading"), NextState->GetPhase(), ELDMatchPhase::Loading);
		TestTrue(TEXT("Data failure can abort without a context"), NextState->SetPhase(ELDMatchPhase::Aborted));
		TestFalse(TEXT("Failed initialization cannot later run"), NextState->SetPhase(ELDMatchPhase::Running));
	}
	World->DestroyWorld(false);
	return true;
}

#endif
