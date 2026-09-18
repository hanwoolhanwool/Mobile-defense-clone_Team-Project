#include "Board/LDBoardManager.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Battle/LDUnitActor.h"
#include "Components/PrimitiveComponent.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Economy/LDEconomyService.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Network/LDCommandProcessor.h"

namespace
{
	struct FGameplayFixture
	{
		UWorld* World = nullptr;
		ULDGameData* Data = nullptr;
		ULDBoardManager* Board = nullptr;
		ULDEconomyService* Economy = nullptr;
		ULDCommandProcessor* Processor = nullptr;
		FLDMatchContext Match;
		FLDParticipantContext Players[2];
		uint32 NextRequest[2] = {1, 1};
		double Now = 1;
		FString Error;
		bool bReady = false;

		explicit FGameplayFixture(FLDPrepareUnit Prepare = {})
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			Data = NewObject<ULDGameData>(World);
			if (!Data->LoadP0(Error))
			{
				return;
			}
			Match.MatchId = FGuid::NewGuid();
			Match.RulesVersion = TEXT("0.3.0");
			Board = NewObject<ULDBoardManager>(World);
			Economy = NewObject<ULDEconomyService>(World);
			Processor = NewObject<ULDCommandProcessor>(World);
			bReady = Board->Initialize(*World, Match, *Data, MoveTemp(Prepare)) &&
			         Economy->Initialize(Match, *Data, 1776) && Processor->Initialize(Match, Data->GetRules()) &&
			         Processor->BindServices(*Board, *Economy);
			for (int32 Player = 0; Player < 2; ++Player)
			{
				Players[Player].MatchId = Match.MatchId;
				Players[Player].PlayerIndex = Player;
				Players[Player].ConnectionEpoch = 1;
				bReady &= Processor->RegisterParticipant(Players[Player]);
			}
			Processor->SetAcceptingCommands(true);
		}

		~FGameplayFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		FLDCommand Make(ELDCommandType Type = ELDCommandType::Summon, int32 Player = 0)
		{
			FLDCommand Command;
			Command.ConnectionEpoch = 1;
			Command.RequestId = NextRequest[Player]++;
			Command.ExpectedBoardRevision = Board->GetSnapshot(Player).BoardRevision;
			Command.CommandType = Type;
			return Command;
		}

		FLDCommandResult Run(const FLDCommand& Command, int32 Player = 0)
		{
			Now += 1;
			return Processor->SubmitAtTime(Players[Player], Command, Now);
		}

		void Reward(uint64 EventId, FName Enemy = TEXT("B01"), double Age = 30)
		{
			FLDCombatDeath Death;
			Death.MatchId = Match.MatchId;
			Death.DeathEventId = EventId;
			Death.EnemyId = EventId;
			Death.SpawnSerial = EventId;
			Death.EnemyTypeId = Enemy;
			Death.SpawnWaveIndex = 1;
			Death.SpawnedServerSeconds = 10;
			Death.DeathServerSeconds = 10 + Age;
			Processor->EnqueueCombatReward(Death);
			Processor->DrainCombatRewards();
		}

		bool AwardKnownUnit(FName UnitId)
		{
			// Explicit board-only setup through the real public preparation API.
			// This neither claims a random paid summon nor mutates the economic source.
			FLDBoardPlan Plan;
			const FLDCommand Award = Make();
			if (Board->TryPrepare(Players[0], Award, UnitId, Now, Plan) != ELDCommandResultCode::Success ||
			    !Board->ValidatePrepared(Plan))
			{
				Board->CancelPrepared(Plan);
				return false;
			}
			Board->CommitPrepared(Plan);
			Board->PublishPrepared(Plan);
			return true;
		}

