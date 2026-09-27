#include "Core/LDPlayerController.h"

#include "Board/LDBoardPresentation.h"
#include "Board/LDViewTransform.h"
#include "Battle/LDUnitActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InputComponent.h"
#include "Core/LDPlayerState.h"
#include "Core/LDGameInstance.h"
#include "Core/LDGameState.h"
#include "Data/LDGameData.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"
#include "Network/LDCommandProcessor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundBase.h"
#include "UI/LDG1BoardWidget.h"
#include "UI/LDGameplayWidget.h"
#include "UI/LDBattleStatusWidget.h"
#include "UI/LDResultWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDBoardInput, Log, All);

namespace
{
	void TraceCommandResponse(const TCHAR* Direction, const FLDCommandResult& Result)
	{
#if !UE_BUILD_SHIPPING
		static const bool bEnabled = FParse::Param(FCommandLine::Get(), TEXT("P0CommandTrace"));
		if (!bEnabled)
			return;
		auto Ids = [](const TArray<uint64>& Values)
		{
			FString Text;
			for (uint64 Id : Values)
				Text += FString::Printf(TEXT("%llu,"), Id);
			return Text.IsEmpty() ? FString(TEXT("-")) : Text;
		};
		UE_LOG(LogLDBoardInput, Display,
		       TEXT("P0WIRE %s match=%s epoch=%llu id=%u code=%d board=%d economy=%d event=%llu created=%s moved=%s removed=%s"),
		            Direction, *Result.MatchId.ToString(), Result.ConnectionEpoch, Result.RequestId,
		            static_cast<int32>(Result.ResultCode), Result.NewBoardRevision, Result.EconomyRevision,
		            Result.EventId, *Ids(Result.CreatedInstanceIds), *Ids(Result.MovedInstanceIds),
		            *Ids(Result.RemovedInstanceIds));
#endif
	}
} // namespace

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
	bAwaitingCommittedSnapshot = false;
	RetryCount = 0;
	bEntryReturnRequested = false;
	ReleaseLocalBoard();
}

bool ALDPlayerController::SubmitLocalCommand(FLDCommand Command)
{
	const ALDGameState* State = GetWorld() ? GetWorld()->GetGameState<ALDGameState>() : nullptr;
	if (!IsLocalController() || !CurrentMatchId.IsValid() || ConnectionEpoch == 0 || HasPendingCommand() ||
	    NextRequestId == MAX_uint32 || (State && State->GetBattleSnapshot().IsTerminal()))
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
	LastCellInputResult = ELDCellInputResult::None;
	PendingCommand = Command;
	LastRequestSeconds = FPlatformTime::Seconds();
	RetryCount = 0;
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
	TraceCommandResponse(TEXT("SERVER"), Result);
	// A flood may delay a response, but must never replace an already cached outcome.
	if (!CommandProcessor || CommandProcessor->CanSendResponse(ServerContext, FPlatformTime::Seconds()))
	{
		ClientCommandResult(Result);
	}
}

void ALDPlayerController::ClientCommandResult_Implementation(const FLDCommandResult& Result)
{
	// Trace arrival before the normal stale/pending guard; diagnostics do not publish UI events.
	TraceCommandResponse(TEXT("CLIENT"), Result);
	if (Result.MatchId != CurrentMatchId || Result.ConnectionEpoch != ConnectionEpoch || !PendingCommand.IsSet() ||
	    Result.RequestId != PendingCommand->RequestId)
	{
		return;
	}
	LastResult = Result;
	if (Result.ResultCode != ELDCommandResultCode::Pending)
	{
		PendingCommand.Reset();
		bAwaitingCommittedSnapshot = Result.ResultCode == ELDCommandResultCode::Success ||
		                             Result.ResultCode == ELDCommandResultCode::RequestExpired;
		if (Result.ResultCode != ELDCommandResultCode::Success && Result.ResultCode != ELDCommandResultCode::NoChange)
		{
			if (GameplayWidget)
			{
				GameplayWidget->ShowRejected();
			}
			if (USoundBase* Sound = RejectedSound.LoadSynchronous())
			{
				UGameplayStatics::PlaySound2D(this, Sound);
			}
		}
	}
	UpdateGameplayView();
	OnCommandCompleted.Broadcast(Result);
}

