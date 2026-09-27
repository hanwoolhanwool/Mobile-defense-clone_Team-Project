#include "UI/LDGameplayWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/TextBlock.h"
#include "Core/LDPlayerController.h"
#include "Engine/World.h"

TSharedRef<SWidget> ULDGameplayWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this);
	}
	if (!WidgetTree->RootWidget)
	{
		USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("GameplaySafeZone"));
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("GameplayCanvas"));
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Safe->AddChild(Canvas);
		WidgetTree->RootWidget = Safe;
		Selection = AddText(TEXT("Selection"));
		Resources = AddText(TEXT("Resources"));
		Feedback = AddText(TEXT("Feedback"));
		UTextBlock* Label = nullptr;
		Summon = AddAction(TEXT("Summon"), NSLOCTEXT("LD", "Summon", "소환"), Label);
		SummonText = Label;
		Merge = AddAction(TEXT("Merge"), NSLOCTEXT("LD", "Merge", "합성"), Label);
		MergeText = Label;
		Sell = AddAction(TEXT("Sell"), NSLOCTEXT("LD", "Sell", "판매"), Label);
		SellText = Label;
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	return Super::RebuildWidget();
}

UTextBlock* ULDGameplayWidget::AddText(FName Name)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Text->SetJustification(ETextJustify::Center);
	Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	Canvas->AddChildToCanvas(Text);
	return Text;
}

UButton* ULDGameplayWidget::AddAction(FName Name, const FText& Text, UTextBlock*& OutLabel)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OutLabel->SetText(Text);
	OutLabel->SetJustification(ETextJustify::Center);
	Button->AddChild(OutLabel);
	Canvas->AddChildToCanvas(Button);
	Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
	return Button;
}

void ULDGameplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Summon->OnClicked.AddUniqueDynamic(this, &ULDGameplayWidget::OnSummon);
	Merge->OnClicked.AddUniqueDynamic(this, &ULDGameplayWidget::OnMerge);
	Sell->OnClicked.AddUniqueDynamic(this, &ULDGameplayWidget::OnSell);
}

void ULDGameplayWidget::NativeDestruct()
{
	if (Summon)
	{
		Summon->OnClicked.RemoveAll(this);
		Merge->OnClicked.RemoveAll(this);
		Sell->OnClicked.RemoveAll(this);
	}
	ActionRects.Reset();
	Super::NativeDestruct();
}

void ULDGameplayWidget::Place(UWidget* Widget, const FVector4& Rect, double Scale, double OffsetX)
{
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
	CanvasSlot->SetPosition(FVector2D(OffsetX + Rect.X * Scale, Rect.Y * Scale));
	CanvasSlot->SetSize(FVector2D(Rect.Z * Scale, Rect.W * Scale));
}

