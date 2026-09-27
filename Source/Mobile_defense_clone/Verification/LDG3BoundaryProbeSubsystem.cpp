#include "Verification/LDG3BoundaryProbeSubsystem.h"

#include "Battle/LDCombatService.h"
#include "Battle/LDEnemyActor.h"
#include "Battle/LDUnitActor.h"
#include "Battle/LDWaveDirector.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Board/LDBoardManager.h"
#include "Core/LDEntryPlayerController.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Data/LDGameData.h"
#include "Economy/LDEconomyService.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "Network/LDCommandProcessor.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "Widgets/SWindow.h"

namespace
{
	bool IsBoundaryProcess()
	{
#if UE_BUILD_SHIPPING
		return false;
#else
		FString Probe;
		return FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe) && Probe == TEXT("G3Boundary");
#endif
	}
	FString Id(uint64 Value)
	{
		return FString::Printf(TEXT("%llu"), Value);
	}
	FString BoardKey(const FLDBoardSnapshot& B)
	{
		FString S =
		    FString::Printf(TEXT("%s/%d/%d/%d"), *B.MatchId.ToString(), B.PlayerIndex, B.BoardRevision, B.Population);
		for (const FLDPlacedUnit& U : B.Units)
			S += FString::Printf(TEXT("|%llu/%s/%d/%d/%.9f"), U.InstanceId, *U.UnitId.ToString(), U.PlayerIndex,
			                          U.CellId, U.MoveBlockedUntilServerSeconds);
		return S;
	}
	FString MoneyKey(const FLDEconomySnapshot& E)
	{
		return FString::Printf(TEXT("%s/%d/%d/%d/%d/%d/%d"), *E.MatchId.ToString(), E.PlayerIndex, E.EconomyRevision,
		                            E.Gold, E.Stars, E.PaidSummonCount, E.NextSummonGold);
	}
	FString BattleKey(const FLDBattleSnapshot& B)
	{
		FString S = FString::Printf(
		    TEXT("%s/%d/%d/%d/%d/%d/%d/%d/%d/%d/%.9f/%.9f/%.9f/%.9f/%.9f"), *B.MatchId.ToString(), B.Revision,
		         int32(B.Phase), B.WaveIndex, B.FinalWave, B.ActiveEnemyCount, B.MaxEnemyCount, B.bFinalSpawnsComplete,
		         int32(B.Result), int32(B.ResultReason), B.LoadingDeadlineServerSeconds, B.PreparationEndServerSeconds,
		         B.WaveEndServerSeconds, B.BossDeadlineServerSeconds, B.ResultServerSeconds);
		for (const FLDBossSnapshot& Boss : B.Bosses)
			S += FString::Printf(TEXT("|%llu/%d/%.9f/%.9f/%d"), Boss.EnemyId, Boss.RouteIndex, Boss.HP, Boss.MaxHP,
			                          Boss.bAlive);
		return S;
	}
	class FBoundaryWireObserver final : public FOutputDevice
	{
	public:
		TFunction<void(const FString&)> Observe;
		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (IsInGameThread() && Category == TEXT("LogLDBoardInput") &&
			                                         FCString::Strstr(V, TEXT("P0WIRE CLIENT ")) && Observe)
				Observe(V);
		}
	};
} // namespace

// The only additional product access is this Development fixture. It cannot write World time or results.
struct FLDG3BoundaryAccess
{
	static bool Begin(ALDGameMode& M, bool bCap, double Now)
	{
#if !UE_BUILD_SHIPPING
		ALDGameState* GS = M.GetGameState<ALDGameState>();
		ULDWaveDirector* D = M.GetWaveDirector();
		if (!IsBoundaryProcess() || !GS || !D || GS->GetPhase() != ELDMatchPhase::Preparing || !M.CanAcceptCommands() ||
		    D->GetTrackedEnemyCount() != 0 || M.GetCombatService()->GetRegisteredEnemyCount() != 0)
			return false;
		// Explicitly skip the remaining preparation and waves 1..9; preserve the actual current World clock.
		if (!GS->SetPhase(ELDMatchPhase::Running))
			return false;
		M.LogicOriginSeconds = Now;
		M.LogicStep = 0;
		D->bStarted = true;
		if (!D->BeginWave(bCap ? 1 : 10, Now))
			return false;
		if (bCap)
			D->NextNormalOrdinal = M.GetGameData()->GetWaves()[0].NormalCountPerGate;
		return true;
#else
		return false;
#endif
	}
	static bool AddNormal(ALDGameMode& M, double Now)
	{
#if !UE_BUILD_SHIPPING
		return IsBoundaryProcess() && M.GetWaveDirector()->SpawnEnemy(0, M.GetGameData()->GetWaves()[0], Now);
#else
		return false;
#endif
	}
};

ALDG3BoundaryPeer::ALDG3BoundaryPeer()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	SetNetUpdateFrequency(20);
}
void ALDG3BoundaryPeer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDG3BoundaryPeer, CaseIndex);
	DOREPLIFETIME(ALDG3BoundaryPeer, bFixtureReady);
	DOREPLIFETIME(ALDG3BoundaryPeer, bNormalWaitObserved);
	DOREPLIFETIME(ALDG3BoundaryPeer, bTerminalCaptured);
	DOREPLIFETIME(ALDG3BoundaryPeer, bMayReturn);
	DOREPLIFETIME(ALDG3BoundaryPeer, Deadline);
	DOREPLIFETIME(ALDG3BoundaryPeer, DueTimes);
	DOREPLIFETIME(ALDG3BoundaryPeer, FinalBattle);
	DOREPLIFETIME(ALDG3BoundaryPeer, FinalBoards);
	DOREPLIFETIME(ALDG3BoundaryPeer, FinalEconomies);
	DOREPLIFETIME(ALDG3BoundaryPeer, ActualBosses);
	DOREPLIFETIME(ALDG3BoundaryPeer, FinalNormalIds);
	DOREPLIFETIME(ALDG3BoundaryPeer, bPriorPayloadChecked);
	DOREPLIFETIME(ALDG3BoundaryPeer, bPriorPayloadUnchanged);
}
void ALDG3BoundaryPeer::ServerObserve_Implementation(FGuid MatchId, int32 Stage, int32 Revision, bool bPass)
{
	const ALDGameState* GS = GetWorld()->GetGameState<ALDGameState>();
	if (!IsBoundaryProcess() || !GS || GS->GetMatchContext().MatchId != MatchId || !bPass)
		return;
	if (Stage == 0 && CaseIndex > 0 && !bPriorPayloadChecked)
	{
		ALDGameMode* M = GetWorld()->GetAuthGameMode<ALDGameMode>();
		const ALDPlayerController* PC = Cast<ALDPlayerController>(GetOwner());
		const ALDPlayerState* PS = PC ? PC->GetPlayerState<ALDPlayerState>() : nullptr;
		if (M && PS && PS->GetParticipantContext().IsValid())
		{
			const int32 Index = PS->GetParticipantContext().PlayerIndex;
			const auto Board = M->GetBoardManager()->GetSnapshot(Index);
			const auto Money = M->GetEconomyService()->GetSnapshot(Index);
			bPriorPayloadChecked = true;
			bPriorPayloadUnchanged = Board.BoardRevision == 0 && Board.Units.IsEmpty() && Board.Population == 0 &&
			                         Money.Gold == 100 && Money.PaidSummonCount == 0 && Money.EconomyRevision == 0 &&
			                         M->GetCommandProcessor()->GetCachedResultCount(Index) == 0 &&
			                         M->GetEconomyService()->GetRandomState(Index) == InitialRandom;
			ForceNetUpdate();
		}
	}
	if (Stage == 1 && bFixtureReady)
		bReadyObserved = true;
	if (Stage == 2 && bTerminalCaptured && FinalBattle.Revision == Revision)
		bTerminalObserved = true;
}