bool ALDPlayerController::HasPendingCommand() const
{
	return PendingCommand.IsSet() || bAwaitingCommittedSnapshot;
}

bool ALDPlayerController::CanRetryPendingCommand() const
{
	return PendingCommand.IsSet() && RetryCount >= 3;
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
	DOREPLIFETIME_CONDITION(ALDPlayerController, GameplaySnapshot, COND_OwnerOnly);
}

void ALDPlayerController::PublishSnapshots(const FLDBoardSnapshot& Board, const FLDEconomySnapshot& Economy)
{
	if (!HasAuthority() || Board.MatchId != ServerContext.MatchId || Economy.MatchId != ServerContext.MatchId ||
	    Board.PlayerIndex != ServerContext.PlayerIndex || Economy.PlayerIndex != ServerContext.PlayerIndex)
	{
		return;
	}
	GameplaySnapshot.ConnectionEpoch = ServerContext.ConnectionEpoch;
	GameplaySnapshot.Board = Board;
	GameplaySnapshot.Economy = Economy;
	OnRep_GameplaySnapshot();
	ForceNetUpdate();
}

const FLDBoardSnapshot& ALDPlayerController::GetBoardSnapshot() const
{
	return GameplaySnapshot.Board;
}

const FLDEconomySnapshot& ALDPlayerController::GetEconomySnapshot() const
{
	return GameplaySnapshot.Economy;
}

bool ALDPlayerController::IsGameplaySnapshotReady() const
{
	const ALDPlayerState* State = GetPlayerState<ALDPlayerState>();
	const FLDParticipantContext Participant = State ? State->GetParticipantContext() : FLDParticipantContext();
	return CurrentMatchId.IsValid() && ConnectionEpoch != 0 && Participant.IsValid() &&
	       Participant.MatchId == CurrentMatchId && Participant.ConnectionEpoch == ConnectionEpoch &&
	       GameplaySnapshot.ConnectionEpoch == ConnectionEpoch && GameplaySnapshot.Board.MatchId == CurrentMatchId &&
	       GameplaySnapshot.Economy.MatchId == CurrentMatchId &&
	       GameplaySnapshot.Board.PlayerIndex == Participant.PlayerIndex &&
	       GameplaySnapshot.Economy.PlayerIndex == Participant.PlayerIndex;
}

void ALDPlayerController::OnRep_GameplaySnapshot()
{
	// Snapshot arrival order is independent from session and local presentation initialization.
	// UI observes the combined readiness predicate; it never creates a replacement source state.
	UpdateGameplayView();
}

bool ALDPlayerController::RequestSummon()
{
	if (!CanUseGameplayActions())
	{
		return false;
	}
	if (CanRetryPendingCommand())
	{
		// An uncertain outcome must retain its original request identity, even after a user retry.
		RetryCount = 0;
		LastRequestSeconds = FPlatformTime::Seconds();
		return RetryPendingCommand();
	}
	if (!IsGameplaySnapshotReady() || !IsLocalBoardReady())
	{
		return false;
	}
	FLDCommand Command;
	Command.ExpectedBoardRevision = GetBoardSnapshot().BoardRevision;
	return SubmitLocalCommand(Command);
}

uint64 ALDPlayerController::GetSelectedInstanceId() const
{
	uint64 Selected = 0;
	for (const FLDPlacedUnit& Unit : GetBoardSnapshot().Units)
	{
		if (Unit.CellId == SelectedCellId && (Selected == 0 || Unit.InstanceId < Selected))
		{
			Selected = Unit.InstanceId;
		}
	}
	return Selected;
}

bool ALDPlayerController::CanMergeSelection() const
{
	int32 Count = 0;
	FName UnitId;
	for (const FLDPlacedUnit& Unit : GetBoardSnapshot().Units)
	{
		if (Unit.CellId == SelectedCellId)
		{
			++Count;
			UnitId = Unit.UnitId;
		}
	}
	FLDUnitRow Row;
	return Count == 3 && LocalGameData && LocalGameData->TryGetUnitRow(UnitId, Row) && Row.Grade != TEXT("Legendary");
}

