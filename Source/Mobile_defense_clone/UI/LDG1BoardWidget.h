#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Board/LDBoardGeometry.h"
#include "LDG1BoardWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

/** G1 labels only. SafeZone supplies measured layout; the widget never edits server board state. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDG1BoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetViewState(int32 PlayerIndex, int32 SelectedCell, ELDCellInputResult Result);
	bool TryGetSafeRectPixels(FBox2D& OutRect) const;
	void SetGameplayOverlayVisible(bool bVisible);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UTextBlock* AddLabel(FName Name, double NormalizedY, int32 FontSize);
	UPROPERTY()
	TObjectPtr<UCanvasPanel> SafeCanvas;
	UPROPERTY()
	TObjectPtr<UTextBlock> Title;
	UPROPERTY()
	TObjectPtr<UTextBlock> OpponentLabel;
	UPROPERTY()
	TObjectPtr<UTextBlock> OwnLabel;
	UPROPERTY()
	TObjectPtr<UTextBlock> SelectionLabel;
	UPROPERTY()
	TObjectPtr<UTextBlock> FeedbackLabel;
	FBox2D SafeRectPixels = FBox2D(ForceInit);
	int32 ViewPlayerIndex = INDEX_NONE;
	int32 ViewSelectedCell = INDEX_NONE;
	ELDCellInputResult ViewInputResult = ELDCellInputResult::NotReady;
};
