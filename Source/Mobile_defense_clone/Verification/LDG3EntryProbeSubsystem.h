#pragma once

#include "CoreMinimal.h"
#include "Data/LDBattleTypes.h"
#include "GameFramework/Actor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "LDG3EntryProbeSubsystem.generated.h"

class ALDEntryPlayerController;
class ALDGameMode;
class ALDPlayerController;
class ULDResultWidget;
class UNetDriver;

// Explicit Development fixture: carries an authoritative observation and acknowledges its display only.
UCLASS()
class ALDG3EntryProbePeer : public AActor
{
	GENERATED_BODY()
public:
	ALDG3EntryProbePeer();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated)
	FLDBattleSnapshot TimeoutBattle;
	UFUNCTION(Server, Reliable)
	void ServerObserveTerminal(FGuid MatchId, int32 Revision, bool bPass);
	bool bAcknowledged = false;
	bool bClientPassed = false;
};

// -P0Probe=G3Entry only. Never changes match clocks, outcomes, registrations or connection errors.
UCLASS()
class ULDG3EntryProbeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
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
	void TickEntry(ALDEntryPlayerController& Controller, double Now);
	void TickMatch(ALDPlayerController& Controller, const FLDBattleSnapshot& Battle, double Now);
	void TickReturned(ALDEntryPlayerController& Controller, double Now);
	void ObserveNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
	bool Click(const FBox2D& Rect);
	bool InspectEntry(ALDEntryPlayerController& Controller, bool bReturned);
	bool InspectTerminal(ALDPlayerController& Controller, const FLDBattleSnapshot& Battle);
	void ObservePhase(const FLDBattleSnapshot& Battle);
	void Check(const FString& Name, bool bPass, const FString& Detail = TEXT(""));
	void Capture(const FString& Name);
	void WriteProgress(const FString& Stage);
	bool ReadPeerStage(const FString& Filename, const FString& Stage) const;
	void Finish();
	void Fail(const FString& Reason);

	FString OutputDirectory;
	FString PeerOutputDirectory;
	FString Role;
	FString RunId;
	FString PeerAddress;
	FString CurrentStage = TEXT("initializing");
	double StartedAt = 0;
	double EntryAt = 0;
	double MatchObservedAt = 0;
	double TerminalAt = 0;
	double TerminalVerifiedAt = 0;
	double PeerAcknowledgedAt = 0;
	double ReturnedAt = 0;
	double LastProgressAt = 0;
	double TimeoutSeconds = 180;
	double EntryIdleSeconds = 0;
	int32 LastPhase = INDEX_NONE;
	int32 LastRevision = INDEX_NONE;
	int32 NetworkFailures = 0;
	int32 ReturnClicks = 0;
	bool bOutputReady = false;
	bool bFailed = false;
	bool bFinished = false;
	bool bIdleVerified = false;
	bool bTravelClicked = false;
	bool bTerminalVerified = false;
	bool bReturnClicked = false;
	bool bReturnedVerified = false;
	bool bSawLoading = false;
	bool bSawNetworkFailureInMatch = false;
	FLDBattleSnapshot TimeoutBattle;
	TWeakObjectPtr<UWorld> OldMatchWorld;
	TWeakObjectPtr<ALDPlayerController> OldController;
	TWeakObjectPtr<ULDResultWidget> OldResult;
	TWeakObjectPtr<ALDG3EntryProbePeer> Peer;
	FDelegateHandle NetworkFailureHandle;
	TArray<TSharedPtr<class FJsonValue>> Checks;
	TArray<TSharedPtr<class FJsonValue>> PhaseObservations;
	TArray<TSharedPtr<class FJsonValue>> NetworkObservations;
	TSharedPtr<class FJsonObject> TerminalObservation;
	TSharedPtr<class FJsonObject> CleanupObservation;
};