struct FLDG3BoundaryState : public TSharedFromThis<FLDG3BoundaryState>
{
	TWeakObjectPtr<ULDG3BoundaryProbeSubsystem> Owner;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<ULDCombatService> Combat;
	TArray<TWeakObjectPtr<ALDG3BoundaryPeer>> Peers;
	TWeakObjectPtr<ALDG3BoundaryPeer> LocalPeer;
	TArray<TWeakObjectPtr<ALDEnemyActor>> BossActors;
	TMap<uint64, FLDEnemyCombatSnapshot> SeenActors;
	TArray<FLDBossSnapshot> LastBossActors;
	TArray<TSharedPtr<FJsonValue>> Checks, Cases, Hits;
	TArray<FGuid> MatchIds;
	TUniquePtr<FBoundaryWireObserver> Wire;
	FDelegateHandle DamageHandle;
	FString Role, Address, Output, OldReply;
	FGuid Match, OldMatch;
	FLDCommand OldSummon;
	double Started = 0, EntryAt = 0, ActionAt = 0, Deadline = 0, TerminalAt = 0, FinishAt = 0, LastProgress = 0;
	double FixtureViewAt = 0;
	double OldSentAt = 0, CapAt = 0, LastKillAt = 0, HitchBefore = 0, HitchAfterWall = 0;
	int32 ActiveCase = 0, Completed = 0, Returns = 0, Timeout = 600, LocalIndex = INDEX_NONE, HitCount = 0;
	uint64 UnitIds[2] = {0, 0};
	uint64 LocalUnitId = 0;
	TWeakObjectPtr<ALDUnitActor> LocalUnitActor;
	double Dues[2] = {0, 0};
	bool bFailed = false, bFinished = false, bEntryClicked = false, bPrepared = false, bTerminalRecorded = false;
	bool bReadyAck = false, bCapTriggered = false, bNormalWait = false, bNormalRescheduled = false;
	bool bHitch = false, bOldSent = false, bOldDone = false, bMoveSent = false, bReturning = false;
	bool bLocalActorCaptured = false;
	bool bTerminalAuthorityChecked = false, bTerminalFrozenChecked = false, bWritableOutput = false;
	bool bPurchaseSent = false, bPurchaseCaptured = false, bLocalNormalWait = false, bOldStateRequested = false;
	void Authority(ALDGameMode& M);
	void Local(ALDPlayerController& PC, const FLDBattleSnapshot& B);
	void Record(ALDPlayerController& PC, ALDG3BoundaryPeer& Peer);
	void Tick();

