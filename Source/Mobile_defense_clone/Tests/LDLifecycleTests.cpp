#include "Core/LDGameState.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Battle/LDCombatService.h"
#include "Battle/LDEnemyActor.h"
#include "Battle/LDUnitActor.h"
#include "Board/LDBoardManager.h"
#include "Core/LDPlayerState.h"
#include "Core/LDGameMode.h"
#include "Core/LDPlayerController.h"
#include "Data/LDGameData.h"
#include "Economy/LDEconomyService.h"
#include "Engine/Engine.h"
#include "Engine/Player.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Network/LDCommandProcessor.h"

namespace
{
	struct FServerLifecycleFixture
	{
		UWorld* World = nullptr;
		ALDGameMode* Mode = nullptr;
		bool bHasWorldContext = false;

		FServerLifecycleFixture()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (World)
			{
				if (GEngine)
				{
					GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
					bHasWorldContext = true;
				}
				Mode = World->SpawnActor<ALDGameMode>();
				if (Mode)
				{
					FString Error;
					Mode->InitGame(TEXT("LDLifecycle"), FString(), Error);
				}
			}
		}

		~FServerLifecycleFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine && bHasWorldContext)
				{
					GEngine->DestroyWorldContext(World);
				}
			}
		}

		ALDPlayerController* CreateController()
		{
			ALDPlayerController* Controller = World->SpawnActor<ALDPlayerController>();
			ALDPlayerState* Player = World->SpawnActor<ALDPlayerState>();
			if (!Controller || !Player)
			{
				return nullptr;
			}
			Controller->SetPlayerState(Player);
			Controller->SetPlayer(NewObject<UPlayer>(Controller));
			return Controller;
		}

		void PrepareServices()
		{
			// Real engine callback creates GameState and invokes the production InitGameState loader/wiring path.
			Mode->PreInitializeComponents();
		}
	};

	FLDCommand CommandFor(ALDPlayerController& Controller, uint32 RequestId)
	{
		FLDCommand Command;
		Command.ConnectionEpoch = Controller.GetPlayerState<ALDPlayerState>()->GetParticipantContext().ConnectionEpoch;
		Command.RequestId = RequestId;
		return Command;
	}
} // namespace

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0LoginReadinessTest, "LD.P0.G0.Integration.LoginReadinessOrders",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0LoginReadinessTest::RunTest(const FString& Parameters)
{
	for (bool bLoginFirst : {true, false})
	{
		FServerLifecycleFixture Fixture;
		if (!TestNotNull(TEXT("Actual authoritative GameMode"), Fixture.Mode))
		{
			return false;
		}
		ALDPlayerController* First = Fixture.CreateController();
		ALDPlayerController* Second = Fixture.CreateController();
		if (!TestNotNull(TEXT("First owning Controller"), First) ||
		                 !TestNotNull(TEXT("Second owning Controller"), Second))
		{
			return false;
		}
		if (bLoginFirst)
		{
			Fixture.Mode->PostLogin(First);
			Fixture.Mode->PostLogin(First);
			TestFalse(TEXT("Login waits without inventing a match identity"),
			               First->GetPlayerState<ALDPlayerState>()->GetParticipantContext().IsValid());
			Fixture.Mode->PostLogin(Second);
		}
		Fixture.PrepareServices();
		if (!bLoginFirst)
		{
			Fixture.Mode->PostLogin(First);
			Fixture.Mode->PostLogin(Second);
		}
		ALDGameState* State = Fixture.Mode->GetGameState<ALDGameState>();
		if (!TestNotNull(TEXT("Production state was created"), State))
		{
			return false;
		}
		const FLDParticipantContext Initial = First->GetPlayerState<ALDPlayerState>()->GetParticipantContext();
		TestTrue(TEXT("Both startup orders assign a real match identity"), Initial.IsValid());
		TestEqual(TEXT("First login retains slot zero"), Initial.PlayerIndex, 0);
		TestEqual(TEXT("Second login retains slot one"), Second->GetPlayerState<ALDPlayerState>()->GetPlayerIndex(), 1);
		TestEqual(TEXT("Registered controller can reach the processor"),
		               First->SubmitServerCommand(CommandFor(*First, 1)).MatchId, State->GetMatchContext().MatchId);
		Fixture.Mode->PostLogin(First);
		Fixture.Mode->InitGameState();
		const FLDParticipantContext Repeated = First->GetPlayerState<ALDPlayerState>()->GetParticipantContext();
		TestEqual(TEXT("Duplicate login/init preserves epoch"), Repeated.ConnectionEpoch, Initial.ConnectionEpoch);
		TestEqual(TEXT("Duplicate login/init preserves match"), Repeated.MatchId, Initial.MatchId);
		FLDCommand Changed = CommandFor(*First, 1);
		Changed.ExpectedBoardRevision = 1;
		TestEqual(TEXT("Duplicate init preserves request cache"), First->SubmitServerCommand(Changed).ResultCode,
		               ELDCommandResultCode::RequestIdConflict);
		TestTrue(TEXT("Exactly two registered participants before BeginPlay"),
		              State->GetReadinessReason().Contains(TEXT("2/2 participants")));
		TestEqual(TEXT("Readiness does not skip the world BeginPlay boundary"), State->GetPhase(),
		               ELDMatchPhase::Loading);
		Fixture.Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0TerminalSessionTest, "LD.P0.G0.Integration.TerminalSessionReplay",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0TerminalSessionTest::RunTest(const FString& Parameters)
{
	FServerLifecycleFixture Fixture;
	if (!TestNotNull(TEXT("Actual authoritative GameMode"), Fixture.Mode))
	{
		return false;
	}
	Fixture.PrepareServices();
	ALDPlayerController* First = Fixture.CreateController();
	ALDPlayerController* Second = Fixture.CreateController();
	if (!TestNotNull(TEXT("First owning Controller"), First) || !TestNotNull(TEXT("Second owning Controller"), Second))
	{
		return false;
	}
	Fixture.Mode->PostLogin(First);
	Fixture.Mode->PostLogin(Second);
	const FLDCommand Original = CommandFor(*First, 1);
	const FLDCommand SecondOriginal = CommandFor(*Second, 1);
	const FLDCommandResult Before = First->SubmitServerCommand(Original);
	Second->SubmitServerCommand(SecondOriginal);
	AddExpectedError(TEXT("Match aborted: lifecycle fixture service failure"), EAutomationExpectedErrorFlags::Contains,
	                      1);
	Fixture.Mode->AbortMatch(TEXT("lifecycle fixture service failure"));
	Fixture.Mode->AbortMatch(TEXT("duplicate abort must be harmless"));
	TestEqual(TEXT("GameMode publishes the terminal phase"), Fixture.Mode->GetGameState<ALDGameState>()->GetPhase(),
	               ELDMatchPhase::Aborted);
	const FLDCommandResult Replay = First->SubmitServerCommand(Original);
	TestEqual(TEXT("Same Controller can replay the original match result after abort"), Replay.MatchId, Before.MatchId);
	TestEqual(TEXT("Cached result code survives abort"), Replay.ResultCode, Before.ResultCode);
	TestEqual(TEXT("Cached event identity survives abort"), Replay.EventId, Before.EventId);
	FLDCommand Conflict = Original;
	Conflict.ExpectedBoardRevision = 1;
	TestEqual(TEXT("Terminal replay still consults original content, not a generic phase rejection"),
	               First->SubmitServerCommand(Conflict).ResultCode, ELDCommandResultCode::RequestIdConflict);
	TestEqual(TEXT("New requests are rejected after terminal admission closes"),
	               First->SubmitServerCommand(CommandFor(*First, 2)).ResultCode, ELDCommandResultCode::PhaseNotAllowed);
	Fixture.Mode->Logout(First);
	TestFalse(TEXT("Logout removes this Controller's server match binding"),
	               First->SubmitServerCommand(Original).MatchId.IsValid());
	TestTrue(TEXT("Other connected session survives a peer's Logout"),
	              Second->SubmitServerCommand(SecondOriginal).MatchId.IsValid());
	Fixture.Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
	TestFalse(TEXT("EndPlay releases remaining server bindings"),
	               Second->SubmitServerCommand(SecondOriginal).MatchId.IsValid());
	TestFalse(TEXT("Session release clears local pending requests"), Second->HasPendingCommand());
	Fixture.Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0PendingLogoutTest, "LD.P0.G0.Integration.PendingLoginLogout",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0PendingLogoutTest::RunTest(const FString& Parameters)
{
	FServerLifecycleFixture Fixture;
	if (!TestNotNull(TEXT("Actual authoritative GameMode"), Fixture.Mode))
	{
		return false;
	}
	ALDPlayerController* Leaving = Fixture.CreateController();
	ALDPlayerController* Staying = Fixture.CreateController();
	if (!TestNotNull(TEXT("Leaving Controller"), Leaving) || !TestNotNull(TEXT("Staying Controller"), Staying))
	{
		return false;
	}
	Fixture.Mode->PostLogin(Leaving);
	Fixture.Mode->Logout(Leaving);
	Fixture.Mode->PostLogin(Staying);
	Fixture.PrepareServices();
	TestFalse(TEXT("Logged out pending login is not revived on data readiness"),
	               Leaving->GetPlayerState<ALDPlayerState>()->GetParticipantContext().IsValid());
	TestEqual(TEXT("Remaining connection receives available first slot"),
	               Staying->GetPlayerState<ALDPlayerState>()->GetPlayerIndex(), 0);
	Fixture.Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0OpenFrameBoundaryTest, "LD.P0.G2.Integration.TimerBeforeSameWorldTimeSale",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0OpenFrameBoundaryTest::RunTest(const FString& Parameters)
{
	for (bool bSellAtBoundary : {true, false})
	{
		FServerLifecycleFixture Fixture;
		if (!TestNotNull(TEXT("Actual GameMode"), Fixture.Mode))
		{
			return false;
		}
		Fixture.PrepareServices();
		// Explicit combat-only G2 fixture; normal G3 loading/preparation/waves are tested separately.
		Fixture.Mode->bG2Probe = true;
		Fixture.Mode->WaveDirector = nullptr;
		Fixture.Mode->GetGameState<ALDGameState>()->SetPhase(ELDMatchPhase::Preparing);
		ALDPlayerController* First = Fixture.CreateController();
		ALDPlayerController* Second = Fixture.CreateController();
		if (!TestNotNull(TEXT("Host controller"), First) || !TestNotNull(TEXT("Peer controller"), Second))
		{
			return false;
		}
		Fixture.World->TimeSeconds = 0;
		Fixture.Mode->PostLogin(First);
		Fixture.Mode->PostLogin(Second);
		Fixture.Mode->DispatchBeginPlay();
		if (!TestTrue(TEXT("Real readiness opens commands"), Fixture.Mode->CanAcceptCommands()))
		{
			return false;
		}
		const FLDCommandResult Summoned = First->SubmitServerCommand(CommandFor(*First, 1));
		if (!TestEqual(TEXT("Real first paid summon"), Summoned.ResultCode, ELDCommandResultCode::Success) ||
		               !TestEqual(TEXT("One actual created unit"), Summoned.CreatedInstanceIds.Num(), 1))
		{
			return false;
		}
		ALDUnitActor* Unit = nullptr;
		if (!TestTrue(TEXT("Board committed actor"), Fixture.Mode->GetBoardManager()->TryGetCommittedUnitActor(
		                                                 Summoned.CreatedInstanceIds[0], Unit)) ||
		              !TestNotNull(TEXT("Committed unit"), Unit))
		{
			return false;
		}
		ALDEnemyActor* Enemy = Fixture.World->SpawnActor<ALDEnemyActor>();
		FLDEnemyRow Row;
		Fixture.Mode->GetGameData()->TryGetEnemyRow(TEXT("N01"), Row);
		const FVector Position = Unit->GetActorLocation() + FVector(100, 0, 0);
		const TArray<FVector> Route = {Position, Position + FVector(500, 0, 0), Position + FVector(500, 500, 0),
		                               Position + FVector(0, 500, 0)};
		const FGuid MatchId = Fixture.World->GetGameState<ALDGameState>()->GetMatchContext().MatchId;
		if (!TestNotNull(TEXT("Explicit one-HP timing fixture"), Enemy) ||
		                 !TestTrue(TEXT("Fixture route"), Enemy->InitializeRoute(MatchId, 9001, 0, Route, 0, 0)) ||
		                           !TestTrue(TEXT("Fixture HP"), Enemy->InitializeCombat(Row, 1, 9001, 1, 0)) ||
		                                     !TestTrue(TEXT("Actual combat registration"),
		                                                    Fixture.Mode->GetCombatService()->RegisterEnemy(*Enemy)))
		{
			return false;
		}
		Fixture.World->TimeSeconds = .20;
		// Invoke the exact production timer delegate body. No test calls AdvanceCombatBefore/To to choose its order.
		Fixture.Mode->AdvanceLogic();
		double Due = 0;
		Fixture.Mode->GetCombatService()->TryGetUnitAttackState(Unit->GetPlacement().InstanceId, Due);
		TestEqual(TEXT("Initial attack is scheduled at .25"), Due, .25);
		Fixture.World->TimeSeconds = .25;
		Fixture.Mode->AdvanceLogic();
		TestEqual(TEXT("Timer does not close the current WorldTime before late input"), Enemy->GetCombatSnapshot().HP,
		               1.0);
		// Same WorldTime after the timer: this models late host Slate input or next-frame pre-time-update RPC dispatch.
		FLDCommand Sell = CommandFor(*First, 2);
		Sell.CommandType = ELDCommandType::Sell;
		Sell.InstanceId = Summoned.CreatedInstanceIds[0];
		Sell.ExpectedBoardRevision = Fixture.Mode->GetBoardManager()->GetSnapshot(0).BoardRevision;
		if (bSellAtBoundary)
		{
			TestEqual(TEXT("Actual controller sale succeeds at unchanged .25 WorldTime"),
			               First->SubmitServerCommand(Sell).ResultCode, ELDCommandResultCode::Success);
			TestEqual(TEXT("Sale unregisters actual combat source"),
			               Fixture.Mode->GetCombatService()->GetRegisteredUnitCount(), 0);
		}
		Fixture.World->TimeSeconds = .30;
		Fixture.Mode->AdvanceLogic();
		TestEqual(TEXT("Next frame cancels a sold attack or resolves the retained attack exactly once"),
		               Enemy->GetCombatSnapshot().HP, bSellAtBoundary ? 1.0 : 0.0);
		TestEqual(TEXT("Peer reward follows actual death only"), Second->GetEconomySnapshot().Gold,
		               bSellAtBoundary ? 100 : 101);
		Fixture.Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0CrossMatchEpochTest, "LD.P0.G3.Lifetime.PreviousMatchRequestRejected",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0CrossMatchEpochTest::RunTest(const FString& Parameters)
{
	FLDCommand PreviousRequest;
	FGuid PreviousMatch;
	{
		FServerLifecycleFixture First;
		First.PrepareServices();
		ALDPlayerController* Owner = First.CreateController();
		ALDPlayerController* Peer = First.CreateController();
		if (!Owner || !Peer)
		{
			AddError(TEXT("First match needs two real controllers"));
			return false;
		}
		First.Mode->PostLogin(Owner);
		First.Mode->PostLogin(Peer);
		First.Mode->DispatchBeginPlay();
		PreviousRequest = CommandFor(*Owner, 1);
		PreviousMatch = Owner->GetPlayerState<ALDPlayerState>()->GetParticipantContext().MatchId;
		TestEqual(TEXT("Previous payload was a valid committed summon"),
		               Owner->SubmitServerCommand(PreviousRequest).ResultCode, ELDCommandResultCode::Success);
		First.Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
	}
	FServerLifecycleFixture Next;
	Next.PrepareServices();
	ALDPlayerController* Owner = Next.CreateController();
	ALDPlayerController* Peer = Next.CreateController();
	if (!Owner || !Peer)
	{
		AddError(TEXT("Next match needs two real controllers"));
		return false;
	}
	Next.Mode->PostLogin(Owner);
	Next.Mode->PostLogin(Peer);
	Next.Mode->DispatchBeginPlay();
	const FLDParticipantContext Current = Owner->GetPlayerState<ALDPlayerState>()->GetParticipantContext();
	TestTrue(TEXT("A fresh World owns a distinct match"), Current.MatchId != PreviousMatch);
	TestTrue(TEXT("New match does not reuse the earlier wire generation"),
	              Current.ConnectionEpoch != PreviousRequest.ConnectionEpoch);
	const int32 RandomBefore = Next.Mode->GetEconomyService()->GetRandomState(0);
	const FLDCommandResult Replayed = Owner->SubmitServerCommand(PreviousRequest);
	TestEqual(TEXT("Old wire payload rejected before fresh-match purchase"), Replayed.ResultCode,
	               ELDCommandResultCode::InvalidEpoch);
	TestEqual(TEXT("Fresh balance remains 100"), Next.Mode->GetEconomyService()->GetSnapshot(0).Gold, 100);
	TestEqual(TEXT("Fresh paid summon count remains zero"),
	               Next.Mode->GetEconomyService()->GetSnapshot(0).PaidSummonCount, 0);
	TestEqual(TEXT("Fresh population remains zero"), Next.Mode->GetBoardManager()->GetSnapshot(0).Population, 0);
	TestEqual(TEXT("Fresh board revision remains zero"), Next.Mode->GetBoardManager()->GetSnapshot(0).BoardRevision, 0);
	TestEqual(TEXT("Stale payload does not draw randomness"), Next.Mode->GetEconomyService()->GetRandomState(0),
	               RandomBefore);
	TestEqual(TEXT("Stale payload is not entered into new-match cache"),
	               Next.Mode->GetCommandProcessor()->GetCachedResultCount(0), 0);
	Next.Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
	return true;
}

#endif
