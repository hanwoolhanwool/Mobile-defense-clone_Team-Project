#include "Battle/LDEnemyActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ALDEnemyActor::ALDEnemyActor()
{
	// Initial-only route geometry is published only after initialization has prepared every field.
	bReplicates = false;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(20);
	SetMinNetUpdateFrequency(10);
	PrimaryActorTick.bCanEverTick = true;
	CanonicalRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CanonicalRoot"));
	SetRootComponent(CanonicalRoot);
	PresentationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PresentationMesh"));
	PresentationMesh->SetupAttachment(CanonicalRoot);
	PresentationMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PresentationMesh->SetGenerateOverlapEvents(false);
	PresentationMesh->SetRelativeScale3D(FVector(0.45));
	PresentationMesh->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		PresentationMesh->SetStaticMesh(Sphere.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/LD/Materials/M_P0Flat.M_P0Flat"));
	if (Material.Succeeded())
	{
		PresentationMesh->SetMaterial(0, Material.Object);
	}
}

void ALDEnemyActor::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
	}
	else
	{
		PresentationMaterial = PresentationMesh->CreateDynamicMaterialInstance(0);
		RefreshColor();
		RefreshPresentation(GetPresentationServerSeconds());
	}
}

void ALDEnemyActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// This tick presents a server sample; it never advances the authoritative route clock.
	RefreshPresentation(GetPresentationServerSeconds());
}

void ALDEnemyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopRoute();
	bEnding = true;
	SetActorTickEnabled(false);
	PresentationMesh->SetVisibility(false);
	PresentationMaterial = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ALDEnemyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ALDEnemyActor, RoutePoints, COND_InitialOnly);
	DOREPLIFETIME(ALDEnemyActor, RouteSnapshot);
}

bool ALDEnemyActor::InitializeRoute(const FGuid& MatchId, uint64 EnemyId, int32 RouteIndex,
                                    const TArray<FVector>& Points, double SpeedCmPerSecond, double StartServerSeconds)
{
	if (!HasAuthority() || bEnding || bRouteStopped || !MatchId.IsValid() || EnemyId == 0 || RouteIndex < 0 ||
	    RouteIndex > 1 || !FMath::IsFinite(SpeedCmPerSecond) || SpeedCmPerSecond <= 0 ||
	    !FMath::IsFinite(StartServerSeconds) || StartServerSeconds < 0)
	{
		return false;
	}
	if (RouteSnapshot.EnemyId != 0)
	{
		return RouteSnapshot.MatchId == MatchId && RouteSnapshot.EnemyId == EnemyId &&
		       RouteSnapshot.RouteIndex == RouteIndex && RouteSnapshot.SpeedCmPerSecond == SpeedCmPerSecond &&
		       InitialServerSeconds == StartServerSeconds && RoutePoints == Points;
	}
	FLDRouteModel CandidateModel;
	FString Error;
	if (!CandidateModel.TryInitialize(Points, Error))
	{
		return false;
	}
	RouteModel = MoveTemp(CandidateModel);
	RoutePoints = Points;
	InitialServerSeconds = StartServerSeconds;
	RouteSnapshot.MatchId = MatchId;
	RouteSnapshot.EnemyId = EnemyId;
	RouteSnapshot.RouteIndex = RouteIndex;
	RouteSnapshot.TotalDistanceCm = 0;
	RouteSnapshot.SampleServerSeconds = StartServerSeconds;
	RouteSnapshot.SpeedCmPerSecond = SpeedCmPerSecond;
	RouteSnapshot.bActive = true;
	SetReplicates(true);
	ApplyCanonicalSnapshot();
	RefreshColor();
	ForceNetUpdate();
	return true;
}

bool ALDEnemyActor::AdvanceRouteTo(double ServerSeconds)
{
	if (!HasAuthority() || bEnding || !RouteSnapshot.bActive || !FMath::IsFinite(ServerSeconds) ||
	    ServerSeconds < RouteSnapshot.SampleServerSeconds)
	{
		return false;
	}
	if (ServerSeconds == RouteSnapshot.SampleServerSeconds)
	{
		return true;
	}
	const double CandidateDistance = (ServerSeconds - InitialServerSeconds) * RouteSnapshot.SpeedCmPerSecond;
	FVector Position;
	FVector Tangent;
	uint64 Lap = 0;
	if (!RouteModel.TrySample(CandidateDistance, Position, Tangent, Lap))
	{
		return false;
	}
	RouteSnapshot.TotalDistanceCm = CandidateDistance;
	RouteSnapshot.SampleServerSeconds = ServerSeconds;
	SetActorLocation(Position);
	RefreshPresentation(ServerSeconds);
	return true;
}