	void Check(const FString& Name, bool Pass, const FString& Detail = TEXT(""))
	{
		bFailed |= !Pass;
		TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("name"), Name);
		J->SetBoolField(TEXT("pass"), Pass);
		J->SetStringField(TEXT("detail"), Detail);
		J->SetStringField(TEXT("matchId"), Match.ToString());
		J->SetNumberField(TEXT("caseIndex"), ActiveCase);
		J->SetNumberField(TEXT("wallSeconds"), FPlatformTime::Seconds() - Started);
		Checks.Add(MakeShared<FJsonValueObject>(J));
		UE_LOG(LogTemp, Display, TEXT("P0BOUNDARY %s %s %s"), Pass ? TEXT("PASS") : TEXT("FAIL"), *Name, *Detail);
	}
	void Write(bool Final)
	{
		if (!bWritableOutput)
			return;
		TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("result"), Final ? (bFailed ? TEXT("Fail") : TEXT("Pass")) : TEXT("Running"));
		J->SetStringField(
		    TEXT("scope"),
		         TEXT("Development two-process boundary fixture; paid summon/move; skips remaining preparation/waves; boss HP 6000->1 and case2 normal HP 70->1 lowered nonlethally; attack registration times selected; case3 extra normal spawns and remaining scheduled normal spawns skipped; actual World clock and production result/reward/death paths unchanged. Not natural play or balance."));
		J->SetStringField(TEXT("role"), Role);
		J->SetNumberField(TEXT("processId"), FPlatformProcess::GetCurrentProcessId());
		J->SetNumberField(TEXT("completedCases"), Completed);
		J->SetNumberField(TEXT("returns"), Returns);
		J->SetNumberField(TEXT("elapsedWallSeconds"), FPlatformTime::Seconds() - Started);
		J->SetArrayField(TEXT("checks"), Checks);
		J->SetArrayField(TEXT("cases"), Cases);
		FString Text;
		FJsonSerializer::Serialize(J.ToSharedRef(), TJsonWriterFactory<>::Create(&Text));
		if (!FFileHelper::SaveStringToFile(Text, *(Output / (Final ? TEXT("result.json") : TEXT("progress.json")))))
			bFailed = true;
		LastProgress = FPlatformTime::Seconds();
	}
	void Finish()
	{
		if (bFinished)
			return;
		Check(TEXT("four-distinct-matches"), Completed == 4 && MatchIds.Num() == 4);
		Check(TEXT("three-actual-return-clicks"), Returns == 3);
		bFinished = true;
		Write(true);
		FPlatformMisc::RequestExit(false);
	}
	bool Click(const FBox2D& Rect)
	{
		UWorld* W = World.Get();
		if (!W || !W->GetGameViewport() || !W->GetGameViewport()->GetWindow() || !FSlateApplication::IsInitialized())
			return false;
		FVector2D P;
		USlateBlueprintLibrary::ScreenToWidgetAbsolute(Owner.Get(), Rect.GetCenter(), P);
		const FPointerEvent Down(0, P, P, {EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState());
		const FPointerEvent Up(0, P, P, {}, EKeys::LeftMouseButton, 0, FModifierKeysState());
		FSlateApplication& Slate = FSlateApplication::Get();
		Slate.ProcessMouseMoveEvent(Down, true);
		Slate.ProcessMouseButtonDownEvent(W->GetGameViewport()->GetWindow()->GetNativeWindow(), Down);
		Slate.ProcessMouseButtonUpEvent(Up);
		return true;
	}
	void Release()
	{
		if (Combat.IsValid())
			Combat->OnDamageCommitted.Remove(DamageHandle);
		Combat.Reset();
		DamageHandle.Reset();
		Peers.Reset();
		LocalPeer.Reset();
		BossActors.Reset();
	}
	void Begin(const FLDBattleSnapshot& B)
	{
		Release();
		Match = B.MatchId;
		ActiveCase = Completed;
		Check(TEXT("new-match-context"), !MatchIds.Contains(Match));
		MatchIds.Add(Match);
		SeenActors.Reset();
		LastBossActors.Reset();
		Hits.Reset();
		HitCount = 0;
		UnitIds[0] = UnitIds[1] = 0;
		LocalUnitId = 0;
		LocalUnitActor.Reset();
		bLocalActorCaptured = false;
		FixtureViewAt = 0;
		Deadline = TerminalAt = CapAt = LastKillAt = OldSentAt = 0;
		bPrepared = bTerminalRecorded = bReadyAck = bCapTriggered = bNormalWait = bNormalRescheduled = bHitch = false;
		bOldSent = bOldDone = bMoveSent = bReturning = bTerminalAuthorityChecked = false;
		bPurchaseSent = bPurchaseCaptured = bLocalNormalWait = bTerminalFrozenChecked = bOldStateRequested = false;
		OldReply.Empty();
		EntryAt = 0;
		LocalIndex = INDEX_NONE;
	}
	void ObserveActors()
	{
		for (TActorIterator<ALDEnemyActor> It(World.Get()); It; ++It)
			if (It->GetRouteSnapshot().MatchId == Match)
				SeenActors.Add(It->GetRouteSnapshot().EnemyId, It->GetCombatSnapshot());
	}
	bool Schedule(ALDGameMode& M, int32 Player, double At)
	{
		ALDUnitActor* Unit = nullptr;
		double Actual = 0;
		if (!M.GetBoardManager()->TryGetCommittedUnitActor(UnitIds[Player], Unit) || !Unit)
			return false;
		M.GetCombatService()->UnregisterUnit(UnitIds[Player]);
		M.GetCombatService()->RegisterCommittedUnit(*Unit, At - M.GetGameData()->GetRules().InitialAttackDelaySeconds);
		return M.GetCombatService()->TryGetUnitAttackState(UnitIds[Player], Actual) && FMath::Abs(Actual - At) < 1.e-9;
	}
	void Damage(const FLDDamageEvent& E, int32 Player, int32 Effective)
	{
		const bool ResidualHit = ActiveCase == 2 && bNormalRescheduled;
		const double ExpectedDue = ResidualHit ? Deadline + 2 : ((Player >= 0 && Player < 2) ? Dues[Player] : -1);
		Check(
		    TEXT("committed-attack-kept-original-scheduled-time"),
		         Player >= 0 && Player < 2 && E.SourceInstanceId == UnitIds[Player] &&
		             FMath::Abs(E.AttackServerSeconds - ExpectedDue) < 1.e-8 && Effective == 1,
		         FString::Printf(TEXT("unit=%llu player=%d expected=%.9f actual=%.9f observedWorld=%.9f effective=%d"),
		                              E.SourceInstanceId, Player, ExpectedDue, E.AttackServerSeconds,
		                              World->GetTimeSeconds(), Effective));
		if (ActiveCase == 0)
			Check(TEXT("hitch-hit-arrived-after-deadline-with-unchanged-time"),
			           bHitch && World->GetTimeSeconds() > Deadline);
		++HitCount;
		LastKillAt = E.AttackServerSeconds;
		ObserveActors();
		TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("enemyId"), Id(E.EnemyId));
		J->SetStringField(TEXT("unitId"), Id(E.SourceInstanceId));
		J->SetStringField(TEXT("damageEventId"), Id(E.DamageEventId));
		J->SetNumberField(TEXT("player"), Player);
		J->SetNumberField(TEXT("scheduledSeconds"), E.AttackServerSeconds);
		J->SetNumberField(TEXT("observedWorldSeconds"), World->GetTimeSeconds());
		J->SetNumberField(TEXT("effectiveDamage"), Effective);
		Hits.Add(MakeShared<FJsonValueObject>(J));
		for (FLDBossSnapshot& B : LastBossActors)
			if (const FLDEnemyCombatSnapshot* A = SeenActors.Find(B.EnemyId))
			{
				B.HP = A->HP;
				B.bAlive = A->bAlive;
			}
		if (ActiveCase == 2 && !bNormalRescheduled && LastBossActors.Num() == 2 && !LastBossActors[0].bAlive &&
		    !LastBossActors[1].bAlive)
		{
			if (ALDGameMode* M = World->GetAuthGameMode<ALDGameMode>())
			{
				bNormalRescheduled = true;
				Check(TEXT("residual-normal-attack-delay-fixture"), Schedule(*M, 0, Deadline + 2));
			}
		}
	}
	bool Prepare(ALDGameMode& M)
	{
		const double Now = World->GetTimeSeconds();
		for (int32 P = 0; P < 2; ++P)
		{
			const FLDBoardSnapshot& B = M.GetBoardManager()->GetSnapshot(P);
			if (B.Units.Num() != 1 || B.Units[0].CellId != (P == 0 ? 5 : 35) || B.BoardRevision != 2)
				return false;
			UnitIds[P] = B.Units[0].InstanceId;
			Check(TEXT("paid-fixture-baseline"), M.GetEconomyService()->GetSnapshot(P).Gold == 80 && B.Population == 1);
		}
		if (!FLDG3BoundaryAccess::Begin(M, ActiveCase == 3, Now))
		{
			Check(TEXT("fixture-jump-no-existing-enemies"), false);
			return false;
		}
		bPrepared = true;
		Deadline =
		    ActiveCase == 3 ? Now + 60 : M.GetGameState<ALDGameState>()->GetBattleSnapshot().BossDeadlineServerSeconds;
		Check(TEXT("real-clock-sixty-second-deadline"), ActiveCase == 3 || FMath::Abs(Deadline - Now - 60) < 1.e-9);
		for (int32 P = 0; P < 2; ++P)
		{
			Dues[P] = Deadline + (ActiveCase == 0 && P == 0 ? -.001 : (ActiveCase == 1 && P == 1 ? .001 : 0));
			Check(TEXT("registered-exact-due-time"), Schedule(M, P, Dues[P]),
			           FString::Printf(TEXT("player%d unit=%llu due=%.9f"), P, UnitIds[P], Dues[P]));
		}
		Combat = M.GetCombatService();
		TWeakPtr<FLDG3BoundaryState> Weak = AsShared();
		DamageHandle = Combat->OnDamageCommitted.AddLambda(
		    [Weak](const FLDDamageEvent& E, int32 P, int32 A)
		    {
			    if (auto S = Weak.Pin())
				    S->Damage(E, P, A);
		    });
		if (ActiveCase == 3)
		{
			while (M.GetGameState<ALDGameState>()->GetBattleSnapshot().ActiveEnemyCount < 99)
				if (!FLDG3BoundaryAccess::AddNormal(M, Now))
				{
					Check(TEXT("prepare-n99"), false);
					break;
				}
			Check(TEXT("prepare-n99"), M.GetGameState<ALDGameState>()->GetBattleSnapshot().ActiveEnemyCount == 99);
		}
		else
		{
			if (ActiveCase == 2)
				Check(TEXT("one-residual-normal-fixture"), FLDG3BoundaryAccess::AddNormal(M, Now));
			for (TActorIterator<ALDEnemyActor> It(World.Get()); It; ++It)
			{
				if (It->GetRouteSnapshot().MatchId != Match)
					continue;
				const auto Before = It->GetCombatSnapshot();
				FLDDamageEvent Prep;
				Prep.MatchId = Match;
				Prep.EnemyId = It->GetRouteSnapshot().EnemyId;
				Prep.DamageEventId = 1000000 + Prep.EnemyId;
				Prep.SourceInstanceId = 800000;
				Prep.Amount = FMath::RoundToInt(Before.HP - 1);
				Prep.AttackServerSeconds = Now;
				FLDCombatDeath NoDeath;
				Check(TEXT("nonlethal-hp-one-fixture"), It->TryApplyDamage(Prep, NoDeath) == ELDDamageResult::Applied &&
				                                            It->GetCombatSnapshot().HP == 1);
				if (It->GetEnemyRow().Kind == TEXT("Boss"))
				{
					BossActors.Add(*It);
					FLDBossSnapshot B;
					B.EnemyId = Prep.EnemyId;
					B.RouteIndex = It->GetRouteSnapshot().RouteIndex;
					B.HP = 1;
					B.MaxHP = Before.MaxHP;
					B.bAlive = true;
					LastBossActors.Add(B);
				}
			}
			M.GetWaveDirector()->RefreshCombatView();
			LastBossActors.Sort([](const FLDBossSnapshot& A, const FLDBossSnapshot& B)
			                    { return A.RouteIndex < B.RouteIndex; });
			Check(TEXT("two-real-boss-actors"), LastBossActors.Num() == 2);
		}
		for (auto P : Peers)
			if (P.IsValid())
			{
				P->bFixtureReady = true;
				P->Deadline = Deadline;
				P->DueTimes = {Dues[0], Dues[1]};
				P->ForceNetUpdate();
			}
		return true;
	}
};

