#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LDBattleTypes.h"
#include "LDBattleStatusWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDBattleStatusWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void UpdateView(const FLDBattleSnapshot& Snapshot, double ServerNow);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY()
	TObjectPtr<UTextBlock> Status;
	UPROPERTY()
	TObjectPtr<UTextBlock> BossStatus;
};
