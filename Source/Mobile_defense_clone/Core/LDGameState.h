#pragma once

#include "CoreMinimal.h"
#include "Data/LDMatchTypes.h"
#include "GameFramework/GameStateBase.h"
#include "LDGameState.generated.h"

UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	bool InitializeMatch(const FLDMatchContext& Context);
	bool SetPhase(ELDMatchPhase NewPhase);
	ELDMatchPhase GetPhase() const;
	const FLDMatchContext& GetMatchContext() const;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	FLDMatchContext MatchContext;
	UPROPERTY(Replicated)
	ELDMatchPhase Phase = ELDMatchPhase::Loading;
};
