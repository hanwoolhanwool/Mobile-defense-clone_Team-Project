#include "Verification/LDG3LoadProbeSubsystem.h"

#include "Battle/LDCombatService.h"
#include "Battle/LDEnemyActor.h"
#include "Battle/LDUnitActor.h"
#include "Board/LDBoardManager.h"
#include "Core/LDGameMode.h"
#include "Core/LDGameState.h"
#include "Core/LDPlayerController.h"
#include "Core/LDPlayerState.h"
#include "Economy/LDEconomyService.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "Network/LDCommandProcessor.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDG3Load, Log, All);
CSV_DEFINE_CATEGORY(LDG3Load, true);

namespace
{
	constexpr int32 BatchSize = 80;
	constexpr int32 BatchCount = 25;
	constexpr double SustainedHP = 100000000;

	// The same canonical closed route and direction, with an explicit fixture phase offset.
	TArray<FVector> PhaseShiftedRoute(const TArray<FVector>& Points, double Distance)
	{
		for (int32 Segment = 0; Segment < Points.Num(); ++Segment)
		{
			const double Length = FVector::Distance(Points[Segment], Points[(Segment + 1) % Points.Num()]);
			if (Distance < Length)
			{
				const FVector Start =
				    FMath::Lerp(Points[Segment], Points[(Segment + 1) % Points.Num()], Distance / Length);
				TArray<FVector> Result = {Start};
				for (int32 Offset = 1; Offset <= Points.Num(); ++Offset)
				{
					const FVector Point = Points[(Segment + Offset) % Points.Num()];
					if (!Point.Equals(Start, .001))
					{
						Result.Add(Point);
					}
				}
				return Result;
			}
			Distance -= Length;
		}
		return Points;
	}

	float Percentile95(TArray<float> Values)
	{
		if (Values.IsEmpty())
		{
			return 0;
		}
		Values.Sort();
		return Values[FMath::FloorToInt((Values.Num() - 1) * .95)];
	}
} // namespace

ALDG3LoadProbeState::ALDG3LoadProbeState()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(10);
}

void ALDG3LoadProbeState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDG3LoadProbeState, Phase);
	DOREPLIFETIME(ALDG3LoadProbeState, Batch);
	DOREPLIFETIME(ALDG3LoadProbeState, BatchFirstEnemyId);
	DOREPLIFETIME(ALDG3LoadProbeState, SustainStartServerSeconds);
	DOREPLIFETIME(ALDG3LoadProbeState, SustainDurationSeconds);
	DOREPLIFETIME(ALDG3LoadProbeState, bServerPassed);
}

void ALDG3LoadProbeState::ServerReportObservation_Implementation(int32 ObservedPhase, int32 ObservedBatch, bool bPass)
{
#if !UE_BUILD_SHIPPING
	if (ObservedPhase != Phase || (Phase == 2 && ObservedBatch != Batch))
	{
		return;
	}
	bClientPassed &= bPass;
	if (Phase == 1)
	{
		bClientSustainReady = bPass;
	}
	else if (Phase == 2 && bPass)
	{
		ClientObservedBatch = ObservedBatch;
	}
	else if (Phase == 5)
	{
		bClientFinished = true;
	}
#endif
}

bool ULDG3LoadProbeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const UWorld* World = Cast<UWorld>(Outer);
	FString Probe;
	return World && World->IsGameWorld() &&
	       FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe) &&
	                     Probe.Equals(TEXT("G3Load"), ESearchCase::IgnoreCase) && Super::ShouldCreateSubsystem(Outer);
#endif
}

void ULDG3LoadProbeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CreatedAt = FPlatformTime::Seconds();
	LastTickAt = CreatedAt;
	LastSampleAt = CreatedAt;
	FParse::Value(FCommandLine::Get(), TEXT("P0LoadSeconds="), LoadSeconds);
	LoadSeconds = FMath::Clamp(LoadSeconds, 10.0, 7200.0);
	FParse::Value(FCommandLine::Get(), TEXT("P0ProbeOutput="), OutputDirectory);
	if (OutputDirectory.IsEmpty())
	{
		OutputDirectory = FPaths::ProjectSavedDir() / TEXT("P0Runs/G3Load") / FGuid::NewGuid().ToString();
	}
	OutputDirectory = FPaths::ConvertRelativePathToFull(OutputDirectory);
	BaselineRss = FPlatformMemory::GetStats().UsedPhysical;
}

void ULDG3LoadProbeSubsystem::EnsureOutputDirectory()
{
	if (bOutputReady)
	{
		return;
	}
	// Transitional worlds also create subsystems. Only a world that produces evidence claims the output path.
	if (FPaths::FileExists(OutputDirectory / TEXT("result.json")) ||
	                       FPaths::FileExists(OutputDirectory / TEXT("samples.csv")))
	{
		OutputDirectory /= FGuid::NewGuid().ToString();
	}
	IFileManager::Get().MakeDirectory(*OutputDirectory, true);
	const FString Header = TEXT("wallSeconds,worldSeconds,phase,batch,label,frames,frameMsP95,processCpuPercent,processCpuOneCorePercent,rssBytes,peakRssBytes,unitActors,enemyActors,aliveEnemies,registeredUnits,registeredEnemies,damageEvents,uniqueDeaths,routeDistanceSumCm\n");
	FFileHelper::SaveStringToFile(Header, *(OutputDirectory / TEXT("samples.csv")),
	                                        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	bOutputReady = true;
}

TStatId ULDG3LoadProbeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULDG3LoadProbeSubsystem, STATGROUP_Tickables);
}

