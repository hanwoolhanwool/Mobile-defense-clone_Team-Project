#include "Battle/LDCombatRules.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Battle/LDCombatService.h"
#include "Battle/LDEnemyActor.h"
#include "Battle/LDUnitActor.h"
#include "Board/LDBoardManager.h"
#include "Data/LDGameData.h"
#include "Economy/LDEconomyService.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Network/LDCommandProcessor.h"
#include <limits>

namespace
{
	struct FCombatFixture
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		ULDGameData* Data = nullptr;
		ULDCombatService* Combat = nullptr;
		FLDMatchContext Context;
		uint64 NextEnemyId = 100;

		bool Initialize(FString& Error)
		{
			if (!World)
			{
				return false;
			}
			Data = NewObject<ULDGameData>(World);
			if (!Data->LoadP0(Error))
			{
				return false;
			}
			Context.MatchId = FGuid::NewGuid();
			Context.RulesVersion = Data->GetRules().RulesVersion;
			Combat = NewObject<ULDCombatService>(World);
			return Combat->Initialize(Context, Data->GetRules());
		}

		ALDUnitActor* Unit(FName UnitId, uint64 Id = 1, int32 Player = 0, FVector Position = FVector::ZeroVector,
		                   double CommitTime = 0)
		{
			FLDUnitRow Row;
			if (!Data->TryGetUnitRow(UnitId, Row))
			{
				return nullptr;
			}
			ALDUnitActor* Actor = World->SpawnActor<ALDUnitActor>();
			FLDPlacedUnit Placement;
			Placement.InstanceId = Id;
			Placement.UnitId = UnitId;
			Placement.PlayerIndex = Player;
			Placement.CellId = Player * 18;
			if (!Actor || !Actor->InitializePrepared(Placement, Row, FTransform(Position)))
			{
				return nullptr;
			}
			Actor->ApplyCommittedPlacement(Placement, FTransform(Position));
			Combat->RegisterCommittedUnit(*Actor, CommitTime);
			return Actor;
		}

		ALDEnemyActor* Enemy(double HP = 70, FVector Position = FVector(100, 0, 0), int32 Route = 0,
		                     double SpawnTime = 0, double Speed = 0, bool bTowardOrigin = false)
		{
			ALDEnemyActor* Actor = World->SpawnActor<ALDEnemyActor>();
			FLDEnemyRow Row;
			Data->TryGetEnemyRow(TEXT("N01"), Row);
			const uint64 Id = NextEnemyId++;
			const double X = bTowardOrigin ? -500 : 500;
			const TArray<FVector> Points = {Position, Position + FVector(X, 0, 0), Position + FVector(X, 500, 0),
			                                Position + FVector(0, 500, 0)};
			if (!Actor || !Actor->InitializeRoute(Context.MatchId, Id, Route, Points, Speed, SpawnTime) ||
			    !Actor->InitializeCombat(Row, HP, Id, 1, SpawnTime) || !Combat->RegisterEnemy(*Actor))
			{
				return nullptr;
			}
			return Actor;
		}

