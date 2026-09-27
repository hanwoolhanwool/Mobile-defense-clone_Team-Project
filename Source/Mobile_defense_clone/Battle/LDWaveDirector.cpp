#include "Battle/LDWaveDirector.h"

#include "Battle/LDCombatService.h"
#include "Battle/LDEnemyActor.h"
#include "Core/LDGameState.h"
#include "Data/LDGameData.h"
#include "Engine/World.h"
#include <limits>

UWorld* ULDWaveDirector::GetWorld() const
{
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

bool ULDWaveDirector::Initialize(ULDGameData& Data, ALDGameState& State, ULDCombatService& Combat,
                                 FLDEnemyActorFactory ActorFactory)
{
	if (bInitialized || bStopped || !GetWorld() || !State.HasAuthority() || State.GetWorld() != GetWorld() ||
	    Combat.GetWorld() != GetWorld() || !Data.IsLoaded() || !State.GetMatchContext().IsValid() ||
	    State.GetMatchContext().RulesVersion != Data.GetRules().RulesVersion)
	{
		return false;
	}
	GameData = &Data;
	GameState = &State;
	CombatService = &Combat;
	SpawnActor = MoveTemp(ActorFactory);
	bInitialized = true;
	return true;
}

bool ULDWaveDirector::StartAt(double ServerSeconds)
{
	if (!bInitialized || bStarted || bStopped || !FMath::IsFinite(ServerSeconds) || ServerSeconds < 0 ||
	    GameState->GetPhase() != ELDMatchPhase::Running)
	{
		return false;
	}
	bStarted = true;
	return BeginWave(1, ServerSeconds);
}

bool ULDWaveDirector::BeginWave(int32 WaveIndex, double ServerSeconds)
{
	const TArray<FLDWaveRow>& Waves = GameData->GetWaves();
	if (bStopped || !Waves.IsValidIndex(WaveIndex - 1) || WaveIndex > GameData->GetRules().FinalWave)
	{
		return false;
	}
	const FLDWaveRow& Wave = Waves[WaveIndex - 1];
	WaveStartSeconds = ServerSeconds;
	NextNormalOrdinal = 0;
	bBossDeadlineProcessed = false;
	FLDBattleSnapshot Snapshot = GameState->GetBattleSnapshot();
	Snapshot.WaveIndex = WaveIndex;
	Snapshot.WaveEndServerSeconds = Wave.BossCountPerGate > 0 ? 0 : ServerSeconds + Wave.DurationSeconds;
	Snapshot.BossDeadlineServerSeconds = Wave.BossCountPerGate > 0 ? ServerSeconds + Wave.BossDeadlineSeconds : 0;
	Snapshot.Bosses.Reset();
	GameState->UpdateBattle(Snapshot);
	if (Wave.BossCountPerGate > 0)
	{
		for (int32 Route = 0; Route < 2 && !bStopped; ++Route)
		{
			if (!SpawnEnemy(Route, Wave, ServerSeconds))
			{
				return false;
			}
		}
		if (!bStopped)
		{
			Snapshot = GameState->GetBattleSnapshot();
			Snapshot.bFinalSpawnsComplete = WaveIndex == Snapshot.FinalWave;
			GameState->UpdateBattle(Snapshot);
		}
	}
	else if (Wave.FirstSpawnOffsetSeconds == 0)
	{
		for (int32 Route = 0; Route < 2 && !bStopped; ++Route)
		{
			if (!SpawnEnemy(Route, Wave, ServerSeconds))
			{
				return false;
			}
		}
		++NextNormalOrdinal;
	}
	return !bStopped;
}

bool ULDWaveDirector::SpawnEnemy(int32 RouteIndex, const FLDWaveRow& Wave, double ServerSeconds)
{
	if (bStopped)
	{
		return false;
	}
	const bool bBoss = Wave.BossCountPerGate > 0;
	FLDEnemyRow Row;
	const FName TypeId = bBoss ? Wave.BossId : FName(TEXT("N01"));
	if (!GameData->TryGetEnemyRow(TypeId, Row))
	{
		RequestTerminal(ELDMatchResult::Aborted, ELDResultReason::InitializationFailure, ServerSeconds);
		return false;
	}
	ALDEnemyActor* Enemy = SpawnActor ? SpawnActor(*GetWorld()) : GetWorld()->SpawnActor<ALDEnemyActor>();
	const uint64 EnemyId = NextEnemyId;
	const FLDGameRules& Rules = GameData->GetRules();
	const double HP = bBoss ? Row.FixedHP : Wave.NormalBaseHP * Row.HPScale;
	if (!Enemy || Enemy->GetWorld() != GetWorld() || !Enemy->HasAuthority() ||
	    !Enemy->InitializeRoute(GameState->GetMatchContext().MatchId, EnemyId, RouteIndex,
	                            Rules.PointsByGateCm[RouteIndex], Row.SpeedCmPerSec, ServerSeconds) ||
	    !Enemy->InitializeCombat(Row, HP, EnemyId, Wave.WaveIndex, ServerSeconds) ||
	    !CombatService->RegisterEnemy(*Enemy))
	{
		if (Enemy && Enemy->GetWorld() == GetWorld())
		{
			Enemy->Destroy();
		}
		RequestTerminal(ELDMatchResult::Aborted, ELDResultReason::InitializationFailure, ServerSeconds);
		return false;
	}
	++NextEnemyId;
	LivingEnemies.Add(EnemyId, Enemy);
	FLDBattleSnapshot Snapshot = GameState->GetBattleSnapshot();
	if (bBoss)
	{
		FLDBossSnapshot Boss;
		Boss.EnemyId = EnemyId;
		Boss.RouteIndex = RouteIndex;
		Boss.HP = HP;
		Boss.MaxHP = HP;
		Boss.bAlive = true;
		Snapshot.Bosses.Add(Boss);
	}
	else
	{
		NormalEnemies.Add(EnemyId);
		++Snapshot.ActiveEnemyCount;
	}
	// Close admission before publishing N=100: GameState observers may synchronously submit a command.
	if (!bBoss && Snapshot.ActiveEnemyCount >= Snapshot.MaxEnemyCount)
	{
		RequestTerminal(ELDMatchResult::Defeat, ELDResultReason::EnemyLimit, ServerSeconds);
	}
	GameState->UpdateBattle(Snapshot);
	return !bStopped;
}

double ULDWaveDirector::GetNextEventSeconds() const
{
	const double Never = std::numeric_limits<double>::infinity();
	if (!bStarted || bStopped)
	{
		return Never;
	}
	const FLDBattleSnapshot& Snapshot = GameState->GetBattleSnapshot();
	const FLDWaveRow& Wave = GameData->GetWaves()[Snapshot.WaveIndex - 1];
	if (Wave.BossCountPerGate > 0)
	{
		return bBossDeadlineProcessed ? Never : Snapshot.BossDeadlineServerSeconds;
	}
	const double NextSpawn =
	    NextNormalOrdinal < Wave.NormalCountPerGate
	        ? WaveStartSeconds + Wave.FirstSpawnOffsetSeconds + NextNormalOrdinal * Wave.SpawnIntervalSeconds
	        : Never;
	return FMath::Min(NextSpawn, Snapshot.WaveEndServerSeconds);
}

void ULDWaveDirector::ProcessEventsAt(double ServerSeconds)
{
	if (!bStarted || bStopped)
	{
		return;
	}
	const FLDBattleSnapshot Snapshot = GameState->GetBattleSnapshot();
	const FLDWaveRow& Wave = GameData->GetWaves()[Snapshot.WaveIndex - 1];
	if (Wave.BossCountPerGate == 0)
	{
		const double NextSpawn =
		    WaveStartSeconds + Wave.FirstSpawnOffsetSeconds + NextNormalOrdinal * Wave.SpawnIntervalSeconds;
		if (NextNormalOrdinal < Wave.NormalCountPerGate && NextSpawn == ServerSeconds)
		{
			for (int32 Route = 0; Route < 2 && !bStopped; ++Route)
			{
				SpawnEnemy(Route, Wave, ServerSeconds);
			}
			++NextNormalOrdinal;
		}
	}
	else if (!bBossDeadlineProcessed && ServerSeconds == Snapshot.BossDeadlineServerSeconds)
	{
		bBossDeadlineProcessed = true;
		RefreshCombatView();
		for (const FLDBossSnapshot& Boss : GameState->GetBattleSnapshot().Bosses)
		{
			if (Boss.bAlive)
			{
				RequestTerminal(ELDMatchResult::Defeat, ELDResultReason::BossTimeout, ServerSeconds);
				break;
			}
		}
	}
	EvaluateVictory(ServerSeconds);
	if (!bStopped && Wave.BossCountPerGate == 0 && ServerSeconds == Snapshot.WaveEndServerSeconds)
	{
		BeginWave(Snapshot.WaveIndex + 1, ServerSeconds);
	}
}

bool ULDWaveDirector::HandleEnemyDeath(const FLDCombatDeath& Death)
{
	if (bStopped || !bStarted || Death.MatchId != GameState->GetMatchContext().MatchId || Death.EnemyId == 0 ||
	    Death.DeathEventId != Death.EnemyId)
	{
		return false;
	}
	const TWeakObjectPtr<ALDEnemyActor>* Registered = LivingEnemies.Find(Death.EnemyId);
	ALDEnemyActor* Enemy = Registered ? Registered->Get() : nullptr;
	if (!Enemy || Enemy->GetWorld() != GetWorld() || !Enemy->HasAuthority() || Enemy->GetWorld() != GetWorld() ||
	    Enemy->GetCombatSnapshot().bAlive || Enemy->GetCombatSnapshot().HP != 0 ||
	    Enemy->GetCombatSnapshot().SpawnSerial != Death.SpawnSerial ||
	    Enemy->GetCombatSnapshot().SpawnWaveIndex != Death.SpawnWaveIndex ||
	    Enemy->GetCombatSnapshot().SpawnedServerSeconds != Death.SpawnedServerSeconds ||
	    Enemy->GetCombatSnapshot().DeathServerSeconds != Death.DeathServerSeconds ||
	    Enemy->GetCombatSnapshot().EnemyTypeId != Death.EnemyTypeId)
	{
		return false;
	}
	FLDBattleSnapshot Snapshot = GameState->GetBattleSnapshot();
	if (NormalEnemies.Remove(Death.EnemyId) > 0)
	{
		--Snapshot.ActiveEnemyCount;
	}
	for (FLDBossSnapshot& Boss : Snapshot.Bosses)
	{
		if (Boss.EnemyId == Death.EnemyId)
		{
			Boss.HP = 0;
			Boss.bAlive = false;
		}
	}
	LivingEnemies.Remove(Death.EnemyId);
	LastDeathServerSeconds = FMath::Max(LastDeathServerSeconds, Death.DeathServerSeconds);
	CombatService->UnregisterEnemy(Death.EnemyId);
	GameState->UpdateBattle(Snapshot);
	// Keep the replicated death briefly observable, then release the actor, route and damage-id history.
	Enemy->SetLifeSpan(0.25f);
	return true;
}

void ULDWaveDirector::RefreshCombatView()
{
	if (bStopped || !bStarted)
	{
		return;
	}
	FLDBattleSnapshot Snapshot = GameState->GetBattleSnapshot();
	bool bChanged = false;
	for (FLDBossSnapshot& Boss : Snapshot.Bosses)
	{
		const TWeakObjectPtr<ALDEnemyActor>* Found = LivingEnemies.Find(Boss.EnemyId);
		const ALDEnemyActor* Enemy = Found ? Found->Get() : nullptr;
		if (Enemy && Boss.HP != Enemy->GetCombatSnapshot().HP)
		{
			Boss.HP = Enemy->GetCombatSnapshot().HP;
			bChanged = true;
		}
	}
	if (bChanged)
	{
		GameState->UpdateBattle(Snapshot);
	}
}

void ULDWaveDirector::EvaluateVictory(double ServerSeconds)
{
	if (!bStarted || bStopped)
	{
		return;
	}
	const FLDBattleSnapshot& Snapshot = GameState->GetBattleSnapshot();
	if (!Snapshot.bFinalSpawnsComplete || Snapshot.WaveIndex != Snapshot.FinalWave || Snapshot.ActiveEnemyCount != 0 ||
	    Snapshot.Bosses.Num() != 2 || LastDeathServerSeconds > ServerSeconds)
	{
		return;
	}
	for (const FLDBossSnapshot& Boss : Snapshot.Bosses)
	{
		if (Boss.EnemyId == 0 || Boss.bAlive || Boss.HP != 0)
		{
			return;
		}
	}
	RequestTerminal(ELDMatchResult::Victory, ELDResultReason::None, LastDeathServerSeconds);
}

void ULDWaveDirector::RequestTerminal(ELDMatchResult Result, ELDResultReason Reason, double ServerSeconds)
{
	if (!bStopped)
	{
		bStopped = true;
		OnTerminalRequested.Broadcast(Result, Reason, ServerSeconds);
	}
}

void ULDWaveDirector::Stop()
{
	bStopped = true;
	LivingEnemies.Reset();
	NormalEnemies.Reset();
	SpawnActor = {};
	OnTerminalRequested.Clear();
}

int32 ULDWaveDirector::GetTrackedEnemyCount() const
{
	return LivingEnemies.Num();
}
