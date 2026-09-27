#pragma once

#include "CoreMinimal.h"
#include "Board/LDBoardTypes.h"
#include "Data/LDBattleTypes.h"
#include "Economy/LDEconomyTypes.h"
#include "GameFramework/Actor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "LDG3BoundaryProbeSubsystem.generated.h"

// Observation-only owner channel for the explicitly selected Development boundary fixture.
UCLASS()
class ALDG3BoundaryPeer : public AActor
{
	GENERATED_BODY()
public:
	ALDG3BoundaryPeer();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated)
	int32 CaseIndex = 0;
	UPROPERTY(Replicated)
	bool bFixtureReady = false;
	UPROPERTY(Replicated)
	bool bNormalWaitObserved = false;
	UPROPERTY(Replicated)
	bool bTerminalCaptured = false;
	UPROPERTY(Replicated)
	bool bMayReturn = false;
	UPROPERTY(Replicated)
	double Deadline = 0;
	UPROPERTY(Replicated)
	TArray<double> DueTimes;
	UPROPERTY(Replicated)
	FLDBattleSnapshot FinalBattle;
	UPROPERTY(Replicated)
	TArray<FLDBoardSnapshot> FinalBoards;
	UPROPERTY(Replicated)
	TArray<FLDEconomySnapshot> FinalEconomies;
	UPROPERTY(Replicated)
	TArray<FLDBossSnapshot> ActualBosses;
	UPROPERTY(Replicated)
	TArray<uint64> FinalNormalIds;
	UPROPERTY(Replicated)
	bool bPriorPayloadChecked = false;
	UPROPERTY(Replicated)
	bool bPriorPayloadUnchanged = false;
	int32 InitialRandom = 0;
	bool bReadyObserved = false;
	bool bTerminalObserved = false;
	UFUNCTION(Server, Reliable)
	void ServerObserve(FGuid MatchId, int32 Stage, int32 Revision, bool bPass);
};

struct FLDG3BoundaryState;

// No subsystem exists without -P0Probe=G3Boundary. It never substitutes for natural-play validation.
UCLASS()
class ULDG3BoundaryProbeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

private:
	TSharedPtr<FLDG3BoundaryState> State;
};