void ULDGameplayWidget::NativeTick(const FGeometry& MyGeometry, float DeltaSeconds)
{
	Super::NativeTick(MyGeometry, DeltaSeconds);
	ALDPlayerController* Controller = Cast<ALDPlayerController>(GetOwningPlayer());
	if (!Controller || !Canvas)
	{
		return;
	}
	const FVector2D Size = Canvas->GetCachedGeometry().GetLocalSize();
	const double Scale = FMath::Min(Size.X / 1080.0, Size.Y / 2340.0);
	const double OffsetX = (Size.X - 1080.0 * Scale) * 0.5;
	if (Scale <= 0)
	{
		return;
	}
	Place(Selection, FVector4(144, 1610, 744, 90), Scale, OffsetX);
	Place(Merge, FVector4(304, 1718, 268, 54), Scale, OffsetX);
	Place(Sell, FVector4(588, 1718, 268, 54), Scale, OffsetX);
	Place(Resources, FVector4(240, 1840, 562, 66), Scale, OffsetX);
	Place(Summon, FVector4(336, 1940, 408, 220), Scale, OffsetX);
	Place(Feedback, FVector4(90, 2180, 900, 95), Scale, OffsetX);
	for (UTextBlock* Label :
	     {Selection.Get(), Resources.Get(), Feedback.Get(), SummonText.Get(), MergeText.Get(), SellText.Get()})
	{
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = FMath::Max(10, FMath::RoundToInt(30 * Scale));
		Label->SetFont(Font);
	}
	const bool bReady = Controller->CanUseGameplayActions();
	const bool bPending = Controller->HasPendingCommand();
	const bool bRetry = Controller->CanRetryPendingCommand();
	const FLDEconomySnapshot& Economy = Controller->GetEconomySnapshot();
	const int32 Population = Controller->GetBoardSnapshot().Population;
	Summon->SetIsEnabled(bReady && (!bPending || bRetry));
	Sell->SetIsEnabled(bReady && !bPending && Controller->GetSelectedInstanceId() != 0);
	Merge->SetIsEnabled(bReady && !bPending && Controller->CanMergeSelection());
	Resources->SetText(FText::Format(NSLOCTEXT("LD", "Resources", "골드 {0}   ◆ {1}   인구 {2}/20"), Economy.Gold,
	                                           Economy.Stars, Population));
	SummonText->SetText(bRetry             ? NSLOCTEXT("LD", "RetryRequest", "응답 재확인")
	                                       : bPending
	                                       ? NSLOCTEXT("LD", "RequestWaiting", "처리 중…")
	                                                   : FText::Format(NSLOCTEXT("LD", "SummonPrice", "소환\n{0} 골드"),
	                                                                             Economy.NextSummonGold));
	Selection->SetText(Controller->GetSelectionText());
	Feedback->SetText(!bReady             ? NSLOCTEXT("LD", "SnapshotWaiting", "참가자와 보드 정보를 기다리고 있습니다")
	                                      : Controller->GetCommandFeedback());
	Summon->SetBackgroundColor(GetWorld()->GetTimeSeconds() < RejectedUntil ? FLinearColor(0.9f, .2f, .18f)
	                                                                        : FLinearColor(.4f, .65f, .6f));
	ActionRects.Reset();
	for (UButton* Button : {Summon.Get(), Merge.Get(), Sell.Get()})
	{
		const FGeometry Geometry = Button->GetCachedGeometry();
		FVector2D Min;
		FVector2D Max;
		FVector2D WidgetPosition;
		USlateBlueprintLibrary::LocalToViewport(this, Geometry, FVector2D::ZeroVector, Min, WidgetPosition);
		USlateBlueprintLibrary::LocalToViewport(this, Geometry, Geometry.GetLocalSize(), Max, WidgetPosition);
		ActionRects.Add(FBox2D(Min, Max));
	}
}

bool ULDGameplayWidget::IsOverAction(const FVector2D& ScreenPixels) const
{
	for (const FBox2D& Rect : ActionRects)
	{
		if (Rect.IsInsideOrOn(ScreenPixels))
		{
			return true;
		}
	}
	return false;
}

bool ULDGameplayWidget::GetActionScreenRect(ELDCommandType Type, FBox2D& OutRect) const
{
	const int32 Index = Type == ELDCommandType::Summon
	                        ? 0
	                        : (Type == ELDCommandType::Merge ? 1 : (Type == ELDCommandType::Sell ? 2 : INDEX_NONE));
	if (!ActionRects.IsValidIndex(Index) || !ActionRects[Index].bIsValid)
	{
		return false;
	}
	OutRect = ActionRects[Index];
	return OutRect.GetArea() > 0;
}

void ULDGameplayWidget::ShowRejected()
{
	RejectedUntil = GetWorld()->GetTimeSeconds() + .25;
}

void ULDGameplayWidget::OnSummon()
{
	if (ALDPlayerController* Controller = Cast<ALDPlayerController>(GetOwningPlayer()))
	{
		Controller->RequestSummon();
	}
}

void ULDGameplayWidget::OnMerge()
{
	if (ALDPlayerController* Controller = Cast<ALDPlayerController>(GetOwningPlayer()))
	{
		Controller->RequestMergeSelection();
	}
}

void ULDGameplayWidget::OnSell()
{
	if (ALDPlayerController* Controller = Cast<ALDPlayerController>(GetOwningPlayer()))
	{
		Controller->RequestSellSelection();
	}
}