void FLDG3BoundaryState::Authority(ALDGameMode& M)
{
	ALDGameState* GS = M.GetGameState<ALDGameState>();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ALDPlayerController* PC = Cast<ALDPlayerController>(It->Get());
		const ALDPlayerState* PS = PC ? PC->GetPlayerState<ALDPlayerState>() : nullptr;
		if (!PS || !PS->GetParticipantContext().IsValid())
			continue;
		bool Found = false;
		for (auto P : Peers)
			Found |= P.IsValid() && P->GetOwner() == PC;
		if (!Found)
		{
			FActorSpawnParameters Params;
			Params.Owner = PC;
			ALDG3BoundaryPeer* P = World->SpawnActor<ALDG3BoundaryPeer>(Params);
			if (!P)
			{
				Check(TEXT("owner-peer-created"), false);
				return;
			}
			P->CaseIndex = ActiveCase;
			P->InitialRandom = M.GetEconomyService()->GetRandomState(PS->GetParticipantContext().PlayerIndex);
			Peers.Add(P);
		}
	}
	if (Peers.Num() != 2)
		return;
	if (!bPrepared && GS->GetPhase() == ELDMatchPhase::Preparing)
	{
		if (!Prepare(M))
			return;
		for (int32 P = 0; P < 2; ++P)
			Check(TEXT("only-paid-summon-and-move-cached"), M.GetCommandProcessor()->GetCachedResultCount(P) == 2);
	}
	if (!bPrepared)
		return;
	const double Now = World->GetTimeSeconds();
	if (ActiveCase == 0 && !bHitch && Now >= Deadline - .025 && Now < Deadline)
	{
		bHitch = true;
		HitchBefore = Now;
		const double BeforeWall = FPlatformTime::Seconds();
		FPlatformProcess::Sleep(.2f);
		HitchAfterWall = FPlatformTime::Seconds() - BeforeWall;
		Check(TEXT("actual-host-hitch"), HitchAfterWall >= .19,
		           FString::Printf(TEXT("worldBefore=%.9f deadline=%.9f sleptWall=%.6f; World clock never assigned"),
		                                Now, Deadline, HitchAfterWall));
	}
	if (ActiveCase == 2 && !bNormalWait && Now > Deadline + .1 && !GS->GetBattleSnapshot().IsTerminal())
	{
		const auto& B = GS->GetBattleSnapshot();
		bNormalWait = B.Bosses.Num() == 2 && !B.Bosses[0].bAlive && !B.Bosses[1].bAlive && B.ActiveEnemyCount == 1;
		Check(TEXT("both-bosses-dead-normal-one-past-deadline"), bNormalWait,
		           FString::Printf(TEXT("world=%.9f deadline=%.9f N=%d phase=%d"), Now, Deadline, B.ActiveEnemyCount,
		                                int32(B.Phase)));
		for (auto P : Peers)
			if (P.IsValid())
			{
				P->bNormalWaitObserved = bNormalWait;
				P->ForceNetUpdate();
			}
	}
	bool BothReady = true;
	for (auto P : Peers)
		BothReady &= P.IsValid() && P->bReadyObserved;
	if (ActiveCase == 3 && BothReady && !bCapTriggered)
	{
		bCapTriggered = true;
		CapAt = Now;
		Check(TEXT("n99-before-single-spawn"), GS->GetBattleSnapshot().ActiveEnemyCount == 99);
		const bool Continued = FLDG3BoundaryAccess::AddNormal(M, Now);
		Check(TEXT("n100-immediately-closes-admission"),
		           !Continued && GS->GetBattleSnapshot().ActiveEnemyCount == 100 && !M.CanAcceptCommands());
	}
	if (!GS->GetBattleSnapshot().IsTerminal())
		return;
	if (!bTerminalAuthorityChecked)
	{
		bTerminalAuthorityChecked = true;
		const auto& B = GS->GetBattleSnapshot();
		const bool Win = ActiveCase == 0 || ActiveCase == 2;
		Check(TEXT("both-peers-saw-fixture-before-terminal"), BothReady);
		Check(TEXT("production-result-kind"), B.Result == (Win ? ELDMatchResult::Victory : ELDMatchResult::Defeat));
		Check(TEXT("production-result-reason"),
		           B.ResultReason ==
		               (Win ? ELDResultReason::None
		                    : (ActiveCase == 1 ? ELDResultReason::BossTimeout : ELDResultReason::EnemyLimit)));
		const double ExpectedAt = ActiveCase == 3 ? CapAt : (ActiveCase == 2 ? LastKillAt : Deadline);
		Check(TEXT("scheduled-terminal-time"), FMath::Abs(B.ResultServerSeconds - ExpectedAt) < 1.e-8,
		           FString::Printf(TEXT("expected=%.9f actual=%.9f observedWorld=%.9f"), ExpectedAt,
		                                B.ResultServerSeconds, Now));
		Check(TEXT("terminal-population"), B.ActiveEnemyCount == (ActiveCase == 3 ? 100 : 0));
		Check(TEXT("committed-hit-count"),
		           HitCount == (ActiveCase == 3 ? 0 : (ActiveCase == 1 ? 1 : (ActiveCase == 2 ? 3 : 2))),
		           FString::FromInt(HitCount));
		if (ActiveCase == 0)
			Check(TEXT("hitch-crossed-deadline"), bHitch && HitchBefore < Deadline && Now > Deadline);
		if (ActiveCase == 2)
			Check(TEXT("victory-waited-for-normal"), bNormalWait && LastKillAt >= Deadline + 2);
		Check(TEXT("terminal-service-cleanup"), !M.IsLogicTimerActive() &&
		                                            M.GetCombatService()->GetRegisteredUnitCount() == 0 &&
		                                            M.GetCombatService()->GetRegisteredEnemyCount() == 0 &&
		                                            M.GetWaveDirector()->GetTrackedEnemyCount() == 0);
		for (int32 P = 0; P < 2; ++P)
		{
			const FLDEconomySnapshot E = M.GetEconomyService()->GetSnapshot(P);
			const int32 ExpectedGold = ActiveCase == 3 ? 80 : (ActiveCase == 1 ? 180 : (ActiveCase == 2 ? 281 : 280));
			Check(TEXT("exact-production-reward"),
			           E.Gold == ExpectedGold && E.Stars == (ActiveCase == 3 ? 0 : (ActiveCase == 1 ? 2 : 4)) &&
			               E.PaidSummonCount == 1,
			           FString::Printf(TEXT("player%d gold%d stars%d"), P, E.Gold, E.Stars));
		}
		for (auto P : Peers)
			if (P.IsValid())
			{
				P->FinalBattle = B;
				P->ActualBosses = LastBossActors;
				if (ActiveCase == 3)
				{
					ObserveActors();
					for (const auto& A : SeenActors)
						if (A.Value.EnemyTypeId == TEXT("N01") && A.Value.bAlive)
							P->FinalNormalIds.Add(A.Key);
					P->FinalNormalIds.Sort();
					Check(TEXT("actual-one-hundred-normal-actors"), P->FinalNormalIds.Num() == 100);
				}
				for (int32 Index = 0; Index < 2; ++Index)
				{
					P->FinalBoards.Add(M.GetBoardManager()->GetSnapshot(Index));
					P->FinalEconomies.Add(M.GetEconomyService()->GetSnapshot(Index));
				}
				P->bTerminalCaptured = true;
				P->ForceNetUpdate();
			}
	}
	bool BothDone = true;
	for (auto P : Peers)
		BothDone &= P.IsValid() && P->bTerminalObserved;
	if (BothDone)
		for (auto P : Peers)
			if (P.IsValid())
			{
				P->bMayReturn = true;
				P->ForceNetUpdate();
			}
}

