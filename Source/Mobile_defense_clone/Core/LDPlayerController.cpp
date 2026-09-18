#include "Core/LDPlayerController.h"

#include "Board/LDBoardPresentation.h"
#include "Board/LDViewTransform.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "Core/LDPlayerState.h"
#include "Data/LDGameData.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"
#include "Network/LDCommandProcessor.h"
#include "UI/LDG1BoardWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDBoardInput, Log, All);

void ALDPlayerController::InitializeServerSession(const FLDParticipantContext& Context, ULDCommandProcessor& Processor)
{
	if (!HasAuthority() || !Context.IsValid())
	{
		return;
	}
	ServerContext = Context;
	CommandProcessor = &Processor;
	if (ConnectionEpoch != Context.ConnectionEpoch || CurrentMatchId != Context.MatchId)
	{
		CurrentMatchId = Context.MatchId;
		ConnectionEpoch = Context.ConnectionEpoch;
		OnRep_ConnectionEpoch();
	}
	ForceNetUpdate();
}

void ALDPlayerController::ShutdownServerSession()
{
	CommandProcessor = nullptr;
	ServerContext = {};
	CurrentMatchId.Invalidate();
	ConnectionEpoch = 0;
	OnRep_ConnectionEpoch();
	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

void ALDPlayerController::OnRep_ConnectionEpoch()
{
	// Never replay an uncertain request into a different server connection epoch.
	PendingCommand.Reset();
	NextRequestId = 1;
	LastResult = {};
	ReleaseLocalBoard();
}

bool ALDPlayerController::SubmitLocalCommand(FLDCommand Command)
{
	if (!IsLocalController() || !CurrentMatchId.IsValid() || ConnectionEpoch == 0 || PendingCommand.IsSet() ||
	    NextRequestId == MAX_uint32)
	{
		return false;
	}
	Command.ConnectionEpoch = ConnectionEpoch;
	Command.RequestId = NextRequestId;
	if (!Command.IsValidPayload())
	{
		return false;
	}
	++NextRequestId;
	PendingCommand = Command;
	ServerRequestCommand(Command);
	return true;
}

bool ALDPlayerController::RetryPendingCommand()
{
	if (!IsLocalController() || !PendingCommand.IsSet())
	{
		return false;
	}
	ServerRequestCommand(PendingCommand.GetValue());
	return true;
}

FLDCommandResult ALDPlayerController::SubmitServerCommand(const FLDCommand& Command)
{
	if (!HasAuthority() || !CommandProcessor || !ServerContext.IsValid())
	{
		FLDCommandResult Result;
		Result.MatchId = CurrentMatchId;
		Result.ConnectionEpoch = Command.ConnectionEpoch;
		Result.RequestId = Command.RequestId;
		Result.ResultCode = ELDCommandResultCode::PhaseNotAllowed;
		return Result;
	}
	return CommandProcessor->Submit(ServerContext, Command);
}

void ALDPlayerController::ServerRequestCommand_Implementation(const FLDCommand& Command)
{
	const FLDCommandResult Result = SubmitServerCommand(Command);
	// A flood may delay a response, but must never replace an already cached outcome.
	if (!CommandProcessor || CommandProcessor->CanSendResponse(ServerContext, FPlatformTime::Seconds()))
	{
		ClientCommandResult(Result);
	}
}

void ALDPlayerController::ClientCommandResult_Implementation(const FLDCommandResult& Result)
{
	if (Result.MatchId != CurrentMatchId || Result.ConnectionEpoch != ConnectionEpoch || !PendingCommand.IsSet() ||
	    Result.RequestId != PendingCommand->RequestId)
	{
		return;
	}
	LastResult = Result;
	if (Result.ResultCode != ELDCommandResultCode::Pending)
	{
		PendingCommand.Reset();
	}
	OnCommandCompleted.Broadcast(Result);
}

bool ALDPlayerController::HasPendingCommand() const
{
	return PendingCommand.IsSet();
}

const FLDCommandResult& ALDPlayerController::GetLastResult() const
{
	return LastResult;
}

void ALDPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownServerSession();
	OnCommandCompleted.Clear();
	OnLocalViewReady.Clear();
	Super::EndPlay(EndPlayReason);
}

void ALDPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ALDPlayerController, CurrentMatchId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ALDPlayerController, ConnectionEpoch, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ALDPlayerController, BoardSnapshot, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ALDPlayerController, EconomySnapshot, COND_OwnerOnly);
}

void ALDPlayerController::PublishSnapshots(const FLDBoardSnapshot& Board, const FLDEconomySnapshot& Economy)
{
	if (!HasAuthority() || Board.MatchId != ServerContext.MatchId || Economy.MatchId != ServerContext.MatchId ||
	    Board.PlayerIndex != ServerContext.PlayerIndex || Economy.PlayerIndex != ServerContext.PlayerIndex)
	{
		return;
	}
	BoardSnapshot = Board;
	EconomySnapshot = Economy;
	OnRep_GameplaySnapshot();
	ForceNetUpdate();
}

