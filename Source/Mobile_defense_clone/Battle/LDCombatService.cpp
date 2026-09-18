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
	if (!bInitialized || bStopped || !Unit.HasAuthority() || !Unit.IsCommitted() ||
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
	if (!bInitialized || bStopped || !Enemy.HasAuthority() || !Enemy.IsCombatAlive() ||
	    Enemy.GetRouteSnapshot().MatchId != MatchContext.MatchId)
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

ALDEnemyActor* ULDCombatService::SelectTarget(const ALDUnitActor& Unit) const
{
	TArray<FLDTargetCandidate> Candidates;
	TArray<ALDEnemyActor*> Actors;
	for (const auto& Entry : Enemies)
	{
		ALDEnemyActor* Enemy = Entry.Value.Get();
		if (Enemy && Enemy->GetRouteSnapshot().MatchId == MatchContext.MatchId &&
		    Enemy->GetCombatSnapshot().SpawnedServerSeconds <= LastAdvanceSeconds)
		{
			Candidates.Add({Enemy->GetRouteSnapshot().EnemyId, Enemy->GetCombatSnapshot().SpawnSerial,
			                Enemy->GetActorLocation(), Enemy->IsCombatAlive()});
			Actors.Add(Enemy);
		}
	}
	const int32 Selected = FLDCombatRules::SelectTarget(Unit.GetActorLocation(), Unit.GetUnitRow().RangeCm, Candidates);
	return Actors.IsValidIndex(Selected) ? Actors[Selected] : nullptr;
}

bool ULDCombatService::IsValidTarget(const ALDUnitActor& Unit, const ALDEnemyActor& Enemy) const
{
	const TWeakObjectPtr<ALDEnemyActor>* Registered = Enemies.Find(Enemy.GetRouteSnapshot().EnemyId);
	return Unit.IsCommitted() && Enemy.IsCombatAlive() && Registered && Registered->Get() == &Enemy &&
	       Enemy.GetRouteSnapshot().MatchId == MatchContext.MatchId &&
	       FVector::DistSquaredXY(Unit.GetActorLocation(), Enemy.GetActorLocation()) <=
	           FMath::Square(Unit.GetUnitRow().RangeCm);
}

bool ULDCombatService::AdvanceCombatTo(double ServerSeconds)
{
	if (!bInitialized || bStopped || bAdvancing || !FMath::IsFinite(ServerSeconds) || ServerSeconds < 0 ||
	    ServerSeconds < LastAdvanceSeconds)
	{
		return false;
	}
	if (ServerSeconds == LastAdvanceSeconds)
	{
		return true;
	}
	TGuardValue<bool> AdvancingGuard(bAdvancing, true);
	LastAdvanceSeconds = ServerSeconds;
	for (const auto& Entry : Enemies)
	{
		if (ALDEnemyActor* Enemy = Entry.Value.Get())
		{
			Enemy->AdvanceRouteTo(ServerSeconds);
		}
	}
	struct FDueAttack
	{
		uint64 InstanceId;
		double DueSeconds;
	};
	TArray<FDueAttack> DueAttacks;
	for (auto& Entry : Units)
	{
		FUnitAttackState& State = Entry.Value;
		ALDUnitActor* Unit = State.Unit.Get();
		if (!Unit || !Unit->IsCommitted())
		{
			State.ReservedTarget.Reset();
			continue;
		}
		const double Earliest = FMath::Max(State.NextAttackAt, Unit->GetPlacement().MoveBlockedUntilServerSeconds);
		ALDEnemyActor* Target = State.ReservedTarget.Get();
		if (Target && !IsValidTarget(*Unit, *Target))
		{
			State.ReservedTarget.Reset();
			Target = nullptr;
		}
		if (!Target)
		{
			Target = SelectTarget(*Unit);
			if (Target)
			{
				State.ReservedTarget = Target;
				// A newly observed target cannot receive an attack backdated into a targetless interval.
				State.ReservedAttackAt = FMath::Max(Earliest, ServerSeconds);
			}
		}
		else
		{
			State.ReservedAttackAt = FMath::Max(State.ReservedAttackAt, Earliest);
			// Re-evaluate nearest on the current canonical positions rather than locking an old target indefinitely.
			ALDEnemyActor* Nearest = SelectTarget(*Unit);
			if (Nearest && Nearest != Target)
			{
				Target = Nearest;
				State.ReservedTarget = Target;
				State.ReservedAttackAt =
				    FMath::Max(State.ReservedAttackAt, Target->GetCombatSnapshot().SpawnedServerSeconds);
			}
		}
		if (Target && State.ReservedAttackAt <= ServerSeconds)
		{
			DueAttacks.Add({Entry.Key, State.ReservedAttackAt});
		}
	}
	DueAttacks.Sort(
	    [](const FDueAttack& A, const FDueAttack& B)
	    { return A.DueSeconds == B.DueSeconds ? A.InstanceId < B.InstanceId : A.DueSeconds < B.DueSeconds; });
	for (const FDueAttack& Due : DueAttacks)
	{
		FUnitAttackState* State = Units.Find(Due.InstanceId);
		if (bStopped || !State)
		{
			continue;
		}
		ALDUnitActor* Unit = State->Unit.Get();
		ALDEnemyActor* Target = State->ReservedTarget.Get();
		State->ReservedTarget.Reset();
		if (!Unit || !Target || !IsValidTarget(*Unit, *Target) ||
		    Due.DueSeconds < Unit->GetPlacement().MoveBlockedUntilServerSeconds)
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
		Event.SourceInstanceId = Due.InstanceId;
		Event.EnemyId = Target->GetRouteSnapshot().EnemyId;
		Event.Amount = Damage;
		Event.AttackServerSeconds = Due.DueSeconds;
		FLDCombatDeath Death;
		const ELDDamageResult Result = Target->TryApplyDamage(Event, Death);
		if (Result != ELDDamageResult::Applied && Result != ELDDamageResult::Killed)
		{
			continue;
		}
		State->NextAttackAt = Due.DueSeconds + Unit->GetUnitRow().AttackIntervalSeconds;
		Unit->PresentCommittedAttack(Event.DamageEventId, Target->GetActorLocation(), Due.DueSeconds);
		if (Result == ELDDamageResult::Killed)
		{
			// State references must not be used after a delegate: the owner may remove units or end the match.
			OnEnemyDeathCommitted.Broadcast(Death);
		}
	}
	return true;
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