void FLDG3BoundaryState::Record(ALDPlayerController& PC, ALDG3BoundaryPeer& Peer)
{
	const FLDBattleSnapshot B = World->GetGameState<ALDGameState>()->GetBattleSnapshot();
	Check(TEXT("replicated-terminal-battle-identical"), BattleKey(B) == BattleKey(Peer.FinalBattle), BattleKey(B));
	Check(TEXT("replicated-owner-board-identical"),
	           BoardKey(PC.GetBoardSnapshot()) == BoardKey(Peer.FinalBoards[LocalIndex]));
	Check(TEXT("replicated-owner-economy-identical"),
	           MoneyKey(PC.GetEconomySnapshot()) == MoneyKey(Peer.FinalEconomies[LocalIndex]));
	bool ActorMatch = B.Bosses.Num() == Peer.ActualBosses.Num();
	for (const auto& Boss : B.Bosses)
	{
		const FLDBossSnapshot* Actual =
		    Peer.ActualBosses.FindByPredicate([&](const auto& V) { return V.EnemyId == Boss.EnemyId; });
		const FLDEnemyCombatSnapshot* Seen = SeenActors.Find(Boss.EnemyId);
		ActorMatch &= Actual && Seen && Actual->HP == Boss.HP && Seen->HP == Boss.HP && Seen->bAlive == Boss.bAlive;
	}
	Check(TEXT("actual-replicated-actor-hp-agrees-with-battle"), ActorMatch);
	if (Peer.CaseIndex == 2)
		Check(TEXT("client-observed-normal-blocking-victory"), bLocalNormalWait && Peer.bNormalWaitObserved);
	Check(TEXT("local-fixture-ready-was-observed"), bReadyAck);
	Check(TEXT("same-paid-unit-and-board-revision"),
	           PC.GetBoardSnapshot().Units.Num() == 1 && PC.GetBoardSnapshot().Units[0].InstanceId == LocalUnitId &&
	               LocalUnitActor.IsValid() && LocalUnitActor->GetPlacement().InstanceId == LocalUnitId &&
	               PC.GetBoardSnapshot().BoardRevision == 2 && PC.GetBoardSnapshot().Population == 1);
	Check(TEXT("terminal-actions-closed"), !PC.CanUseGameplayActions());
	bool AllNormalsSeen = Peer.FinalNormalIds.Num() == (Peer.CaseIndex == 3 ? 100 : 0);
	TArray<TSharedPtr<FJsonValue>> NormalJson;
	for (uint64 EnemyId : Peer.FinalNormalIds)
	{
		const FLDEnemyCombatSnapshot* A = SeenActors.Find(EnemyId);
		AllNormalsSeen &= A && A->EnemyTypeId == TEXT("N01") && A->HP == 70 && A->bAlive;
		NormalJson.Add(MakeShared<FJsonValueString>(Id(EnemyId)));
	}
	Check(TEXT("actual-terminal-normal-identities-and-hp"), AllNormalsSeen);
	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	const TCHAR* Names[] = {TEXT("early-and-exact-victory"), TEXT("late-boss-timeout"), TEXT("normal-blocks-victory"),
	                                                                                         TEXT("enemy-cap")};
	J->SetStringField(TEXT("name"), Names[Peer.CaseIndex]);
	J->SetNumberField(TEXT("caseIndex"), Peer.CaseIndex);
	J->SetStringField(TEXT("matchId"), Match.ToString());
	J->SetStringField(TEXT("battle"), BattleKey(B));
	J->SetStringField(TEXT("ownerBoard"), BoardKey(PC.GetBoardSnapshot()));
	J->SetStringField(TEXT("ownerEconomy"), MoneyKey(PC.GetEconomySnapshot()));
	J->SetNumberField(TEXT("deadline"), Peer.Deadline);
	J->SetNumberField(TEXT("resultSeconds"), B.ResultServerSeconds);
	J->SetNumberField(TEXT("result"), int32(B.Result));
	J->SetNumberField(TEXT("reason"), int32(B.ResultReason));
	J->SetNumberField(TEXT("revision"), B.Revision);
	J->SetNumberField(TEXT("normalCount"), B.ActiveEnemyCount);
	TArray<TSharedPtr<FJsonValue>> DueJson, BossJson;
	for (double Due : Peer.DueTimes)
		DueJson.Add(MakeShared<FJsonValueNumber>(Due));
	for (const auto& Boss : Peer.ActualBosses)
	{
		TSharedPtr<FJsonObject> V = MakeShared<FJsonObject>();
		V->SetStringField(TEXT("enemyId"), Id(Boss.EnemyId));
		V->SetNumberField(TEXT("route"), Boss.RouteIndex);
		V->SetNumberField(TEXT("actualActorHP"), Boss.HP);
		BossJson.Add(MakeShared<FJsonValueObject>(V));
	}
	J->SetArrayField(TEXT("dueTimes"), DueJson);
	J->SetArrayField(TEXT("actualNormalIds"), NormalJson);
	J->SetStringField(TEXT("originalLocalUnitId"), Id(LocalUnitId));
	J->SetArrayField(TEXT("actualBosses"), BossJson);
	TArray<TSharedPtr<FJsonValue>> BoardJson, MoneyJson;
	for (const auto& V : Peer.FinalBoards)
		BoardJson.Add(MakeShared<FJsonValueString>(BoardKey(V)));
	for (const auto& V : Peer.FinalEconomies)
		MoneyJson.Add(MakeShared<FJsonValueString>(MoneyKey(V)));
	J->SetArrayField(TEXT("authorityBoards"), BoardJson);
	J->SetArrayField(TEXT("authorityEconomies"), MoneyJson);
	J->SetNumberField(TEXT("hitchBeforeWorldSeconds"), HitchBefore);
	J->SetNumberField(TEXT("hitchSleptWallSeconds"), HitchAfterWall);
	J->SetArrayField(TEXT("committedHitsOnAuthority"), Hits);
	Cases.Add(MakeShared<FJsonValueObject>(J));
	FScreenshotRequest::RequestScreenshot(Output / FString::Printf(TEXT("case%d-result.png"), Peer.CaseIndex), false,
	                                                               false);
	bTerminalRecorded = true;
	++Completed;
	Peer.ServerObserve(Match, 2, B.Revision, !bFailed);
	Write(false);
}