void ULDG3LoadProbeSubsystem::Check(const FString& Name, bool bPass, const FString& Detail)
{
	bFailed |= !bPass;
	TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
	Entry->SetStringField(TEXT("name"), Name);
	Entry->SetBoolField(TEXT("pass"), bPass);
	Entry->SetStringField(TEXT("detail"), Detail);
	Entry->SetNumberField(TEXT("worldSeconds"), GetWorld()->GetTimeSeconds());
	Checks.Add(MakeShared<FJsonValueObject>(Entry));
	UE_LOG(LogLDG3Load, Display, TEXT("%s %s %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), *Name, *Detail);
}

bool ULDG3LoadProbeSubsystem::PrepareUnits(ALDGameMode& Mode)
{
	TArray<FName> UnitTypes;
	for (const auto& Pair : Mode.GetGameData()->GetUnits())
	{
		if (Pair.Value.bEnabledInP0)
		{
			UnitTypes.Add(Pair.Key);
		}
	}
	UnitTypes.Sort([](FName A, FName B) { return A.LexicalLess(B); });
	if (UnitTypes.Num() != 16)
	{
		return false;
	}
	// Authored workload layout: 175cm melee rows must face the central route. Alphabetic placement
	// would put E01 in the middle row, 280cm from every path, silently reducing the active workload.
	TArray<FName> Melee;
	TArray<FName> Ranged;
	for (FName Type : UnitTypes)
	{
		FLDUnitRow Row;
		Mode.GetGameData()->TryGetUnitRow(Type, Row);
		(Row.RangeCm < Mode.GetGameData()->GetRules().CellSizeCm * 2 ? Melee : Ranged).Add(Type);
	}
	UnitTypes.Reset();
	int32 MeleeIndex = 0;
	int32 RangedIndex = 0;
	for (int32 Index = 0; Index < 16; ++Index)
	{
		UnitTypes.Add(Index % 3 == 0 && Melee.IsValidIndex(MeleeIndex) ? Melee[MeleeIndex++] : Ranged[RangedIndex++]);
	}
	for (int32 Player = 0; Player < 2; ++Player)
	{
		if (!Participants[Player].IsValid() || Mode.GetBoardManager()->GetSnapshot(Player).Population != 0)
		{
			return false;
		}
		for (int32 Index = 0; Index < 20; ++Index)
		{
			FLDCommand Command;
			Command.ConnectionEpoch = Participants[Player].ConnectionEpoch;
			Command.RequestId = Index + 1;
			Command.ExpectedBoardRevision = Mode.GetBoardManager()->GetSnapshot(Player).BoardRevision;
			FLDBoardPlan Plan;
			const FName Type = Index < 16 ? UnitTypes[Index] : FName(*FString::Printf(TEXT("C%02d"), Index - 15));
			if (Mode.GetBoardManager()->TryPrepare(Participants[Player], Command, Type, GetWorld()->GetTimeSeconds(),
			                                       Plan) != ELDCommandResultCode::Success ||
			    !Mode.GetBoardManager()->ValidatePrepared(Plan))
			{
				Mode.GetBoardManager()->CancelPrepared(Plan);
				return false;
			}
			Mode.GetBoardManager()->CommitPrepared(Plan);
			Mode.GetBoardManager()->PublishPrepared(Plan);
		}
		for (const FLDPlacedUnit& Unit : Mode.GetBoardManager()->GetSnapshot(Player).Units)
		{
			ALDUnitActor* Actor = nullptr;
			if (!Mode.GetBoardManager()->TryGetCommittedUnitActor(Unit.InstanceId, Actor) || !Actor)
			{
				return false;
			}
			AllUnits.Add(Actor);
			AuthoredUnitIds.Add(Unit.InstanceId);
		}
	}
	return Mode.GetCombatService()->GetRegisteredUnitCount() == 40;
}

ALDEnemyActor* ULDG3LoadProbeSubsystem::SpawnEnemy(ALDGameMode& Mode, bool bBoss, double HP, int32 Ordinal)
{
	FLDEnemyRow Row;
	if (!Mode.GetGameData()->TryGetEnemyRow(bBoss ? TEXT("B01") : TEXT("N01"), Row))
	{
		return nullptr;
	}
	const FLDGameRules& Rules = Mode.GetGameData()->GetRules();
	const int32 Route = Ordinal % 2;
	const double Offset = Rules.LengthPerGateCm * ((Ordinal / 2) % 51) / 51.0;
	const TArray<FVector> Points = PhaseShiftedRoute(Rules.PointsByGateCm[Route], Offset);
	ALDEnemyActor* Enemy = GetWorld()->SpawnActor<ALDEnemyActor>();
	const uint64 ID = NextEnemyId++;
	const double Now = GetWorld()->GetTimeSeconds();
	if (!Enemy || !Enemy->InitializeRoute(Participants[0].MatchId, ID, Route, Points, Row.SpeedCmPerSec, Now) ||
	    !Enemy->InitializeCombat(Row, HP, ID, bBoss ? 10 : 1, Now) || !Mode.GetCombatService()->RegisterEnemy(*Enemy))
	{
		if (Enemy)
		{
			Enemy->Destroy();
		}
		return nullptr;
	}
	Enemies.Add(Enemy);
	AllEnemies.Add(Enemy);
	if (bBoss)
	{
		BossActors.Add(Enemy);
	}
	return Enemy;
}

bool ULDG3LoadProbeSubsystem::PrepareLoad(ALDGameMode& Mode)
{
	ALDPlayerController* Remote = nullptr;
	for (TActorIterator<ALDPlayerController> It(GetWorld()); It; ++It)
	{
		const ALDPlayerState* Player = It->GetPlayerState<ALDPlayerState>();
		if (Player && Player->GetParticipantContext().IsValid())
		{
			Participants[Player->GetPlayerIndex()] = Player->GetParticipantContext();
			if (Player->GetPlayerIndex() == 1)
			{
				Remote = *It;
			}
		}
	}
	if (!Remote || Mode.GetWaveDirector())
	{
		return false;
	}
	State = GetWorld()->SpawnActor<ALDG3LoadProbeState>();
	State->SetOwner(Remote);
	State->SustainDurationSeconds = LoadSeconds;
	PhaseStartedAt = FPlatformTime::Seconds();
	Sample(TEXT("before-fixture"), PhaseStartedAt);
	if (!PrepareUnits(Mode))
	{
		return false;
	}
	// Authoring is complete. Keep the product clock running but reject external gameplay mutations.
	Mode.GetCommandProcessor()->SetAcceptingCommands(false);
	DamageHandle = Mode.GetCombatService()->OnDamageCommitted.AddUObject(this, &ULDG3LoadProbeSubsystem::OnDamage);
	DeathHandle = Mode.GetCombatService()->OnEnemyDeathCommitted.AddUObject(this, &ULDG3LoadProbeSubsystem::OnDeath);
	for (int32 Index = 0; Index < 101; ++Index)
	{
		if (!SpawnEnemy(Mode, Index >= 99, SustainedHP, Index))
		{
			return false;
		}
	}
	Check(TEXT("server-representative-population"), Mode.GetCombatService()->GetRegisteredUnitCount() == 40 &&
	                                                    Mode.GetCombatService()->GetRegisteredEnemyCount() == 101 &&
	                                                    Mode.GetCombatService()->GetLivingEnemyCount() == 101);
	ALDGameState* GameState = GetWorld()->GetGameState<ALDGameState>();
	FLDBattleSnapshot Battle = GameState->GetBattleSnapshot();
	Battle.WaveIndex = 10;
	Battle.ActiveEnemyCount = 99;
	Battle.bFinalSpawnsComplete = true;
	Battle.BossDeadlineServerSeconds = GetWorld()->GetTimeSeconds() + LoadSeconds + 120;
	Battle.Bosses.Reset();
	for (const TWeakObjectPtr<ALDEnemyActor>& Weak : BossActors)
	{
		const ALDEnemyActor* Enemy = Weak.Get();
		FLDBossSnapshot Boss;
		Boss.EnemyId = Enemy->GetRouteSnapshot().EnemyId;
		Boss.RouteIndex = Enemy->GetRouteSnapshot().RouteIndex;
		Boss.HP = Enemy->GetCombatSnapshot().HP;
		Boss.MaxHP = Enemy->GetCombatSnapshot().MaxHP;
		Boss.bAlive = true;
		Battle.Bosses.Add(Boss);
	}
	Check(TEXT("fixture-battle-HUD-state-authored"), GameState->UpdateBattle(Battle));
	State->ForceNetUpdate();
	return true;
}

void ULDG3LoadProbeSubsystem::OnDamage(const FLDDamageEvent& Event, int32 PlayerIndex, int32 EffectiveDamage)
{
	++DamageEvents;
	AttackedEnemyIds.Add(Event.EnemyId);
	if (ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>())
	{
		FLDPlacedUnit Unit;
		if (Mode->GetBoardManager()->TryGetUnit(Event.SourceInstanceId, Unit))
		{
			AttackingUnitTypes.Add(Unit.UnitId);
		}
		if (State.IsValid() && State->Phase == 1)
		{
			for (const TWeakObjectPtr<ALDEnemyActor>& Weak : BossActors)
			{
				const ALDEnemyActor* Boss = Weak.Get();
				if (Boss && Boss->GetRouteSnapshot().EnemyId == Event.EnemyId)
				{
					ALDGameState* GameState = GetWorld()->GetGameState<ALDGameState>();
					FLDBattleSnapshot Battle = GameState->GetBattleSnapshot();
					for (FLDBossSnapshot& BossView : Battle.Bosses)
					{
						if (BossView.EnemyId == Event.EnemyId)
						{
							BossView.HP = Boss->GetCombatSnapshot().HP;
						}
					}
					GameState->UpdateBattle(Battle);
				}
			}
		}
	}
}

void ULDG3LoadProbeSubsystem::OnDeath(const FLDCombatDeath& Death)
{
	if (!State.IsValid() || State->Phase != 2)
	{
		Check(TEXT("unexpected-death-outside-lifetime-batch"), false);
		return;
	}
	if (DeathIds.Contains(Death.DeathEventId))
	{
		Check(TEXT("combat-publishes-death-once"), false);
		return;
	}
	DeathIds.Add(Death.DeathEventId);
	++NaturalDeaths;
	if (ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>())
	{
		// Original publication already reaches GameMode. Two extra copies exercise reward deduplication.
		Mode->GetCommandProcessor()->EnqueueCombatReward(Death);
		Mode->GetCommandProcessor()->EnqueueCombatReward(Death);
	}
}

void ULDG3LoadProbeSubsystem::DestroyEnemies(ALDGameMode& Mode)
{
	for (const TWeakObjectPtr<ALDEnemyActor>& Weak : Enemies)
	{
		if (ALDEnemyActor* Enemy = Weak.Get())
		{
			Mode.GetCombatService()->UnregisterEnemy(Enemy->GetRouteSnapshot().EnemyId);
			Enemy->Destroy();
		}
	}
	GEngine->ForceGarbageCollection(true);
	PhaseStartedAt = FPlatformTime::Seconds();
}

int32 ULDG3LoadProbeSubsystem::CountUncollected(const TArray<TWeakObjectPtr<ALDEnemyActor>>& Actors) const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ALDEnemyActor>& Actor : Actors)
	{
		// Thread-safe serial/index check deliberately ignores the garbage flag: Destroy alone is insufficient.
		Count += !Actor.IsStale(false, true);
	}
	return Count;
}