		FString Signature(int32 Player = 0) const
		{
			const FLDEconomySnapshot Money = Economy->GetSnapshot(Player);
			const FLDBoardSnapshot State = Board->GetSnapshot(Player);
			FString Value = FString::Printf(
			    TEXT("%d/%d/%d/%d/%d/%d/%d"), Money.Gold, Money.Stars, Money.PaidSummonCount, Money.EconomyRevision,
			         Economy->GetRandomState(Player), State.BoardRevision, State.Population);
			for (const FLDPlacedUnit& Unit : State.Units)
			{
				Value += FString::Printf(TEXT("|%llu:%s:%d:%.6f"), Unit.InstanceId, *Unit.UnitId.ToString(),
				                              Unit.CellId, Unit.MoveBlockedUntilServerSeconds);
			}
			return Value;
		}
	};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0EconomyAtomicTest, "LD.P0.G2.Commands.AtomicSummonAndFailure",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0EconomyAtomicTest::RunTest(const FString& Parameters)
{
	FGameplayFixture Fixture;
	if (!TestTrue(TEXT("Real data and server services initialize"), Fixture.bReady))
	{
		return false;
	}
	const FLDCommand First = Fixture.Make();
	bool bObservedJointCommit = false;
	Fixture.Board->OnBoardCommitted.AddLambda(
	    [&](const FLDBoardCommit& Commit)
	    {
		    if (Commit.BoardRevision == 1)
		    {
			    bObservedJointCommit =
			        Fixture.Economy->GetSnapshot(0).Gold == 80 && Fixture.Board->GetSnapshot(0).Population == 1;
			    TestEqual(TEXT("Published joint commit already has a terminal cached outcome"),
			                   Fixture.Processor->SubmitAtTime(Fixture.Players[0], First, Fixture.Now).ResultCode,
			                   ELDCommandResultCode::Success);
		    }
	    });
	const FLDCommandResult FirstResult = Fixture.Run(First);
	TestEqual(TEXT("First summon commits"), FirstResult.ResultCode, ELDCommandResultCode::Success);
	TestTrue(TEXT("Publication observes both committed sources"), bObservedJointCommit);
	const FString AfterFirst = Fixture.Signature();
	TestEqual(TEXT("Same request replays original event"), Fixture.Run(First).EventId, FirstResult.EventId);
	TestEqual(TEXT("Replay changes no gameplay source"), Fixture.Signature(), AfterFirst);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestEqual(TEXT("First four purchases fit starting gold"), Fixture.Run(Fixture.Make()).ResultCode,
		               ELDCommandResultCode::Success);
	}
	TestEqual(TEXT("Independent 100-(20+22+24+26) expectation"), Fixture.Economy->GetSnapshot(0).Gold, 8);
	TestEqual(TEXT("Only successful paid summons increment n"), Fixture.Economy->GetSnapshot(0).PaidSummonCount, 4);
	const FString BeforeFailure = Fixture.Signature();
	TestEqual(TEXT("Fifth purchase rejects insufficient gold"), Fixture.Run(Fixture.Make()).ResultCode,
	               ELDCommandResultCode::InsufficientResource);
	TestEqual(TEXT("Failure keeps gold stars n RNG board and revisions"), Fixture.Signature(), BeforeFailure);
	FLDCommand Sell = Fixture.Make(ELDCommandType::Sell);
	Sell.InstanceId = 1;
	TestEqual(TEXT("Common sale succeeds"), Fixture.Run(Sell).ResultCode, ELDCommandResultCode::Success);
	TestEqual(TEXT("Sale uses half current next price28, not original cost20"), Fixture.Economy->GetSnapshot(0).Gold,
	               22);
	TestEqual(TEXT("Sale does not change next summon price"), Fixture.Economy->GetSnapshot(0).NextSummonGold, 28);
	FGameplayFixture FailedPreparation(
	    [](UWorld&, const FLDPlacedUnit&, const FLDUnitRow&, const FTransform&) -> ALDUnitActor* { return nullptr; });
	const FString BeforePreparation = FailedPreparation.Signature();
	TestEqual(TEXT("Explicit unavailable actor adapter rejects preparation"),
	               FailedPreparation.Run(FailedPreparation.Make()).ResultCode, ELDCommandResultCode::InvalidData);
	TestEqual(TEXT("Actor preparation failure preserves all gameplay state"), FailedPreparation.Signature(),
	               BeforePreparation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0StackCommandsTest, "LD.P0.G2.Commands.StackMoveMergeAndRefill",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0StackCommandsTest::RunTest(const FString& Parameters)
{
	FGameplayFixture Fixture;
	if (!TestTrue(TEXT("Services initialize"), Fixture.bReady))
	{
		return false;
	}
	Fixture.Reward(1);
	for (int32 Count = 1; Count <= 7; ++Count)
	{
		TestEqual(TEXT("Fixed seed purchases pass through actual command pipeline"),
		               Fixture.Run(Fixture.Make()).ResultCode, ELDCommandResultCode::Success);
		const FLDBoardSnapshot State = Fixture.Board->GetSnapshot(0);
		TestEqual(TEXT("One population per successful summon"), State.Population, Count);
		for (const FLDPlacedUnit& Unit : State.Units)
		{
			TestEqual(TEXT("Independent seed1776 first seven expected results"), Unit.UnitId, FName(TEXT("C01")));
			const int32 ExpectedCell = Unit.InstanceId <= 3 ? 17 : (Unit.InstanceId <= 6 ? 11 : 5);
			TestEqual(TEXT("Partial stack then screen-column first empty cell"), Unit.CellId, ExpectedCell);
		}
	}
	ALDUnitActor* Seventh = nullptr;
	Fixture.Board->TryGetCommittedUnitActor(7, Seventh);
	FLDCommand Sell = Fixture.Make(ELDCommandType::Sell);
	Sell.InstanceId = 1;
	const FLDCommandResult Sale = Fixture.Run(Sell);
	ALDUnitActor* Refilled = nullptr;
	Fixture.Board->TryGetCommittedUnitActor(7, Refilled);
	TestTrue(TEXT("Automatic refill moves existing actor and identity"), Seventh && Seventh == Refilled);
	if (!Refilled)
	{
		return false;
	}
	TestEqual(TEXT("Sale refill transfers from old partial into sold full stack"), Refilled->GetPlacement().CellId, 17);
	TestEqual(TEXT("Sale decreases population exactly one"), Fixture.Board->GetSnapshot(0).Population, 6);
	TestEqual(TEXT("Refill creates no summon ID"), Sale.CreatedInstanceIds.Num(), 0);
	FLDCommand Move = Fixture.Make(ELDCommandType::Move);
	Move.InstanceId = 2;
	Move.DestinationCellId = 0;
	const FLDCommandResult Moved = Fixture.Run(Move);
	TestEqual(TEXT("Whole source stack moves"), Moved.MovedInstanceIds.Num(), 3);
	const FString LockedState = Fixture.Signature();
	FLDCommand TooSoon = Fixture.Make(ELDCommandType::Move);
	TooSoon.InstanceId = 2;
	TooSoon.DestinationCellId = 1;
	TestEqual(TEXT("Move before .30s rejects"),
	               Fixture.Processor->SubmitAtTime(Fixture.Players[0], TooSoon, Fixture.Now + .2).ResultCode,
	               ELDCommandResultCode::Locked);
	TestEqual(TEXT("Locked move changes no source"), Fixture.Signature(), LockedState);
	FLDCommand Merge = Fixture.Make(ELDCommandType::Merge);
	Merge.InstanceId = 2;
	Merge.ConsumedInstanceId0 = 2;
	Merge.ConsumedInstanceId1 = 3;
	Merge.ConsumedInstanceId2 = 7;
	const FLDCommandResult Merged = Fixture.Run(Merge);
	TestEqual(TEXT("Selected stack merges"), Merged.ResultCode, ELDCommandResultCode::Success);
	TestEqual(TEXT("Merge removes exact selected three"), Merged.RemovedInstanceIds.Num(), 3);
	TestEqual(TEXT("Merge creates one result"), Merged.CreatedInstanceIds.Num(), 1);
	if (Merged.CreatedInstanceIds.IsEmpty())
	{
		return false;
	}
	FLDPlacedUnit ResultUnit;
	Fixture.Board->TryGetUnit(Merged.CreatedInstanceIds[0], ResultUnit);
	TestEqual(TEXT("First screen empty slot wins over selected cell0"), ResultUnit.CellId, 17);
	TestEqual(TEXT("Merge population decreases two"), Fixture.Board->GetSnapshot(0).Population, 4);
	TestEqual(TEXT("Merge and sale keep paid count7"), Fixture.Economy->GetSnapshot(0).PaidSummonCount, 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0RewardAndRevalidationTest, "LD.P0.G2.Commands.RewardsAndPreparedRevision",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0RewardAndRevalidationTest::RunTest(const FString& Parameters)
{
	FGameplayFixture Fixture;
	if (!TestTrue(TEXT("Services initialize"), Fixture.bReady))
	{
		return false;
	}
	Fixture.Reward(1, TEXT("N01"));
	Fixture.Reward(1, TEXT("N01"));
	Fixture.Reward(2, TEXT("B01"), 30.0);
	Fixture.Reward(3, TEXT("B01"), 30.0001);
	for (int32 Player = 0; Player < 2; ++Player)
	{
		TestEqual(TEXT("One normal and two boss gold rewards, duplicated death ignored"),
		               Fixture.Economy->GetSnapshot(Player).Gold, 301);
		TestEqual(TEXT("Exactly30s fast boss included; later boss no bonus"),
		               Fixture.Economy->GetSnapshot(Player).Stars, 5);
	}
	const FLDCommand Pending = Fixture.Make();
	FLDEconomyPlan EconomyPlan;
	FLDBoardPlan BoardPlan;
	TestEqual(TEXT("Plan can prepare without changing sources"),
	               Fixture.Economy->TryPrepare(0, Pending, NAME_None, EconomyPlan), ELDCommandResultCode::Success);
	TestEqual(
	    TEXT("Board actor prepares while hidden"),
	         Fixture.Board->TryPrepare(Fixture.Players[0], Pending, EconomyPlan.ResultUnitId, Fixture.Now, BoardPlan),
	         ELDCommandResultCode::Success);
	TestEqual(TEXT("Another admitted request commits first"), Fixture.Run(Fixture.Make()).ResultCode,
	               ELDCommandResultCode::Success);
	const FString CommittedState = Fixture.Signature();
	TestFalse(TEXT("Old board plan must fail final revision validation"), Fixture.Board->ValidatePrepared(BoardPlan));
	TestFalse(TEXT("Old economy plan must fail final revision validation"),
	               Fixture.Economy->ValidatePrepared(EconomyPlan));
	Fixture.Board->CancelPrepared(BoardPlan);
	Fixture.Board->CancelPrepared(BoardPlan);
	TestEqual(TEXT("Repeated cancellation preserves the later committed sources"), Fixture.Signature(), CommittedState);
	Fixture.Processor->Close();
	Fixture.Reward(4);
	TestEqual(TEXT("Terminal new death does not change balance or sources"), Fixture.Signature(), CommittedState);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0PopulationOwnershipTest,
                                 "LD.P0.G2.Commands.PopulationOwnershipAndPreparedIsolation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0PopulationOwnershipTest::RunTest(const FString& Parameters)
{
	FGameplayFixture Fixture;
	if (!TestTrue(TEXT("Services initialize"), Fixture.bReady))
	{
		return false;
	}
	// Explicit board-only fixture: known unit awards, no client economy or random-summon success is simulated.
	for (int32 Count = 0; Count < 20; ++Count)
	{
		FLDBoardPlan Plan;
		const FLDCommand Award = Fixture.Make(ELDCommandType::Summon, 1);
		TestEqual(TEXT("Known C01 fixture fits board up to population20"),
		               Fixture.Board->TryPrepare(Fixture.Players[1], Award, TEXT("C01"), 1, Plan),
		                                         ELDCommandResultCode::Success);
		if (!TestTrue(TEXT("Prepared fixture is valid"), Fixture.Board->ValidatePrepared(Plan)))
		{
			return false;
		}
		ALDUnitActor* Uncommitted = nullptr;
		TestFalse(TEXT("Prepared ID is never exposed as a committed actor"),
		               Fixture.Board->TryGetCommittedUnitActor(Plan.ExpectedNextInstanceId, Uncommitted));
		Fixture.Board->CommitPrepared(Plan);
		Fixture.Board->PublishPrepared(Plan);
	}
	const FString FullState = Fixture.Signature(1);
	TestEqual(TEXT("Population cap rejects before random draw"),
	               Fixture.Run(Fixture.Make(ELDCommandType::Summon, 1), 1).ResultCode,
	               ELDCommandResultCode::LimitReached);
	TestEqual(TEXT("Population rejection preserves economy RNG board and revisions"), Fixture.Signature(1), FullState);
	FLDCommand ForeignSale = Fixture.Make(ELDCommandType::Sell);
	ForeignSale.InstanceId = 1;
	const FString OwnBefore = Fixture.Signature();
	TestEqual(TEXT("Actual foreign instance sale rejects owner mismatch"), Fixture.Run(ForeignSale).ResultCode,
	               ELDCommandResultCode::NotOwner);
	TestEqual(TEXT("Owner mismatch leaves requester state untouched"), Fixture.Signature(), OwnBefore);
	TestEqual(TEXT("Owner mismatch leaves actual owner state untouched"), Fixture.Signature(1), FullState);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0EpochDuringPublicationTest, "LD.P0.G2.Commands.EpochReplacementDuringPublication",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0EpochDuringPublicationTest::RunTest(const FString& Parameters)
{
	FGameplayFixture Fixture;
	if (!TestTrue(TEXT("Services initialize"), Fixture.bReady))
	{
		return false;
	}
	Fixture.Board->OnBoardCommitted.AddLambda(
	    [&](const FLDBoardCommit& Commit)
	    {
		    if (Commit.BoardRevision == 1)
		    {
			    Fixture.Players[0].ConnectionEpoch = 2;
			    TestTrue(TEXT("Real publish callback can replace the server connection epoch"),
			                  Fixture.Processor->RegisterParticipant(Fixture.Players[0]));
		    }
	    });
	const FLDCommandResult OldResult = Fixture.Run(Fixture.Make());
	TestEqual(TEXT("Original response retains its admitted epoch despite caller context mutation"),
	               OldResult.ConnectionEpoch, uint64(1));
	FLDCommand Fresh;
	Fresh.ConnectionEpoch = 2;
	Fresh.RequestId = 1;
	Fresh.ExpectedBoardRevision = 1;
	const FLDCommandResult NewResult = Fixture.Run(Fresh);
	TestEqual(TEXT("New epoch request1 executes instead of seeing old epoch cached result"), NewResult.ConnectionEpoch,
	               uint64(2));
	TestEqual(TEXT("Fresh epoch purchase commits"), NewResult.ResultCode, ELDCommandResultCode::Success);
	TestEqual(TEXT("Only two real purchases are reflected"), Fixture.Board->GetSnapshot(0).Population, 2);
	TestEqual(TEXT("Second purchase uses next price22"), Fixture.Economy->GetSnapshot(0).Gold, 58);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0ExpiredSnapshotTest, "LD.P0.G2.Commands.ExpiredRequiresCurrentRevisions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0ExpiredSnapshotTest::RunTest(const FString& Parameters)
{
	FGameplayFixture Fixture;
	if (!TestTrue(TEXT("Services initialize"), Fixture.bReady))
	{
		return false;
	}
	const FLDCommand First = Fixture.Make();
	TestEqual(TEXT("Original request purchases exactly one unit"), Fixture.Run(First).ResultCode,
	               ELDCommandResultCode::Success);
	for (int32 Index = 0; Index < 256; ++Index)
	{
		FLDCommand Stale = Fixture.Make();
		Stale.ExpectedBoardRevision = 0;
		TestEqual(TEXT("Valid later requests evict history without changing the board"), Fixture.Run(Stale).ResultCode,
		               ELDCommandResultCode::StaleBoard);
	}
	Fixture.Reward(1, TEXT("N01"));
	const FString BeforeExpired = Fixture.Signature();
	const FLDCommandResult Expired = Fixture.Run(First);
	TestEqual(TEXT("Evicted request is never replayed as a new purchase"), Expired.ResultCode,
	               ELDCommandResultCode::RequestExpired);
	TestEqual(TEXT("Expired response requires board revision of the existing purchase"), Expired.NewBoardRevision, 1);
	TestEqual(TEXT("Expired response includes economic revision after the later reward"), Expired.EconomyRevision, 2);
	TestEqual(TEXT("History expiry mutates no gameplay source"), Fixture.Signature(), BeforeExpired);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0ControllerSnapshotOrderTest, "LD.P0.G2.Commands.ControllerResponseSnapshotOrders",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0ControllerSnapshotOrderTest::RunTest(const FString& Parameters)
{
	for (bool bExpired : {false, true})
	{
		for (bool bSnapshotFirst : {false, true})
		{
			FGameplayFixture Fixture;
			if (!TestTrue(TEXT("Real services initialize"), Fixture.bReady))
			{
				return false;
			}
			const FLDBoardSnapshot InitialBoard = Fixture.Board->GetSnapshot(0);
			const FLDEconomySnapshot InitialEconomy = Fixture.Economy->GetSnapshot(0);
			FLDCommand Original;
			Original.ConnectionEpoch = 1;
			Original.RequestId = 1;
			if (bExpired)
			{
				Fixture.Run(Fixture.Make());
				for (int32 Index = 0; Index < 256; ++Index)
				{
					FLDCommand Stale = Fixture.Make();
					Stale.ExpectedBoardRevision = 0;
					Fixture.Run(Stale);
				}
				Fixture.Reward(1, TEXT("N01"));
			}
			ALDPlayerController* Controller = Fixture.World->SpawnActor<ALDPlayerController>();
			ALDPlayerState* State = Fixture.World->SpawnActor<ALDPlayerState>();
			if (!TestNotNull(TEXT("Actual Controller"), Controller) || !TestNotNull(TEXT("Actual PlayerState"), State))
			{
				return false;
			}
			// An explicit local owner without a viewport exercises actual PC handlers, not a copied UI state model.
			Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
			State->InitializeParticipant(Fixture.Players[0]);
			Controller->SetPlayerState(State);
			Controller->InitializeServerSession(Fixture.Players[0], *Fixture.Processor);
			Controller->PublishSnapshots(InitialBoard, InitialEconomy);
			TestTrue(TEXT("Session participant and owner envelope are coherent without rendering"),
			              Controller->IsGameplaySnapshotReady());
			TestFalse(TEXT("A headless owner cannot issue a visual summon intent"), Controller->RequestSummon());
			// Advance only the existing response bucket clock to suppress the automatic synchronous response.
			// The test then delivers that real result through the public client handler in a chosen arrival order.
			const double DelayedResponseClock = FPlatformTime::Seconds() + 60;
			for (int32 Index = 0; Index < 12; ++Index)
			{
				Fixture.Processor->CanSendResponse(Fixture.Players[0], DelayedResponseClock);
			}
			TestTrue(TEXT("Actual local command path admits one request"), Controller->SubmitLocalCommand(Original));
			TestTrue(TEXT("Suppressed response leaves the original request pending"), Controller->HasPendingCommand());
			const FLDCommandResult Result = Fixture.Run(Original);
			TestEqual(TEXT("The real server result has the selected success or expiry outcome"), Result.ResultCode,
			               bExpired ? ELDCommandResultCode::RequestExpired : ELDCommandResultCode::Success);
			const FLDBoardSnapshot CurrentBoard = Fixture.Board->GetSnapshot(0);
			const FLDEconomySnapshot CurrentEconomy = Fixture.Economy->GetSnapshot(0);
			if (bSnapshotFirst)
			{
				Controller->PublishSnapshots(CurrentBoard, CurrentEconomy);
				TestTrue(TEXT("Snapshot cannot complete an unanswered request"), Controller->HasPendingCommand());
			}
			Controller->ClientCommandResult_Implementation(Result);
			if (!bSnapshotFirst)
			{
				TestTrue(TEXT("Response alone waits for both source revisions"), Controller->HasPendingCommand());
				TestFalse(TEXT("A new request is rejected while response state is missing"),
				               Controller->SubmitLocalCommand(Original));
				Controller->PublishSnapshots(CurrentBoard, InitialEconomy);
				TestTrue(TEXT("Board revision alone does not open input"), Controller->HasPendingCommand());
				Controller->PublishSnapshots(InitialBoard, CurrentEconomy);
				TestTrue(TEXT("Economy revision alone does not open input"), Controller->HasPendingCommand());
				Controller->PublishSnapshots(CurrentBoard, CurrentEconomy);
			}
			TestFalse(TEXT("Both revisions and response release the pending gate in either order"),
			               Controller->HasPendingCommand());
			TestEqual(TEXT("Response handling never repeats the original purchase"),
			               Fixture.Board->GetSnapshot(0).Population, 1);
			FLDParticipantContext NextEpoch = Fixture.Players[0];
			NextEpoch.ConnectionEpoch = 2;
			Fixture.Processor->RegisterParticipant(NextEpoch);
			Controller->InitializeServerSession(NextEpoch, *Fixture.Processor);
			TestFalse(TEXT("New session cannot reuse the old participant or snapshot epoch"),
			               Controller->IsGameplaySnapshotReady());
			ALDPlayerState* NextState = Fixture.World->SpawnActor<ALDPlayerState>();
			NextState->InitializeParticipant(NextEpoch);
			Controller->SetPlayerState(NextState);
			TestFalse(TEXT("New participant alone cannot reuse the old owner envelope"),
			               Controller->IsGameplaySnapshotReady());
			Controller->PublishSnapshots(CurrentBoard, CurrentEconomy);
			TestTrue(TEXT("Current participant session and envelope open the snapshot gate"),
			              Controller->IsGameplaySnapshotReady());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0WholeStackSwapTest, "LD.P0.G2.Commands.WholeStackSwapKinds",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0WholeStackSwapTest::RunTest(const FString& Parameters)
{
	for (bool bSameKind : {true, false})
	{
		FGameplayFixture Fixture;
		if (!TestTrue(TEXT("Services initialize"), Fixture.bReady))
		{
			return false;
		}
		for (int32 Index = 0; Index < 5; ++Index)
		{
			if (!TestTrue(TEXT("Explicit 3+2 stack preparation succeeds"),
			                   Fixture.AwardKnownUnit(Index < 3 || bSameKind ? TEXT("C01") : TEXT("C02"))))
			{
				return false;
			}
		}
		const FLDEconomySnapshot MoneyBefore = Fixture.Economy->GetSnapshot(0);
		const int32 RandomBefore = Fixture.Economy->GetRandomState(0);
		TMap<uint64, ALDUnitActor*> ActorsBefore;
		for (uint64 Id = 1; Id <= 5; ++Id)
		{
			ALDUnitActor* Actor = nullptr;
			Fixture.Board->TryGetCommittedUnitActor(Id, Actor);
			ActorsBefore.Add(Id, Actor);
		}
		FLDCommand Move = Fixture.Make(ELDCommandType::Move);
		Move.InstanceId = 1;
		Move.DestinationCellId = 11;
		const FLDCommandResult Result = Fixture.Run(Move);
		TestEqual(TEXT("Same and different kinds both swap whole destination stacks"), Result.ResultCode,
		               ELDCommandResultCode::Success);
		TestEqual(TEXT("Exactly five existing IDs are moved"), Result.MovedInstanceIds.Num(), 5);
		TestEqual(TEXT("Swap creates no IDs"), Result.CreatedInstanceIds.Num(), 0);
		TestEqual(TEXT("Swap removes no IDs"), Result.RemovedInstanceIds.Num(), 0);
		TestEqual(TEXT("Population remains five, never absorbs same-kind stacks"),
		               Fixture.Board->GetSnapshot(0).Population, 5);
		for (uint64 Id = 1; Id <= 5; ++Id)
		{
			FLDPlacedUnit Unit;
			TestTrue(TEXT("Every original ID still exists"), Fixture.Board->TryGetUnit(Id, Unit));
			TestEqual(TEXT("Whole source and destination memberships exchange"), Unit.CellId, Id <= 3 ? 11 : 17);
			TestEqual(TEXT("Swap preserves each original kind"), Unit.UnitId,
			               FName(Id <= 3 || bSameKind ? TEXT("C01") : TEXT("C02")));
			TestTrue(TEXT("Both complete stacks receive the independent t2+.30 movement lock"),
			              FMath::IsNearlyEqual(Unit.MoveBlockedUntilServerSeconds, 2.3, 0.000001));
			ALDUnitActor* Actor = nullptr;
			TestTrue(TEXT("Original actors remain committed"), Fixture.Board->TryGetCommittedUnitActor(Id, Actor));
			TestTrue(TEXT("Whole-stack swap does not recreate actors"), Actor == ActorsBefore.FindRef(Id));
		}
		const FLDEconomySnapshot MoneyAfter = Fixture.Economy->GetSnapshot(0);
		TestEqual(TEXT("Swap preserves gold"), MoneyAfter.Gold, MoneyBefore.Gold);
		TestEqual(TEXT("Swap preserves stars"), MoneyAfter.Stars, MoneyBefore.Stars);
		TestEqual(TEXT("Swap preserves paid summon count"), MoneyAfter.PaidSummonCount, MoneyBefore.PaidSummonCount);
		TestEqual(TEXT("Swap preserves next price"), MoneyAfter.NextSummonGold, MoneyBefore.NextSummonGold);
		TestEqual(TEXT("Swap preserves economic revision"), MoneyAfter.EconomyRevision, MoneyBefore.EconomyRevision);
		TestEqual(TEXT("Swap consumes no random state"), Fixture.Economy->GetRandomState(0), RandomBefore);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0GradeEconomicRulesTest, "LD.P0.G2.Commands.GradeSalesAndLegendaryMerge",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0GradeEconomicRulesTest::RunTest(const FString& Parameters)
{
	const FName Kinds[] = {TEXT("R01"), TEXT("E01"), TEXT("L01")};
	const int32 ExpectedStars[] = {1, 2, 4};
	for (int32 Grade = 0; Grade < 3; ++Grade)
	{
		FGameplayFixture Fixture;
		if (!TestTrue(TEXT("Services and explicit grade award initialize"),
		                   Fixture.bReady && Fixture.AwardKnownUnit(Kinds[Grade])))
		{
			return false;
		}
		for (int32 Purchase = 0; Purchase < 4; ++Purchase)
		{
			if (!TestEqual(TEXT("Four real purchases establish a nonzero paid summon history"),
			                    Fixture.Run(Fixture.Make()).ResultCode, ELDCommandResultCode::Success))
			{
				return false;
			}
		}
		const int32 RandomBefore = Fixture.Economy->GetRandomState(0);
		FLDCommand Sell = Fixture.Make(ELDCommandType::Sell);
		Sell.InstanceId = 1;
		TestEqual(TEXT("Actual command sells the known higher grade"), Fixture.Run(Sell).ResultCode,
		               ELDCommandResultCode::Success);
		const FLDEconomySnapshot Money = Fixture.Economy->GetSnapshot(0);
		TestEqual(TEXT("Rare Epic Legendary grant independent star amounts1 2 4"), Money.Stars, ExpectedStars[Grade]);
		TestEqual(TEXT("Higher-grade sale gives no gold after four paid purchases"), Money.Gold, 8);
		TestEqual(TEXT("Sale preserves the nonzero paid summon count"), Money.PaidSummonCount, 4);
		TestEqual(TEXT("Sale preserves next summon price28"), Money.NextSummonGold, 28);
		TestEqual(TEXT("Sale consumes no random draw"), Fixture.Economy->GetRandomState(0), RandomBefore);
		TestEqual(TEXT("Exactly the awarded actor leaves the four paid units behind"),
		               Fixture.Board->GetSnapshot(0).Population, 4);
	}
	FGameplayFixture Legendary;
	if (!TestTrue(TEXT("Legendary fixture initializes"), Legendary.bReady))
	{
		return false;
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (!TestTrue(TEXT("Known legendary materials use real board preparation"),
		                   Legendary.AwardKnownUnit(TEXT("L01"))))
		{
			return false;
		}
	}
	const FString BeforeMerge = Legendary.Signature();
	FLDCommand Merge = Legendary.Make(ELDCommandType::Merge);
	Merge.InstanceId = Merge.ConsumedInstanceId0 = 1;
	Merge.ConsumedInstanceId1 = 2;
	Merge.ConsumedInstanceId2 = 3;
	TestEqual(TEXT("Three valid legendary materials still reject P1 progression"), Legendary.Run(Merge).ResultCode,
	               ELDCommandResultCode::FeatureDisabled);
	TestEqual(TEXT("Legendary rejection preserves gold stars n RNG revisions IDs cells locks"), Legendary.Signature(),
	               BeforeMerge);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0PreparedLifetimeTest, "LD.P0.G2.Commands.PreparedIsolationAndIdReservation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0PreparedLifetimeTest::RunTest(const FString& Parameters)
{
	FGameplayFixture Fixture;
	if (!TestTrue(TEXT("Services initialize"), Fixture.bReady))
	{
		return false;
	}
	FLDBoardPlan Prepared;
	TestEqual(TEXT("Explicit R02 prepares through real actor initialization"),
	               Fixture.Board->TryPrepare(Fixture.Players[0], Fixture.Make(), TEXT("R02"), 1, Prepared),
	                                         ELDCommandResultCode::Success);
	if (!TestEqual(TEXT("Exactly one actor is prepared"), Prepared.PreparedActors.Num(), 1))
	{
		return false;
	}
	ALDUnitActor* Actor = Prepared.PreparedActors[0];
	TArray<UPrimitiveComponent*> Primitives;
	Actor->GetComponents(Primitives);
	TestTrue(TEXT("Prepared actor contains real primitive components to inspect"), !Primitives.IsEmpty());
	for (const UPrimitiveComponent* Primitive : Primitives)
	{
		TestFalse(TEXT("Every prepared primitive is actually invisible"), Primitive->IsVisible());
		TestEqual(TEXT("Every prepared primitive has collision disabled"), Primitive->GetCollisionEnabled(),
		               ECollisionEnabled::NoCollision);
		TestFalse(TEXT("Every prepared primitive disables overlap generation"), Primitive->GetGenerateOverlapEvents());
	}
	TestFalse(TEXT("Prepared actor is actually not replicated"), Actor->GetIsReplicated());
	TestFalse(TEXT("Prepared actor is not committed"), Actor->IsCommitted());
	Fixture.Board->CancelPrepared(Prepared);
	TestTrue(TEXT("Cancellation enters the engine actor destruction path"), Actor->IsActorBeingDestroyed());
	const FLDCommandResult AfterCancel = Fixture.Run(Fixture.Make());
	TestTrue(TEXT("Cancelled reservation leaves the next successful ID at1"),
	              AfterCancel.CreatedInstanceIds == TArray<uint64>{1});
	int32 Attempts = 0;
	FGameplayFixture FailedOnce(
	    [&Attempts](UWorld& World, const FLDPlacedUnit& Unit, const FLDUnitRow& Row, const FTransform& Transform)
	    {
		    if (++Attempts == 1)
		    {
			    return static_cast<ALDUnitActor*>(nullptr);
		    }
		    ALDUnitActor* UnitActor = World.SpawnActor<ALDUnitActor>();
		    if (UnitActor && UnitActor->InitializePrepared(Unit, Row, Transform))
		    {
			    return UnitActor;
		    }
		    if (UnitActor)
		    {
			    UnitActor->Destroy();
		    }
		    return static_cast<ALDUnitActor*>(nullptr);
	    });
	if (!TestTrue(TEXT("Recovering adapter fixture initializes"), FailedOnce.bReady))
	{
		return false;
	}
	const FString BeforeFailure = FailedOnce.Signature();
	TestEqual(TEXT("Explicit first adapter failure rejects before commit"),
	               FailedOnce.Run(FailedOnce.Make()).ResultCode, ELDCommandResultCode::InvalidData);
	TestEqual(TEXT("Preparation failure leaves all gameplay sources unchanged"), FailedOnce.Signature(), BeforeFailure);
	const FLDCommandResult Retry = FailedOnce.Run(FailedOnce.Make());
	TestEqual(TEXT("Real retry succeeds after adapter recovers"), Retry.ResultCode, ELDCommandResultCode::Success);
	TestTrue(TEXT("Failed preparation did not consume ID1"), Retry.CreatedInstanceIds == TArray<uint64>{1});
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0RepeatedBossRewardsTest, "LD.P0.G2.Commands.TenfoldDeathAndTwoFastBosses",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0RepeatedBossRewardsTest::RunTest(const FString& Parameters)
{
	FGameplayFixture Fixture;
	if (!TestTrue(TEXT("Services initialize"), Fixture.bReady))
	{
		return false;
	}
	const int32 RandomBefore[] = {Fixture.Economy->GetRandomState(0), Fixture.Economy->GetRandomState(1)};
	for (uint64 EventId = 1; EventId <= 2; ++EventId)
	{
		for (int32 Repeat = 0; Repeat < 10; ++Repeat)
		{
			Fixture.Reward(EventId, TEXT("B01"), 30);
		}
	}
	for (int32 Player = 0; Player < 2; ++Player)
	{
		const FLDEconomySnapshot Money = Fixture.Economy->GetSnapshot(Player);
		TestEqual(TEXT("Each recipient gains exactly200 gold from two fast bosses"), Money.Gold - 100, 200);
		TestEqual(TEXT("Each recipient gains exactly6 stars including both fast bonuses"), Money.Stars, 6);
		TestEqual(TEXT("Ten deliveries per death still commit only two economic revisions"), Money.EconomyRevision, 2);
		TestEqual(TEXT("Repeated rewards consume no random state"), Fixture.Economy->GetRandomState(Player),
		               RandomBefore[Player]);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0DefensivePlacementTest, "LD.P0.G2.Commands.DefensivePlacementFailure",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0DefensivePlacementTest::RunTest(const FString& Parameters)
{
	FGameplayFixture Fixture;
	if (!TestTrue(TEXT("Services initialize"), Fixture.bReady))
	{
		return false;
	}
	// Normal P0 cannot fill18 cells: at most16 partial stacks plus two full stacks needs at least22 units.
	// Exercise only the public defensive non-P0 result rejection, not an invented reachable full-board state.
	const FString Before = Fixture.Signature();
	const FLDCommand Command = Fixture.Make();
	FLDEconomyPlan EconomyPlan;
	TestEqual(TEXT("Actual economic preparation may draw on its private copy"),
	               Fixture.Economy->TryPrepare(0, Command, NAME_None, EconomyPlan), ELDCommandResultCode::Success);
	FLDBoardPlan BoardPlan;
	TestEqual(TEXT("Non-P0 prepared result is rejected by defensive placement boundary"),
	               Fixture.Board->TryPrepare(Fixture.Players[0], Command, TEXT("M01"), 1, BoardPlan),
	                                         ELDCommandResultCode::NoSpace);
	Fixture.Board->CancelPrepared(BoardPlan);
	TestEqual(TEXT("Defensive placement failure preserves all original state including RNG"), Fixture.Signature(),
	               Before);
	return true;
}

#endif