void FLDG3BoundaryState::Local(ALDPlayerController& PC, const FLDBattleSnapshot& B)
{
	LocalIndex = PC.GetLocalParticipantIndex();
	if (LocalIndex < 0 || LocalIndex > 1 || !PC.IsGameplaySnapshotReady())
		return;
	if (!LocalPeer.IsValid())
		for (TActorIterator<ALDG3BoundaryPeer> It(World.Get()); It; ++It)
			if (It->GetOwner() == &PC)
			{
				LocalPeer = *It;
				break;
			}
	ObserveActors();
	if (LocalUnitId != 0 && !bLocalActorCaptured)
		for (TActorIterator<ALDUnitActor> It(World.Get()); It; ++It)
			if (It->GetPlacement().InstanceId == LocalUnitId)
			{
				LocalUnitActor = *It;
				bLocalActorCaptured = true;
			}
	const double Now = FPlatformTime::Seconds();
	if (!B.IsTerminal() && !bPrepared && (!LocalPeer.IsValid() || !LocalPeer->bFixtureReady))
	{
		if (B.Phase != ELDMatchPhase::Preparing || !PC.IsLocalBoardReady() || PC.HasPendingCommand())
			return;
		const auto& Board = PC.GetBoardSnapshot();
		if (!bOldDone)
		{
			if (Role != TEXT("client") || !OldMatch.IsValid())
				bOldDone = true;
			else if (!bOldSent)
			{
				const auto* PS = PC.GetPlayerState<ALDPlayerState>();
				if (!PS || !PS->GetParticipantContext().IsValid() || !LocalPeer.IsValid())
					return;
				Check(TEXT("prior-payload-fresh-board-before-purchase"),
				           Board.Units.IsEmpty() && Board.BoardRevision == 0 && PC.GetEconomySnapshot().Gold == 100);
				Check(TEXT("actual-prior-generation-differs"),
				           OldSummon.ConnectionEpoch != PS->GetParticipantContext().ConnectionEpoch &&
				               OldMatch != Match,
				           FString::Printf(TEXT("priorMatch=%s priorEpoch=%llu newMatch=%s newEpoch=%llu id=%u expectedBoard=%d; previous payload deliberately sent on NEW Controller, not old Actor packet delivery"),
				               *OldMatch.ToString(), OldSummon.ConnectionEpoch, *Match.ToString(),
				               PS->GetParticipantContext().ConnectionEpoch, OldSummon.RequestId,
				               OldSummon.ExpectedBoardRevision));
				bOldSent = true;
				OldSentAt = Now;
				PC.ServerRequestCommand(OldSummon);
				return;
			}
			else if (!OldReply.IsEmpty())
			{
				if (!LocalPeer.IsValid())
					return;
				if (!bOldStateRequested)
				{
					bOldStateRequested = true;
					LocalPeer->ServerObserve(Match, 0, 0, true);
					return;
				}
				if (!LocalPeer->bPriorPayloadChecked)
					return;
				Check(TEXT("prior-payload-authority-board-economy-cache-rng-unchanged"),
				           LocalPeer->bPriorPayloadUnchanged);
				Check(TEXT("prior-payload-actual-rpc-invalid-epoch"),
				           OldReply.Contains(
				               FString::Printf(TEXT("code=%d "), int32(ELDCommandResultCode::InvalidEpoch))), OldReply);
				Check(TEXT("prior-payload-economy-board-unchanged"),
				           Board.Units.IsEmpty() && Board.BoardRevision == 0 && PC.GetEconomySnapshot().Gold == 100 &&
				               PC.GetEconomySnapshot().PaidSummonCount == 0);
				bOldDone = true;
			}
			else
			{
				if (Now - OldSentAt > 5)
					Check(TEXT("prior-payload-rpc-response-timeout"), false);
				return;
			}
		}
		if (Now - ActionAt < .3)
			return;
		FBox2D Rect;
		if (Board.Units.IsEmpty() && !bPurchaseSent && PC.GetActionScreenRect(ELDCommandType::Summon, Rect))
		{
			ActionAt = Now;
			bPurchaseSent = Click(Rect);
			Check(TEXT("paid-summon-slate-click"), bPurchaseSent);
			return;
		}
		if (Board.Units.Num() == 1 && bPurchaseSent && !bPurchaseCaptured)
		{
			const auto& R = PC.GetLastResult();
			Check(TEXT("paid-summon-actual-response"), R.ResultCode == ELDCommandResultCode::Success &&
			                                               R.CreatedInstanceIds.Num() == 1 &&
			                                               PC.GetEconomySnapshot().Gold == 80);
			LocalUnitId = Board.Units[0].InstanceId;
			OldSummon = {};
			OldSummon.ConnectionEpoch = R.ConnectionEpoch;
			OldSummon.RequestId = R.RequestId;
			OldSummon.CommandType = ELDCommandType::Summon;
			OldSummon.ExpectedBoardRevision = 0;
			OldMatch = Match;
			bPurchaseCaptured = true;
		}
		if (Board.Units.Num() == 1 && bPurchaseCaptured && !bMoveSent)
		{
			ActionAt = Now;
			bMoveSent = PC.RequestMove(Board.Units[0].InstanceId, LocalIndex == 0 ? 5 : 35);
			Check(TEXT("paid-unit-real-move-request"), bMoveSent);
			return;
		}
	}
	if (!LocalPeer.IsValid())
		return;
	ALDG3BoundaryPeer& Peer = *LocalPeer;
	if (Peer.bFixtureReady && !bReadyAck && B.Phase == ELDMatchPhase::Running)
	{
		bool Ready = Peer.CaseIndex == 3 ? B.ActiveEnemyCount == 99 && SeenActors.Num() == 99 : B.Bosses.Num() == 2;
		if (Peer.CaseIndex != 3)
			for (const auto& Boss : B.Bosses)
			{
				const auto* A = SeenActors.Find(Boss.EnemyId);
				Ready &= A && A->HP == 1 && Boss.HP == 1 && Boss.MaxHP == 6000;
			}
		if (Ready && FixtureViewAt == 0)
			FixtureViewAt = Now;
		if (Ready && Now - FixtureViewAt >= .25)
		{
			bReadyAck = true;
			Check(TEXT("actual-fixture-actors-replicated"), true, BattleKey(B));
			Peer.ServerObserve(Match, 1, B.Revision, true);
			FScreenshotRequest::RequestScreenshot(Output / FString::Printf(TEXT("case%d-ready.png"), Peer.CaseIndex),
			                                                               false, false);
		}
	}
	if (Peer.CaseIndex == 2 && !bLocalNormalWait && Peer.bNormalWaitObserved && B.Phase == ELDMatchPhase::Running &&
	    B.ActiveEnemyCount == 1 && B.Bosses.Num() == 2 && !B.Bosses[0].bAlive && !B.Bosses[1].bAlive)
	{
		bLocalNormalWait = true;
		Check(TEXT("replicated-normal-one-past-boss-deadline"),
		           World->GetGameState<ALDGameState>()->GetServerWorldTimeSeconds() > Peer.Deadline);
		FScreenshotRequest::RequestScreenshot(Output / TEXT("case2-normal-wait.png"), false, false);
	}
	if (!B.IsTerminal())
		return;
	if (TerminalAt == 0)
		TerminalAt = Now;
	if (!Peer.bTerminalCaptured || Peer.FinalBoards.Num() != 2 || Peer.FinalEconomies.Num() != 2 ||
	    Now - TerminalAt < .6)
		return;
	if (!bTerminalRecorded)
		Record(PC, Peer);
	if (!Peer.bMayReturn)
		return;
	if (!bTerminalFrozenChecked)
	{
		bTerminalFrozenChecked = true;
		Check(TEXT("terminal-snapshot-remains-fixed"), BattleKey(B) == BattleKey(Peer.FinalBattle));
	}
	if (Completed == 4)
	{
		if (FinishAt == 0)
			FinishAt = Now + (Role == TEXT("host") ? 5 : 1);
		return;
	}
	if (!bReturning && Now - TerminalAt > (Role == TEXT("host") ? 2.5 : 1.0))
	{
		FBox2D Rect;
		if (PC.GetReturnButtonScreenRect(Rect))
		{
			bReturning = Click(Rect);
			Check(TEXT("result-return-slate-click"), bReturning);
			if (bReturning)
			{
				++Returns;
				bEntryClicked = false;
				EntryAt = 0;
			}
		}
	}
}