void ULDG3LoadProbeSubsystem::BeginBatch(ALDGameMode& Mode)
{
	Enemies.Reset();
	++State->Batch;
	State->BatchFirstEnemyId = NextEnemyId;
	State->Phase = 2;
	PhaseStartedAt = FPlatformTime::Seconds();
	for (int32 Index = 0; Index < BatchSize; ++Index)
	{
		if (!SpawnEnemy(Mode, false, 1, Index))
		{
			FailAndExit(TEXT("batch-spawn-failed"));
			return;
		}
	}
	State->ForceNetUpdate();
	Sample(FString::Printf(TEXT("batch-%d-spawn"), State->Batch + 1), PhaseStartedAt);
}

void ULDG3LoadProbeSubsystem::BeginStop(ALDGameMode& Mode)
{
	// Explicit fixture teardown via normal board plans: remove the authored free units before services close.
	for (uint64 InstanceId : AuthoredUnitIds)
	{
		FLDPlacedUnit Unit;
		if (!Mode.GetBoardManager()->TryGetUnit(InstanceId, Unit))
		{
			FailAndExit(TEXT("authored-unit-missing-before-teardown"));
			return;
		}
		const int32 Player = Unit.PlayerIndex;
		const FLDBoardSnapshot Before = Mode.GetBoardManager()->GetSnapshot(Player);
		FLDCommand Command;
		Command.CommandType = ELDCommandType::Sell;
		Command.InstanceId = InstanceId;
		Command.ExpectedBoardRevision = Before.BoardRevision;
		FLDBoardPlan Plan;
		if (Mode.GetBoardManager()->TryPrepare(Participants[Player], Command, NAME_None, GetWorld()->GetTimeSeconds(),
		                                       Plan) != ELDCommandResultCode::Success ||
		    !Mode.GetBoardManager()->ValidatePrepared(Plan))
		{
			Mode.GetBoardManager()->CancelPrepared(Plan);
			FailAndExit(TEXT("fixture-unit-teardown-failed"));
			return;
		}
		Mode.GetBoardManager()->CommitPrepared(Plan);
		Mode.GetBoardManager()->PublishPrepared(Plan);
	}
	Check(TEXT("fixture-only-teardown-leaves-empty-boards"),
	           Mode.GetBoardManager()->GetSnapshot(0).Population == 0 &&
	               Mode.GetBoardManager()->GetSnapshot(1).Population == 0);
	DamageEventsAtStop = DamageEvents;
	bExpectedTerminal = true;
	Mode.AbortMatch(TEXT("G3Load fixture complete: explicit Stop/GC lifetime verification"));
	Check(TEXT("stop-turns-off-logic-timer"), !Mode.IsLogicTimerActive());
	Check(TEXT("stop-clears-combat-registration"), Mode.GetCombatService()->GetRegisteredUnitCount() == 0 &&
	                                                   Mode.GetCombatService()->GetRegisteredEnemyCount() == 0);
	Check(TEXT("stop-clears-combat-subscriptions"), !Mode.GetCombatService()->OnDamageCommitted.IsBound() &&
	                                                    !Mode.GetCombatService()->OnEnemyDeathCommitted.IsBound());
	Check(TEXT("stop-clears-clock-subscriptions"),
	           !Mode.GetCommandProcessor()->BeforeExternalCommand.IsBound() &&
	               !Mode.GetCommandProcessor()->AfterExternalCommandClock.IsBound());
	State->Phase = 4;
	State->ForceNetUpdate();
	PhaseStartedAt = FPlatformTime::Seconds();
	GEngine->ForceGarbageCollection(true);
}

