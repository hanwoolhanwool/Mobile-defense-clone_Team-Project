#pragma once

#include "CoreMinimal.h"
#include "Battle/LDCombatEvents.h"
#include "Data/LDMatchTypes.h"
#include "UObject/Object.h"
#include "LDCombatService.generated.h"

class ALDUnitActor;
class ALDEnemyActor;
struct FLDGameRules;

DECLARE_MULTICAST_DELEGATE_OneParam(FLDEnemyDeathCommitted, const FLDCombatDeath&);

// GameMode owns the lifetime and the fixed clock. No service lookup or economic mutation occurs here.
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDCombatService : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;
	bool Initialize(const FLDMatchContext& Context, const FLDGameRules& Rules);
	void RegisterCommittedUnit(ALDUnitActor& Unit, double CommitServerSeconds);
	void UnregisterUnit(uint64 InstanceId);
	bool RegisterEnemy(ALDEnemyActor& Enemy);
	void UnregisterEnemy(uint64 EnemyId);
	bool AdvanceCombatTo(double ServerSeconds);
	void Stop();
	bool TryGetUnitAttackState(uint64 InstanceId, double& OutNextAttackAt) const;
	int32 GetRegisteredUnitCount() const;
	int32 GetLivingEnemyCount() const;
	FLDEnemyDeathCommitted OnEnemyDeathCommitted;

private:
	struct FUnitAttackState
	{
		TWeakObjectPtr<ALDUnitActor> Unit;
		TWeakObjectPtr<ALDEnemyActor> ReservedTarget;
		double NextAttackAt = 0;
		double ReservedAttackAt = 0;
	};
	ALDEnemyActor* SelectTarget(const ALDUnitActor& Unit) const;
	bool IsValidTarget(const ALDUnitActor& Unit, const ALDEnemyActor& Enemy) const;
	FLDMatchContext MatchContext;
	TMap<uint64, FUnitAttackState> Units;
	TMap<uint64, TWeakObjectPtr<ALDEnemyActor>> Enemies;
	double InitialAttackDelaySeconds = 0;
	double LastAdvanceSeconds = -1;
	uint64 NextDamageEventId = 1;
	bool bInitialized = false;
	bool bStopped = false;
	bool bAdvancing = false;
};
