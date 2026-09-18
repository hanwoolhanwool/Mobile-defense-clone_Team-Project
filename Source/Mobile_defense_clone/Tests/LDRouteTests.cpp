#include "Battle/LDRouteModel.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Battle/LDEnemyActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	TArray<FVector> ReferenceRoute(int32 RouteIndex)
	{
		const double Y = RouteIndex == 0 ? -560.0 : 560.0;
		return {FVector(490, Y, 0), FVector(490, 0, 0), FVector(-490, 0, 0), FVector(-490, Y, 0)};
	}

	struct FRouteWorldFixture
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);

		~FRouteWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
			}
		}
	};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0RoutePolylineTest, "LD.P0.G1.Route.PolylineCornersAndTwoLaps",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0RoutePolylineTest::RunTest(const FString& Parameters)
{
	struct FExpectedPoint
	{
		double Distance;
		FVector Lower;
		FVector Upper;
		uint64 Lap;
	};
	// Expectations are taken from the four specified corners, not computed by the implementation under test.
	const FExpectedPoint Cases[] = {{0, FVector(490, -560, 0), FVector(490, 560, 0), 0},
	                                {280, FVector(490, -280, 0), FVector(490, 280, 0), 0},
	                                {560, FVector(490, 0, 0), FVector(490, 0, 0), 0},
	                                {1050, FVector(0, 0, 0), FVector(0, 0, 0), 0},
	                                {1540, FVector(-490, 0, 0), FVector(-490, 0, 0), 0},
	                                {2100, FVector(-490, -560, 0), FVector(-490, 560, 0), 0},
	                                {3080, FVector(490, -560, 0), FVector(490, 560, 0), 1},
	                                {6160, FVector(490, -560, 0), FVector(490, 560, 0), 2},
	                                {6440, FVector(490, -280, 0), FVector(490, 280, 0), 2}};
	for (int32 RouteIndex = 0; RouteIndex < 2; ++RouteIndex)
	{
		FLDRouteModel Model;
		FString Error;
		TestTrue(TEXT("Specified route initializes"), Model.TryInitialize(ReferenceRoute(RouteIndex), Error));
		TestEqual(TEXT("Closed perimeter is 3080cm"), Model.GetLengthCm(), 3080.0);
		for (const FExpectedPoint& Case : Cases)
		{
			FVector Position;
			FVector Tangent;
			uint64 Lap = MAX_uint64;
			const FString Label = FString::Printf(TEXT("Route%d distance%.3f"), RouteIndex, Case.Distance);
			TestTrue(Label + TEXT(" sampled"), Model.TrySample(Case.Distance, Position, Tangent, Lap));
			TestTrue(Label + TEXT(" specified coordinate"),
			                      Position.Equals(RouteIndex == 0 ? Case.Lower : Case.Upper, 1.e-6));
			TestEqual(Label + TEXT(" completed laps"), Lap, Case.Lap);
		}
		const double YSign = RouteIndex == 0 ? 1.0 : -1.0;
		const FVector OutgoingTangents[] = {FVector(0, YSign, 0), FVector(-1, 0, 0), FVector(0, -YSign, 0),
		                                    FVector(1, 0, 0), FVector(0, YSign, 0)};
		const double Corners[] = {0, 560, 1540, 2100, 3080};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Corners); ++Index)
		{
			FVector Position;
			FVector Tangent;
			uint64 Lap = 0;
			TestTrue(TEXT("Corner sample"), Model.TrySample(Corners[Index], Position, Tangent, Lap));
			TestTrue(TEXT("Exact corner selects outgoing segment"), Tangent.Equals(OutgoingTangents[Index], 1.e-6));
			FVector After;
			TestTrue(TEXT("After corner sample"), Model.TrySample(Corners[Index] + 0.001, After, Tangent, Lap));
			TestTrue(TEXT("After corner continuous"), FVector::Distance(Position, After) <= 0.001001);
			if (Corners[Index] > 0)
			{
				FVector Before;
				TestTrue(TEXT("Before corner sample"), Model.TrySample(Corners[Index] - 0.001, Before, Tangent, Lap));
				TestTrue(TEXT("Before corner continuous"), FVector::Distance(Position, Before) <= 0.001001);
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0RouteInvalidTest, "LD.P0.G1.Route.InvalidInputPreservesOutput",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0RouteInvalidTest::RunTest(const FString& Parameters)
{
	FLDRouteModel Model;
	FString Error;
	FVector Position(7, 8, 9);
	FVector Tangent(4, 5, 6);
	uint64 Lap = 77;
	TestFalse(TEXT("Uninitialized model refuses query"), Model.TrySample(0, Position, Tangent, Lap));
	TestTrue(TEXT("Valid baseline"), Model.TryInitialize(ReferenceRoute(0), Error));
	TArray<FVector> Bad = ReferenceRoute(0);
	Bad[1] = Bad[0];
	TestFalse(TEXT("Zero segment rejected"), Model.TryInitialize(Bad, Error));
	TestEqual(TEXT("Failed rebuild preserves validated perimeter"), Model.GetLengthCm(), 3080.0);
	TestFalse(TEXT("Two-point path is not a loop"),
	               Model.TryInitialize({FVector::ZeroVector, FVector(1, 0, 0)}, Error));
	Bad = ReferenceRoute(0);
	Bad[0].X = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("NaN coordinate rejected"), Model.TryInitialize(Bad, Error));
	for (double InvalidDistance : {-1.0, std::numeric_limits<double>::infinity(),
	                               std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::max()})
	{
		TestFalse(TEXT("Invalid distance rejected"), Model.TrySample(InvalidDistance, Position, Tangent, Lap));
		TestTrue(TEXT("Failed query does not change position"), Position.Equals(FVector(7, 8, 9)));
		TestTrue(TEXT("Failed query does not change tangent"), Tangent.Equals(FVector(4, 5, 6)));
		TestEqual(TEXT("Failed query does not change lap"), Lap, static_cast<uint64>(77));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0RoutePredictionTest, "LD.P0.G1.Route.DelayedPresentationClock",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0RoutePredictionTest::RunTest(const FString& Parameters)
{
	double Predicted = -1;
	TestTrue(TEXT("100ms delayed sample is displayable"),
	              FLDRouteModel::TryPredictPresentationDistance(100, 10, 150, true, 10.1, Predicted));
	TestTrue(TEXT("100ms prediction advances 15cm"), FMath::IsNearlyEqual(Predicted, 115.0, 1.e-9));
	TestTrue(TEXT("Long outage is bounded"),
	              FLDRouteModel::TryPredictPresentationDistance(100, 10, 150, true, 30, Predicted));
	TestEqual(TEXT("At most 250ms / 37.5cm prediction"), Predicted, 137.5);
	TestTrue(TEXT("Clock before latest sample is accepted without rewind"),
	              FLDRouteModel::TryPredictPresentationDistance(100, 10, 150, true, 9, Predicted));
	TestEqual(TEXT("Old local clock uses latest authoritative sample"), Predicted, 100.0);
	TestTrue(TEXT("Stopped actor never predicts"),
	              FLDRouteModel::TryPredictPresentationDistance(100, 10, 150, false, 30, Predicted));
	TestEqual(TEXT("Stopped display distance frozen"), Predicted, 100.0);
	TestFalse(TEXT("Invalid clock rejected"),
	               FLDRouteModel::TryPredictPresentationDistance(100, 10, 150, true,
	                                                             std::numeric_limits<double>::quiet_NaN(), Predicted));
	TestEqual(TEXT("Invalid clock preserves previous output"), Predicted, 100.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0RouteActorTest, "LD.P0.G1.Route.ActorIdentityTwoLapsAndStop",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0RouteActorTest::RunTest(const FString& Parameters)
{
	FRouteWorldFixture Fixture;
	if (!TestNotNull(TEXT("Transient actor world"), Fixture.World))
	{
		return false;
	}
	const FGuid MatchId = FGuid::NewGuid();
	ALDEnemyActor* Actors[2] = {Fixture.World->SpawnActor<ALDEnemyActor>(), Fixture.World->SpawnActor<ALDEnemyActor>()};
	for (int32 RouteIndex = 0; RouteIndex < 2; ++RouteIndex)
	{
		ALDEnemyActor* Enemy = Actors[RouteIndex];
		if (!TestNotNull(TEXT("Enemy actor"), Enemy))
		{
			return false;
		}
		const uint64 EnemyId = 1001 + RouteIndex;
		TestFalse(TEXT("Unprepared actor is not published"), Enemy->GetIsReplicated());
		TestTrue(TEXT("Route initialization"),
		              Enemy->InitializeRoute(MatchId, EnemyId, RouteIndex, ReferenceRoute(RouteIndex), 150, 100));
		TestTrue(TEXT("Prepared actor is replicated"), Enemy->GetIsReplicated());
		TestFalse(TEXT("No competing movement replication"), Enemy->IsReplicatingMovement());
		TestFalse(TEXT("No mesh before local participant ready"), Enemy->IsPresentationVisible());
		for (const double Elapsed : {0.0, 560.0 / 150.0, 3080.0 / 150.0, 6160.0 / 150.0, 6440.0 / 150.0})
		{
			TestTrue(TEXT("Delayed server step handles full elapsed time"), Enemy->AdvanceRouteTo(100 + Elapsed));
			TestEqual(TEXT("EnemyId survives crossings"), Enemy->GetRouteSnapshot().EnemyId, EnemyId);
			TestEqual(TEXT("RouteIndex survives central segment/laps"), Enemy->GetRouteSnapshot().RouteIndex,
			               RouteIndex);
			TestTrue(TEXT("MatchId survives crossings"), Enemy->GetRouteSnapshot().MatchId == MatchId);
		}
		TestTrue(TEXT("Cumulative distance is not wrapped"),
		              FMath::IsNearlyEqual(Enemy->GetRouteSnapshot().TotalDistanceCm, 6440.0, 1.e-6));
		const FVector ExpectedPosition(490, RouteIndex == 0 ? -280 : 280, 0);
		TestTrue(TEXT("Same actor returns around original board"),
		              Enemy->GetActorLocation().Equals(ExpectedPosition, 1.e-6));
		const double BeforeDuplicate = Enemy->GetRouteSnapshot().TotalDistanceCm;
		const double LastTime = Enemy->GetRouteSnapshot().SampleServerSeconds;
		TestTrue(TEXT("Duplicate time is no-op"), Enemy->AdvanceRouteTo(LastTime));
		TestFalse(TEXT("Clock rewind rejected"), Enemy->AdvanceRouteTo(LastTime - 1));
		TestFalse(TEXT("NaN time rejected"), Enemy->AdvanceRouteTo(std::numeric_limits<double>::quiet_NaN()));
		TestTrue(TEXT("Identical initialize preserves existing instance"),
		              Enemy->InitializeRoute(MatchId, EnemyId, RouteIndex, ReferenceRoute(RouteIndex), 150, 100));
		TestFalse(
		    TEXT("Different ID cannot replace existing instance"),
		         Enemy->InitializeRoute(MatchId, EnemyId + 100, RouteIndex, ReferenceRoute(RouteIndex), 150, 100));
		TestFalse(TEXT("Route cannot be swapped"), Enemy->InitializeRoute(MatchId, EnemyId, 1 - RouteIndex,
		                                                                  ReferenceRoute(1 - RouteIndex), 150, 100));
		TestEqual(TEXT("All failed/duplicate mutations preserve distance"), Enemy->GetRouteSnapshot().TotalDistanceCm,
		               BeforeDuplicate);
		Enemy->StopRoute();
		Enemy->StopRoute();
		TestFalse(TEXT("Stopped snapshot is inactive"), Enemy->GetRouteSnapshot().bActive);
		TestFalse(TEXT("Late step after stop refused"), Enemy->AdvanceRouteTo(LastTime + 50));
		TestFalse(TEXT("Initialize cannot restart stopped actor"),
		               Enemy->InitializeRoute(MatchId, EnemyId, RouteIndex, ReferenceRoute(RouteIndex), 150, 100));
		TestEqual(TEXT("Stop does not replace identity"), Enemy->GetRouteSnapshot().EnemyId, EnemyId);
		TestEqual(TEXT("Stop preserves final distance"), Enemy->GetRouteSnapshot().TotalDistanceCm, BeforeDuplicate);
	}
	int32 ActorCount = 0;
	for (TActorIterator<ALDEnemyActor> It(Fixture.World); It; ++It)
	{
		++ActorCount;
	}
	TestEqual(TEXT("Loops never create additional actors"), ActorCount, 2);
	ALDEnemyActor* Cancelled = Fixture.World->SpawnActor<ALDEnemyActor>();
	if (TestNotNull(TEXT("Cancelled preparation actor"), Cancelled))
	{
		Cancelled->StopRoute();
		TestFalse(TEXT("Late initialization after cancellation refused"),
		               Cancelled->InitializeRoute(MatchId, 1003, 0, ReferenceRoute(0), 150, 100));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0RouteViewTest, "LD.P0.G1.Route.LocalViewDoesNotMutateCanonicalState",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0RouteViewTest::RunTest(const FString& Parameters)
{
	FRouteWorldFixture Fixture;
	if (!TestNotNull(TEXT("Transient view world"), Fixture.World))
	{
		return false;
	}
	ALDEnemyActor* Enemy = Fixture.World->SpawnActor<ALDEnemyActor>();
	if (!TestNotNull(TEXT("View actor"), Enemy))
	{
		return false;
	}
	const FGuid MatchId = FGuid::NewGuid();
	TestTrue(TEXT("Upper route starts in canonical upper board"),
	              Enemy->InitializeRoute(MatchId, 2001, 1, ReferenceRoute(1), 150, 0));
	TestTrue(TEXT("Player zero view installed"), Enemy->SetLocalViewPlayerIndex(0));
	TestTrue(TEXT("Player zero mesh displays upper board"),
	              Enemy->GetPresentationLocation().Equals(FVector(490, 560, 35), 1.e-6));
	TestTrue(TEXT("Player one view installed"), Enemy->SetLocalViewPlayerIndex(1));
	TestTrue(TEXT("Player one mesh displays own lower board"),
	              Enemy->GetPresentationLocation().Equals(FVector(490, -560, 35), 1.e-6));
	TestTrue(TEXT("Canonical root unchanged by view"), Enemy->GetActorLocation().Equals(FVector(490, 560, 0), 1.e-6));
	TestEqual(TEXT("View never edits route identity"), Enemy->GetRouteSnapshot().RouteIndex, 1);
	TestEqual(TEXT("View never advances authority distance"), Enemy->GetRouteSnapshot().TotalDistanceCm, 0.0);
	TestTrue(TEXT("Delayed presentation moves mesh"), Enemy->RefreshPresentation(0.1));
	TestTrue(TEXT("Reflected display predicts toward center"),
	              Enemy->GetPresentationLocation().Equals(FVector(490, -545, 35), 1.e-6));
	TestTrue(TEXT("Presentation never moves authority root"),
	              Enemy->GetActorLocation().Equals(FVector(490, 560, 0), 1.e-6));
	TestFalse(TEXT("Invalid local participant refused"), Enemy->SetLocalViewPlayerIndex(2));
	Enemy->SetRole(ROLE_SimulatedProxy);
	TestFalse(TEXT("Client cannot advance route"), Enemy->AdvanceRouteTo(5));
	TestFalse(TEXT("Client cannot reinitialize identity"),
	               Enemy->InitializeRoute(MatchId, 2002, 0, ReferenceRoute(0), 150, 0));
	TestEqual(TEXT("Rejected client authority mutation keeps EnemyId"), Enemy->GetRouteSnapshot().EnemyId,
	               static_cast<uint64>(2001));
	Enemy->SetRole(ROLE_Authority);
	Enemy->StopRoute();
	TestTrue(TEXT("Stopped mesh can be displayed"), Enemy->RefreshPresentation(100));
	TestTrue(TEXT("Stopped view returns to final authoritative position"),
	              Enemy->GetPresentationLocation().Equals(FVector(490, -560, 35), 1.e-6));
	Enemy->EndPlay(EEndPlayReason::Destroyed);
	TestFalse(TEXT("EndPlay hides mesh"), Enemy->IsPresentationVisible());
	TestFalse(TEXT("Late view callback after EndPlay refused"), Enemy->SetLocalViewPlayerIndex(0));
	TestFalse(TEXT("Late presentation after EndPlay refused"), Enemy->RefreshPresentation(100));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDP0HostPresentationClockTest, "LD.P0.G1.Route.HostStepKeepsCurrentViewClock",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLDP0HostPresentationClockTest::RunTest(const FString& Parameters)
{
	FRouteWorldFixture Fixture;
	if (!TestNotNull(TEXT("Transient host world"), Fixture.World))
	{
		return false;
	}
	AGameStateBase* State = Fixture.World->SpawnActor<AGameStateBase>();
	ALDEnemyActor* Enemy = Fixture.World->SpawnActor<ALDEnemyActor>();
	if (!TestNotNull(TEXT("Host GameState clock"), State) || !TestNotNull(TEXT("Host route actor"), Enemy))
	{
		return false;
	}
	Fixture.World->SetGameState(State);
	// Set the transient fixture clock directly: no editor, game loop, socket or network simulation is involved.
	Fixture.World->TimeSeconds = 100.0;
	TestTrue(TEXT("Initialize at clock 100"),
	              Enemy->InitializeRoute(FGuid::NewGuid(), 3001, 0, ReferenceRoute(0), 150, 100));
	TestTrue(TEXT("Host view ready"), Enemy->SetLocalViewPlayerIndex(0));
	Fixture.World->TimeSeconds = 100.125;
	Enemy->Tick(0.125f);
	const FVector BeforeServerStep = Enemy->GetPresentationLocation();
	TestTrue(TEXT("Current display clock predicts 18.75cm"), BeforeServerStep.Equals(FVector(490, -541.25, 35), 1.e-6));
	// The 20Hz authoritative step can execute after the display tick in the same frame.
	TestTrue(TEXT("Due server step at 100.10"), Enemy->AdvanceRouteTo(100.10));
	TestTrue(TEXT("Canonical authority is still the 15cm fixed step"),
	              Enemy->GetActorLocation().Equals(FVector(490, -545, 0), 1.e-6));
	TestTrue(TEXT("Server step does not rewind current host display"),
	              Enemy->GetPresentationLocation().Equals(BeforeServerStep, 1.e-6));
	TestTrue(TEXT("Snapshot keeps fixed-step time"),
	              FMath::IsNearlyEqual(Enemy->GetRouteSnapshot().SampleServerSeconds, 100.10, 1.e-9));
	TestTrue(TEXT("Snapshot keeps fixed-step distance"),
	              FMath::IsNearlyEqual(Enemy->GetRouteSnapshot().TotalDistanceCm, 15.0, 1.e-9));
	return true;
}

#endif