void ULDG3LoadProbeSubsystem::TickAuthority(ALDGameMode& Mode, double Now)
{
	ALDG3LoadProbeState& Probe = *State.Get();
	if (!Probe.bClientPassed)
	{
		FailAndExit(TEXT("client-observation-failed"));
		return;
	}
	if (Probe.Phase == 1)
	{
		if (SustainStartedAt == 0)
		{
			if (Probe.bClientSustainReady)
			{
				SustainStartedAt = Now;
				Probe.SustainStartServerSeconds = GetWorld()->GetTimeSeconds();
				Probe.ForceNetUpdate();
				Sample(TEXT("sustain-start"), Now);
			}
			else if (Now - PhaseStartedAt > 60)
			{
				FailAndExit(TEXT("client-representative-load-not-observed"));
			}
			return;
		}
		// Keep the replicated workload alive beyond the exact target so both processes measure the full duration.
		if (Now - SustainStartedAt < LoadSeconds + 2)
		{
			return;
		}
		Check(TEXT("sustained-actual-attacks"), DamageEvents > 0 && AttackingUnitTypes.Num() == 16,
		           FString::Printf(TEXT("events=%llu types=%d"), DamageEvents, AttackingUnitTypes.Num()));
		Check(TEXT("sustained-all-actors-survive"), Mode.GetCombatService()->GetRegisteredUnitCount() == 40 &&
		                                                Mode.GetCombatService()->GetLivingEnemyCount() == 101);
		for (int32 Player = 0; Player < 2; ++Player)
		{
			const FLDEconomySnapshot Economy = Mode.GetEconomyService()->GetSnapshot(Player);
			BaselineGold[Player] = Economy.Gold;
			BaselineStars[Player] = Economy.Stars;
			BaselineEconomyRevision[Player] = Economy.EconomyRevision;
			BaselineRng[Player] = Mode.GetEconomyService()->GetRandomState(Player);
		}
		Sample(TEXT("sustain-end"), Now);
		EndProfileCapture();
		DestroyEnemies(Mode);
		Probe.Phase = 3;
		Probe.ForceNetUpdate();
		return;
	}
	if (Probe.Phase == 2)
	{
		int32 Living = Mode.GetCombatService()->GetLivingEnemyCount();
		if (Living > 0 && Now - PhaseStartedAt > 20)
		{
			// Bounded fallback is clearly counted separately from normal service attacks and balance results.
			for (const TWeakObjectPtr<ALDEnemyActor>& Weak : Enemies)
			{
				ALDEnemyActor* Enemy = Weak.Get();
				if (!Enemy || !Enemy->IsCombatAlive())
				{
					continue;
				}
				FLDDamageEvent Damage;
				Damage.MatchId = Participants[0].MatchId;
				Damage.DamageEventId = FixtureDamageId++;
				Damage.SourceInstanceId = AllUnits[0]->GetPlacement().InstanceId;
				Damage.EnemyId = Enemy->GetRouteSnapshot().EnemyId;
				Damage.Amount = 1000000;
				Damage.AttackServerSeconds = GetWorld()->GetTimeSeconds();
				FLDCombatDeath Death;
				if (Enemy->TryApplyDamage(Damage, Death) == ELDDamageResult::Killed)
				{
					Mode.GetCombatService()->OnEnemyDeathCommitted.Broadcast(Death);
					--NaturalDeaths;
					++FixtureDeaths;
				}
			}
			Mode.GetCommandProcessor()->DrainCombatRewards();
			Living = Mode.GetCombatService()->GetLivingEnemyCount();
		}
		if (Living == 0 && Now - PhaseStartedAt >= 3 && Probe.ClientObservedBatch == Probe.Batch)
		{
			Mode.GetCommandProcessor()->DrainCombatRewards();
			const int32 ExpectedDeaths = (Probe.Batch + 1) * BatchSize;
			bool bEconomyCorrect = true;
			for (int32 Player = 0; Player < 2; ++Player)
			{
				const FLDEconomySnapshot Economy = Mode.GetEconomyService()->GetSnapshot(Player);
				bEconomyCorrect &= Economy.Gold == BaselineGold[Player] + ExpectedDeaths &&
				                   Economy.Stars == BaselineStars[Player] && Economy.PaidSummonCount == 0 &&
				                   Economy.EconomyRevision == BaselineEconomyRevision[Player] + ExpectedDeaths &&
				                   Mode.GetEconomyService()->GetRandomState(Player) == BaselineRng[Player];
			}
			Check(FString::Printf(TEXT("batch%d-death-and-reward-once"), Probe.Batch + 1),
			                      DeathIds.Num() == ExpectedDeaths && bEconomyCorrect);
			++CompletedBatches;
			DestroyEnemies(Mode);
			Probe.Phase = 3;
			Probe.ForceNetUpdate();
		}
		else if (Now - PhaseStartedAt > 60)
		{
			FailAndExit(TEXT("batch-death-or-client-observation-timeout"));
		}
		return;
	}
	if (Probe.Phase == 3 && Now - PhaseStartedAt >= 1)
	{
		if (CountUncollected(Enemies) > 0)
		{
			if (Now - PhaseStartedAt > 10)
			{
				FailAndExit(TEXT("destroyed-enemies-remain-after-full-gc"));
			}
			return;
		}
		Check(FString::Printf(TEXT("batch%d-registry-and-collected-weak-zero"), Probe.Batch + 1),
		                      Mode.GetCombatService()->GetRegisteredEnemyCount() == 0 &&
		                          CountUncollected(Enemies) == 0);
		Sample(FString::Printf(TEXT("batch-%d-after-gc"), Probe.Batch + 1), Now);
		if (CompletedBatches == BatchCount)
		{
			BeginStop(Mode);
		}
		else
		{
			BeginBatch(Mode);
		}
		return;
	}
	if (Probe.Phase == 4 && Now - PhaseStartedAt >= 2)
	{
		if (!PollProfileWrite())
		{
			if (Now - PhaseStartedAt > 120)
			{
				FailAndExit(TEXT("server-profile-async-write-timeout"));
			}
			return;
		}
		int32 UncollectedUnits = 0;
		for (const TWeakObjectPtr<ALDUnitActor>& Unit : AllUnits)
		{
			UncollectedUnits += !Unit.IsStale(false, true);
		}
		if ((UncollectedUnits > 0 || CountUncollected(AllEnemies) > 0) && Now - PhaseStartedAt < 10)
		{
			return;
		}
		Check(TEXT("all-2141-fixture-actors-garbage-collected"), AllUnits.Num() == 40 && AllEnemies.Num() == 2101 &&
		                                                             UncollectedUnits == 0 &&
		                                                             CountUncollected(AllEnemies) == 0);
		Check(TEXT("2000-deaths-and-25-batches"), DeathIds.Num() == 2000 && CompletedBatches == 25);
		Check(TEXT("no-callback-after-stop"), DamageEvents == DamageEventsAtStop && !Mode.IsLogicTimerActive());
		Sample(TEXT("after-stop-and-gc"), Now);
		WriteResult(false);
		Probe.bServerPassed = !bFailed;
		Probe.Phase = 5;
		Probe.ForceNetUpdate();
		PhaseStartedAt = Now;
		return;
	}
	if (Probe.Phase == 5)
	{
		if (Probe.bClientFinished)
		{
			Check(TEXT("client-result-written-and-acknowledged"), Probe.bClientPassed);
			Probe.Phase = 6;
			WriteResult(true);
			Probe.bServerPassed = !bFailed;
			Probe.ForceNetUpdate();
			ExitAt = Now + 5;
		}
		else if (Now - PhaseStartedAt > 30)
		{
			FailAndExit(TEXT("client-completion-handshake-timeout"));
		}
	}
}

