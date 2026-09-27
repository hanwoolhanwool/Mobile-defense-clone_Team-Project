#pragma once

#include "CoreMinimal.h"
#include "Board/LDBoardGeometry.h"
#include "Board/LDBoardTypes.h"
#include "Economy/LDEconomyTypes.h"
#include "Data/LDMatchTypes.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Network/LDCommandTypes.h"
#include "LDPlayerController.generated.h"

class ULDCommandProcessor;
class ULDGameData;
class ALDBoardPresentation;
class ULDG1BoardWidget;
class ULDGameplayWidget;
class ULDBattleStatusWidget;
class ULDResultWidget;
class USoundBase;

USTRUCT()
struct FLDOwnerGameplaySnapshot
{
	GENERATED_BODY()
	UPROPERTY()
	uint64 ConnectionEpoch = 0;
	UPROPERTY()
	FLDBoardSnapshot Board;
	UPROPERTY()
	FLDEconomySnapshot Economy;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FLDCommandCompleted, const FLDCommandResult&);
DECLARE_MULTICAST_DELEGATE_OneParam(FLDLocalViewReady, int32);

UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void InitializeServerSession(const FLDParticipantContext& Context, ULDCommandProcessor& Processor);
	void ShutdownServerSession();
	bool SubmitLocalCommand(FLDCommand Command);
	bool RetryPendingCommand();
	bool CanRetryPendingCommand() const;
	bool HasPendingCommand() const;
	const FLDCommandResult& GetLastResult() const;
	void PublishSnapshots(const FLDBoardSnapshot& Board, const FLDEconomySnapshot& Economy);
	const FLDBoardSnapshot& GetBoardSnapshot() const;
	const FLDEconomySnapshot& GetEconomySnapshot() const;
	bool IsGameplaySnapshotReady() const;
	bool CanUseGameplayActions() const;
	bool GetReturnButtonScreenRect(FBox2D& OutRect) const;
	bool RequestReturnToEntry();
	bool RequestSummon();
	bool RequestMergeSelection();
	bool RequestSellSelection();
	bool RequestMove(uint64 InstanceId, int32 DestinationCellId);
	uint64 GetSelectedInstanceId() const;
	bool CanMergeSelection() const;
	FText GetSelectionText() const;
	FText GetCommandFeedback() const;
	bool GetActionScreenRect(ELDCommandType Type, FBox2D& OutRect) const;
	// Shared authority-checked submission boundary used by the owning Controller's Server RPC.
	FLDCommandResult SubmitServerCommand(const FLDCommand& Command);
	FLDCommandCompleted OnCommandCompleted;
	bool IsLocalBoardReady() const;
	int32 GetLocalParticipantIndex() const;
	bool InputScreenPosition(const FVector2D& ScreenPixels);
	bool ProjectCellToScreen(int32 CellId, FVector2D& OutScreenPixels) const;
	int32 GetSelectedCellId() const;
	int32 GetLastHitCellId() const;
	ELDCellInputResult GetLastCellInputResult() const;
	const FLDBoardViewportLayout& GetBoardViewportLayout() const;
	FLDLocalViewReady OnLocalViewReady;

	UFUNCTION(Server, Reliable)
	void ServerRequestCommand(const FLDCommand& Command);
	UFUNCTION(Client, Reliable)
	void ClientCommandResult(const FLDCommandResult& Result);
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PlayerTick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UFUNCTION()
	void OnRep_ConnectionEpoch();
	UFUNCTION()
	void OnRep_GameplaySnapshot();
	void TryInitializeLocalBoard();
	void RefreshBoardViewport();
	void ReleaseLocalBoard();
	void HandleBoardMousePressed();
	void HandleBoardTouchPressed(ETouchIndex::Type FingerIndex, FVector ScreenPosition);
	void HandleBoardMouseReleased();
	void HandleBoardTouchReleased(ETouchIndex::Type FingerIndex, FVector ScreenPosition);
	void BeginBoardPointer(const FVector2D& ScreenPixels);
	void EndBoardPointer(const FVector2D& ScreenPixels);
	void HandleSummonKey();
	void HandleMergeKey();
	void HandleSellKey();
	void UpdateGameplayView();
	void UpdateBattleView();
	void HandleReturnRequested();
	void PublishCellFeedback();
	UPROPERTY()
	TObjectPtr<ULDCommandProcessor> CommandProcessor;
	FLDParticipantContext ServerContext;
	UPROPERTY(ReplicatedUsing = OnRep_ConnectionEpoch)
	FGuid CurrentMatchId;
	UPROPERTY(ReplicatedUsing = OnRep_ConnectionEpoch)
	uint64 ConnectionEpoch = 0;
	UPROPERTY(ReplicatedUsing = OnRep_GameplaySnapshot)
	FLDOwnerGameplaySnapshot GameplaySnapshot;
	uint32 NextRequestId = 1;
	TOptional<FLDCommand> PendingCommand;
	FLDCommandResult LastResult;
	UPROPERTY()
	TObjectPtr<ULDGameData> LocalGameData;
	UPROPERTY()
	TObjectPtr<ALDBoardPresentation> LocalBoard;
	UPROPERTY()
	TObjectPtr<ULDG1BoardWidget> LocalBoardWidget;
	UPROPERTY()
	TObjectPtr<ULDGameplayWidget> GameplayWidget;
	UPROPERTY()
	TObjectPtr<ULDBattleStatusWidget> BattleStatusWidget;
	UPROPERTY()
	TObjectPtr<ULDResultWidget> ResultWidget;
	bool bEntryReturnRequested = false;
	UPROPERTY()
	TSoftObjectPtr<USoundBase> RejectedSound =
	    TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/LD/Audio/S_P0Rejected.S_P0Rejected")));
	bool bAwaitingCommittedSnapshot = false;
	double LastRequestSeconds = 0;
	int32 RetryCount = 0;
	uint64 DragSourceInstanceId = 0;
	FVector2D DragStartPixels = FVector2D::ZeroVector;
	uint64 RangeInstanceId = 0;
	int32 RangeCellId = INDEX_NONE;
	FLDBoardViewportLayout BoardViewport;
	int32 LocalParticipantIndex = INDEX_NONE;
	int32 SelectedCellId = INDEX_NONE;
	int32 LastHitCellId = INDEX_NONE;
	ELDCellInputResult LastCellInputResult = ELDCellInputResult::NotReady;
	double LastTouchSeconds = -1;
	bool bLocalInitializationFailed = false;
};
