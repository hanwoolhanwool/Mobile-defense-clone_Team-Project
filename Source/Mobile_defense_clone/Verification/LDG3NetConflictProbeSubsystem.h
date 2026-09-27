#pragma once

#include "CoreMinimal.h"
#include "Board/LDBoardTypes.h"
#include "Economy/LDEconomyTypes.h"
#include "GameFramework/Actor.h"
#include "Network/LDCommandTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "LDG3NetConflictProbeSubsystem.generated.h"

class ALDGameMode;
class ALDPlayerController;
class ULDBoardManager;

/** Explicit Development fixture evidence only; these RPCs cannot mutate product gameplay state. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDG3NetConflictPeer : public AActor
{
	GENERATED_BODY()
public:
	ALDG3NetConflictPeer();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated)
	int32 Phase = 1;
	UPROPERTY(Replicated)
	FGuid MatchId;
	UPROPERTY(Replicated)
	FLDBoardSnapshot BeforeBoard;
	UPROPERTY(Replicated)
	FLDEconomySnapshot BeforeEconomy;
	UPROPERTY(Replicated)
	FLDBoardSnapshot AfterBoard;
	UPROPERTY(Replicated)
	FLDEconomySnapshot AfterEconomy;
	UPROPERTY(Replicated)
	FLDCommand First;
	UPROPERTY(Replicated)
	FLDCommand Second;
	UPROPERTY(Replicated)
	TArray<FLDCommandResult> ServerResponses;
	UPROPERTY(Replicated)
	bool bServerPassed = false;
	UPROPERTY(Replicated)
	bool bServerCleaned = false;
	UFUNCTION(Server, Reliable)
	void ServerReportObservation(int32 ObservedPhase, FGuid ObservedMatch, int32 BoardRevision, bool bPass);
	bool bClientReady = false;
	bool bClientVerified = false;
	bool bClientCleaned = false;
	bool bClientFinished = false;
	bool bClientPassed = true;
};

/** -P0Probe=G3NetConflict only. A real remote RPC burst, not natural play or UI Pending verification. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDG3NetConflictProbeSubsystem : public UGameInstanceSubsystem,
                                                                public FTickableGameObject
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
	bool PrepareFixture(ALDGameMode& Mode, ALDPlayerController& Remote);
	bool InstallObserver(ALDPlayerController& Controller);
	bool ObserveEvent(AActor* Actor, UFunction* Function, void* Parameters);
	void OnBoardCommitted(const FLDBoardCommit& Commit);
	void TickAuthority(ALDGameMode& Mode);
	void TickClient(ALDPlayerController& Controller);
	void VerifyServer(ALDGameMode& Mode);
	void VerifyClient(ALDPlayerController& Controller);
	bool ActorSetMatches(const FLDBoardSnapshot& Board) const;
	bool RestoreObservers();
	void Check(const FString& Name, bool bPass, const FString& Detail = TEXT(""));
	void FailAndExit(const FString& Reason);
	bool WriteResult(bool bHandshake);
	TWeakObjectPtr<ALDG3NetConflictPeer> Peer;
	TWeakObjectPtr<ALDPlayerController> ObservedController;
	TWeakObjectPtr<ULDBoardManager> ObservedBoard;
	TWeakObjectPtr<UWorld> MatchWorld;
	FDelegateHandle EventHandle;
	FDelegateHandle BoardHandle;
	TArray<FLDCommand> Requests;
	TArray<FLDCommandResult> Responses;
	TArray<uint64> RequestFrames;
	TArray<FLDBoardCommit> MergeCommits;
	TArray<TSharedPtr<class FJsonValue>> Checks;
	TArray<TSharedPtr<class FJsonValue>> Wire;
	FLDBoardSnapshot HostBeforeBoard;
	FLDEconomySnapshot HostBeforeEconomy;
	FString OutputDirectory;
	FGuid MatchId;
	FName ExpectedRare;
	int32 RngBefore = 0;
	int32 ExpectedRngAfter = 0;
	int32 HostRngBefore = 0;
	int32 LocalPlayerIndex = INDEX_NONE;
	int32 TimeoutSeconds = 60;
	int32 LocalPhase = 0;
	uint64 FirstSendFrame = 0;
	uint64 SecondSendFrame = 0;
	double CreatedAt = 0;
	double ExitAt = 0;
	double BurstSentAt = 0;
	bool bActivated = false;
	bool bFixturePrepared = false;
	bool bEventObserverInstalled = false;
	bool bObserversRestored = true;
	bool bFailed = false;
	bool bFinished = false;
	bool bWritten = false;
	bool bOutputAvailable = false;
};
