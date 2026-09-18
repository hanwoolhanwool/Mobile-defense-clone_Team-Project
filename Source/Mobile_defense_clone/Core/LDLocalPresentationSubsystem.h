#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LDLocalPresentationSubsystem.generated.h"

class ALDPlayerController;

// Local composition boundary: domain actors never discover or call a concrete Controller/widget.
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDLocalPresentationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	void FindLocalController();
	void OnViewReady(int32 PlayerIndex);
	void OnActorSpawned(AActor* Actor);
	TWeakObjectPtr<ALDPlayerController> LocalController;
	FTimerHandle ReadinessTimer;
	FDelegateHandle ViewReadyHandle;
	FDelegateHandle ActorSpawnedHandle;
	int32 LocalPlayerIndex = INDEX_NONE;
};
