#include "Board/LDBoardManager.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Battle/LDUnitActor.h"
#include "Economy/LDEconomyService.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
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
			    TestEqual(TEXT("Reentrant same request remains pending until its first publication finishes"),
			                   Fixture.Processor->SubmitAtTime(Fixture.Players[0], First, Fixture.Now).ResultCode,
			                   ELDCommandResultCode::Pending);
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

#endif