void ULDG3LoadProbeSubsystem::TickLocal(ALDPlayerController& Controller, double Now)
{
	ALDG3LoadProbeState& Probe = *State.Get();
	int32 Units = 0;
	int32 Normal = 0;
	int32 Bosses = 0;
	int32 BatchSeen = 0;
	double RouteSum = 0;
	for (TActorIterator<ALDUnitActor> It(GetWorld()); It; ++It)
	{
		if (It->IsCommitted())
		{
			++Units;
			const uint64 ID = It->GetPlacement().InstanceId;
			if (!ObservedUnitIds.Contains(ID))
			{
				ObservedUnitIds.Add(ID);
				if (LocalPlayerIndex == 1)
				{
					AllUnits.Add(*It);
				}
			}
		}
	}
	for (TActorIterator<ALDEnemyActor> It(GetWorld()); It; ++It)
	{
		const uint64 ID = It->GetRouteSnapshot().EnemyId;
		if (ID == 0)
		{
			continue;
		}
		Normal += It->IsCombatAlive() && It->GetCombatSnapshot().EnemyTypeId == TEXT("N01") ? 1 : 0;
		Bosses += It->IsCombatAlive() && It->GetCombatSnapshot().EnemyTypeId == TEXT("B01") ? 1 : 0;
		RouteSum += It->GetRouteSnapshot().TotalDistanceCm;
		BatchSeen += ID >= Probe.BatchFirstEnemyId && ID < Probe.BatchFirstEnemyId + BatchSize ? 1 : 0;
		if (!ObservedEnemyIds.Contains(ID))
		{
			ObservedEnemyIds.Add(ID);
			if (LocalPlayerIndex == 1)
			{
				AllEnemies.Add(*It);
			}
		}
	}
	if (Probe.Phase != LocalPhase || Probe.Batch != LocalBatch)
	{
		LocalPhase = Probe.Phase;
		LocalBatch = Probe.Batch;
		if (LocalPlayerIndex == 1 && (Probe.Phase == 3 || Probe.Phase == 4))
		{
			EndProfileCapture();
			GEngine->ForceGarbageCollection(true);
		}
	}
	if (Probe.Phase == 1)
	{
		const ALDGameState* GameState = GetWorld()->GetGameState<ALDGameState>();
		const bool bBattleHudReady = GameState && GameState->GetBattleSnapshot().WaveIndex == 10 &&
		                             GameState->GetBattleSnapshot().ActiveEnemyCount == 99 &&
		                             GameState->GetBattleSnapshot().Bosses.Num() == 2;
		if (!bSustainObserved && Units == 40 && Normal == 99 && Bosses == 2 &&
		    Controller.GetBoardSnapshot().Population == 20 && bBattleHudReady)
		{
			bSustainObserved = true;
			if (LocalPlayerIndex == 1)
			{
				AuthoredUnitIds = ObservedUnitIds;
			}
			Check(TEXT("local-representative-actors-and-owner-board"), true);
			if (LocalPlayerIndex == 1)
			{
				Probe.ServerReportObservation(1, -1, true);
			}
		}
		bRouteMovementObserved |= LastRouteSum > 0 && RouteSum > LastRouteSum;
		LastRouteSum = RouteSum;
		if (bSustainObserved && Probe.SustainStartServerSeconds > 0 && Now - LastIntegrityAt >= 1)
		{
			LastIntegrityAt = Now;
			++IntegritySamples;
			const int32 Population = Controller.GetBoardSnapshot().Population;
			MinimumUnits = FMath::Min(MinimumUnits, Units);
			MinimumNormals = FMath::Min(MinimumNormals, Normal);
			MinimumBosses = FMath::Min(MinimumBosses, Bosses);
			MinimumOwnerPopulation = FMath::Min(MinimumOwnerPopulation, Population);
			bool bCorrect = Units == 40 && Normal == 99 && Bosses == 2 && Population == 20 && bBattleHudReady &&
			                AuthoredUnitIds.Num() == 40 && ObservedUnitIds.Num() == 40;
			ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>();
			for (const TWeakObjectPtr<ALDUnitActor>& Weak : AllUnits)
			{
				const ALDUnitActor* Unit = Weak.Get();
				bCorrect &= Unit && Unit->IsCommitted() && AuthoredUnitIds.Contains(Unit->GetPlacement().InstanceId);
				if (Mode && Unit)
				{
					double NextAttack = 0;
					ALDUnitActor* RegisteredActor = nullptr;
					bCorrect &=
					    Mode->GetCombatService()->TryGetUnitAttackState(Unit->GetPlacement().InstanceId, NextAttack) &&
					    Mode->GetBoardManager()->TryGetCommittedUnitActor(Unit->GetPlacement().InstanceId,
					                                                      RegisteredActor) &&
					    RegisteredActor == Unit;
				}
			}
			if (Mode)
			{
				bCorrect &= Mode->GetBoardManager()->GetSnapshot(0).Population == 20 &&
				            Mode->GetBoardManager()->GetSnapshot(1).Population == 20 &&
				            Mode->GetCombatService()->GetRegisteredUnitCount() == 40 &&
				            Mode->GetCombatService()->GetRegisteredEnemyCount() == 101 &&
				            Mode->GetCombatService()->GetLivingEnemyCount() == 101;
			}
			if (!bCorrect && !bIntegrityFailed)
			{
				Check(TEXT("sustained-population-or-registration-changed"), false,
				           FString::Printf(TEXT("units=%d normal=%d boss=%d ownerPopulation=%d"), Units, Normal, Bosses,
				                                Population));
				bIntegrityFailed = true;
			}
		}
	}
	if (LocalPlayerIndex == 1 && Probe.Phase == 2 && BatchSeen == BatchSize && Probe.ClientObservedBatch != Probe.Batch)
	{
		// ClientObservedBatch is local bookkeeping here; the server updates its independent copy via the owned RPC.
		Probe.ClientObservedBatch = Probe.Batch;
		Check(FString::Printf(TEXT("client-batch%d-all80-actors-replicated"), Probe.Batch + 1), true);
		Probe.ServerReportObservation(2, Probe.Batch, true);
	}
	if (LocalPlayerIndex == 1 && Probe.Phase == 5 && !bResultWritten)
	{
		if (!PollProfileWrite())
		{
			return;
		}
		GEngine->ForceGarbageCollection(true);
		int32 UnitResidue = 0;
		for (const TWeakObjectPtr<ALDUnitActor>& Unit : AllUnits)
		{
			UnitResidue += !Unit.IsStale(false, true);
		}
		if (UnitResidue != 0 || CountUncollected(AllEnemies) != 0)
		{
			return;
		}
		Check(TEXT("client-server-completed-workload"), Probe.bServerPassed && Probe.Batch == 24);
		Check(TEXT("client-2141-observed-actors-collected"), ObservedUnitIds.Num() == 40 &&
		                                                         ObservedEnemyIds.Num() == 2101 && UnitResidue == 0 &&
		                                                         CountUncollected(AllEnemies) == 0);
		Sample(TEXT("client-after-stop-and-gc"), Now);
		WriteResult(false);
		Probe.ServerReportObservation(5, Probe.Batch, !bFailed);
	}
	if (LocalPlayerIndex == 1 && Probe.Phase == 6 && ExitAt == 0)
	{
		Check(TEXT("server-confirms-completion-handshake"), Probe.bServerPassed);
		WriteResult(true);
		ExitAt = Now + 1;
	}
}

