#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LDBattleTypes.h"
#include "LDResultWidget.generated.h"

class UButton;
class UCanvasPanel;
class UTextBlock;
DECLARE_MULTICAST_DELEGATE(FOnLDReturnRequested);

UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDResultWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void UpdateView(const FLDBattleSnapshot& Snapshot, double ServerNow);
	bool GetReturnButtonScreenRect(FBox2D& OutRect) const;
	FOnLDReturnRequested OnReturnRequested;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaSeconds) override;

private:
	UFUNCTION()
	void HandleReturn();
	UPROPERTY()
	TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY()
	TObjectPtr<UTextBlock> ResultText;
	UPROPERTY()
	TObjectPtr<UButton> ReturnButton;
	UPROPERTY()
	TObjectPtr<UTextBlock> ReturnText;
};
