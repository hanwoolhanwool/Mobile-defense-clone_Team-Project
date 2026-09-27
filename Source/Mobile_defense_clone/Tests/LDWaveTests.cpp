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
#include "Data/LDGameData.h"
#include "Economy/LDEconomyService.h"
#include "Engine/Engine.h"
#include "Engine/Player.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
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
};

namespace
{
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
			const int32 Revision = F.State()->GetBattleSnapshot().Revision;
			FLDWaveTestAccess::Advance(*F.Mode, 12);
			TestEqual(TEXT("Later timeline cannot rescue latch"), F.State()->GetBattleSnapshot().Revision, Revision);
		}
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

#endif