bool ALDPlayerController::RequestMergeSelection()
{
	if (!CanUseGameplayActions() || !IsLocalBoardReady() || !CanMergeSelection())
	{
		return false;
	}
	TArray<uint64> Ids;
	for (const FLDPlacedUnit& Unit : GetBoardSnapshot().Units)
	{
		if (Unit.CellId == SelectedCellId)
		{
			Ids.Add(Unit.InstanceId);
		}
	}
	Ids.Sort();
	FLDCommand Command;
	Command.CommandType = ELDCommandType::Merge;
	Command.ExpectedBoardRevision = GetBoardSnapshot().BoardRevision;
	Command.InstanceId = Ids[0];
	Command.ConsumedInstanceId0 = Ids[0];
	Command.ConsumedInstanceId1 = Ids[1];
	Command.ConsumedInstanceId2 = Ids[2];
	return SubmitLocalCommand(Command);
}

bool ALDPlayerController::RequestSellSelection()
{
	if (!CanUseGameplayActions() || !IsLocalBoardReady() || GetSelectedInstanceId() == 0)
	{
		return false;
	}
	FLDCommand Command;
	Command.CommandType = ELDCommandType::Sell;
	Command.ExpectedBoardRevision = GetBoardSnapshot().BoardRevision;
	Command.InstanceId = GetSelectedInstanceId();
	return SubmitLocalCommand(Command);
}

bool ALDPlayerController::RequestMove(uint64 InstanceId, int32 DestinationCellId)
{
	if (!CanUseGameplayActions() || !IsLocalBoardReady())
	{
		return false;
	}
	FLDCommand Command;
	Command.CommandType = ELDCommandType::Move;
	Command.ExpectedBoardRevision = GetBoardSnapshot().BoardRevision;
	Command.InstanceId = InstanceId;
	Command.DestinationCellId = DestinationCellId;
	return SubmitLocalCommand(Command);
}

FText ALDPlayerController::GetSelectionText() const
{
	FName UnitId;
	int32 Count = 0;
	for (const FLDPlacedUnit& Unit : GetBoardSnapshot().Units)
	{
		if (Unit.CellId == SelectedCellId)
		{
			UnitId = Unit.UnitId;
			++Count;
		}
	}
	FLDUnitRow Row;
	if (!LocalGameData || !LocalGameData->TryGetUnitRow(UnitId, Row))
	{
		return NSLOCTEXT("LD", "SelectStack", "유닛을 선택하거나 뭉치를 끌어 이동하세요");
	}
	return FText::Format(NSLOCTEXT("LD", "SelectedStack", "{0} × {1}  ·  사거리 {2}칸"),
	                               FText::FromString(Row.DisplayName), Count,
	                               Row.RangeCm / LocalGameData->GetRules().CellSizeCm);
}

