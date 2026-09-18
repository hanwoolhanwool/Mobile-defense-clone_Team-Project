#include "UI/LDG1BoardWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> ULDG1BoardWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this);
	}
	if (!WidgetTree->RootWidget)
	{
		USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("DeviceSafeZone"));
		SafeCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SafeCanvas"));
		Safe->AddChild(SafeCanvas);
		WidgetTree->RootWidget = Safe;
		Title = AddLabel(TEXT("Title"), 0.125, 26);
		Title->SetText(NSLOCTEXT("LD", "G1Title", "협동 방어"));
		OpponentLabel = AddLabel(TEXT("OpponentBoard"), 0.205, 16);
		OwnLabel = AddLabel(TEXT("OwnBoard"), 0.704, 16);
		SelectionLabel = AddLabel(TEXT("Selection"), 0.755, 20);
		FeedbackLabel = AddLabel(TEXT("Feedback"), 0.81, 16);
		SetViewState(ViewPlayerIndex, ViewSelectedCell, ViewInputResult);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

UTextBlock* ULDG1BoardWidget::AddLabel(FName Name, double NormalizedY, int32 FontSize)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = FontSize;
	Label->SetFont(Font);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.92f, 0.87f)));
	Label->SetJustification(ETextJustify::Center);
	UCanvasPanelSlot* Slot = SafeCanvas->AddChildToCanvas(Label);
	Slot->SetAnchors(FAnchors(0.5f, static_cast<float>(NormalizedY)));
	Slot->SetAlignment(FVector2D(0.5, 0.5));
	Slot->SetAutoSize(true);
	return Label;
}

void ULDG1BoardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!SafeCanvas)
	{
		return;
	}
	const FGeometry Geometry = SafeCanvas->GetCachedGeometry();
	if (Geometry.GetLocalSize().X <= 0 || Geometry.GetLocalSize().Y <= 0)
	{
		return;
	}
	FVector2D PixelMin;
	FVector2D PixelMax;
	FVector2D WidgetPosition;
	USlateBlueprintLibrary::LocalToViewport(this, Geometry, FVector2D::ZeroVector, PixelMin, WidgetPosition);
	USlateBlueprintLibrary::LocalToViewport(this, Geometry, Geometry.GetLocalSize(), PixelMax, WidgetPosition);
	SafeRectPixels = FBox2D(PixelMin, PixelMax);
}

bool ULDG1BoardWidget::TryGetSafeRectPixels(FBox2D& OutRect) const
{
	if (!SafeRectPixels.bIsValid)
	{
		return false;
	}
	OutRect = SafeRectPixels;
	return true;
}

void ULDG1BoardWidget::SetViewState(int32 PlayerIndex, int32 SelectedCell, ELDCellInputResult Result)
{
	ViewPlayerIndex = PlayerIndex;
	ViewSelectedCell = SelectedCell;
	ViewInputResult = Result;
	if (!SelectionLabel)
	{
		return;
	}
	if (PlayerIndex >= 0 && PlayerIndex <= 1)
	{
		OwnLabel->SetText(FText::Format(NSLOCTEXT("LD", "G1OwnBoard", "내 보드 · 플레이어 {0}"), PlayerIndex + 1));
		OpponentLabel->SetText(
		    FText::Format(NSLOCTEXT("LD", "G1OtherBoard", "상대 보드 · 플레이어 {0}"), 2 - PlayerIndex));
	}
	if (SelectedCell >= 0)
	{
		const int32 Column = 5 - SelectedCell % 6;
		const int32 Row = PlayerIndex == 0 ? 2 - (SelectedCell % 18) / 6 : (SelectedCell % 18) / 6;
		SelectionLabel->SetText(
		    FText::Format(NSLOCTEXT("LD", "G1Selection", "선택한 칸 · {0}열 {1}행"), Column + 1, Row + 1));
	}
	else
	{
		SelectionLabel->SetText(NSLOCTEXT("LD", "G1SelectionHint", "아래 내 보드의 칸을 눌러 주세요"));
	}
	if (Result == ELDCellInputResult::NotOwner)
	{
		FeedbackLabel->SetText(NSLOCTEXT("LD", "G1NotOwner", "상대 보드는 조작할 수 없습니다"));
	}
	else if (Result == ELDCellInputResult::NotReady)
	{
		FeedbackLabel->SetText(NSLOCTEXT("LD", "G1Preparing", "전장을 준비하고 있습니다"));
	}
	else if (Result == ELDCellInputResult::OutsideBoard || Result == ELDCellInputResult::InvalidProjection)
	{
		FeedbackLabel->SetText(NSLOCTEXT("LD", "G1Outside", "내 보드 안의 칸을 선택해 주세요"));
	}
	else
	{
		FeedbackLabel->SetText(NSLOCTEXT("LD", "G1Ready", "왼쪽 입구 → 중앙 길 → 각자의 보드 둘레"));
	}
}
