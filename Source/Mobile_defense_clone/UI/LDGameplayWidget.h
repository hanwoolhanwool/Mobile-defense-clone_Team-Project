#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Network/LDCommandTypes.h"
#include "LDGameplayWidget.generated.h"

class UButton;
class UCanvasPanel;
class UTextBlock;

/** P0 actions only. State is read from the owner Controller; clicks submit intents without local purchases. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDGameplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	bool IsOverAction(const FVector2D& ScreenPixels) const;
	void ShowRejected();
	bool GetActionScreenRect(ELDCommandType Type, FBox2D& OutRect) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaSeconds) override;

private:
	UTextBlock* AddText(FName Name);
	UButton* AddAction(FName Name, const FText& Text, UTextBlock*& OutLabel);
	void Place(UWidget* Widget, const FVector4& Rect, double Scale, double OffsetX);
	UFUNCTION()
	void OnSummon();
	UFUNCTION()
	void OnMerge();
	UFUNCTION()
	void OnSell();
	UPROPERTY()
	TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY()
	TObjectPtr<UTextBlock> Selection;
	UPROPERTY()
	TObjectPtr<UTextBlock> Resources;
	UPROPERTY()
	TObjectPtr<UTextBlock> Feedback;
	UPROPERTY()
	TObjectPtr<UButton> Summon;
	UPROPERTY()
	TObjectPtr<UButton> Merge;
	UPROPERTY()
	TObjectPtr<UButton> Sell;
	UPROPERTY()
	TObjectPtr<UTextBlock> SummonText;
	UPROPERTY()
	TObjectPtr<UTextBlock> MergeText;
	UPROPERTY()
	TObjectPtr<UTextBlock> SellText;
	TArray<FBox2D> ActionRects;
	double RejectedUntil = 0;
};
