#include "Core/LDLocalPresentationSubsystem.h"

#include "Battle/LDEnemyActor.h"
#include "Battle/LDUnitActor.h"
#include "Core/LDPlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

bool ULDLocalPresentationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && World->GetNetMode() != NM_DedicatedServer &&
	       Super::ShouldCreateSubsystem(Outer);
}

void ULDLocalPresentationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(
	    FOnActorSpawned::FDelegate::CreateUObject(this, &ULDLocalPresentationSubsystem::OnActorSpawned));
	FindLocalController();
	InWorld.GetTimerManager().SetTimer(ReadinessTimer, this, &ULDLocalPresentationSubsystem::FindLocalController, 0.1f,
	                                   true);
}

void ULDLocalPresentationSubsystem::FindLocalController()
{
	ALDPlayerController* Controller = Cast<ALDPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!Controller || !Controller->IsLocalController())
	{
		return;
	}
	if (!LocalController.IsValid())
	{
		LocalController = Controller;
		ViewReadyHandle = Controller->OnLocalViewReady.AddUObject(this, &ULDLocalPresentationSubsystem::OnViewReady);
	}
	if (Controller->IsLocalBoardReady())
	{
		OnViewReady(Controller->GetLocalParticipantIndex());
	}
}

void ULDLocalPresentationSubsystem::OnViewReady(int32 PlayerIndex)
{
	LocalPlayerIndex = PlayerIndex;
	for (TActorIterator<ALDEnemyActor> It(GetWorld()); It; ++It)
	{
		It->SetLocalViewPlayerIndex(PlayerIndex);
	}
	for (TActorIterator<ALDUnitActor> It(GetWorld()); It; ++It)
	{
		It->SetLocalViewPlayerIndex(PlayerIndex);
	}
	GetWorld()->GetTimerManager().ClearTimer(ReadinessTimer);
}

void ULDLocalPresentationSubsystem::OnActorSpawned(AActor* Actor)
{
	if (LocalPlayerIndex != INDEX_NONE)
	{
		if (ALDEnemyActor* Enemy = Cast<ALDEnemyActor>(Actor))
		{
			Enemy->SetLocalViewPlayerIndex(LocalPlayerIndex);
		}
		else if (ALDUnitActor* Unit = Cast<ALDUnitActor>(Actor))
		{
			Unit->SetLocalViewPlayerIndex(LocalPlayerIndex);
		}
	}
}

void ULDLocalPresentationSubsystem::Deinitialize()
{
	GetWorld()->GetTimerManager().ClearTimer(ReadinessTimer);
	GetWorld()->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	if (LocalController.IsValid())
	{
		LocalController->OnLocalViewReady.Remove(ViewReadyHandle);
	}
	LocalController.Reset();
	Super::Deinitialize();
}