FText ALDPlayerController::GetCommandFeedback() const
{
	if (LastCellInputResult == ELDCellInputResult::NotOwner)
	{
		return NSLOCTEXT("LD", "GameplayForeignCell", "상대 보드는 조작할 수 없습니다");
	}
	if (bAwaitingCommittedSnapshot)
	{
		return NSLOCTEXT("LD", "AwaitSnapshot", "최신 보드와 재화를 동기화하고 있습니다");
	}
	if (PendingCommand.IsSet())
	{
		return RetryCount >= 3
		    ? NSLOCTEXT("LD", "RequestUncertain", "응답을 기다리고 있습니다. 아래 버튼으로 같은 요청을 재확인하세요")
		                : NSLOCTEXT("LD", "RequestPending", "요청을 처리하고 있습니다");
	}
	if (LastResult.RequestId == 0)
	{
		return NSLOCTEXT("LD", "GameplayHint", "소환 · 뭉치 이동 · 같은 유닛 3마리 합성 · 한 마리 판매");
	}
	switch (LastResult.ResultCode)
	{
	case ELDCommandResultCode::Success:
		return NSLOCTEXT("LD", "CommandSuccess", "완료했습니다");
	case ELDCommandResultCode::InsufficientResource:
		return NSLOCTEXT("LD", "InsufficientGold", "골드가 부족합니다");
	case ELDCommandResultCode::LimitReached:
		return NSLOCTEXT("LD", "PopulationLimit", "인구 상한에 도달했습니다");
	case ELDCommandResultCode::NotOwner:
		return NSLOCTEXT("LD", "ForeignUnit", "상대 보드는 조작할 수 없습니다");
	case ELDCommandResultCode::Locked:
		return NSLOCTEXT("LD", "MoveLocked", "이동 직후입니다. 잠시 뒤 다시 이동하세요");
	case ELDCommandResultCode::StaleBoard:
	case ELDCommandResultCode::MissingInstance:
		return NSLOCTEXT("LD", "BoardChanged", "보드가 바뀌었습니다. 다시 선택해 주세요");
	case ELDCommandResultCode::NoSpace:
		return NSLOCTEXT("LD", "NoSpace", "소환할 공간이 없습니다");
	case ELDCommandResultCode::RateLimited:
		return NSLOCTEXT("LD", "TooManyCommands", "요청이 너무 빠릅니다. 잠시 뒤 다시 눌러 주세요");
	case ELDCommandResultCode::NoChange:
		return NSLOCTEXT("LD", "NoChange", "같은 칸입니다");
	case ELDCommandResultCode::PhaseNotAllowed:
		return NSLOCTEXT("LD", "MatchClosed", "지금은 조작할 수 없습니다");
	case ELDCommandResultCode::RequestExpired:
		return NSLOCTEXT("LD", "ExpiredRequest", "이전 요청 기록이 만료되었습니다. 최신 상태에서 다시 선택해 주세요");
	default:
		return NSLOCTEXT("LD", "CommandRejected", "요청이 거절되었습니다. 상태를 확인한 뒤 다시 선택해 주세요");
	}
}

void ALDPlayerController::UpdateGameplayView()
{
	if (!IsLocalController())
	{
		return;
	}
	UpdateBattleView();
	if (IsGameplaySnapshotReady() && bAwaitingCommittedSnapshot &&
	    GetBoardSnapshot().BoardRevision >= LastResult.NewBoardRevision &&
	    GetEconomySnapshot().EconomyRevision >= LastResult.EconomyRevision)
	{
		bAwaitingCommittedSnapshot = false;
		if (LastResult.ResultCode == ELDCommandResultCode::RequestExpired)
		{
			SelectedCellId = INDEX_NONE;
			DragSourceInstanceId = 0;
			if (LocalBoard)
			{
				LocalBoard->SetSelectedCell(INDEX_NONE);
			}
		}
	}
	if (GameplayWidget && !GameplayWidget->IsInViewport())
	{
		// A removed UI has already unbound its click delegates in NativeDestruct.
		// Rebuild the view from current replicated snapshots without changing gameplay state.
		GameplayWidget = nullptr;
	}
	if (IsGameplaySnapshotReady() && IsLocalBoardReady() && !GameplayWidget)
	{
		GameplayWidget = CreateWidget<ULDGameplayWidget>(this, ULDGameplayWidget::StaticClass());
		if (GameplayWidget)
		{
			GameplayWidget->AddToViewport(20);
			if (LocalBoardWidget)
			{
				LocalBoardWidget->SetGameplayOverlayVisible(true);
			}
		}
	}
	if (!LocalBoard)
	{
		return;
	}
	const ALDGameState* MatchState = GetWorld()->GetGameState<ALDGameState>();
	if (!IsGameplaySnapshotReady() || (MatchState && MatchState->GetBattleSnapshot().IsTerminal()))
	{
		LocalBoard->SetRangePresentation(FVector::ZeroVector, 0);
		return;
	}
	const uint64 Selected = GetSelectedInstanceId();
	if (Selected == 0)
	{
		LocalBoard->SetRangePresentation(FVector::ZeroVector, 0);
		RangeInstanceId = 0;
		RangeCellId = INDEX_NONE;
	}
	else if (RangeInstanceId != Selected || RangeCellId != SelectedCellId)
	{
		for (TActorIterator<ALDUnitActor> It(GetWorld()); It; ++It)
		{
			if (It->IsCommitted() && It->GetPlacement().InstanceId == Selected &&
			    It->GetPlacement().CellId == SelectedCellId)
			{
				LocalBoard->SetRangePresentation(It->GetActorLocation(), It->GetRangeCm());
				RangeInstanceId = Selected;
				RangeCellId = SelectedCellId;
				break;
			}
		}
	}
}

