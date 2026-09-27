#pragma once

#include "CoreMinimal.h"
#include "Board/LDBoardTypes.h"
#include "Data/LDBattleTypes.h"
#include "Economy/LDEconomyTypes.h"
#include "Network/LDCommandTypes.h"
#include "GameFramework/Actor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "LDG3ProbeSubsystem.generated.h"

class ALDGameMode;
class ALDPlayerController;
class ULDGameplayWidget;
class ULDCombatService;
class ULDBoardManager;
struct FLDDamageEvent;

// Only the explicit Development G3 probe spawns this owner-only evidence channel.
// It cannot buy units, alter HP, choose results, or change any product state.
UCLASS()
class ALDG3ProbePeer : public AActor
{
	GENERATED_BODY()
public:
	ALDG3ProbePeer();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated)
	FLDBattleSnapshot FinalBattle;
	UPROPERTY(Replicated)
	TArray<FLDBoardSnapshot> FinalBoards;
	UPROPERTY(Replicated)
	TArray<FLDEconomySnapshot> FinalEconomies;
	UPROPERTY(Replicated)
	TArray<int64> EffectiveDamageByPlayer;
	UPROPERTY(Replicated)
	TArray<double> FirstMergeAtByPlayer;
	UPROPERTY(Replicated)
	bool bTerminalCaptured = false;
	UPROPERTY(Replicated)
	bool bFinishSuite = false;
	UPROPERTY(Replicated)
	bool bMayReturn = false;
	bool bAcknowledged = false;
	UFUNCTION(Server, Reliable)
	void ServerAcknowledge(FGuid MatchId, int32 Revision);
	UFUNCTION(Server, Unreliable)
	void ServerPing(int32 Serial, double SentAt);
	UFUNCTION(Client, Unreliable)
	void ClientPong(int32 Serial, double SentAt);
	TArray<double> RoundTripsMs;
	int32 PingsSent = 0;
	int32 PongsReceived = 0;
};

UCLASS()
class ULDG3ProbeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
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
	void BeginMatch(ALDGameMode* Mode, const FLDBattleSnapshot& Battle);
	void TickAuthority(ALDGameMode& Mode);
	void TickLocal(ALDPlayerController& Controller, const FLDBattleSnapshot& Battle);
	void PlayAction(ALDPlayerController& Controller);
	bool Click(const FBox2D& Rect);
	void Check(const FString& Name, bool bPass, const FString& Detail = TEXT(""));
	void RecordMatch(ALDPlayerController& Controller, ALDG3ProbePeer& Peer);
	void Finish();
	void WriteProgress();
	void HandleDamage(const FLDDamageEvent& Event, int32 PlayerIndex, int32 EffectiveDamage);
	void HandleBoardCommit(const FLDBoardCommit& Commit);
	FString OutputDirectory;
	FString Role;
	FString PeerAddress;
	int32 RequestedMatches = 1;
	int32 CompletedMatches = 0;
	int32 MinimumSeconds = 0;
	int32 TimeoutSeconds = 1500;
	int32 RecoverAfterSeconds = 0;
	bool bNetworkRecovered = false;
	TWeakObjectPtr<class UNetDriver> RecoveredDriver;
	double StartedAt = 0;
	double LastActionAt = 0;
	double LastPingAt = 0;
	double LastProgressAt = 0;
	double TerminalAt = 0;
	double EntryAt = 0;
	double FinishAt = 0;
	double ConnectedGameplaySeconds = 0;
	double LastTickAt = 0;
	double CaptureWaveAt = 0;
	double LastReplayAt = 0;
	int32 ReplaysSent = 0;
	FLDCommand FirstSummon;
	bool bReplayAfterChange = false;
	bool bReplayAfterTerminal = false;
	int64 EffectiveDamage[2] = {0, 0};
	double FirstMergeAt[2] = {-1, -1};
	int32 LocalPlayer = INDEX_NONE;
	int32 LastWave = -1;
	int32 SuccessfulCommands = 0;
	int32 FailedCommands = 0;
	int32 DuplicateRequests = 0;
	int32 HUDRecreations = 0;
	uint32 LastResultId = 0;
	bool bFailed = false;
	bool bFinished = false;
	bool bRecordedTerminal = false;
	bool bReturned = false;
	bool bSold = false;
	bool bMoved = false;
	bool bFirstMatch = true;
	FGuid CurrentMatch;
	TWeakObjectPtr<UWorld> CurrentWorld;
	TWeakObjectPtr<ALDG3ProbePeer> LocalPeer;
	TArray<TWeakObjectPtr<ALDG3ProbePeer>> ServerPeers;
	TWeakObjectPtr<ULDGameplayWidget> RemovedHUD;
	bool bWaitingForNewHUD = false;
	FDelegateHandle DamageHandle;
	FDelegateHandle BoardHandle;
	TWeakObjectPtr<ULDCombatService> ObservedCombat;
	TWeakObjectPtr<ULDBoardManager> ObservedBoard;
	TArray<float> FrameMilliseconds;
	TArray<TSharedPtr<class FJsonValue>> Checks;
	TArray<TSharedPtr<class FJsonValue>> Matches;
	TArray<TSharedPtr<class FJsonValue>> Samples;
};
