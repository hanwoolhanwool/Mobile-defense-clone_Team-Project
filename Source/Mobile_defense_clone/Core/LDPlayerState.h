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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool InitializeParticipant(const FLDParticipantContext& Context);
	const FLDParticipantContext& GetParticipantContext() const;
	int32 GetPlayerIndex() const;

private:
	UPROPERTY(Replicated)
	FLDParticipantContext ParticipantContext;

	UPROPERTY(Replicated)
	int32 PublicPlayerIndex = INDEX_NONE;
};
