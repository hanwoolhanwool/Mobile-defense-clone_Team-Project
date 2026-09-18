#pragma once

#include "CoreMinimal.h"
#include "Board/LDBoardTypes.h"
#include "Economy/LDEconomyTypes.h"
#include "GameFramework/Actor.h"
#include "Network/LDCommandTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "LDG2ProbeSubsystem.generated.h"

class ALDGameMode;
class ALDPlayerController;
class ALDEnemyActor;
class ALDUnitActor;
class ULDGameplayWidget;

// Read-only fixture coordination, spawned only by the explicit non-Shipping G2 probe.
// It never accepts client RPCs and is not a gameplay state owner.
UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDG2ProbeState : public AActor
{
	GENERATED_BODY()
public:
	ALDG2ProbeState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated)
	int32 Stage = -1;
	UPROPERTY(Replicated)
	int32 ActingPlayer = 0;
	UPROPERTY(Replicated)
	FLDCommand Command;
	UPROPERTY(Replicated)
	bool bCheckpoint = false;
	UPROPERTY(Replicated)
	TArray<FLDBoardSnapshot> Boards;
	UPROPERTY(Replicated)
	TArray<FLDEconomySnapshot> Economies;
};

UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDG2ProbeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	void BeginStage(ALDGameMode& Mode, int32 Stage);
	void TickAuthority(ALDGameMode& Mode);
	void TickLocal(ALDPlayerController& Controller);
	bool ClickAction(ALDPlayerController& Controller, ELDCommandType Type);
	void Checkpoint(ALDGameMode& Mode);
	void Check(const FString& Name, bool bPass, const FString& Detail = TEXT(""));
	void Finish();
	ALDEnemyActor* SpawnEnemy(ALDGameMode& Mode, double HP);
	FString OutputDirectory;
	double CreatedAt = 0;
	double StageStartedAt = 0;
	double CheckpointAt = -1;
	int32 LocalStage = -1;
	int32 InspectedStage = -1;
	int32 LocalPlayer = INDEX_NONE;
	int32 BeforeCacheCount = 0;
	int32 FarmSpawned = 0;
	int32 FarmDead = 0;
	uint64 NextEnemyId = 1000;
	int32 RNG[2] = {0, 0};
	FLDBoardSnapshot BeforeBoards[2];
	FLDEconomySnapshot BeforeEconomies[2];
	TMap<uint64, double> AttackTimes;
	TMap<uint64, TWeakObjectPtr<ALDUnitActor>> UnitIdentities;
	FLDCommand LastSent;
	FLDCommand LastMerge;
	FLDCommandResult LastMergeResult;
	TSet<int32> ResultStages;
	uint32 PreviousResultId = 0;
	bool bWaitingResult = false;
	bool bActionPending = false;
	double LocalActionAt = 0;
	int32 DragPhase = 0;
	double DragStepAt = 0;
	FVector2D DragDestination = FVector2D::ZeroVector;
	bool bFailed = false;
	bool bFinished = false;
	TWeakObjectPtr<ALDG2ProbeState> State;
	TWeakObjectPtr<ALDEnemyActor> FirstEnemy;
	TWeakObjectPtr<ULDGameplayWidget> RemovedWidget;
	bool bWidgetRecreated = false;
	FDelegateHandle DuplicateDeathHandle;
	TArray<TWeakObjectPtr<ALDEnemyActor>> FarmEnemies;
	TArray<TSharedPtr<class FJsonValue>> Checks;
	TSet<int32> InspectedStages;
	TArray<float> FrameMilliseconds;
};
