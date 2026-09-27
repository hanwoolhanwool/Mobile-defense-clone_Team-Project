#include "Battle/LDCombatService.h"

#include "Battle/LDCombatRules.h"
#include "Battle/LDEnemyActor.h"
#include "Battle/LDUnitActor.h"
#include "Data/LDGameData.h"
#include "Engine/World.h"

UWorld* ULDCombatService::GetWorld() const
{
	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

bool ULDCombatService::Initialize(const FLDMatchContext& Context, const FLDGameRules& Rules)
{
	if (bInitialized || bStopped || !Context.IsValid() || Context.RulesVersion != Rules.RulesVersion || !GetWorld() ||
	    GetWorld()->GetNetMode() == NM_Client || !FMath::IsFinite(Rules.InitialAttackDelaySeconds) ||
	    Rules.InitialAttackDelaySeconds < 0)
	{
		return false;
	}
	MatchContext = Context;
	InitialAttackDelaySeconds = Rules.InitialAttackDelaySeconds;
	bInitialized = true;
	return true;
}

void ULDCombatService::RegisterCommittedUnit(ALDUnitActor& Unit, double CommitServerSeconds)
{
	if (!bInitialized || bStopped || !Unit.HasAuthority() || Unit.GetWorld() != GetWorld() || !Unit.IsCommitted() ||
	    !FMath::IsFinite(CommitServerSeconds) || CommitServerSeconds < 0)
	{
		return;
	}
	const uint64 Id = Unit.GetPlacement().InstanceId;
	if (Units.Contains(Id))
	{
		// Repeated board publication, movement and replenishment never replace the existing timer or actor.
		return;
	}
	FUnitAttackState State;
	State.Unit = &Unit;
	State.NextAttackAt = CommitServerSeconds + InitialAttackDelaySeconds;
	Units.Add(Id, State);
}

void ULDCombatService::UnregisterUnit(uint64 InstanceId)
{
	Units.Remove(InstanceId);
}

bool ULDCombatService::RegisterEnemy(ALDEnemyActor& Enemy)
{
	if (!bInitialized || bStopped || !Enemy.HasAuthority() || Enemy.GetWorld() != GetWorld() ||
	    !Enemy.IsCombatAlive() || Enemy.GetRouteSnapshot().MatchId != MatchContext.MatchId)
	{
		return false;
	}
	const uint64 Id = Enemy.GetRouteSnapshot().EnemyId;
	if (const TWeakObjectPtr<ALDEnemyActor>* Existing = Enemies.Find(Id))
	{
		return Existing->Get() == &Enemy;
	}
	Enemies.Add(Id, &Enemy);
	return true;
}

void ULDCombatService::UnregisterEnemy(uint64 EnemyId)
{
	Enemies.Remove(EnemyId);
}

ALDEnemyActor* ULDCombatService::SelectTarget(const ALDUnitActor& Unit, double SampleSeconds) const
{
	TArray<FLDTargetCandidate> Candidates;
	TArray<ALDEnemyActor*> Actors;
	for (const auto& Entry : Enemies)
	{
		ALDEnemyActor* Enemy = Entry.Value.Get();
		FVector Position;
		if (Enemy && Enemy->GetRouteSnapshot().MatchId == MatchContext.MatchId &&
		    Enemy->GetCombatSnapshot().SpawnedServerSeconds <= SampleSeconds &&
		    Enemy->TryGetCanonicalPositionAt(SampleSeconds, Position))
		{
			Candidates.Add({Enemy->GetRouteSnapshot().EnemyId, Enemy->GetCombatSnapshot().SpawnSerial, Position,
			                Enemy->IsCombatAlive()});
			Actors.Add(Enemy);
		}
	}
	const int32 Selected = FLDCombatRules::SelectTarget(Unit.GetActorLocation(), Unit.GetUnitRow().RangeCm, Candidates);
	return Actors.IsValidIndex(Selected) ? Actors[Selected] : nullptr;
}

bool ULDCombatService::IsValidTarget(const ALDUnitActor& Unit, const ALDEnemyActor& Enemy, double SampleSeconds) const
{
	const TWeakObjectPtr<ALDEnemyActor>* Registered = Enemies.Find(Enemy.GetRouteSnapshot().EnemyId);
	FVector Position;
	return Unit.IsCommitted() && Unit.GetWorld() == GetWorld() && Enemy.GetWorld() == GetWorld() &&
	       Enemy.IsCombatAlive() && Registered && Registered->Get() == &Enemy &&
	       Enemy.GetRouteSnapshot().MatchId == MatchContext.MatchId &&
	       Enemy.GetCombatSnapshot().SpawnedServerSeconds <= SampleSeconds &&
	       Enemy.TryGetCanonicalPositionAt(SampleSeconds, Position) &&
	       FVector::DistSquaredXY(Unit.GetActorLocation(), Position) <= FMath::Square(Unit.GetUnitRow().RangeCm);
}

bool ULDCombatService::AdvanceCombatTo(double ServerSeconds)
{
	return AdvanceCombatInternal(ServerSeconds, true);
}

bool ULDCombatService::AdvanceCombatBefore(double ServerSeconds)
{
	return AdvanceCombatInternal(ServerSeconds, false);
}

bool ULDCombatService::AdvanceCombatInternal(double ServerSeconds, bool bIncludeBoundary)
{
	if (!bInitialized || bStopped || bAdvancing || !FMath::IsFinite(ServerSeconds) || ServerSeconds < 0 ||
	    ServerSeconds < LastAdvanceSeconds)
	{
		return false;
	}
	if (ServerSeconds == LastAdvanceSeconds && (bLastAdvanceIncludedBoundary || !bIncludeBoundary))
	{
		return true;
	}
	TGuardValue<bool> AdvancingGuard(bAdvancing, true);
	LastAdvanceSeconds = ServerSeconds;
	bLastAdvanceIncludedBoundary = bIncludeBoundary;
	// Existing events retain their exact times and are resolved globally before observing a new target.
	ResolveScheduledAttacks(ServerSeconds, bIncludeBoundary);
	if (bStopped)
	{
		return true;
	}
	for (auto& Entry : Units)
	{
		FUnitAttackState& State = Entry.Value;
		ALDUnitActor* Unit = State.Unit.Get();
		if (!Unit || !Unit->IsCommitted())
		{
			State.ReservedTarget.Reset();
			continue;
		}
		ALDEnemyActor* Target = SelectTarget(*Unit, ServerSeconds);
		if (!Target)
		{
			State.ReservedTarget.Reset();
			continue;
		}
		const double Earliest = FMath::Max(State.NextAttackAt, Unit->GetPlacement().MoveBlockedUntilServerSeconds);
		if (!State.ReservedTarget.IsValid())
		{
			// Newly observed targets cannot inherit an attack in a previously targetless interval.
			State.ReservedAttackAt = FMath::Max(Earliest, ServerSeconds);
		}
		else
		{
			State.ReservedAttackAt = FMath::Max(State.ReservedAttackAt, Earliest);
		}
		State.ReservedTarget = Target;
	}
	ResolveScheduledAttacks(ServerSeconds, bIncludeBoundary);
	for (const auto& Entry : Enemies)
	{
		if (ALDEnemyActor* Enemy = Entry.Value.Get())
		{
			Enemy->AdvanceRouteTo(ServerSeconds);
		}
	}
	return true;
}

void ULDCombatService::ResolveScheduledAttacks(double ServerSeconds, bool bIncludeBoundary)
{
	while (!bStopped)
	{
		uint64 SelectedId = 0;
		double DueSeconds = 0;
		for (auto& Entry : Units)
		{
			FUnitAttackState& State = Entry.Value;
			ALDUnitActor* Unit = State.Unit.Get();
			if (!Unit || !Unit->IsCommitted() || !State.ReservedTarget.IsValid())
			{
				continue;
			}
			State.ReservedAttackAt =
			    FMath::Max(State.ReservedAttackAt,
			               FMath::Max(State.NextAttackAt, Unit->GetPlacement().MoveBlockedUntilServerSeconds));
			const double Due = State.ReservedAttackAt;
			if (Due > ServerSeconds || (!bIncludeBoundary && Due == ServerSeconds))
			{
				continue;
			}
			if (SelectedId == 0 || Due < DueSeconds || (Due == DueSeconds && Entry.Key < SelectedId))
			{
				SelectedId = Entry.Key;
				DueSeconds = Due;
			}
		}
		if (SelectedId == 0)
		{
			return;
		}
		FUnitAttackState* State = Units.Find(SelectedId);
		ALDUnitActor* Unit = State ? State->Unit.Get() : nullptr;
		if (!State || !Unit)
		{
			continue;
		}
		State->ReservedTarget.Reset();
		ALDEnemyActor* Target = SelectTarget(*Unit, DueSeconds);
		if (!Target || Unit->GetPlacement().InstanceId != SelectedId || !IsValidTarget(*Unit, *Target, DueSeconds))
		{
			continue;
		}
		int32 Damage = 0;
		if (!FLDCombatRules::TryCalculateDamage(Unit->GetUnitRow(), Target->GetEnemyRow().Armor,
		                                        Target->GetEnemyRow().MagicResistance, Damage))
		{
			continue;
		}
		FLDDamageEvent Event;
		Event.MatchId = MatchContext.MatchId;
		Event.DamageEventId = NextDamageEventId++;
		Event.SourceInstanceId = SelectedId;
		Event.EnemyId = Target->GetRouteSnapshot().EnemyId;
		Event.Amount = Damage;
		Event.AttackServerSeconds = DueSeconds;
		FVector HitPosition;
		Target->TryGetCanonicalPositionAt(DueSeconds, HitPosition);
		// The event timeline is monotonic. Advancing the victim before death preserves its exact final location.
		Target->AdvanceRouteTo(DueSeconds);
		FLDCombatDeath Death;
		const int32 AttackingPlayer = Unit->GetPlacement().PlayerIndex;
		const int32 EffectiveDamage = FMath::Min(Event.Amount, FMath::CeilToInt(Target->GetCombatSnapshot().HP));
		const ELDDamageResult Result = Target->TryApplyDamage(Event, Death);
		if (Result != ELDDamageResult::Applied && Result != ELDDamageResult::Killed)
		{
			continue;
		}
		State->NextAttackAt = DueSeconds + Unit->GetUnitRow().AttackIntervalSeconds;
		if (Target->IsCombatAlive())
		{
			State->ReservedTarget = Target;
			State->ReservedAttackAt = State->NextAttackAt;
		}
		Unit->PresentCommittedAttack(Event.DamageEventId, HitPosition, DueSeconds);
		// These are copied values; observers may stop services or remove actors during either notification.
		OnDamageCommitted.Broadcast(Event, AttackingPlayer, EffectiveDamage);
		if (Result == ELDDamageResult::Killed && !bStopped)
		{
			// The subscriber may mutate Units or stop the match; do not retain map references across this call.
			OnEnemyDeathCommitted.Broadcast(Death);
		}
	}
}
void ULDCombatService::Stop()
{
	if (bStopped)
	{
		return;
	}
	bStopped = true;
	for (const auto& Entry : Enemies)
	{
		if (ALDEnemyActor* Enemy = Entry.Value.Get())
		{
			Enemy->StopCombat();
			Enemy->StopRoute();
		}
	}
	Units.Reset();
	Enemies.Reset();
	OnEnemyDeathCommitted.Clear();
	OnDamageCommitted.Clear();
}

bool ULDCombatService::TryGetUnitAttackState(uint64 InstanceId, double& OutNextAttackAt) const
{
	const FUnitAttackState* State = Units.Find(InstanceId);
	if (!State)
	{
		return false;
	}
	OutNextAttackAt = State->NextAttackAt;
	return true;
}

int32 ULDCombatService::GetRegisteredUnitCount() const
{
	return Units.Num();
}

int32 ULDCombatService::GetLivingEnemyCount() const
{
	int32 Count = 0;
	for (const auto& Entry : Enemies)
	{
		const ALDEnemyActor* Enemy = Entry.Value.Get();
		Count += Enemy && Enemy->IsCombatAlive() ? 1 : 0;
	}
	return Count;
}
