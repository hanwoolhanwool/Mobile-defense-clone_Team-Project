#pragma once

#include "CoreMinimal.h"
#include "Data/LDBattleTypes.h"
#include "GameFramework/GameStateBase.h"
#include "LDGameState.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnLDMatchStateChanged);

UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALDGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool InitializeMatch(const FLDMatchContext& Context);
	bool SetPhase(ELDMatchPhase NewPhase);
	void SetReadinessReason(const FString& Reason);
	const FLDMatchContext& GetMatchContext() const;
	ELDMatchPhase GetPhase() const;
	const FString& GetReadinessReason() const;
	const FLDBattleSnapshot& GetBattleSnapshot() const;
	bool UpdateBattle(const FLDBattleSnapshot& Snapshot);
	bool FinalizeResult(ELDMatchResult Result, ELDResultReason Reason, double ServerSeconds);
	FOnLDMatchStateChanged OnMatchStateChanged;

private:
	UFUNCTION()
	void OnRep_CommonState();

	UPROPERTY(ReplicatedUsing = OnRep_CommonState)
	FLDMatchContext MatchContext;

	UPROPERTY(ReplicatedUsing = OnRep_CommonState)
	FLDBattleSnapshot BattleSnapshot;

	UPROPERTY(ReplicatedUsing = OnRep_CommonState)
	FString ReadinessReason = TEXT("Loading required P0 data");
};