void ULDG3LoadProbeSubsystem::Sample(const FString& Label, double Now)
{
	EnsureOutputDirectory();
	int32 Units = 0;
	int32 EnemyActors = 0;
	int32 Alive = 0;
	double RouteSum = 0;
	for (TActorIterator<ALDUnitActor> It(GetWorld()); It; ++It)
	{
		++Units;
	}
	for (TActorIterator<ALDEnemyActor> It(GetWorld()); It; ++It)
	{
		++EnemyActors;
		Alive += It->IsCombatAlive() ? 1 : 0;
		RouteSum += It->GetRouteSnapshot().TotalDistanceCm;
	}
	const FPlatformMemoryStats Memory = FPlatformMemory::GetStats();
	PeakRss = FMath::Max(PeakRss, Memory.UsedPhysical);
	FinalRss = Memory.UsedPhysical;
	FPlatformTime::UpdateCPUTime(1.0f);
	const FCPUTime CPU = FPlatformTime::GetCPUTime();
	const ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>();
	const int32 RegisteredUnits =
	    Mode && Mode->GetCombatService() ? Mode->GetCombatService()->GetRegisteredUnitCount() : -1;
	const int32 RegisteredEnemies =
	    Mode && Mode->GetCombatService() ? Mode->GetCombatService()->GetRegisteredEnemyCount() : -1;
	const int32 Phase = State.IsValid() ? State->Phase : 0;
	const int32 Batch = State.IsValid() ? State->Batch : -1;
	const FString Row = FString::Printf(
	    TEXT("%.6f,%.6f,%d,%d,%s,%d,%.6f,%.6f,%.6f,%llu,%llu,%d,%d,%d,%d,%d,%llu,%d,%.3f\n"), Now - CreatedAt,
	         double(GetWorld()->GetTimeSeconds()), Phase, Batch, *Label, SampleFrameMs.Num(),
	         Percentile95(SampleFrameMs), CPU.CPUTimePct, CPU.CPUTimePctRelative, Memory.UsedPhysical, PeakRss, Units,
	         EnemyActors, Alive, RegisteredUnits, RegisteredEnemies, DamageEvents, DeathIds.Num(), RouteSum);
	FFileHelper::SaveStringToFile(Row, *(OutputDirectory / TEXT("samples.csv")),
	                                     FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(),
	                                     FILEWRITE_Append);
	if (Label != TEXT("periodic"))
	{
		TSharedPtr<FJsonObject> Point = MakeShared<FJsonObject>();
		Point->SetStringField(TEXT("label"), Label);
		Point->SetNumberField(TEXT("rssBytes"), double(Memory.UsedPhysical));
		Point->SetNumberField(TEXT("worldSeconds"), GetWorld()->GetTimeSeconds());
		Point->SetNumberField(TEXT("unitActors"), Units);
		Point->SetNumberField(TEXT("enemyActors"), EnemyActors);
		Point->SetNumberField(TEXT("registeredEnemies"), RegisteredEnemies);
		MemoryCheckpoints.Add(MakeShared<FJsonValueObject>(Point));
	}
	SampleFrameMs.Reset();
	LastSampleAt = Now;
}

