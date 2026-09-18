#pragma once

#include "CoreMinimal.h"
#include "Battle/LDCombatEvents.h"
#include "Data/LDGameData.h"
#include "Data/LDMatchTypes.h"
#include "Economy/LDEconomyTypes.h"
#include "Network/LDCommandTypes.h"
#include "UObject/Object.h"
#include "LDEconomyService.generated.h"

struct FLDEconomyPlan
{
	FLDEconomySnapshot Before;
	FLDEconomySnapshot After;
	FRandomStream NextRandom;
	int32 ExpectedRandomSeed = 0;
	FName ResultUnitId = NAME_None;
	bool bChangesEconomy = false;
};

/** Server source for private balances and randomness. Preparation never changes a participant. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDEconomyService : public UObject
{
	GENERATED_BODY()

public:
	bool Initialize(const FLDMatchContext& Context, ULDGameData& Data, int32 Seed);
	ELDCommandResultCode TryPrepare(int32 PlayerIndex, const FLDCommand& Command, FName SourceUnitId,
	                                FLDEconomyPlan& OutPlan) const;
	bool ValidatePrepared(const FLDEconomyPlan& Plan) const;
	void CommitPrepared(const FLDEconomyPlan& Plan);
	void PublishPrepared(const FLDEconomyPlan& Plan);
	bool ApplyCombatReward(const FLDCombatDeath& Death);
	FLDEconomySnapshot GetSnapshot(int32 PlayerIndex) const;
	int32 GetRandomState(int32 PlayerIndex) const;
	void Close();
	FLDEconomyChanged OnEconomyChanged;

private:
	FName DrawUnit(FName Grade, FRandomStream& Random) const;
	UPROPERTY()
	TObjectPtr<ULDGameData> GameData;
	FLDMatchContext MatchContext;
	TArray<FLDEconomySnapshot> States;
	TArray<FRandomStream> RandomStreams;
	TSet<uint64> RewardedDeaths;
	bool bClosed = false;
};
