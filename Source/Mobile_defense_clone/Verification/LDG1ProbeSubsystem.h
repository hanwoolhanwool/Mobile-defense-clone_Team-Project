#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LDG1ProbeSubsystem.generated.h"

class ALDEnemyActor;
class ALDPlayerController;
class FJsonObject;

// Explicit Development-only fixture. Normal matches never create this subsystem.
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDG1ProbeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	bool StartRoutes();
	void InspectRoutes(ALDPlayerController& Controller);
	void InspectViewport(ALDPlayerController& Controller, int32 AspectIndex);
	void PumpTouchInput(ALDPlayerController& Controller);
	void Check(const FString& Name, bool bPassed, const FString& Detail);
	void Finish();
	FString OutputDirectory;
	double CreatedAt = 0;
	double ReadyAt = -1;
	double RouteStartedAt = -1;
	double NextRouteStep = 0;
	double NextRouteSample = 0;
	double FinishAt = -1;
	int32 ResizeStage = -1;
	int32 InspectedStage = -1;
	int32 LocalPlayerIndex = INDEX_NONE;
	int32 TouchCell = INDEX_NONE;
	int32 TouchPhase = 0;
	int32 TouchExpectedSelection = INDEX_NONE;
	int32 TouchAspect = INDEX_NONE;
	FVector2D TouchPosition = FVector2D::ZeroVector;
	bool bFinished = false;
	bool bFailed = false;
	bool bRoutesStarted = false;
	TArray<TWeakObjectPtr<ALDEnemyActor>> ServerRoutes;
	TMap<uint64, TWeakObjectPtr<ALDEnemyActor>> ObservedActors;
	TSet<FString> RecordedChecks;
	TArray<TSharedPtr<class FJsonValue>> Checks;
	TArray<TSharedPtr<class FJsonValue>> Samples;
	TArray<TSharedPtr<class FJsonValue>> Views;
	TArray<float> FrameMilliseconds;
};