		~FCombatFixture()
		{
			if (Combat)
			{
				Combat->Stop();
			}
			if (World)
			{
				World->DestroyWorld(false);
			}
		}
	};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDAllBasicAttacksTest, "LD.P0.G2.Combat.AllSixteenBasicAttacks",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDAllBasicAttacksTest::RunTest(const FString& Parameters)
{
	// Hand-entered expected attacks/intervals from the approved design table, not the implementation result.
	struct FExpected
	{
		const TCHAR* Id;
		int32 Damage;
		double Interval;
		double Range;
		const TCHAR* Presentation;
	};
	const FExpected Cases[] = {{TEXT("C01"), 15, 1, 175, TEXT("Melee")},
	                            {TEXT("C02"), 12, .8, 490, TEXT("Projectile")},
	                             {TEXT("C03"), 19, 1.2, 350, TEXT("Projectile")},
	                              {TEXT("C04"), 10, .7, 350, TEXT("Projectile")},
	                               {TEXT("R01"), 55, 1.1, 175, TEXT("Melee")},
	                                {TEXT("R02"), 40, .8, 490, TEXT("Projectile")},
	                                 {TEXT("R03"), 64, 1.2, 350, TEXT("Projectile")},
	                                  {TEXT("R04"), 38, 1, 350, TEXT("Projectile")},
	                                   {TEXT("E01"), 165, 1, 175, TEXT("Melee")},
	                                    {TEXT("E02"), 120, .75, 490, TEXT("Projectile")},
	                                     {TEXT("E03"), 215, 1.25, 350, TEXT("Projectile")},
	                                      {TEXT("E04"), 110, 1, 350, TEXT("Projectile")},
	                                       {TEXT("L01"), 650, 1.2, 175, TEXT("Melee")},
	                                        {TEXT("L02"), 460, 1, 490, TEXT("Projectile")},
	                                         {TEXT("L03"), 480, .8, 350, TEXT("Projectile")},
	                                          {TEXT("L04"), 390, 1, 350, TEXT("Projectile")}};
	for (const FExpected& Expected : Cases)
	{
		FCombatFixture Fixture;
		FString Error;
		if (!TestTrue(TEXT("Real P0 data/service initializes: ") + Error, Fixture.Initialize(Error)))
		{
			return false;
		}
		ALDUnitActor* Unit = Fixture.Unit(Expected.Id);
		ALDEnemyActor* Enemy = Fixture.Enemy(10000, FVector(Expected.Range, 0, 0));
		if (!TestNotNull(Expected.Id, Unit) || !TestNotNull(TEXT("Stationary explicit target"), Enemy))
		{
			return false;
		}
		Fixture.Combat->AdvanceCombatTo(.20);
		TestEqual(TEXT("No hit before initial .25"), Enemy->GetCombatSnapshot().HP, 10000.0);
		Fixture.Combat->AdvanceCombatTo(.25);
		TestEqual(FString(Expected.Id) + TEXT(" exact-range first hit"), Enemy->GetCombatSnapshot().HP,
		                                      10000.0 - Expected.Damage);
		double NextAt = 0;
		TestTrue(TEXT("Timer query"), Fixture.Combat->TryGetUnitAttackState(1, NextAt));
		TestEqual(TEXT("Next attack uses individual interval"), NextAt, .25 + Expected.Interval);
		TestEqual(TEXT("Presentation is per-row"), Unit->GetUnitRow().AttackPresentation, FName(Expected.Presentation));
		TestTrue(TEXT("Authoritative hit publishes cosmetic cue"), Unit->GetLastAttackCue().DamageEventId != 0);
		Fixture.Combat->AdvanceCombatTo(.30);
		Fixture.Combat->AdvanceCombatTo(.25 + Expected.Interval);
		TestEqual(TEXT("Second basic attack"), Enemy->GetCombatSnapshot().HP, 10000.0 - 2 * Expected.Damage);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCombatRulesBoundaryTest, "LD.P0.G2.Combat.DamageAndTargetBoundaries",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDCombatRulesBoundaryTest::RunTest(const FString& Parameters)
{
	FLDUnitRow Unit;
	Unit.BaseAttack = 15;
	Unit.DamageType = TEXT("Physical");
	int32 Damage = -7;
	TestTrue(TEXT("Physical 20 armor"), FLDCombatRules::TryCalculateDamage(Unit, 20, 0, Damage));
	TestEqual(TEXT("12.5 rounds upward once"), Damage, 13);
	FLDCombatRules::TryCalculateDamage(Unit, -500, 0, Damage);
	TestEqual(TEXT("Armor floor -50"), Damage, 30);
	Unit.BaseAttack = 19;
	Unit.DamageType = TEXT("Magic");
	FLDCombatRules::TryCalculateDamage(Unit, 900, .10, Damage);
	TestEqual(TEXT("Magic ignores armor"), Damage, 17);
	FLDCombatRules::TryCalculateDamage(Unit, -50, .95, Damage);
	TestEqual(TEXT("Resistance caps at .75"), Damage, 5);
	Unit.DamageType = TEXT("Unsupported");
	TestFalse(TEXT("Unknown damage kind"), FLDCombatRules::TryCalculateDamage(Unit, 0, 0, Damage));
	TestEqual(TEXT("Failure does not overwrite output"), Damage, 5);
	TArray<FLDTargetCandidate> Targets = {{30, 3, FVector(175.001, 0, 0), true},
	                                      {20, 2, FVector(175, 0, 999), true},
	                                      {40, 1, FVector(-175, 0, 0), true},
	                                      {10, 1, FVector(0, 175, 0), true}};
	TestEqual(TEXT("XY inclusive range, spawn tie then EnemyId"),
	               FLDCombatRules::SelectTarget(FVector::ZeroVector, 175, Targets), 3);
	Targets[3].bAlive = false;
	TestEqual(TEXT("Dead target excluded"), FLDCombatRules::SelectTarget(FVector::ZeroVector, 175, Targets), 2);
	Targets[2].CanonicalPosition = FVector(0, 100, 0);
	TestEqual(TEXT("Nearest beats other ordering"), FLDCombatRules::SelectTarget(FVector::ZeroVector, 175, Targets), 2);
	Targets.SetNum(1);
	TestEqual(TEXT("Range plus epsilon excluded"), FLDCombatRules::SelectTarget(FVector::ZeroVector, 175, Targets),
	               INDEX_NONE);
	double Predicted = -1;
	TestTrue(TEXT("Stationary fixture display supported"),
	              FLDRouteModel::TryPredictPresentationDistance(0, 0, 0, true, 10, Predicted));
	TestEqual(TEXT("Zero-speed display stays fixed"), Predicted, 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCombatDeathTest, "LD.P0.G2.Combat.DuplicateDamageAndSingleDeath",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDCombatDeathTest::RunTest(const FString& Parameters)
{
	FCombatFixture Fixture;
	FString Error;
	if (!TestTrue(TEXT("Fixture initializes"), Fixture.Initialize(Error)))
	{
		return false;
	}
	ALDEnemyActor* Enemy = Fixture.Enemy();
	if (!TestNotNull(TEXT("Enemy"), Enemy))
	{
		return false;
	}
	FLDDamageEvent Hit;
	Hit.MatchId = Fixture.Context.MatchId;
	Hit.DamageEventId = 1;
	Hit.SourceInstanceId = 1;
	Hit.EnemyId = Enemy->GetRouteSnapshot().EnemyId;
	Hit.Amount = 40;
	Hit.AttackServerSeconds = .25;
	FLDCombatDeath Death;
	Enemy->SetRole(ROLE_SimulatedProxy);
	TestTrue(TEXT("Client authority refused"), Enemy->TryApplyDamage(Hit, Death) == ELDDamageResult::Rejected);
	Enemy->SetRole(ROLE_Authority);
	const FGuid Match = Hit.MatchId;
	Hit.MatchId = FGuid::NewGuid();
	TestTrue(TEXT("Wrong match refused"), Enemy->TryApplyDamage(Hit, Death) == ELDDamageResult::Rejected);
	Hit.MatchId = Match;
	TestTrue(TEXT("First40"), Enemy->TryApplyDamage(Hit, Death) == ELDDamageResult::Applied);
	TestEqual(TEXT("HP70-40"), Enemy->GetCombatSnapshot().HP, 30.0);
	TestTrue(TEXT("Same damage duplicate"), Enemy->TryApplyDamage(Hit, Death) == ELDDamageResult::Duplicate);
	TestEqual(TEXT("Duplicate HP unchanged"), Enemy->GetCombatSnapshot().HP, 30.0);
	Hit.DamageEventId = 2;
	Hit.SourceInstanceId = 2;
	TestTrue(TEXT("Concurrent second40 kills once"), Enemy->TryApplyDamage(Hit, Death) == ELDDamageResult::Killed);
	TestEqual(TEXT("No negative HP"), Enemy->GetCombatSnapshot().HP, 0.0);
	const uint64 DeathId = Death.DeathEventId;
	TestTrue(TEXT("Stable nonzero death key"), DeathId != 0);
	TestTrue(TEXT("Killing damage repeated"), Enemy->TryApplyDamage(Hit, Death) == ELDDamageResult::Duplicate);
	Hit.DamageEventId = 3;
	TestTrue(TEXT("Different late damage cannot kill again"),
	              Enemy->TryApplyDamage(Hit, Death) == ELDDamageResult::Rejected);
	TestEqual(TEXT("Rejected damage output does not manufacture death"), Death.DeathEventId, DeathId);
	TestEqual(TEXT("No living enemy after single death"), Fixture.Combat->GetLivingEnemyCount(), 0);
	Enemy->StopCombat();
	Enemy->StopCombat();
	TestTrue(TEXT("Stopped damage cannot replay"), Enemy->TryApplyDamage(Hit, Death) == ELDDamageResult::Rejected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCombatCadenceTest, "LD.P0.G2.Combat.InitialCadenceMoveAndReplenishment",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDCombatCadenceTest::RunTest(const FString& Parameters)
{
	FCombatFixture Fixture;
	FString Error;
	if (!TestTrue(TEXT("Fixture initializes"), Fixture.Initialize(Error)))
	{
		return false;
	}
	ALDUnitActor* Unit = Fixture.Unit(TEXT("C01"));
	ALDEnemyActor* Enemy = Fixture.Enemy();
	if (!TestNotNull(TEXT("Unit"), Unit) || !TestNotNull(TEXT("Enemy"), Enemy))
	{
		return false;
	}
	int32 DeathCount = 0;
	Fixture.Combat->OnEnemyDeathCommitted.AddLambda([&DeathCount](const FLDCombatDeath&) { ++DeathCount; });
	const double HitTimes[] = {.25, 1.25, 2.25, 3.25, 4.25};
	const double HP[] = {55, 40, 25, 10, 0};
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Fixture.Combat->AdvanceCombatTo(HitTimes[Index] - .05);
		Fixture.Combat->AdvanceCombatTo(HitTimes[Index]);
		TestEqual(TEXT("C01 five independent expected HP values"), Enemy->GetCombatSnapshot().HP, HP[Index]);
		Fixture.Combat->AdvanceCombatTo(HitTimes[Index]);
		TestEqual(TEXT("Same step duplicate no HP change"), Enemy->GetCombatSnapshot().HP, HP[Index]);
	}
	TestEqual(TEXT("Single server death callback"), DeathCount, 1);
	ALDEnemyActor* Durable = Fixture.Enemy(10000);
	if (!TestNotNull(TEXT("Durable explicit fixture"), Durable))
	{
		return false;
	}
	Fixture.Combat->AdvanceCombatTo(5);
	double NextAt = 0;
	Fixture.Combat->TryGetUnitAttackState(1, NextAt);
	TestEqual(TEXT("NextAttackAt retained before move"), NextAt, 5.25);
	FLDPlacedUnit Moved = Unit->GetPlacement();
	Moved.CellId = 1;
	Moved.MoveBlockedUntilServerSeconds = 5.4;
	Unit->ApplyCommittedPlacement(Moved, FTransform(FVector(0, 10, 0)));
	Fixture.Combat->RegisterCommittedUnit(*Unit, 5.1);
	Fixture.Combat->TryGetUnitAttackState(1, NextAt);
	TestEqual(TEXT("Manual move never resets next timer"), NextAt, 5.25);
	TestEqual(TEXT("Duplicate register keeps one unit"), Fixture.Combat->GetRegisteredUnitCount(), 1);
	Fixture.Combat->AdvanceCombatTo(5.25);
	TestEqual(TEXT("Move blocks otherwise due hit"), Durable->GetCombatSnapshot().HP, 10000.0);
	Fixture.Combat->AdvanceCombatTo(5.4);
	TestEqual(TEXT("Move lock exact boundary allows hit"), Durable->GetCombatSnapshot().HP, 9985.0);
	Fixture.Combat->TryGetUnitAttackState(1, NextAt);
	TestEqual(TEXT("Subsequent cooldown follows actual scheduled attack"), NextAt, 6.4);
	Moved.CellId = 2;
	Unit->ApplyCommittedPlacement(Moved, FTransform(FVector(0, 20, 0)));
	Fixture.Combat->RegisterCommittedUnit(*Unit, 5.45);
	Fixture.Combat->TryGetUnitAttackState(1, NextAt);
	TestEqual(TEXT("Sale replenishment keeps next timer"), NextAt, 6.4);
	TestEqual(TEXT("Sale replenishment keeps original lock"), Unit->GetPlacement().MoveBlockedUntilServerSeconds, 5.4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCombatSharedAndStopTest, "LD.P0.G2.Combat.SharedTargetRemovalAndStop",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDCombatSharedAndStopTest::RunTest(const FString& Parameters)
{
	FCombatFixture Fixture;
	FString Error;
	if (!TestTrue(TEXT("Fixture initializes"), Fixture.Initialize(Error)))
	{
		return false;
	}
	ALDUnitActor* Lower = Fixture.Unit(TEXT("C01"), 1, 0, FVector(0, -100, 0));
	ALDUnitActor* Upper = Fixture.Unit(TEXT("C01"), 2, 1, FVector(0, 100, 0));
	ALDEnemyActor* Enemy = Fixture.Enemy(1000, FVector::ZeroVector, 1);
	if (!TestNotNull(TEXT("Lower"), Lower) || !TestNotNull(TEXT("Upper"), Upper) ||
	                                                       !TestNotNull(TEXT("Shared enemy"), Enemy))
	{
		return false;
	}
	Fixture.Combat->AdvanceCombatTo(.20);
	Fixture.Combat->AdvanceCombatTo(.25);
	TestEqual(TEXT("Both owners attack same central route1 enemy"), Enemy->GetCombatSnapshot().HP, 970.0);
	Fixture.Combat->AdvanceCombatTo(.30);
	Fixture.Combat->UnregisterUnit(1);
	Lower->DeactivateCommitted();
	Fixture.Combat->AdvanceCombatTo(1.25);
	TestEqual(TEXT("Sold unit reserved attack cancelled"), Enemy->GetCombatSnapshot().HP, 955.0);
	Fixture.Combat->Stop();
	Fixture.Combat->Stop();
	TestEqual(TEXT("Stop clears unit registrations"), Fixture.Combat->GetRegisteredUnitCount(), 0);
	TestFalse(TEXT("Late combat after Stop rejected"), Fixture.Combat->AdvanceCombatTo(10));
	TestEqual(TEXT("Stop leaves HP unchanged"), Enemy->GetCombatSnapshot().HP, 955.0);
	TestFalse(TEXT("Stop prevents enemy registration"), Fixture.Combat->RegisterEnemy(*Enemy));
	TestFalse(TEXT("Stop clears subscriptions"), Fixture.Combat->OnEnemyDeathCommitted.IsBound());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDUnitPresentationTest, "LD.P0.G2.Combat.UnitPresentationPreservesCanonical",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDUnitPresentationTest::RunTest(const FString& Parameters)
{
	FCombatFixture Fixture;
	FString Error;
	if (!TestTrue(TEXT("Fixture initializes"), Fixture.Initialize(Error)))
	{
		return false;
	}
	ALDUnitActor* Unit = Fixture.Unit(TEXT("C02"), 1, 1, FVector(0, 100, 0));
	if (!TestNotNull(TEXT("Unit"), Unit))
	{
		return false;
	}
	Fixture.World->TimeSeconds = 1;
	Unit->SetPresentationSlot(0, .15);
	Unit->SetLocalViewPlayerIndex(1);
	const FVector Start = Unit->GetPresentationLocation();
	Unit->SetPresentationSlot(1, .15);
	TestTrue(TEXT("Cell slot changes visible position"), !Unit->GetPresentationLocation().Equals(Start));
	TestTrue(TEXT("Slot does not alter canonical center"), Unit->GetActorLocation().Equals(FVector(0, 100, 0)));
	FLDPlacedUnit Moved = Unit->GetPlacement();
	Moved.CellId = 19;
	Unit->ApplyCommittedPlacement(Moved, FTransform(FVector(140, 100, 0)));
	TestTrue(TEXT("Move canonical is immediate"), Unit->GetActorLocation().Equals(FVector(140, 100, 0)));
	TestTrue(TEXT("Visual starts at previous position"), FMath::IsNearlyEqual(Unit->GetPresentationLocation().X, 28.0));
	Fixture.World->TimeSeconds = 1.075;
	Unit->Tick(.075f);
	TestTrue(TEXT("Half of .15 movement visually interpolated"),
	              FMath::IsNearlyEqual(Unit->GetPresentationLocation().X, 98.0, 1.e-3));
	Fixture.World->TimeSeconds = 1.15;
	Unit->Tick(.075f);
	TestTrue(TEXT("Visual reaches destination"), FMath::IsNearlyEqual(Unit->GetPresentationLocation().X, 168.0, 1.e-3));
	double NextAt = 0;
	Fixture.Combat->TryGetUnitAttackState(1, NextAt);
	TestEqual(TEXT("Display never changes attack timer"), NextAt, .25);
	Unit->EndPlay(EEndPlayReason::Destroyed);
	TestFalse(TEXT("Late local view after EndPlay refused"), Unit->SetLocalViewPlayerIndex(0));
	TestFalse(TEXT("Ended unit cannot attack"), Unit->IsCommitted());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCombatDuePositionTest, "LD.P0.G2.Combat.ExactDuePositionAndNoBackdating",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDCombatDuePositionTest::RunTest(const FString& Parameters)
{
	for (bool bEntering : {false, true})
	{
		FCombatFixture Fixture;
		FString Error;
		if (!TestTrue(TEXT("Fixture initializes"), Fixture.Initialize(Error)))
		{
			return false;
		}
		ALDUnitActor* Unit = Fixture.Unit(TEXT("C01"), 1, 0, FVector::ZeroVector, .025);
		ALDEnemyActor* Enemy = Fixture.Enemy(1000, FVector(bEntering ? 202.501 : 147.5, 0, 0), 0, 0, 100, bEntering);
		if (!TestNotNull(TEXT("Offset commit unit"), Unit) || !TestNotNull(TEXT("Moving boundary enemy"), Enemy))
		{
			return false;
		}
		if (bEntering)
		{
			// Reserve against a different target, then remove it before the due time. The entering target is
			// outside at .275 and first observable inside at .30, so its hit must not inherit the old due time.
			ALDEnemyActor* Old = Fixture.Enemy(1000, FVector(100, 0, 0));
			Fixture.Combat->AdvanceCombatTo(.20);
			Fixture.Combat->UnregisterEnemy(Old->GetRouteSnapshot().EnemyId);
		}
		else
		{
			Fixture.Combat->AdvanceCombatTo(.20);
		}
		Fixture.Combat->AdvanceCombatTo(.30);
		TestEqual(TEXT("One hit at valid selected time"), Enemy->GetCombatSnapshot().HP, 985.0);
		const double ExpectedTime = bEntering ? .30 : .275;
		TestTrue(TEXT("Precise due time or first observation, never backdated entry"),
		              FMath::IsNearlyEqual(Unit->GetLastAttackCue().ServerSeconds, ExpectedTime, 1.e-9));
		TestTrue(TEXT("Canonical root stays at current .30 sample"),
		              FMath::IsNearlyEqual(Enemy->GetActorLocation().X, bEntering ? 172.501 : 177.5, 1.e-6));
		FVector Exact;
		TestTrue(TEXT("Pure due query available"), Enemy->TryGetCanonicalPositionAt(.275, Exact));
		TestTrue(TEXT("Independent due range boundary"),
		              FMath::IsNearlyEqual(Exact.X, bEntering ? 175.001 : 175, 1.e-6));
	}
	FCombatFixture A;
	FCombatFixture B;
	FString Error;
	if (!TestTrue(TEXT("World A"), A.Initialize(Error)) || !TestTrue(TEXT("World B"), B.Initialize(Error)))
	{
		return false;
	}
	ALDUnitActor* Foreign = B.Unit(TEXT("C01"));
	if (!TestNotNull(TEXT("Foreign unit"), Foreign))
	{
		return false;
	}
	A.Combat->RegisterCommittedUnit(*Foreign, 0);
	TestEqual(TEXT("Cross-world committed unit refused"), A.Combat->GetRegisteredUnitCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCombatCommandClockTest,
                                 "LD.P0.G2.Combat.CommandClockOrdersEarlierHitAndSameTimeSale",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDCombatCommandClockTest::RunTest(const FString& Parameters)
{
	for (double CommandTime : {10.025, 10.04})
	{
		FCombatFixture Fixture;
		FString Error;
		if (!TestTrue(TEXT("Fixture initializes"), Fixture.Initialize(Error)))
		{
			return false;
		}
		ALDUnitActor* Unit = Fixture.Unit(TEXT("C01"), 1, 0, FVector::ZeroVector, 9.775);
		ALDEnemyActor* Enemy = Fixture.Enemy(15);
		if (!TestNotNull(TEXT("Unit"), Unit) || !TestNotNull(TEXT("Enemy"), Enemy))
		{
			return false;
		}
		int32 DeathCount = 0;
		double DeathTime = 0;
		Fixture.Combat->OnEnemyDeathCommitted.AddLambda(
		    [&](const FLDCombatDeath& Death)
		    {
			    ++DeathCount;
			    DeathTime = Death.DeathServerSeconds;
		    });
		Fixture.Combat->AdvanceCombatTo(10.0);
		TestTrue(TEXT("External command flushes strictly earlier hits"),
		              Fixture.Combat->AdvanceCombatBefore(CommandTime));
		TestEqual(TEXT("Equal timestamp command wins, later command follows hit"), DeathCount,
		               CommandTime == 10.025 ? 0 : 1);
		Fixture.Combat->UnregisterUnit(
		    1); // Explicit sale boundary fixture: command takes effect after the strict flush.
		Unit->DeactivateCommitted();
		TestTrue(TEXT("Inclusive same-time finalization follows command"),
		              Fixture.Combat->AdvanceCombatTo(CommandTime));
		Fixture.Combat->AdvanceCombatTo(10.05);
		TestEqual(TEXT("Sale prevents any remaining same-time attack"), DeathCount, CommandTime == 10.025 ? 0 : 1);
		if (CommandTime > 10.025)
		{
			TestTrue(TEXT("Earlier hit keeps 10.025 precise death time"),
			              FMath::IsNearlyEqual(DeathTime, 10.025, 1.e-9));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCombatRewardBeforePurchaseTest, "LD.P0.G2.Combat.EarlierKillFundsExternalPurchase",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDCombatRewardBeforePurchaseTest::RunTest(const FString& Parameters)
{
	FCombatFixture Fixture;
	FString Error;
	if (!TestTrue(TEXT("Fixture initializes"), Fixture.Initialize(Error)))
	{
		return false;
	}
	ULDBoardManager* Board = NewObject<ULDBoardManager>(Fixture.World);
	ULDEconomyService* Economy = NewObject<ULDEconomyService>(Fixture.World);
	ULDCommandProcessor* Processor = NewObject<ULDCommandProcessor>(Fixture.World);
	if (!TestTrue(TEXT("Real board initializes"), Board->Initialize(*Fixture.World, Fixture.Context, *Fixture.Data)) ||
	              !TestTrue(TEXT("Real economy initializes"), Economy->Initialize(Fixture.Context, *Fixture.Data, 1)) ||
	                        !TestTrue(TEXT("Processor initializes"),
	                                       Processor->Initialize(Fixture.Context, Fixture.Data->GetRules())) ||
	                                  !TestTrue(TEXT("Real services bind"), Processor->BindServices(*Board, *Economy)))
	{
		return false;
	}
	FLDParticipantContext Participant;
	Participant.MatchId = Fixture.Context.MatchId;
	Participant.PlayerIndex = 0;
	Participant.ConnectionEpoch = 1;
	Processor->RegisterParticipant(Participant);
	Participant.PlayerIndex = 1;
	Processor->RegisterParticipant(Participant);
	Participant.PlayerIndex = 0;
	Processor->SetAcceptingCommands(true);
	Processor->BeforeExternalCommand.BindLambda([&](double Time) { Fixture.Combat->AdvanceCombatBefore(Time); });
	Fixture.Combat->OnEnemyDeathCommitted.AddLambda([&](const FLDCombatDeath& Death)
	                                                { Processor->EnqueueCombatReward(Death); });
	for (uint32 Request = 1; Request <= 4; ++Request)
	{
		FLDCommand Summon;
		Summon.RequestId = Request;
		Summon.ConnectionEpoch = 1;
		Summon.ExpectedBoardRevision = Board->GetSnapshot(0).BoardRevision;
		TestTrue(TEXT("Setup four real paid purchases"),
		              Processor->SubmitAtTime(Participant, Summon, Request * .2).ResultCode ==
		                  ELDCommandResultCode::Success);
	}
	TestEqual(TEXT("100-20-22-24-26 leaves8"), Economy->GetSnapshot(0).Gold, 8);
	// Explicit reward-history fixture primes gold27. The twentieth death below comes from actual combat.
	for (uint64 Id = 1000; Id < 1019; ++Id)
	{
		FLDCombatDeath Fact;
		Fact.MatchId = Fixture.Context.MatchId;
		Fact.DeathEventId = Fact.EnemyId = Fact.SpawnSerial = Id;
		Fact.EnemyTypeId = TEXT("N01");
		Fact.SpawnWaveIndex = 1;
		Fact.DeathServerSeconds = 1;
		Processor->EnqueueCombatReward(Fact);
	}
	Processor->DrainCombatRewards();
	TestEqual(TEXT("Precondition one gold short"), Economy->GetSnapshot(0).Gold, 27);
	TestEqual(TEXT("Fifth paid price28"), Economy->GetSnapshot(0).NextSummonGold, 28);
	ALDUnitActor* Unit = Fixture.Unit(TEXT("C01"), 100, 0, FVector::ZeroVector, 9.775);
	ALDEnemyActor* Enemy = Fixture.Enemy(15);
	if (!TestNotNull(TEXT("Combat fixture unit"), Unit) || !TestNotNull(TEXT("Actual kill target"), Enemy))
	{
		return false;
	}
	Fixture.Combat->AdvanceCombatTo(10);
	FLDCommand Purchase;
	Purchase.RequestId = 5;
	Purchase.ConnectionEpoch = 1;
	Purchase.ExpectedBoardRevision = Board->GetSnapshot(0).BoardRevision;
	const FLDCommandResult Result = Processor->SubmitAtTime(Participant, Purchase, 10.04);
	TestTrue(TEXT("10.025 kill funds 10.04 purchase before validation"),
	              Result.ResultCode == ELDCommandResultCode::Success);
	TestEqual(TEXT("27+1-28 equals0"), Economy->GetSnapshot(0).Gold, 0);
	TestEqual(TEXT("Other participant receives twentieth kill once"), Economy->GetSnapshot(1).Gold, 120);
	TestEqual(TEXT("Fifth purchase commits once"), Economy->GetSnapshot(0).PaidSummonCount, 5);
	Processor->SubmitAtTime(Participant, Purchase, 10.05);
	TestEqual(TEXT("Duplicate response does not repeat kill or purchase"), Economy->GetSnapshot(0).Gold, 0);
	Processor->Close();
	Board->Close();
	Economy->Close();
	return true;
}

#endif