bool ALDPlayerController::GetActionScreenRect(ELDCommandType Type, FBox2D& OutRect) const
{
	return GameplayWidget && GameplayWidget->GetActionScreenRect(Type, OutRect);
}

bool ALDPlayerController::CanUseGameplayActions() const
{
	const ALDGameState* State = GetWorld() ? GetWorld()->GetGameState<ALDGameState>() : nullptr;
	if (!State || !IsGameplaySnapshotReady() || bEntryReturnRequested)
	{
		return false;
	}
	const FLDBattleSnapshot& Snapshot = State->GetBattleSnapshot();
	return Snapshot.MatchId == CurrentMatchId &&
	       (Snapshot.Phase == ELDMatchPhase::Preparing || Snapshot.Phase == ELDMatchPhase::Running);
}

void ALDPlayerController::UpdateBattleView()
{
#if !UE_BUILD_SHIPPING
	// G1 and G2 are explicit earlier-gate fixtures: preserve their original presentation and assertions.
	static const bool bLegacyProbe = []()
	{
		FString Probe;
		FParse::Value(FCommandLine::Get(), TEXT("P0Probe="), Probe);
		return Probe.Equals(TEXT("G1"), ESearchCase::IgnoreCase) || Probe.Equals(TEXT("G2"), ESearchCase::IgnoreCase);
	}();
	if (bLegacyProbe)
	{
		return;
	}
#endif
	ALDGameState* State = GetWorld() ? GetWorld()->GetGameState<ALDGameState>() : nullptr;
	if (!State)
	{
		return;
	}
	if (BattleStatusWidget && !BattleStatusWidget->IsInViewport())
	{
		BattleStatusWidget = nullptr;
	}
	if (!BattleStatusWidget)
	{
		BattleStatusWidget = CreateWidget<ULDBattleStatusWidget>(this, ULDBattleStatusWidget::StaticClass());
		if (BattleStatusWidget)
		{
			BattleStatusWidget->AddToViewport(30);
		}
	}
	const FLDBattleSnapshot& Snapshot = State->GetBattleSnapshot();
	const double ServerNow = State->GetServerWorldTimeSeconds();
	if (BattleStatusWidget)
	{
		BattleStatusWidget->UpdateView(Snapshot, ServerNow);
	}
	if (LocalBoardWidget)
	{
		LocalBoardWidget->SetBattleOverlayVisible(BattleStatusWidget && BattleStatusWidget->IsInViewport());
	}
	if (ResultWidget && (!ResultWidget->IsInViewport() || !Snapshot.IsTerminal()))
	{
		ResultWidget->OnReturnRequested.RemoveAll(this);
		ResultWidget->RemoveFromParent();
		ResultWidget = nullptr;
	}
	if (!Snapshot.IsTerminal())
	{
		return;
	}
	DragSourceInstanceId = 0;
	if (LocalBoard)
	{
		LocalBoard->SetRangePresentation(FVector::ZeroVector, 0);
	}
	if (!ResultWidget)
	{
		ResultWidget = CreateWidget<ULDResultWidget>(this, ULDResultWidget::StaticClass());
		if (ResultWidget)
		{
			ResultWidget->OnReturnRequested.AddUObject(this, &ALDPlayerController::HandleReturnRequested);
			ResultWidget->AddToViewport(100);
		}
	}
	if (ResultWidget)
	{
		ResultWidget->UpdateView(Snapshot, ServerNow);
	}
}

bool ALDPlayerController::GetReturnButtonScreenRect(FBox2D& OutRect) const
{
	return ResultWidget && ResultWidget->GetReturnButtonScreenRect(OutRect);
}

bool ALDPlayerController::RequestReturnToEntry()
{
	const ALDGameState* State = GetWorld() ? GetWorld()->GetGameState<ALDGameState>() : nullptr;
	ULDGameInstance* Instance = GetGameInstance<ULDGameInstance>();
	if (!IsLocalController() || bEntryReturnRequested || !State || !State->GetBattleSnapshot().IsTerminal() ||
	    !Instance)
	{
		return false;
	}
	bEntryReturnRequested = Instance->RequestEntryReturn(FText::GetEmpty());
	return bEntryReturnRequested;
}

