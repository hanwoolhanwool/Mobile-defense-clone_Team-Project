#include "Battle/LDWaveDirector.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Battle/LDCombatService.h"
#include "Battle/LDEnemyActor.h"
#include "Battle/LDUnitActor.h"
#include "Board/LDBoardManager.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "CoreGlobals.h"
#include "Data/LDGameData.h"
#include "Economy/LDEconomyService.h"
#include "Engine/Engine.h"
#include "Engine/Player.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Network/LDCommandProcessor.h"

// Explicit test fixture control. Product admissions never expose time/HP/wave setters.
struct FLDWaveTestAccess
{
	static void Advance(ALDGameMode& Mode, double BeforeSeconds)
	{
		Mode.AdvanceTimelineBefore(BeforeSeconds);
		Mode.GetCommandProcessor()->DrainCombatRewards();
		Mode.FinalizePendingTerminal();
	}
	static TArray<ALDEnemyActor*> Enemies(ULDWaveDirector& Director)
	{
		TArray<ALDEnemyActor*> Result;
		for (const auto& Item : Director.LivingEnemies)
		{
			if (Item.Value.IsValid())
			{
				Result.Add(Item.Value.Get());
			}
		}
		return Result;
	}
	static bool Kill(ALDGameMode& Mode, ALDEnemyActor& Enemy, double At)
	{
		FLDDamageEvent Hit;
		Hit.MatchId = Enemy.GetRouteSnapshot().MatchId;
		Hit.EnemyId = Enemy.GetRouteSnapshot().EnemyId;
		Hit.DamageEventId = 900000 + Hit.EnemyId;
		Hit.SourceInstanceId = 800000;
		Hit.Amount = 1000000;
		Hit.AttackServerSeconds = At;
		FLDCombatDeath Death;
		if (Enemy.TryApplyDamage(Hit, Death) != ELDDamageResult::Killed)
		{
			return false;
		}
		Mode.HandleEnemyDeath(Death);
		return true;
	}
	static void KillAll(ALDGameMode& Mode, double At, bool bNormalOnly = false)
	{
		for (ALDEnemyActor* Enemy : Enemies(*Mode.GetWaveDirector()))
		{
			if (!bNormalOnly || Enemy->GetEnemyRow().Kind == TEXT("Normal"))
			{
				Kill(Mode, *Enemy, At);
			}
		}
		Mode.GetCommandProcessor()->DrainCombatRewards();
	}
	static void JumpToFinal(ALDGameMode& Mode, double At)
	{
		ULDWaveDirector* Director = Mode.GetWaveDirector();
		Mode.GetGameState<ALDGameState>()->SetPhase(ELDMatchPhase::Running);
		Mode.LogicOriginSeconds = At;
		Mode.LogicStep = 0;
		Director->bStarted = true;
		Director->BeginWave(10, At);
	}
	static bool ExtraNormal(ALDGameMode& Mode, double At)
	{
		return Mode.GetWaveDirector()->SpawnEnemy(0, Mode.GetGameData()->GetWaves()[0], At);
	}
	static bool ReplaceSpawnFactory(ALDGameMode& Mode, FLDEnemyActorFactory Factory)
	{
		Mode.WaveDirector->Stop();
		Mode.WaveDirector = NewObject<ULDWaveDirector>(&Mode);
		Mode.WaveDirector->OnTerminalRequested.AddUObject(&Mode, &ALDGameMode::RequestTerminal);
		return Mode.WaveDirector->Initialize(*Mode.GameData, *Mode.GetGameState<ALDGameState>(), *Mode.CombatService,
		                                     MoveTemp(Factory));
	}
};

namespace
{
	struct FScopedProbeCommandLine
	{
		FString Previous;
		explicit FScopedProbeCommandLine(const TCHAR* Probe) : Previous(FCommandLine::Get())
		{
			FCommandLine::Set(*FString::Printf(TEXT("-P0Probe=%s %s"), Probe, *Previous));
		}
		~FScopedProbeCommandLine()
		{
			FCommandLine::Set(*Previous);
		}
	};