void ALDEnemyActor::StopRoute()
{
	bRouteStopped = true;
	if (HasAuthority() && RouteSnapshot.bActive)
	{
		RouteSnapshot.bActive = false;
		ForceNetUpdate();
	}
	// Keep the final position visible until the caller destroys the actor; stopping is not a death event.
	RefreshPresentation(RouteSnapshot.SampleServerSeconds);
}

bool ALDEnemyActor::SetLocalViewPlayerIndex(int32 PlayerIndex)
{
	if (bEnding || PlayerIndex < 0 || PlayerIndex > 1)
	{
		return false;
	}
	LocalViewPlayerIndex = PlayerIndex;
	RefreshPresentation(GetPresentationServerSeconds());
	return true;
}

bool ALDEnemyActor::RefreshPresentation(double ViewServerSeconds)
{
	if (bEnding || LocalViewPlayerIndex == INDEX_NONE || RouteSnapshot.EnemyId == 0 || !RouteModel.IsInitialized())
	{
		PresentationMesh->SetVisibility(false);
		return false;
	}
	double DisplayDistance = 0;
	FVector RawPosition;
	FVector RawTangent;
	uint64 Lap = 0;
	if (!FLDRouteModel::TryPredictPresentationDistance(RouteSnapshot.TotalDistanceCm, RouteSnapshot.SampleServerSeconds,
	                                                   RouteSnapshot.SpeedCmPerSecond, RouteSnapshot.bActive,
	                                                   ViewServerSeconds, DisplayDistance) ||
	    !RouteModel.TrySample(DisplayDistance, RawPosition, RawTangent, Lap))
	{
		return false;
	}
	// Identity/Y-reflection fixture until the shared stateless view transform is integrated.
	const double YSign = LocalViewPlayerIndex == 1 ? -1.0 : 1.0;
	const FVector DisplayPosition(RawPosition.X, RawPosition.Y * YSign, RawPosition.Z + PresentationHeightCm);
	const FVector DisplayTangent(RawTangent.X, RawTangent.Y * YSign, RawTangent.Z);
	PresentationMesh->SetWorldLocationAndRotation(DisplayPosition, DisplayTangent.Rotation());
	PresentationMesh->SetVisibility(true);
	return true;
}

const FLDEnemyRouteSnapshot& ALDEnemyActor::GetRouteSnapshot() const
{
	return RouteSnapshot;
}

FVector ALDEnemyActor::GetPresentationLocation() const
{
	return PresentationMesh->GetComponentLocation();
}

bool ALDEnemyActor::IsPresentationVisible() const
{
	return PresentationMesh->IsVisible();
}

void ALDEnemyActor::OnRep_RoutePoints()
{
	FString Error;
	if (RouteModel.TryInitialize(RoutePoints, Error))
	{
		ApplyCanonicalSnapshot();
	}
	else
	{
		PresentationMesh->SetVisibility(false);
	}
}

void ALDEnemyActor::OnRep_RouteSnapshot()
{
	ApplyCanonicalSnapshot();
	RefreshColor();
}

void ALDEnemyActor::ApplyCanonicalSnapshot()
{
	FVector Position;
	FVector Tangent;
	uint64 Lap = 0;
	if (!bEnding && RouteSnapshot.EnemyId != 0 &&
	    RouteModel.TrySample(RouteSnapshot.TotalDistanceCm, Position, Tangent, Lap))
	{
		SetActorLocation(Position);
		RefreshPresentation(GetPresentationServerSeconds());
	}
}

void ALDEnemyActor::RefreshColor()
{
	if (PresentationMaterial)
	{
		const FLinearColor Color =
		    RouteSnapshot.RouteIndex == 0 ? FLinearColor(1.0f, 0.35f, 0.08f) : FLinearColor(0.05f, 0.7f, 1.0f);
		PresentationMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

double ALDEnemyActor::GetPresentationServerSeconds() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* State = World->GetGameState())
		{
			return State->GetServerWorldTimeSeconds();
		}
		if (HasAuthority())
		{
			return World->GetTimeSeconds();
		}
	}
	return RouteSnapshot.SampleServerSeconds;
}
