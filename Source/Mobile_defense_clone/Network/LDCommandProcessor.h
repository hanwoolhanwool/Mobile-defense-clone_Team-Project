#pragma once

#include "CoreMinimal.h"
#include "Battle/LDCombatEvents.h"
#include "Data/LDMatchTypes.h"
#include "Network/LDCommandTypes.h"
#include "UObject/Object.h"
#include "LDCommandProcessor.generated.h"

struct FLDGameRules;
class ULDBoardManager;
class ULDEconomyService;

/** Server-owned admission, replay cache and atomic board/economy coordination. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDCommandProcessor : public UObject
{
	GENERATED_BODY()

public:
	bool Initialize(const FLDMatchContext& Context, const FLDGameRules& Rules);
	bool RegisterParticipant(const FLDParticipantContext& Context);
	bool BindServices(ULDBoardManager& Board, ULDEconomyService& Economy);
	void EnqueueCombatReward(const FLDCombatDeath& Death);
	void DrainCombatRewards();
	void SetAcceptingCommands(bool bAccept);
	void Close();
	FLDCommandResult Submit(const FLDParticipantContext& Context, const FLDCommand& Command);
	// Deterministic monotonic clock entrypoint for admission tests and the server owner.
	FLDCommandResult SubmitAtTime(const FLDParticipantContext& Context, const FLDCommand& Command, double NowSeconds);
	bool CanSendResponse(const FLDParticipantContext& Context, double NowSeconds);
	int32 GetCachedResultCount(int32 PlayerIndex) const;

private:
	struct FCachedCommand
	{
		FLDCommand Command;
		FLDCommandResult Result;
	};
	struct FSession
	{
		FLDParticipantContext Context;
		TMap<uint32, FCachedCommand> Results;
		TArray<uint32> Order;
		uint32 HighestAdmittedRequestId = 0;
		double Tokens = 0;
		double LastSeconds = 0;
		double ResponseTokens = 0;
		double LastResponseSeconds = 0;
	};

	FSession* FindSession(const FLDParticipantContext& Context);
	bool ConsumeToken(double& Tokens, double& LastSeconds, double NowSeconds) const;
	void CacheResult(FSession& Session, const FLDCommand& Command, const FLDCommandResult& Result);
	void ExecuteCommand(const FLDParticipantContext& Context, const FLDCommand& Command, double ServerSeconds,
	                    FLDCommandResult& Result);
	UPROPERTY()
	TObjectPtr<ULDBoardManager> BoardManager;
	UPROPERTY()
	TObjectPtr<ULDEconomyService> EconomyService;
	TArray<FLDCombatDeath> RewardQueue;
	TSet<uint64> QueuedDeathIds;
	TOptional<FLDCommand> ExecutingCommand;
	int32 ExecutingPlayer = INDEX_NONE;
	uint64 NextEventId = 1;
	bool bProcessing = false;
	FLDMatchContext MatchContext;
	TMap<int32, FSession> Sessions;
	int32 CacheCapacity = 256;
	double RatePerSecond = 8;
	double Burst = 12;
	bool bInitialized = false;
	bool bAcceptingCommands = false;
	bool bClosed = false;
};
