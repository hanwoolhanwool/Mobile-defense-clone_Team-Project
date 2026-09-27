#include "Battle/LDUnitActor.h"

#include "Board/LDViewTransform.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ALDUnitActor::ALDUnitActor()
{
	bReplicates = false;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(20);
	PrimaryActorTick.bCanEverTick = true;
	CanonicalRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CanonicalRoot"));
	SetRootComponent(CanonicalRoot);
	PresentationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PresentationMesh"));
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Flat(TEXT("/Game/LD/Materials/M_P0Flat.M_P0Flat"));
	for (UStaticMeshComponent* Mesh : {PresentationMesh.Get(), ProjectileMesh.Get()})
	{
		Mesh->SetupAttachment(CanonicalRoot);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetVisibility(false);
		if (Flat.Succeeded())
		{
			Mesh->SetMaterial(0, Flat.Object);
		}
	}
	PresentationMesh->SetRelativeScale3D(FVector(0.50, 0.50, 0.60));
	ProjectileMesh->SetRelativeScale3D(FVector(0.12));
	if (Cube.Succeeded())
	{
		PresentationMesh->SetStaticMesh(Cube.Object);
	}
	if (Sphere.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(Sphere.Object);
	}
}

void ALDUnitActor::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
	}
	else
	{
		Material = PresentationMesh->CreateDynamicMaterialInstance(0);
		RefreshPresentation();
	}
}

void ALDUnitActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshPresentation();
}

void ALDUnitActor::EndPlay(const EEndPlayReason::Type Reason)
{
	DeactivateCommitted();
	bEnding = true;
	SetActorTickEnabled(false);
	Material = nullptr;
	Super::EndPlay(Reason);
}

void ALDUnitActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALDUnitActor, Placement);
	DOREPLIFETIME(ALDUnitActor, CanonicalPosition);
	DOREPLIFETIME(ALDUnitActor, bCommitted);
	DOREPLIFETIME(ALDUnitActor, AttackPresentation);
	DOREPLIFETIME(ALDUnitActor, RangeCm);
	DOREPLIFETIME(ALDUnitActor, UnitColor);
	DOREPLIFETIME(ALDUnitActor, LastAttackCue);
	DOREPLIFETIME(ALDUnitActor, PresentationSlot);
	DOREPLIFETIME(ALDUnitActor, MovePresentationSeconds);
}

bool ALDUnitActor::InitializePrepared(const FLDPlacedUnit& Unit, const FLDUnitRow& Row, const FTransform& Transform)
{
	if (!HasAuthority() || bEnding || bPrepared || Unit.InstanceId == 0 || Unit.UnitId != Row.UnitId ||
	    Unit.PlayerIndex < 0 || Unit.PlayerIndex > 1 || Unit.CellId < 0 || Unit.CellId >= 36 ||
	    !FMath::IsFinite(Unit.MoveBlockedUntilServerSeconds) || Unit.MoveBlockedUntilServerSeconds < 0 ||
	    !Row.bEnabledInP0 || !FMath::IsFinite(Row.BaseAttack) || Row.BaseAttack <= 0 ||
	    !FMath::IsFinite(Row.AttackIntervalSeconds) || Row.AttackIntervalSeconds <= 0 ||
	    !FMath::IsFinite(Row.RangeCm) || Row.RangeCm <= 0 || Transform.ContainsNaN() ||
	    !PresentationMesh->GetStaticMesh() || !PresentationMesh->GetMaterial(0) || !ProjectileMesh->GetStaticMesh())
	{
		return false;
	}
	Placement = Unit;
	UnitRow = Row;
	AttackPresentation = Row.AttackPresentation;
	RangeCm = Row.RangeCm;
	CanonicalPosition = Transform.GetLocation();
	SetActorLocation(CanonicalPosition);
	// Author the grade palette in sRGB, like the board; material parameters require linear values.
	const FColor GradeTint =
	    Row.Grade == TEXT("Legendary") ? FColor(242, 176, 36)
	                                   : Row.Grade == TEXT("Epic") ? FColor(160, 70, 224)
	                                                               : Row.Grade == TEXT("Rare") ? FColor(26, 166, 213)
	                                                                                           : FColor(142, 153, 170);
	UnitColor = FLinearColor::FromSRGBColor(GradeTint);
	bPrepared = true;
	return true;
}

void ALDUnitActor::ApplyCommittedPlacement(const FLDPlacedUnit& Unit, const FTransform& Transform)
{
	// The board has completed all fallible preparation and revalidation before calling this publication method.
	if (!HasAuthority() || !bPrepared || bEnding || Unit.InstanceId != Placement.InstanceId ||
	    Unit.UnitId != Placement.UnitId || Unit.PlayerIndex != Placement.PlayerIndex)
	{
		return;
	}
	Placement = Unit;
	CanonicalPosition = Transform.GetLocation();
	SetActorLocation(CanonicalPosition);
	bCommitted = true;
	SetReplicates(true);
	RefreshPresentation();
	ForceNetUpdate();
}

