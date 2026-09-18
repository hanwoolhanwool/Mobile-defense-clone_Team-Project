#pragma once

#include "CoreMinimal.h"
#include "Data/LDMatchTypes.h"
#include "GameFramework/PlayerState.h"
#include "LDPlayerState.generated.h"

UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	bool InitializeParticipant(const FLDParticipantContext& Context);
	const FLDParticipantContext& GetParticipantContext() const;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	FLDParticipantContext ParticipantContext;
};
