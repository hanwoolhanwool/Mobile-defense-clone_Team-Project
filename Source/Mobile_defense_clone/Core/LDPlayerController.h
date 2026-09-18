#pragma once

#include "CoreMinimal.h"
#include "Board/LDBoardGeometry.h"
#include "Data/LDMatchTypes.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Network/LDCommandTypes.h"
#include "LDPlayerController.generated.h"

class ULDCommandProcessor;
class ULDGameData;
class ALDBoardPresentation;
class ULDG1BoardWidget;

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
	bool HasPendingCommand() const;
	const FLDCommandResult& GetLastResult() const;
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
	void TryInitializeLocalBoard();
	void RefreshBoardViewport();
	void ReleaseLocalBoard();
	void HandleBoardMousePressed();
	void HandleBoardTouchPressed(ETouchIndex::Type FingerIndex, FVector ScreenPosition);
	void PublishCellFeedback();
	UPROPERTY()
	TObjectPtr<ULDCommandProcessor> CommandProcessor;
	FLDParticipantContext ServerContext;
	UPROPERTY(ReplicatedUsing = OnRep_ConnectionEpoch)
	FGuid CurrentMatchId;
	UPROPERTY(ReplicatedUsing = OnRep_ConnectionEpoch)
	uint64 ConnectionEpoch = 0;
	uint32 NextRequestId = 1;
	TOptional<FLDCommand> PendingCommand;
	FLDCommandResult LastResult;
	UPROPERTY()
	TObjectPtr<ULDGameData> LocalGameData;
	UPROPERTY()
	TObjectPtr<ALDBoardPresentation> LocalBoard;
	UPROPERTY()
	TObjectPtr<ULDG1BoardWidget> LocalBoardWidget;
	FLDBoardViewportLayout BoardViewport;
	int32 LocalParticipantIndex = INDEX_NONE;
	int32 SelectedCellId = INDEX_NONE;
	int32 LastHitCellId = INDEX_NONE;
	ELDCellInputResult LastCellInputResult = ELDCellInputResult::NotReady;
	double LastTouchSeconds = -1;
	bool bLocalInitializationFailed = false;
};