void ALDUnitActor::DeactivateCommitted()
{
	if (HasAuthority())
	{
		bCommitted = false;
		LastAttackCue = {};
		ForceNetUpdate();
	}
	PresentationMesh->SetVisibility(false);
	ProjectileMesh->SetVisibility(false);
}

bool ALDUnitActor::SetLocalViewPlayerIndex(int32 PlayerIndex)
{
	if (bEnding || PlayerIndex < 0 || PlayerIndex > 1)
	{
		return false;
	}
	LocalPlayerIndex = PlayerIndex;
	RefreshPresentation();
	return true;
}

void ALDUnitActor::PresentCommittedAttack(uint64 DamageEventId, const FVector& TargetCanonical, double ServerSeconds)
{
	if (!HasAuthority() || !IsCommitted() || DamageEventId == 0 || DamageEventId <= LastAttackCue.DamageEventId)
	{
		return;
	}
	LastAttackCue.DamageEventId = DamageEventId;
	LastAttackCue.TargetCanonical = TargetCanonical;
	LastAttackCue.ServerSeconds = ServerSeconds;
	ForceNetUpdate();
	RefreshPresentation();
}

void ALDUnitActor::SetPresentationSlot(int32 SlotIndex, double VisualMoveSeconds)
{
	if (!HasAuthority() || bEnding || SlotIndex < 0 || SlotIndex > 2 || !FMath::IsFinite(VisualMoveSeconds) ||
	    VisualMoveSeconds <= 0)
	{
		return;
	}
	PresentationSlot = SlotIndex;
	MovePresentationSeconds = VisualMoveSeconds;
	ForceNetUpdate();
	RefreshPresentation();
}

const FLDPlacedUnit& ALDUnitActor::GetPlacement() const
{
	return Placement;
}
const FLDUnitRow& ALDUnitActor::GetUnitRow() const
{
	return UnitRow;
}
bool ALDUnitActor::IsCommitted() const
{
	return bCommitted && !bEnding && !IsActorBeingDestroyed();
}
double ALDUnitActor::GetRangeCm() const
{
	return RangeCm;
}
FVector ALDUnitActor::GetPresentationLocation() const
{
	return PresentationMesh->GetComponentLocation();
}
const FLDUnitAttackCue& ALDUnitActor::GetLastAttackCue() const
{
	return LastAttackCue;
}

void ALDUnitActor::OnRep_Placement()
{
	SetActorLocation(CanonicalPosition);
	RefreshPresentation();
}

double ALDUnitActor::GetPresentationSeconds() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* State = World ? World->GetGameState() : nullptr;
	return State ? State->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0);
}

void ALDUnitActor::RefreshPresentation()
{
	const bool bVisible = IsCommitted() && LocalPlayerIndex != INDEX_NONE;
	PresentationMesh->SetVisibility(bVisible);
	ProjectileMesh->SetVisibility(false);
	if (!bVisible)
	{
		return;
	}
	const double Now = GetPresentationSeconds();
	if (!bVisualInitialized)
	{
		VisualCanonical = VisualMoveTarget = VisualMoveStart = CanonicalPosition;
		bVisualInitialized = true;
	}
	if (VisualMoveTarget != CanonicalPosition)
	{
		VisualMoveStart = VisualCanonical;
		VisualMoveTarget = CanonicalPosition;
		VisualMoveStartedSeconds = Now;
	}
	const double Fraction = FMath::Clamp((Now - VisualMoveStartedSeconds) / MovePresentationSeconds, 0.0, 1.0);
	VisualCanonical = FMath::Lerp(VisualMoveStart, VisualMoveTarget, Fraction);
	const FVector Slots[] = {FVector(-28, 0, 0), FVector(28, 0, 0), FVector(0, 50, 0)};
	const FVector Origin = FLDViewTransform::ToPresentation(
	                           VisualCanonical + Slots[FMath::Clamp(PresentationSlot, 0, 2)], LocalPlayerIndex) +
	                       FVector(0, 0, 35);
	const FVector Target =
	    FLDViewTransform::ToPresentation(LastAttackCue.TargetCanonical, LocalPlayerIndex) + FVector(0, 0, 35);
	const double Age = Now - LastAttackCue.ServerSeconds;
	FVector UnitPosition = Origin;
	// Cosmetic durations do not delay authoritative damage and never have a completion gameplay callback.
	if (LastAttackCue.DamageEventId != 0 && Age >= 0 && Age < 0.18)
	{
		if (AttackPresentation == TEXT("Projectile"))
		{
			ProjectileMesh->SetWorldLocation(FMath::Lerp(Origin, Target, Age / 0.18));
			ProjectileMesh->SetVisibility(true);
		}
		else
		{
			UnitPosition += (Target - Origin).GetSafeNormal2D() * (20 * FMath::Sin(PI * Age / 0.18));
		}
	}
	PresentationMesh->SetWorldLocation(UnitPosition);
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), UnitColor);
	}
}