void ALDPlayerController::HandleReturnRequested()
{
	RequestReturnToEntry();
}

void ALDPlayerController::HandleSummonKey()
{
	RequestSummon();
}
void ALDPlayerController::HandleMergeKey()
{
	RequestMergeSelection();
}
void ALDPlayerController::HandleSellKey()
{
	RequestSellSelection();
}

void ALDPlayerController::BeginBoardPointer(const FVector2D& ScreenPixels)
{
	DragSourceInstanceId = 0;
	if (GameplayWidget && GameplayWidget->IsOverAction(ScreenPixels))
	{
		return;
	}
	if (InputScreenPosition(ScreenPixels) && IsGameplaySnapshotReady() && !HasPendingCommand())
	{
		DragSourceInstanceId = GetSelectedInstanceId();
		DragStartPixels = ScreenPixels;
	}
}

void ALDPlayerController::EndBoardPointer(const FVector2D& ScreenPixels)
{
	const uint64 SourceId = DragSourceInstanceId;
	DragSourceInstanceId = 0;
	if (SourceId == 0 || FVector2D::Distance(DragStartPixels, ScreenPixels) < 12 ||
	    (GameplayWidget && GameplayWidget->IsOverAction(ScreenPixels)))
	{
		return;
	}
	if (InputScreenPosition(ScreenPixels))
	{
		RequestMove(SourceId, SelectedCellId);
	}
}

void ALDPlayerController::HandleBoardMouseReleased()
{
	float X = 0;
	float Y = 0;
	if (FPlatformTime::Seconds() - LastTouchSeconds >= .15 && GetMousePosition(X, Y))
	{
		EndBoardPointer(FVector2D(X, Y));
	}
}

void ALDPlayerController::HandleBoardTouchReleased(ETouchIndex::Type FingerIndex, FVector ScreenPosition)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		LastTouchSeconds = FPlatformTime::Seconds();
		EndBoardPointer(FVector2D(ScreenPosition.X, ScreenPosition.Y));
	}
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
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this,
		                        &ALDPlayerController::HandleBoardMouseReleased);
		InputComponent->BindTouch(IE_Released, this, &ALDPlayerController::HandleBoardTouchReleased);
		InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ALDPlayerController::HandleSummonKey);
		InputComponent->BindKey(EKeys::M, IE_Pressed, this, &ALDPlayerController::HandleMergeKey);
		InputComponent->BindKey(EKeys::X, IE_Pressed, this, &ALDPlayerController::HandleSellKey);
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
	UpdateGameplayView();
	if (PendingCommand.IsSet() && RetryCount < 3 && FPlatformTime::Seconds() - LastRequestSeconds >= 1.0)
	{
		++RetryCount;
		LastRequestSeconds = FPlatformTime::Seconds();
		RetryPendingCommand();
	}
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
	const ALDGameState* State = GetWorld() ? GetWorld()->GetGameState<ALDGameState>() : nullptr;
	if (!IsLocalBoardReady() || (State && State->GetBattleSnapshot().IsTerminal()))
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
		BeginBoardPointer(FVector2D(X, Y));
	}
}

void ALDPlayerController::HandleBoardTouchPressed(ETouchIndex::Type FingerIndex, FVector ScreenPosition)
{
	if (FingerIndex == ETouchIndex::Touch1)
	{
		LastTouchSeconds = FPlatformTime::Seconds();
		BeginBoardPointer(FVector2D(ScreenPosition.X, ScreenPosition.Y));
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
	if (ResultWidget)
	{
		ResultWidget->OnReturnRequested.RemoveAll(this);
		ResultWidget->RemoveFromParent();
		ResultWidget = nullptr;
	}
	if (BattleStatusWidget)
	{
		BattleStatusWidget->RemoveFromParent();
		BattleStatusWidget = nullptr;
	}
	if (GameplayWidget)
	{
		GameplayWidget->RemoveFromParent();
		GameplayWidget = nullptr;
	}
	DragSourceInstanceId = 0;
	RangeInstanceId = 0;
	RangeCellId = INDEX_NONE;
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