const FLDBoardSnapshot& ALDPlayerController::GetBoardSnapshot() const
{
	return BoardSnapshot;
}

const FLDEconomySnapshot& ALDPlayerController::GetEconomySnapshot() const
{
	return EconomySnapshot;
}

bool ALDPlayerController::IsGameplaySnapshotReady() const
{
	return CurrentMatchId.IsValid() && ConnectionEpoch != 0 && LocalParticipantIndex != INDEX_NONE &&
	       BoardSnapshot.MatchId == CurrentMatchId && EconomySnapshot.MatchId == CurrentMatchId &&
	       BoardSnapshot.PlayerIndex == LocalParticipantIndex && EconomySnapshot.PlayerIndex == LocalParticipantIndex;
}

void ALDPlayerController::OnRep_GameplaySnapshot()
{
	// Snapshot arrival order is independent from session and local presentation initialization.
	// UI observes the combined readiness predicate; it never creates a replacement source state.
}

void ALDPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}

void ALDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this,
		                        &ALDPlayerController::HandleBoardMousePressed);
		InputComponent->BindTouch(IE_Pressed, this, &ALDPlayerController::HandleBoardTouchPressed);
	}
}

void ALDPlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);
	if (!IsLocalController())
	{
		return;
	}
	TryInitializeLocalBoard();
	RefreshBoardViewport();
}

void ALDPlayerController::TryInitializeLocalBoard()
{
	if (LocalBoard || bLocalInitializationFailed || !CurrentMatchId.IsValid() || ConnectionEpoch == 0)
	{
		return;
	}
	const ALDPlayerState* State = GetPlayerState<ALDPlayerState>();
	const int32 Index = State ? State->GetPlayerIndex() : INDEX_NONE;
	int32 Width = 0;
	int32 Height = 0;
	GetViewportSize(Width, Height);
	if (Index < 0 || Index > 1 || Width <= 0 || Height <= 0)
	{
		return;
	}
	LocalGameData = NewObject<ULDGameData>(this);
	FString Error;
	FLDBoardGeometry Geometry;
	if (!LocalGameData->LoadP0(Error) || !Geometry.Initialize(LocalGameData->GetRules(), Error))
	{
		bLocalInitializationFailed = true;
		UE_LOG(LogLDBoardInput, Error, TEXT("Local board data unavailable: %s"), *Error);
		return;
	}
	LocalBoard = GetWorld()->SpawnActor<ALDBoardPresentation>();
	if (!LocalBoard || !LocalBoard->Initialize(Geometry, Index, Error))
	{
		if (LocalBoard)
		{
			LocalBoard->Destroy();
			LocalBoard = nullptr;
		}
		bLocalInitializationFailed = true;
		UE_LOG(LogLDBoardInput, Error, TEXT("Local board presentation unavailable: %s"), *Error);
		return;
	}
	LocalParticipantIndex = Index;
	LocalBoardWidget = CreateWidget<ULDG1BoardWidget>(this, ULDG1BoardWidget::StaticClass());
	if (LocalBoardWidget)
	{
		LocalBoardWidget->AddToViewport(10);
	}
	SetViewTarget(LocalBoard);
	LastCellInputResult = ELDCellInputResult::None;
	RefreshBoardViewport();
	PublishCellFeedback();
	OnLocalViewReady.Broadcast(Index);
	UE_LOG(LogLDBoardInput, Display, TEXT("Local view ready: player=%d canonical cells=36"), Index);
}

void ALDPlayerController::RefreshBoardViewport()
{
	if (!LocalBoard)
	{
		return;
	}
	int32 Width = 0;
	int32 Height = 0;
	GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0)
	{
		return;
	}
	const FVector2D ViewportSize(Width, Height);
	FBox2D SafeRect(FVector2D::ZeroVector, ViewportSize);
	if (LocalBoardWidget)
	{
		FBox2D Measured;
		if (LocalBoardWidget->TryGetSafeRectPixels(Measured))
		{
			// Slate rounding can cross the viewport boundary by a fraction of a pixel.
			SafeRect = FBox2D(FVector2D(FMath::Clamp(Measured.Min.X, 0.0, double(Width)),
			                            FMath::Clamp(Measured.Min.Y, 0.0, double(Height))),
			                  FVector2D(FMath::Clamp(Measured.Max.X, 0.0, double(Width)),
			                            FMath::Clamp(Measured.Max.Y, 0.0, double(Height))));
		}
	}
	if (BoardViewport.ViewportSizePixels.Equals(ViewportSize, 0.01) && BoardViewport.SafeRectPixels.bIsValid &&
	    BoardViewport.SafeRectPixels.Min.Equals(SafeRect.Min, 0.25) &&
	    BoardViewport.SafeRectPixels.Max.Equals(SafeRect.Max, 0.25))
	{
		return;
	}
	FLDBoardViewportLayout NextLayout;
	if (NextLayout.Initialize(ViewportSize, SafeRect, LocalBoard->GetGeometry().GetFieldSizeCm()))
	{
		BoardViewport = NextLayout;
		LocalBoard->ApplyViewportLayout(BoardViewport);
		if (PlayerCameraManager)
		{
			PlayerCameraManager->UpdateCamera(0);
		}
	}
}