void FLDG3BoundaryState::Tick()
{
	UWorld* W = Owner.IsValid() ? Owner->GetWorld() : nullptr;
	if (!W || !W->IsGameWorld() || !W->HasBegunPlay() || bFinished)
		return;
	World = W;
	const double Now = FPlatformTime::Seconds();
	if (Now - Started > Timeout)
		Check(TEXT("suite-timeout"), false);
	if (bFailed || (FinishAt > 0 && Now >= FinishAt))
	{
		Finish();
		return;
	}
	if (Now - LastProgress > 5)
		Write(false);
	ALDGameState* GS = W->GetGameState<ALDGameState>();
	if (!GS)
	{
		if (EntryAt == 0)
			EntryAt = Now;
		ALDEntryPlayerController* PC =
		    Cast<ALDEntryPlayerController>(Owner->GetGameInstance()->GetFirstLocalPlayerController());
		if (!PC || bEntryClicked)
			return;
		const bool Host = Role == TEXT("host");
		const double Delay = Completed == 0 ? (Host ? 35.0 : 40.0) : (Host ? 2.0 : 7.0);
		if (!Host)
			PC->SetJoinAddressText(Address);
		FBox2D Rect;
		if (Now - EntryAt >= Delay && PC->GetEntryActionScreenRect(Host, Rect))
		{
			if (Completed == 0)
				Check(TEXT("entry-idle-more-than-loading-timeout"), Now - EntryAt >= 35 && !PC->IsEntryBusy(),
				           FString::SanitizeFloat(Now - EntryAt));
			bEntryClicked = Click(Rect);
			Check(TEXT("entry-slate-action"), bEntryClicked);
		}
		return;
	}
	const FLDBattleSnapshot B = GS->GetBattleSnapshot();
	if (!B.MatchId.IsValid())
		return;
	if (B.MatchId != Match)
		Begin(B);
	if (ALDGameMode* M = W->GetAuthGameMode<ALDGameMode>())
		if (M->GetBoardManager() && M->GetEconomyService() && M->GetCommandProcessor())
			Authority(*M);
	if (ALDPlayerController* PC = Cast<ALDPlayerController>(Owner->GetGameInstance()->GetFirstLocalPlayerController()))
		Local(*PC, GS->GetBattleSnapshot());
}