	struct FWaveFixture
	{
		UWorld* World = nullptr;
		ALDGameMode* Mode = nullptr;
		ALDPlayerController* Players[2] = {nullptr, nullptr};
		FWaveFixture()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World)
			{
				return;
			}
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			Mode = World->SpawnActor<ALDGameMode>();
			FString Error;
			Mode->InitGame(TEXT("LDWaveFixture"), TEXT("?P0Seed=1776"), Error);
			Mode->PreInitializeComponents();
		}
		ALDPlayerController* Login(int32 Index)
		{
			ALDPlayerController* PC = World->SpawnActor<ALDPlayerController>();
			PC->SetPlayerState(World->SpawnActor<ALDPlayerState>());
			PC->SetPlayer(NewObject<UPlayer>(PC));
			Players[Index] = PC;
			Mode->PostLogin(PC);
			return PC;
		}
		void Ready()
		{
			Login(0);
			Login(1);
			Mode->DispatchBeginPlay();
		}
		void TickLogicTimer(double ServerSeconds)
		{
			World->TimeSeconds = ServerSeconds;
			// Drive the real timer delegate through public engine APIs. Activate pending timers first;
			// the fixture scopes/restores frame identity because TimerManager permits one Tick per frame.
			TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter + 1);
			World->GetTimerManager().Tick(0);
			++GFrameCounter;
			World->GetTimerManager().Tick(.1f);
		}
		ALDGameState* State() const
		{
			return Mode->GetGameState<ALDGameState>();
		}
		FLDCommand Command(int32 Player, uint32 RequestId, ELDCommandType Type = ELDCommandType::Summon) const
		{
			FLDCommand Command;
			Command.ConnectionEpoch =
			    Players[Player]->GetPlayerState<ALDPlayerState>()->GetParticipantContext().ConnectionEpoch;
			Command.RequestId = RequestId;
			Command.CommandType = Type;
			Command.ExpectedBoardRevision = Mode->GetBoardManager()->GetSnapshot(Player).BoardRevision;
			return Command;
		}
		~FWaveFixture()
		{
			if (World)
			{
				Mode->EndPlay(EEndPlayReason::EndPlayInEditor);
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}
	};
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDCombatFixtureReadinessTest, "LD.P0.G3.Waves.CombatFixturesWaitForBothParticipants",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDCombatFixtureReadinessTest::RunTest(const FString& Parameters)
{
	const FString OriginalCommandLine = FCommandLine::Get();
	for (const TCHAR* Probe : {TEXT("G2"), TEXT("G3Load")})
	{
		FScopedProbeCommandLine ProbeCommandLine(Probe);
		FWaveFixture F;
		TestNull(TEXT("Actual combat-only probe option does not construct a wave director"), F.Mode->GetWaveDirector());
		F.Mode->DispatchBeginPlay();
		F.TickLogicTimer(.1);
		if (!TestEqual(FString::Printf(TEXT("%s zero-participant timer remains Preparing"), Probe),
		                               F.State()->GetPhase(), ELDMatchPhase::Preparing))
		{
			continue;
		}
		TestFalse(TEXT("Probe cannot start fixture authoring before participants join"), F.Mode->CanAcceptCommands());
		F.Login(0);
		F.TickLogicTimer(35);
		TestEqual(TEXT("One participant still waits beyond normal Loading timeout"), F.State()->GetPhase(),
		               ELDMatchPhase::Preparing);
		TestEqual(TEXT("Fixture wait never produces initialization failure"), F.State()->GetBattleSnapshot().Result,
		               ELDMatchResult::None);
		TestFalse(TEXT("One participant cannot start fixture authoring"), F.Mode->CanAcceptCommands());
		TestTrue(TEXT("Waiting keeps its readiness logic timer alive"), F.Mode->IsLogicTimerActive());
		TestEqual(TEXT("Waiting rejects an actual owned summon request"),
		               F.Players[0]->SubmitServerCommand(F.Command(0, 1)).ResultCode,
		               ELDCommandResultCode::PhaseNotAllowed);
		TestEqual(TEXT("Rejected waiting command spends no gold"), F.Mode->GetEconomyService()->GetSnapshot(0).Gold,
		               100);
		F.Login(1);
		TestEqual(TEXT("Second participant opens combat-only Running immediately"), F.State()->GetPhase(),
		               ELDMatchPhase::Running);
		TestTrue(TEXT("Both participants allow fixture authoring"), F.Mode->CanAcceptCommands());
		F.TickLogicTimer(35.1);
		TestEqual(TEXT("Combat-only timer never creates normal waves"), F.State()->GetBattleSnapshot().WaveIndex, 0);
		TestEqual(TEXT("Combat-only timer has no unauthored enemies"),
		               F.Mode->GetCombatService()->GetRegisteredEnemyCount(), 0);
		TestEqual(TEXT("Ready fixture accepts actual owned summon request"),
		               F.Players[0]->SubmitServerCommand(F.Command(0, 2)).ResultCode, ELDCommandResultCode::Success);
		TestEqual(TEXT("Ready paid summon spends exactly20"), F.Mode->GetEconomyService()->GetSnapshot(0).Gold, 80);
	}
	TestEqual(TEXT("Probe test restores the original process options"), FString(FCommandLine::Get()),
	               OriginalCommandLine);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDWaveReadinessTest, "LD.P0.G3.Waves.LoadingPreparationAndExactReadiness",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDWaveReadinessTest::RunTest(const FString& Parameters)
{
	{
		FWaveFixture F;
		TestEqual(TEXT("Real match remains Loading before participants"), F.State()->GetPhase(),
		               ELDMatchPhase::Loading);
		TestEqual(TEXT("Loading deadline begins at match entry"),
		               F.State()->GetBattleSnapshot().LoadingDeadlineServerSeconds, 30.0);
		F.Login(0);
		F.Mode->DispatchBeginPlay();
		FLDWaveTestAccess::Advance(*F.Mode, 30.0);
		TestEqual(TEXT("Exact 30 remains open for readiness"), F.State()->GetPhase(), ELDMatchPhase::Loading);
		F.World->TimeSeconds = 30;
		F.Login(1);
		TestEqual(TEXT("Exact deadline readiness wins before timeout"), F.State()->GetPhase(),
		               ELDMatchPhase::Preparing);
		TestEqual(TEXT("Preparation has its own ten seconds"),
		               F.State()->GetBattleSnapshot().PreparationEndServerSeconds, 40.0);
		const FLDCommandResult Bought = F.Players[0]->SubmitServerCommand(F.Command(0, 1));
		TestEqual(TEXT("Preparing accepts actual owned command"), Bought.ResultCode, ELDCommandResultCode::Success);
		TestEqual(TEXT("First summon spends exactly20"), F.Mode->GetEconomyService()->GetSnapshot(0).Gold, 80);
		ALDUnitActor* Unit = nullptr;
		F.Mode->GetBoardManager()->TryGetCommittedUnitActor(Bought.CreatedInstanceIds[0], Unit);
		FLDWaveTestAccess::Advance(*F.Mode, 40.0);
		TestEqual(TEXT("No early enemies or Running at open boundary"), F.State()->GetBattleSnapshot().ActiveEnemyCount,
		               0);
		FLDWaveTestAccess::Advance(*F.Mode, 40.0001);
		TestEqual(TEXT("Wave1 starts at preparation deadline"), F.State()->GetPhase(), ELDMatchPhase::Running);
		TestEqual(TEXT("Exactly two first normal actors"), F.State()->GetBattleSnapshot().ActiveEnemyCount, 2);
		ALDUnitActor* After = nullptr;
		F.Mode->GetBoardManager()->TryGetCommittedUnitActor(Bought.CreatedInstanceIds[0], After);
		TestTrue(TEXT("Running preserves prepared participant unit"), After == Unit);
	}
	{
		FWaveFixture F;
		F.Login(0);
		F.Mode->DispatchBeginPlay();
		FLDWaveTestAccess::Advance(*F.Mode, 30.001);
		TestEqual(TEXT("Strictly after30 without readiness aborts"), F.State()->GetBattleSnapshot().ResultReason,
		               ELDResultReason::LoadingTimeout);
		F.Login(1);
		TestEqual(TEXT("Late readiness cannot revive terminal"), F.State()->GetPhase(), ELDMatchPhase::Aborted);
		TestFalse(TEXT("Late participant was not assigned"),
		               F.Players[1]->GetPlayerState<ALDPlayerState>()->GetParticipantContext().IsValid());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDWaveScheduleTest, "LD.P0.G3.Waves.ExactSpawnScheduleAndTenWaves",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDWaveScheduleTest::RunTest(const FString& Parameters)
{
	FWaveFixture F;
	F.Ready();
	FLDWaveTestAccess::Advance(*F.Mode, 12.6001);
	TArray<ALDEnemyActor*> Initial = FLDWaveTestAccess::Enemies(*F.Mode->GetWaveDirector());
	TestEqual(TEXT("Hitch to2.6 creates only0/1/2 from both gates"), Initial.Num(), 6);
	TArray<double> Times;
	for (ALDEnemyActor* Enemy : Initial)
	{
		Times.Add(Enemy->GetCombatSnapshot().SpawnedServerSeconds);
	}
	Times.Sort();
	const TArray<double> Expected = {10, 10, 11, 11, 12, 12};
	TestTrue(TEXT("Actors retain scheduled timestamps"), Times == Expected);
	FLDWaveTestAccess::Advance(*F.Mode, 29.999);
	TestEqual(TEXT("Wave1 contains40 at19.999"), F.Mode->GetWaveDirector()->GetTrackedEnemyCount(), 40);
	FLDWaveTestAccess::Advance(*F.Mode, 30.0001);
	TestEqual(TEXT("Wave2 starts with previous40 plus2"), F.Mode->GetWaveDirector()->GetTrackedEnemyCount(), 42);
	TestEqual(TEXT("Wave index advances exactly once"), F.State()->GetBattleSnapshot().WaveIndex, 2);
	for (ALDEnemyActor* Enemy : Initial)
	{
		TestTrue(TEXT("Previous actor identity survives next wave"), Enemy->IsCombatAlive());
	}
	FLDWaveTestAccess::KillAll(*F.Mode, 30.01);
	for (int32 Second = 31; Second <= 189; ++Second)
	{
		FLDWaveTestAccess::Advance(*F.Mode, Second + .0001);
		FLDWaveTestAccess::KillAll(*F.Mode, Second + .01);
	}
	FLDWaveTestAccess::Advance(*F.Mode, 190.0001);
	const FLDBattleSnapshot Final = F.State()->GetBattleSnapshot();
	TestEqual(TEXT("Only ten waves"), Final.WaveIndex, 10);
	TestEqual(TEXT("Exactly360 normals spawned before bosses"), F.Mode->GetWaveDirector()->NextEnemyId, uint64(363));
	TestEqual(TEXT("Bosses excluded from normal count"), Final.ActiveEnemyCount, 0);
	TestEqual(TEXT("Both bosses exist"), Final.Bosses.Num(), 2);
	TestTrue(TEXT("Final generation waits for both successful spawns"), Final.bFinalSpawnsComplete);
	TestEqual(TEXT("Common absolute deadline250"), Final.BossDeadlineServerSeconds, 250.0);
	TestEqual(TEXT("Each B01 HP6000"), Final.Bosses[0].HP, 6000.0);
	for (ALDEnemyActor* Enemy : FLDWaveTestAccess::Enemies(*F.Mode->GetWaveDirector()))
	{
		TestEqual(TEXT("Boss armor20"), Enemy->GetEnemyRow().Armor, 20.0);
		TestEqual(TEXT("Boss resistance .10"), Enemy->GetEnemyRow().MagicResistance, .1);
	}
	TestEqual(TEXT("360 actual normal deaths paid both players360, no wave transition pay"),
	               F.Mode->GetEconomyService()->GetSnapshot(0).Gold, 460);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDWaveCapAndDeathTest, "LD.P0.G3.Waves.ImmediateCapAndDeathDeduplication",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDWaveCapAndDeathTest::RunTest(const FString& Parameters)
{
	for (int32 KillCount : {0, 1, 2})
	{
		FWaveFixture F;
		F.Ready();
		FLDWaveTestAccess::Advance(*F.Mode, 10.0001);
		for (int32 Index = 2; Index < 99; ++Index)
		{
			FLDWaveTestAccess::ExtraNormal(*F.Mode, 10.01);
		}
		TestEqual(TEXT("N99 itself remains running"), F.State()->GetPhase(), ELDMatchPhase::Running);
		bool bObservedCapBeforeResult = false;
		bool bCapObserverDenied = false;
		F.State()->OnMatchStateChanged.AddLambda(
		    [&]()
		    {
			    if (F.State()->GetBattleSnapshot().ActiveEnemyCount == 100 &&
			        F.State()->GetPhase() == ELDMatchPhase::Running && !bObservedCapBeforeResult)
			    {
				    bObservedCapBeforeResult = true;
				    bCapObserverDenied =
				        !F.Mode->CanAcceptCommands() && F.Players[0]->SubmitServerCommand(F.Command(0, 1)).ResultCode ==
				                                            ELDCommandResultCode::PhaseNotAllowed;
			    }
		    });
		const TArray<ALDEnemyActor*> Enemies = FLDWaveTestAccess::Enemies(*F.Mode->GetWaveDirector());
		for (int32 Index = 0; Index < KillCount; ++Index)
		{
			TestTrue(TEXT("Actual HP transition before equal-time spawn"),
			              FLDWaveTestAccess::Kill(*F.Mode, *Enemies[Index], 11));
		}
		F.Mode->GetCommandProcessor()->DrainCombatRewards();
		FLDWaveTestAccess::Advance(*F.Mode, 11.0001);
		if (KillCount == 2)
		{
			TestEqual(TEXT("99-2+2=99 has no defeat"), F.State()->GetBattleSnapshot().ActiveEnemyCount, 99);
			TestEqual(TEXT("Both earlier kills grant once"), F.Mode->GetEconomyService()->GetSnapshot(1).Gold, 102);
		}
		else
		{
			TestEqual(TEXT("First increase to100 latches defeat"), F.State()->GetBattleSnapshot().Result,
			               ELDMatchResult::Defeat);
			TestEqual(TEXT("Immediate cap records exact spawn time11"),
			               F.State()->GetBattleSnapshot().ResultServerSeconds, 11.0);
			TestEqual(TEXT("No increase after latch"), F.State()->GetBattleSnapshot().ActiveEnemyCount, 100);
			TestTrue(TEXT("N100 is published before final Result"), bObservedCapBeforeResult);
			TestTrue(TEXT("N100 observer cannot admit reentrant purchase"), bCapObserverDenied);
			const int32 Revision = F.State()->GetBattleSnapshot().Revision;
			FLDWaveTestAccess::Advance(*F.Mode, 12);
			TestEqual(TEXT("Later timeline cannot rescue latch"), F.State()->GetBattleSnapshot().Revision, Revision);
		}
		F.State()->OnMatchStateChanged.Clear();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDWaveTerminalTruthTest, "LD.P0.G3.Waves.VictoryRequiresAllThreeConditions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDWaveTerminalTruthTest::RunTest(const FString& Parameters)
{
	for (int32 Mask = 0; Mask < 8; ++Mask)
	{
		FWaveFixture F;
		F.Ready();
		FLDWaveTestAccess::JumpToFinal(*F.Mode, 10);
		FLDBattleSnapshot Snapshot = F.State()->GetBattleSnapshot();
		Snapshot.bFinalSpawnsComplete = (Mask & 1) != 0;
		Snapshot.ActiveEnemyCount = (Mask & 4) != 0 ? 0 : 1;
		for (FLDBossSnapshot& Boss : Snapshot.Bosses)
		{
			Boss.bAlive = (Mask & 2) == 0;
			Boss.HP = Boss.bAlive ? 6000 : 0;
		}
		F.State()->UpdateBattle(Snapshot);
		F.Mode->GetWaveDirector()->EvaluateVictory(20);
		FLDWaveTestAccess::Advance(*F.Mode, 20.0001);
		TestEqual(FString::Printf(TEXT("Independent F/B/Z truth table mask%d"), Mask),
		                          F.State()->GetBattleSnapshot().Result,
		                          Mask == 7 ? ELDMatchResult::Victory : ELDMatchResult::None);
	}
	{
		FWaveFixture F;
		F.Ready();
		FLDWaveTestAccess::JumpToFinal(*F.Mode, 10);
		FLDWaveTestAccess::ExtraNormal(*F.Mode, 10);
		for (ALDEnemyActor* Enemy : FLDWaveTestAccess::Enemies(*F.Mode->GetWaveDirector()))
		{
			if (Enemy->GetEnemyRow().Kind != TEXT("Normal"))
			{
				FLDWaveTestAccess::Kill(*F.Mode, *Enemy, 40);
			}
		}
		FLDWaveTestAccess::Advance(*F.Mode, 70.1);
		TestEqual(TEXT("Both dead bosses at deadline do not time out with normal remaining"), F.State()->GetPhase(),
		               ELDMatchPhase::Running);
		FLDWaveTestAccess::KillAll(*F.Mode, 71);
		FLDWaveTestAccess::Advance(*F.Mode, 71.001);
		TestEqual(TEXT("Last normal enables victory without two-second delay"),
		               F.State()->GetBattleSnapshot().ResultServerSeconds, 71.0);
		TestEqual(TEXT("Final boss reward drained before result"), F.Mode->GetEconomyService()->GetSnapshot(0).Gold,
		               301);
		TestEqual(TEXT("Two exactly30-second bosses give six stars"), F.Mode->GetEconomyService()->GetSnapshot(0).Stars,
		               6);
		TestEqual(TEXT("No wave11"), F.State()->GetBattleSnapshot().WaveIndex, 10);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDWaveModeBoundaryTest, "LD.P0.G3.Waves.ProductionClockBossBoundaryAndSale",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDWaveModeBoundaryTest::RunTest(const FString& Parameters)
{
	// Independent absolute expectations: spawn10, deadline70, both <=70 valid, >70 never retrospectively rescued.
	for (int32 Case = 0; Case < 4; ++Case)
	{
		FWaveFixture F;
		F.Ready();
		TArray<uint64> UnitIds;
		for (int32 Player = 0; Player < 2; ++Player)
		{
			const FLDCommandResult Bought = F.Players[Player]->SubmitServerCommand(F.Command(Player, 1));
			if (!TestEqual(TEXT("Fixture uses real paid command"), Bought.ResultCode, ELDCommandResultCode::Success) ||
			               !TestEqual(TEXT("One actual unit"), Bought.CreatedInstanceIds.Num(), 1))
			{
				return false;
			}
			UnitIds.Add(Bought.CreatedInstanceIds[0]);
			FLDCommand Move = F.Command(Player, 2, ELDCommandType::Move);
			Move.InstanceId = UnitIds[Player];
			Move.DestinationCellId = Player == 0 ? 5 : 35;
			TestEqual(TEXT("Place beside return path using command"),
			               F.Players[Player]->SubmitServerCommand(Move).ResultCode, ELDCommandResultCode::Success);
			ALDUnitActor* Unit = nullptr;
			F.Mode->GetBoardManager()->TryGetCommittedUnitActor(UnitIds[Player], Unit);
			F.Mode->GetCombatService()->UnregisterUnit(UnitIds[Player]);
			const double Due = Case == 0 ? 69.999 : (Case == 2 && Player == 1 ? 70.001 : 70.0);
			// Explicit timing fixture changes registration epoch only; production scheduler/target/damage remain real.
			F.Mode->GetCombatService()->RegisterCommittedUnit(*Unit, Due - .25);
		}
		FLDWaveTestAccess::JumpToFinal(*F.Mode, 10);
		FLDWaveTestAccess::Advance(*F.Mode, 69.9);
		for (ALDEnemyActor* Enemy : FLDWaveTestAccess::Enemies(*F.Mode->GetWaveDirector()))
		{
			FLDDamageEvent Prep;
			Prep.MatchId = F.State()->GetMatchContext().MatchId;
			Prep.EnemyId = Enemy->GetRouteSnapshot().EnemyId;
			Prep.DamageEventId = 500000 + Prep.EnemyId;
			Prep.SourceInstanceId = 800000;
			Prep.Amount = 5999;
			Prep.AttackServerSeconds = 69.9;
			FLDCombatDeath NoDeath;
			TestEqual(TEXT("Explicit fixture lowers each actual B01 to one HP"), Enemy->TryApplyDamage(Prep, NoDeath),
			               ELDDamageResult::Applied);
		}
		bool bResultSawFinalMoney = false;
		F.State()->OnMatchStateChanged.AddLambda(
		    [&]()
		    {
			    if (F.State()->GetPhase() == ELDMatchPhase::Result)
			    {
				    const int32 ExpectedGold = Case == 3 ? 191 : (Case == 2 ? 180 : 280);
				    bResultSawFinalMoney = F.Mode->GetEconomyService()->GetSnapshot(0).Gold == ExpectedGold;
			    }
		    });
		F.World->TimeSeconds = 70;
		F.Mode->AdvanceLogic();
		if (Case != 0)
		{
			TestEqual(TEXT("Timer keeps current deadline open"), F.State()->GetPhase(), ELDMatchPhase::Running);
		}
		if (Case == 3)
		{
			FLDCommand Sale = F.Command(0, 3, ELDCommandType::Sell);
			Sale.InstanceId = UnitIds[0];
			TestEqual(TEXT("Same deadline actual PC sale precedes pending attack"),
			               F.Players[0]->SubmitServerCommand(Sale).ResultCode, ELDCommandResultCode::Success);
		}
		F.World->TimeSeconds = 70.04f;
		const FLDCommand LatePurchase = F.Command(0, 4);
		const FLDCommandResult Closed = F.Players[0]->SubmitServerCommand(LatePurchase);
		TestEqual(TEXT("New command after terminal time is rejected via real clock hook"), Closed.ResultCode,
		               ELDCommandResultCode::PhaseNotAllowed);
		TestEqual(TEXT("Exact/early hits win; late/sold attack loses"), F.State()->GetBattleSnapshot().Result,
		               Case < 2 ? ELDMatchResult::Victory : ELDMatchResult::Defeat);
		TestEqual(TEXT("Result retains actual scheduled boundary"), F.State()->GetBattleSnapshot().ResultServerSeconds,
		               Case == 0 ? 69.999 : 70.0);
		TestTrue(TEXT("Result observer sees final accepted boss rewards first"), bResultSawFinalMoney);
		TestEqual(TEXT("Finalization releases combat unit registrations"),
		               F.Mode->GetCombatService()->GetRegisteredUnitCount(), 0);
		TestEqual(TEXT("Finalization releases director registrations"),
		               F.Mode->GetWaveDirector()->GetTrackedEnemyCount(), 0);
		const int32 Revision = F.State()->GetBattleSnapshot().Revision;
		F.World->TimeSeconds = 71;
		F.Mode->AdvanceLogic();
		TestEqual(TEXT("Terminal timer repeat leaves snapshot unchanged"), F.State()->GetBattleSnapshot().Revision,
		               Revision);
		TestEqual(TEXT("Cached rejection remains replayable after service Close"),
		               F.Players[0]->SubmitServerCommand(LatePurchase).ResultCode, Closed.ResultCode);
		F.State()->OnMatchStateChanged.Clear();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDWaveFailureTest, "LD.P0.G3.Waves.PartialBossSpawnFailureAndForeignWorld",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDWaveFailureTest::RunTest(const FString& Parameters)
{
	for (bool bForeignWorld : {false, true})
	{
		FWaveFixture F;
		F.Ready();
		UWorld* ForeignWorld = nullptr;
		ALDEnemyActor* ForeignActor = nullptr;
		if (bForeignWorld)
		{
			ForeignWorld = UWorld::CreateWorld(EWorldType::Game, false);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(ForeignWorld);
			ForeignActor = ForeignWorld->SpawnActor<ALDEnemyActor>();
		}
		int32 Calls = 0;
		TestTrue(TEXT("One injected actor factory at existing spawn seam"),
		              FLDWaveTestAccess::ReplaceSpawnFactory(*F.Mode,
		                                                     [&](UWorld& World) -> ALDEnemyActor*
		                                                     {
			                                                     ++Calls;
			                                                     return Calls == 2 ? ForeignActor
			                                                                       : World.SpawnActor<ALDEnemyActor>();
		                                                     }));
		FLDWaveTestAccess::JumpToFinal(*F.Mode, 10);
		TestEqual(TEXT("Exactly the second boss creation fails"), Calls, 2);
		TestFalse(TEXT("Partial boss creation never reports final generation complete"),
		               F.State()->GetBattleSnapshot().bFinalSpawnsComplete);
		TestEqual(TEXT("Only successful first boss had a committed snapshot"),
		               F.State()->GetBattleSnapshot().Bosses.Num(), 1);
		FLDWaveTestAccess::Advance(*F.Mode, 10.001);
		TestEqual(TEXT("Required actor failure ends with explicit Aborted"), F.State()->GetBattleSnapshot().Result,
		               ELDMatchResult::Aborted);
		TestEqual(TEXT("Failure reason is initialization, not a victory or timeout"),
		               F.State()->GetBattleSnapshot().ResultReason, ELDResultReason::InitializationFailure);
		TestEqual(TEXT("No combat enemy registration remains"), F.Mode->GetCombatService()->GetRegisteredEnemyCount(),
		               0);
		TestEqual(TEXT("No director registration remains"), F.Mode->GetWaveDirector()->GetTrackedEnemyCount(), 0);
		TestFalse(TEXT("Owned logic timer was cleared"), F.Mode->IsLogicTimerActive());
		const int32 Revision = F.State()->GetBattleSnapshot().Revision;
		FLDWaveTestAccess::Advance(*F.Mode, 100);
		TestEqual(TEXT("No late retry or terminal mutation"), F.State()->GetBattleSnapshot().Revision, Revision);
		TestEqual(TEXT("No spawn failure reward"), F.Mode->GetEconomyService()->GetSnapshot(0).Gold, 100);
		if (ForeignWorld)
		{
			TestFalse(TEXT("Foreign actor is not initialized or deleted by this match"),
			               ForeignActor->IsActorBeingDestroyed());
			TestEqual(TEXT("Foreign actor retains uninitialized ID"), ForeignActor->GetRouteSnapshot().EnemyId,
			               uint64(0));
			ForeignWorld->DestroyWorld(false);
			GEngine->DestroyWorldContext(ForeignWorld);
		}
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDWaveDamageObserverTest, "LD.P0.G3.Waves.CommittedDeathBeforeObserverAbort",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDWaveDamageObserverTest::RunTest(const FString& Parameters)
{
	FWaveFixture F;
	F.Ready();
	FLDWaveTestAccess::Advance(*F.Mode, 10.0001);
	const TArray<ALDEnemyActor*> Enemies = FLDWaveTestAccess::Enemies(*F.Mode->GetWaveDirector());
	if (!TestEqual(TEXT("Observer boundary starts with exactly two real wave enemies"), Enemies.Num(), 2))
	{
		return false;
	}
	for (const ALDEnemyActor* Enemy : Enemies)
	{
		TestEqual(TEXT("Each untouched normal enemy starts at HP70"), Enemy->GetCombatSnapshot().HP, 70.0);
	}
	FLDUnitRow Row;
	F.Mode->GetGameData()->TryGetUnitRow(TEXT("C02"), Row);
	// Explicit observer-lifetime fixture: two strong attacks due at11; no product economy/command bypass.
	Row.BaseAttack = 1000;
	Row.RangeCm = 1000;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FLDPlacedUnit Placement;
		Placement.InstanceId = 800000 + Index;
		Placement.UnitId = Row.UnitId;
		Placement.CellId = Index;
		Placement.PlayerIndex = Index;
		ALDUnitActor* Unit = F.World->SpawnActor<ALDUnitActor>();
		if (!TestTrue(TEXT("Prepare actual observer fixture unit"),
		                   Unit->InitializePrepared(Placement, Row, FTransform::Identity)))
		{
			return false;
		}
		Unit->ApplyCommittedPlacement(Placement, FTransform::Identity);
		F.Mode->GetCombatService()->RegisterCommittedUnit(*Unit, 10.75);
		double Due = 0;
		TestTrue(TEXT("Each fixture unit owns a scheduled attack"),
		              F.Mode->GetCombatService()->TryGetUnitAttackState(Placement.InstanceId, Due));
		TestEqual(TEXT("Both attacks are due at the same exact timestamp"), Due, 11.0);
	}
	int32 DamageCount = 0;
	int32 ObservedDamage = 0;
	uint64 FirstDamagedEnemyId = 0;
	bool bResultSawReward = false;
	F.State()->OnMatchStateChanged.AddLambda(
	    [&]()
	    {
		    if (F.State()->GetPhase() == ELDMatchPhase::Aborted)
		    {
			    bResultSawReward = F.Mode->GetEconomyService()->GetSnapshot(0).Gold == 101 &&
			                       F.Mode->GetEconomyService()->GetSnapshot(1).Gold == 101;
		    }
	    });
	F.Mode->GetCombatService()->OnDamageCommitted.AddLambda(
	    [&](const FLDDamageEvent& Event, int32 PlayerIndex, int32 EffectiveDamage)
	    {
		    ++DamageCount;
		    ObservedDamage = EffectiveDamage;
		    if (FirstDamagedEnemyId == 0)
		    {
			    FirstDamagedEnemyId = Event.EnemyId;
		    }
		    F.Mode->AbortMatch(TEXT("damage observer requests shutdown"));
	    });
	AddExpectedError(TEXT("Match aborted: damage observer requests shutdown"), EAutomationExpectedErrorFlags::Contains,
	                      1);
	F.World->TimeSeconds = 11.1f;
	FLDWaveTestAccess::Advance(*F.Mode, 11.1);
	TestEqual(TEXT("One already committed hit remains observable through self-clear"), DamageCount, 1);
	TestEqual(TEXT("Effective damage excludes930 overkill"), ObservedDamage, 70);
	TestTrue(TEXT("Approved death reward precedes terminal result when observer requests Abort"), bResultSawReward);
	TestEqual(TEXT("First committed death grants each player exactly one reward"),
	               F.Mode->GetEconomyService()->GetSnapshot(0).Gold, 101);
	TestEqual(TEXT("Partner receives the same single accepted death reward"),
	               F.Mode->GetEconomyService()->GetSnapshot(1).Gold, 101);
	for (const ALDEnemyActor* Enemy : Enemies)
	{
		const bool bFirstVictim = Enemy->GetRouteSnapshot().EnemyId == FirstDamagedEnemyId;
		TestEqual(bFirstVictim ? TEXT("Already committed first victim remains dead")
		                       : TEXT("Mode Abort alone prevents second due attack: untouched HP70"),
		                              Enemy->GetCombatSnapshot().HP, bFirstVictim ? 0.0 : 70.0);
	}
	TestEqual(TEXT("Actual terminal retains Abort"), F.State()->GetBattleSnapshot().Result, ELDMatchResult::Aborted);
	TestEqual(TEXT("Mode Abort leaves no combat enemy registration"),
	               F.Mode->GetCombatService()->GetRegisteredEnemyCount(), 0);
	TestFalse(TEXT("Mode Abort clears owned mode timer"), F.Mode->IsLogicTimerActive());
	TestEqual(TEXT("Same timestamp scheduled spawns remain cancelled after observer Abort"),
	               F.State()->GetBattleSnapshot().ActiveEnemyCount, 1);
	F.State()->OnMatchStateChanged.Clear();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLDWaveBossObserverAbortTest,
                                 "LD.P0.G3.Waves.CommittedBossHPBeforeObserverAbortResult",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLDWaveBossObserverAbortTest::RunTest(const FString& Parameters)
{
	FWaveFixture F;
	F.Ready();
	FLDWaveTestAccess::JumpToFinal(*F.Mode, 10);
	const TArray<ALDEnemyActor*> Bosses = FLDWaveTestAccess::Enemies(*F.Mode->GetWaveDirector());
	if (!TestEqual(TEXT("Fixture starts with two actual B01 actors"), Bosses.Num(), 2))
	{
		return false;
	}
	for (const ALDEnemyActor* Boss : Bosses)
	{
		TestEqual(TEXT("Both bosses start with independently specified HP6000"), Boss->GetCombatSnapshot().HP, 6000.0);
		TestEqual(TEXT("B01 physical armor is20"), Boss->GetEnemyRow().Armor, 20.0);
	}
	FLDUnitRow Row;
	F.Mode->GetGameData()->TryGetUnitRow(TEXT("C02"), Row);
	// Explicit nonlethal fixture: physical120 / (1 + armor20/100) = exactly100 damage.
	Row.BaseAttack = 120;
	Row.DamageType = TEXT("Physical");
	Row.RangeCm = 1000;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FLDPlacedUnit Placement;
		Placement.InstanceId = 810000 + Index;
		Placement.UnitId = Row.UnitId;
		Placement.CellId = Index;
		Placement.PlayerIndex = Index;
		ALDUnitActor* Unit = F.World->SpawnActor<ALDUnitActor>();
		if (!TestTrue(TEXT("Prepare actual nonlethal observer fixture unit"),
		                   Unit->InitializePrepared(Placement, Row, FTransform::Identity)))
		{
			return false;
		}
		Unit->ApplyCommittedPlacement(Placement, FTransform::Identity);
		F.Mode->GetCombatService()->RegisterCommittedUnit(*Unit, 10.75);
	}
	int32 DamageCount = 0;
	int32 ResultPublications = 0;
	uint64 FirstDamagedBossId = 0;
	F.State()->OnMatchStateChanged.AddLambda(
	    [&]()
	    {
		    if (F.State()->GetPhase() != ELDMatchPhase::Aborted)
		    {
			    return;
		    }
		    ++ResultPublications;
		    const FLDBattleSnapshot& Result = F.State()->GetBattleSnapshot();
		    TestEqual(TEXT("Result observer receives both boss views"), Result.Bosses.Num(), 2);
		    for (const ALDEnemyActor* Boss : Bosses)
		    {
			    const uint64 BossId = Boss->GetRouteSnapshot().EnemyId;
			    const FLDBossSnapshot* View = Result.Bosses.FindByPredicate([BossId](const FLDBossSnapshot& Candidate)
			                                                                { return Candidate.EnemyId == BossId; });
			    if (TestNotNull(TEXT("Result boss identity still matches its actual actor"), View))
			    {
				    TestEqual(TEXT("Result observer sees final committed actor HP in shared state"), View->HP,
				                   Boss->GetCombatSnapshot().HP);
				    TestEqual(TEXT("Only first100 damage is reflected before Result; no second due attack"), View->HP,
				                   BossId == FirstDamagedBossId ? 5900.0 : 6000.0);
			    }
		    }
	    });
	F.Mode->GetCombatService()->OnDamageCommitted.AddLambda(
	    [&](const FLDDamageEvent& Event, int32 PlayerIndex, int32 EffectiveDamage)
	    {
		    ++DamageCount;
		    FirstDamagedBossId = Event.EnemyId;
		    TestEqual(TEXT("Actual combat commits the independent100 damage expectation"), EffectiveDamage, 100);
		    F.Mode->AbortMatch(TEXT("boss damage observer requests shutdown"));
	    });
	AddExpectedError(TEXT("Match aborted: boss damage observer requests shutdown"),
	                      EAutomationExpectedErrorFlags::Contains, 1);
	F.World->TimeSeconds = 11.1f;
	FLDWaveTestAccess::Advance(*F.Mode, 11.1);
	TestEqual(TEXT("Mode Abort cancels the second nonlethal attack due at11"), DamageCount, 1);
	TestEqual(TEXT("Final committed HP is ready for the first and only terminal publication"), ResultPublications, 1);
	TestEqual(TEXT("Nonlethal damage cannot produce a death reward for owner"),
	               F.Mode->GetEconomyService()->GetSnapshot(0).Gold, 100);
	TestEqual(TEXT("Nonlethal damage cannot produce a death reward for partner"),
	               F.Mode->GetEconomyService()->GetSnapshot(1).Gold, 100);
	TestEqual(TEXT("Synchronizing boss HP does not evaluate victory"), F.State()->GetBattleSnapshot().Result,
	               ELDMatchResult::Aborted);
	const int32 FinalRevision = F.State()->GetBattleSnapshot().Revision;
	FLDWaveTestAccess::Advance(*F.Mode, 12);
	TestEqual(TEXT("Later timeline calls cannot change final boss state"), F.State()->GetBattleSnapshot().Revision,
	               FinalRevision);
	F.State()->OnMatchStateChanged.Clear();
	return true;
}
#endif