void ULDG3LoadProbeSubsystem::Tick(float DeltaTime)
{
	CSV_SCOPED_TIMING_STAT(LDG3Load, ProbeTick);
	if (bFinished || !GetWorld()->HasBegunPlay())
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	const float FrameMs = float((Now - LastTickAt) * 1000);
	LastTickAt = Now;
	SampleFrameMs.Add(FrameMs);
	if (ExitAt > 0 && Now >= ExitAt)
	{
		if (!PollProfileWrite())
		{
			if (Now < ExitAt + 120)
			{
				return;
			}
			Check(TEXT("profile-async-write-timeout"), false);
		}
		WriteResult(bHandshakeComplete);
		bFinished = true;
		FPlatformMisc::RequestExit(false);
		return;
	}
	if (bFailingExit)
	{
		return;
	}
	if (Now - CreatedAt > LoadSeconds + 1800)
	{
		FailAndExit(TEXT("load-total-timeout"));
		return;
	}
	ALDPlayerController* Controller = Cast<ALDPlayerController>(GetWorld()->GetFirstPlayerController());
	ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>();
	const ALDGameState* GameState = GetWorld()->GetGameState<ALDGameState>();
	// Inspect authoritative terminal state before the readiness early return. Otherwise initialization
	// failure can leave the fixture waiting forever for a board that will never become ready.
	if (Mode && GameState && GameState->GetBattleSnapshot().IsTerminal() && !bExpectedTerminal &&
	    (!State.IsValid() || State->Phase < 5))
	{
		const FLDBattleSnapshot& Battle = GameState->GetBattleSnapshot();
		FailAndExit(TEXT("unexpected-terminal-before-load-completion"),
		                 FString::Printf(TEXT("matchPhase=%d result=%d reason=%d at=%.6f readiness=%s probePhase=%d"),
		                                      int32(Battle.Phase), int32(Battle.Result), int32(Battle.ResultReason),
		                                      Battle.ResultServerSeconds, *GameState->GetReadinessReason(),
		                                      State.IsValid() ? State->Phase : -1));
		return;
	}
	if (!State.IsValid() && Now - CreatedAt > 60)
	{
		FailAndExit(
		    TEXT("load-preparation-timeout"),
		         FString::Printf(TEXT("world=%s netMode=%d matchPhase=%d controller=%d boardReady=%d snapshotReady=%d"),
		                              *GetWorld()->GetName(), int32(GetWorld()->GetNetMode()),
		                              GameState ? int32(GameState->GetPhase()) : -1, Controller != nullptr,
		                              Controller && Controller->IsLocalBoardReady(),
		                              Controller && Controller->IsGameplaySnapshotReady()));
		return;
	}
	if (!Controller || !Controller->IsLocalBoardReady() || !Controller->IsGameplaySnapshotReady())
	{
		return;
	}
	LocalPlayerIndex = Controller->GetLocalParticipantIndex();
	if (!State.IsValid())
	{
		for (TActorIterator<ALDG3LoadProbeState> It(GetWorld()); It; ++It)
		{
			State = *It;
		}
		if (!State.IsValid() && Mode && Mode->CanAcceptCommands() && !PrepareLoad(*Mode))
		{
			FailAndExit(TEXT("load-fixture-initialization-failed"));
			return;
		}
	}
	if (!State.IsValid())
	{
		return;
	}
	if (State->Phase == 1 && State->SustainStartServerSeconds > 0 && bSustainObserved)
	{
		if (!bProfileStarted)
		{
			BeginProfileCapture();
			if (bFailingExit)
			{
				return;
			}
		}
		SustainedFrameMs.Add(FrameMs);
		MeasuredSustainSeconds += FrameMs / 1000.0;
		if (!bScreenshotRequested && MeasuredSustainSeconds >= 5)
		{
			bScreenshotRequested = true;
			FScreenshotRequest::RequestScreenshot(OutputDirectory / TEXT("representative-load.png"), true, false);
		}
	}
	CSV_CUSTOM_STAT(LDG3Load, Phase, State->Phase, ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT(LDG3Load, SustainSeconds, float(MeasuredSustainSeconds), ECsvCustomStatOp::Set);
	TickLocal(*Controller, Now);
	if (Mode && !bFinished)
	{
		TickAuthority(*Mode, Now);
	}
	if (Now - LastSampleAt >= 1)
	{
		Sample(TEXT("periodic"), Now);
	}
}

void ULDG3LoadProbeSubsystem::WriteResult(bool bHandshakeConfirmed)
{
	EnsureOutputDirectory();
	bHandshakeComplete = bHandshakeConfirmed;
	if (!bResultWritten)
	{
		Check(TEXT("local-route-motion-observed"), bRouteMovementObserved);
		Check(TEXT("local-sustain-duration"), bSustainObserved && MeasuredSustainSeconds >= LoadSeconds,
		           FString::Printf(TEXT("measured=%.3f configured=%.3f"), MeasuredSustainSeconds, LoadSeconds));
		Check(TEXT("sustained-periodic-actor-registry-and-HUD-integrity"),
		           !bIntegrityFailed && IntegritySamples > 0 && MinimumUnits == 40 && MinimumNormals == 99 &&
		               MinimumBosses == 2 && MinimumOwnerPopulation == 20);
	}
	bResultWritten = true;
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("result"), bFailed ? TEXT("Fail") : TEXT("Pass"));
	Result->SetStringField(TEXT("scope"), TEXT("Development G3Load workload/lifetime fixture. 40 free board-prepared units (all16types per player plus4), 99N01+2B01 HP100000000 on canonical phase-shifted routes, then80HP1 N01 x25. Real actors, combat, movement and replication. No normal-wave victory/balance/physical-input claim. Fallback direct damage after20s is counted separately."));
	Result->SetStringField(
	    TEXT("measurementLimit"),
	         TEXT("Wall-frame intervals include frame cap, stalls and fixture work. CPU columns are OS process percentages; RSS is process working set including engine/allocator caches. No GPU time or per-function CPU claim. Forced GC is outside the sustained sample window."));
	Result->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
	Result->SetStringField(TEXT("outputDirectory"), OutputDirectory);
	Result->SetNumberField(TEXT("localPlayerIndex"), LocalPlayerIndex);
	Result->SetNumberField(TEXT("configuredSustainSeconds"), LoadSeconds);
	Result->SetNumberField(TEXT("measuredSustainSeconds"), MeasuredSustainSeconds);
	Result->SetNumberField(TEXT("sustainedFrameCount"), SustainedFrameMs.Num());
	Result->SetNumberField(TEXT("sustainedFrameMsP95"), Percentile95(SustainedFrameMs));
	Result->SetNumberField(TEXT("baselineRssBytes"), double(BaselineRss));
	Result->SetNumberField(TEXT("peakSampledRssBytes"), double(PeakRss));
	Result->SetNumberField(TEXT("finalRssBytes"), double(FinalRss));
	Result->SetNumberField(TEXT("naturalDamageEvents"), double(DamageEvents));
	Result->SetNumberField(TEXT("naturalDeaths"), NaturalDeaths);
	Result->SetNumberField(TEXT("fixtureDamageDeaths"), FixtureDeaths);
	Result->SetNumberField(TEXT("uniqueDeathEvents"), DeathIds.Num());
	Result->SetNumberField(TEXT("completedBatches"), CompletedBatches);
	Result->SetNumberField(TEXT("observedEnemyIdentities"), ObservedEnemyIds.Num());
	Result->SetNumberField(TEXT("observedUnitIdentities"), ObservedUnitIds.Num());
	Result->SetNumberField(TEXT("attackingUnitTypeCount"), AttackingUnitTypes.Num());
	Result->SetNumberField(TEXT("sustainIntegritySamples"), IntegritySamples);
	Result->SetNumberField(TEXT("minimumSustainUnitActors"), MinimumUnits);
	Result->SetNumberField(TEXT("minimumSustainAliveNormals"), MinimumNormals);
	Result->SetNumberField(TEXT("minimumSustainAliveBosses"), MinimumBosses);
	Result->SetNumberField(TEXT("minimumSustainOwnerPopulation"), MinimumOwnerPopulation);
	Result->SetBoolField(TEXT("completionHandshakeConfirmed"), bHandshakeConfirmed);
	Result->SetBoolField(TEXT("authorityMetricsAvailable"), LocalPlayerIndex == 0);
	Result->SetBoolField(TEXT("profileCsvWriteCompleted"), bProfileWritten);
	Result->SetStringField(TEXT("profileCsvPath"), ProfilePath);
	Result->SetArrayField(TEXT("checks"), Checks);
	Result->SetArrayField(TEXT("memoryCheckpoints"), MemoryCheckpoints);
	FString Json;
	FJsonSerializer::Serialize(Result.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
	FFileHelper::SaveStringToFile(Json, *(OutputDirectory / TEXT("result.json")));
}

void ULDG3LoadProbeSubsystem::FailAndExit(const FString& Reason, const FString& Detail)
{
	Check(Reason, false, Detail);
	EndProfileCapture();
	WriteResult(false);
	bFailingExit = true;
	ExitAt = FPlatformTime::Seconds() + 1;
}

void ULDG3LoadProbeSubsystem::BeginProfileCapture()
{
	EnsureOutputDirectory();
	// This probe owns one capture per process; it does not stop a capture owned by another tool.
#if CSV_PROFILER
	if (FCsvProfiler::Get()->IsCapturing() || FCsvProfiler::Get()->IsWritingFile())
	{
		FailAndExit(TEXT("profile-capture-already-owned"));
		return;
	}
	FCsvProfiler::Get()->BeginCapture(-1, OutputDirectory, TEXT("profile.csv"));
	bProfileStarted = true;
#else
	FailAndExit(TEXT("csv-profiler-disabled-in-this-build"));
#endif
}

void ULDG3LoadProbeSubsystem::EndProfileCapture()
{
#if CSV_PROFILER
	if (bProfileStarted && !bProfileEndRequested)
	{
		ProfileWrite = FCsvProfiler::Get()->EndCapture();
		bProfileEndRequested = true;
	}
#endif
}

bool ULDG3LoadProbeSubsystem::PollProfileWrite()
{
#if CSV_PROFILER
	if (bProfileStarted && !bProfileWritten)
	{
		EndProfileCapture();
		if (!ProfileWrite.IsValid() || !ProfileWrite.IsReady())
		{
			return false;
		}
		ProfilePath = ProfileWrite.Get();
		const bool bExists = !ProfilePath.IsEmpty() && IFileManager::Get().FileSize(*ProfilePath) > 0;
		Check(TEXT("profile-csv-async-write-completed"), bExists, ProfilePath);
		bProfileWritten = true;
	}
#endif
	return true;
}

void ULDG3LoadProbeSubsystem::Deinitialize()
{
	if (ALDGameMode* Mode = GetWorld()->GetAuthGameMode<ALDGameMode>(); Mode && Mode->GetCombatService())
	{
		Mode->GetCombatService()->OnDamageCommitted.Remove(DamageHandle);
		Mode->GetCombatService()->OnEnemyDeathCommitted.Remove(DeathHandle);
	}
	Super::Deinitialize();
}