bool ULDG3BoundaryProbeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return IsBoundaryProcess() && Super::ShouldCreateSubsystem(Outer);
}
void ULDG3BoundaryProbeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	State = MakeShared<FLDG3BoundaryState>();
	State->Owner = this;
	State->Started = FPlatformTime::Seconds();
	FParse::Value(FCommandLine::Get(), TEXT("P0Role="), State->Role);
	FParse::Value(FCommandLine::Get(), TEXT("P0PeerAddress="), State->Address);
	FParse::Value(FCommandLine::Get(), TEXT("P0ProbeOutput="), State->Output);
	FParse::Value(FCommandLine::Get(), TEXT("P0TimeoutSeconds="), State->Timeout);
	State->Check(TEXT("explicit-development-arguments"),
	                  !State->Output.IsEmpty() && !FPaths::IsRelative(State->Output) &&
	                      (State->Role == TEXT("host") || State->Role == TEXT("client")) &&
	                                           FParse::Param(FCommandLine::Get(), TEXT("P0CommandTrace")));
	if (State->bFailed)
	{
		State->bFinished = true;
		FPlatformMisc::RequestExit(false);
		return;
	}
	State->Output = FPaths::ConvertRelativePathToFull(State->Output);
	State->Check(
	    TEXT("new-evidence-output"),
	         !IFileManager::Get().FileExists(*(State->Output / TEXT("result.json"))) &&
	                                         !IFileManager::Get().FileExists(*(State->Output / TEXT("progress.json"))));
	if (State->bFailed)
	{
		State->bFinished = true;
		FPlatformMisc::RequestExit(false);
		return;
	}
	State->bWritableOutput = IFileManager::Get().MakeDirectory(*State->Output, true);
	State->Check(TEXT("evidence-directory-created"), State->bWritableOutput);
	State->Wire = MakeUnique<FBoundaryWireObserver>();
	TWeakPtr<FLDG3BoundaryState> Weak = State;
	State->Wire->Observe = [Weak](const FString& Line)
	{
		if (auto S = Weak.Pin(); S && S->bOldSent && !S->bOldDone)
		{
			const FString Prefix =
			    FString::Printf(TEXT("P0WIRE CLIENT match=%s epoch=%llu id=%u "), *S->Match.ToString(),
			                         S->OldSummon.ConnectionEpoch, S->OldSummon.RequestId);
			if (Line.Contains(Prefix))
				S->OldReply = Line;
		}
	};
	GLog->AddOutputDevice(State->Wire.Get());
}
void ULDG3BoundaryProbeSubsystem::Deinitialize()
{
	if (State.IsValid())
	{
		if (State->Wire)
			GLog->RemoveOutputDevice(State->Wire.Get());
		State->Release();
		State.Reset();
	}
	Super::Deinitialize();
}
void ULDG3BoundaryProbeSubsystem::Tick(float DeltaTime)
{
	if (State.IsValid())
		State->Tick();
}
bool ULDG3BoundaryProbeSubsystem::IsTickable() const
{
	return !IsTemplate() && State.IsValid() && !State->bFinished;
}
TStatId ULDG3BoundaryProbeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULDG3BoundaryProbeSubsystem, STATGROUP_Tickables);
}
UWorld* ULDG3BoundaryProbeSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}