bool ALDPlayerController::IsLocalBoardReady() const
{
	return IsLocalController() && LocalBoard && LocalParticipantIndex >= 0 && BoardViewport.OrthoWidthCm > 0;
}

int32 ALDPlayerController::GetLocalParticipantIndex() const
{
	return LocalParticipantIndex;
}

bool ALDPlayerController::InputScreenPosition(const FVector2D& ScreenPixels)
{
	LastHitCellId = INDEX_NONE;
	LastCellInputResult = ELDCellInputResult::NotReady;
	if (!IsLocalBoardReady())
	{
		PublishCellFeedback();
		return false;
	}
	if (!BoardViewport.IsInsideField(ScreenPixels))
	{
		LastCellInputResult = ELDCellInputResult::OutsideBoard;
		PublishCellFeedback();
		return false;
	}
	FVector Origin;
	FVector Direction;
	if (!DeprojectScreenPositionToWorld(ScreenPixels.X, ScreenPixels.Y, Origin, Direction) ||
	    FMath::Abs(Direction.Z) < 0.0001)
	{
		LastCellInputResult = ELDCellInputResult::InvalidProjection;
		PublishCellFeedback();
		return false;
	}
	const double Distance = -Origin.Z / Direction.Z;
	if (Distance < 0)
	{
		LastCellInputResult = ELDCellInputResult::InvalidProjection;
		PublishCellFeedback();
		return false;
	}
	const FVector Canonical = FLDViewTransform::ToCanonical(Origin + Direction * Distance, LocalParticipantIndex);
	LocalBoard->GetGeometry().TryGetCellAtCanonicalPosition(Canonical, LastHitCellId);
	LastCellInputResult = LocalBoard->GetGeometry().ValidateSelection(LocalParticipantIndex, LastHitCellId);
	if (LastCellInputResult == ELDCellInputResult::Selected)
	{
		SelectedCellId = LastHitCellId;
		LocalBoard->SetSelectedCell(SelectedCellId);
	}
	PublishCellFeedback();
	return LastCellInputResult == ELDCellInputResult::Selected;
}

bool ALDPlayerController::ProjectCellToScreen(int32 CellId, FVector2D& OutScreenPixels) const
{
	FVector Canonical;
	return IsLocalBoardReady() && LocalBoard->GetGeometry().TryGetCellCenter(CellId, Canonical) &&
	       ProjectWorldLocationToScreen(FLDViewTransform::ToPresentation(Canonical, LocalParticipantIndex),
	                                    OutScreenPixels);
}

int32 ALDPlayerController::GetSelectedCellId() const
{
	return SelectedCellId;
}

int32 ALDPlayerController::GetLastHitCellId() const
{
	return LastHitCellId;
}

ELDCellInputResult ALDPlayerController::GetLastCellInputResult() const
{
	return LastCellInputResult;
}

const FLDBoardViewportLayout& ALDPlayerController::GetBoardViewportLayout() const
{
	return BoardViewport;
}

void ALDPlayerController::HandleBoardMousePressed()
{
	if (FPlatformTime::Seconds() - LastTouchSeconds < 0.15)
	{
		return;
	}
	float X = 0;
	float Y = 0;
	if (GetMousePosition(X, Y))
	{
		InputScreenPosition(FVector2D(X, Y));
	}
}

void ALDPlayerController::HandleBoardTouchPressed(ETouchIndex::Type FingerIndex, FVector ScreenPosition)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		LastTouchSeconds = FPlatformTime::Seconds();
		InputScreenPosition(FVector2D(ScreenPosition.X, ScreenPosition.Y));
	}
}

void ALDPlayerController::PublishCellFeedback()
{
	if (LocalBoardWidget)
	{
		LocalBoardWidget->SetViewState(LocalParticipantIndex, SelectedCellId, LastCellInputResult);
	}
}

void ALDPlayerController::ReleaseLocalBoard()
{
	if (LocalBoardWidget)
	{
		LocalBoardWidget->RemoveFromParent();
		LocalBoardWidget = nullptr;
	}
	if (IsValid(LocalBoard))
	{
		LocalBoard->Destroy();
	}
	LocalBoard = nullptr;
	LocalGameData = nullptr;
	LocalParticipantIndex = INDEX_NONE;
	SelectedCellId = INDEX_NONE;
	LastHitCellId = INDEX_NONE;
	LastCellInputResult = ELDCellInputResult::NotReady;
	BoardViewport = {};
	bLocalInitializationFailed = false;
}
