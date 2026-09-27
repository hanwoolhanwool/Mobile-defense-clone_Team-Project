#include "UI/LDResultWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> ULDResultWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this);
	}
	if (!WidgetTree->RootWidget)
	{
		USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("ResultSafeZone"));
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ResultCanvas"));
		Safe->AddChild(Canvas);
		WidgetTree->RootWidget = Safe;
		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResultPanel"));
		Panel->SetBrushColor(FLinearColor(.04f, .055f, .075f, .97f));
		Canvas->AddChildToCanvas(Panel);
		UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Panel->Slot);
		PanelSlot->SetAnchors(FAnchors(.1f, .32f, .9f, .65f));
		PanelSlot->SetOffsets(FMargin(0));
		ResultText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResultText"));
		ResultText->SetJustification(ETextJustify::Center);
		ResultText->SetVisibility(ESlateVisibility::HitTestInvisible);
		Canvas->AddChildToCanvas(ResultText);
		ReturnButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ReturnButton"));
		ReturnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReturnText"));
		ReturnText->SetText(NSLOCTEXT("LD", "ReturnFromResult", "시작 화면으로"));
		ReturnText->SetJustification(ETextJustify::Center);
		ReturnButton->AddChild(ReturnText);
		ReturnButton->SetTouchMethod(EButtonTouchMethod::PreciseTap);
		Canvas->AddChildToCanvas(ReturnButton);
	}
	return Super::RebuildWidget();
}

void ULDResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ReturnButton->OnClicked.AddUniqueDynamic(this, &ULDResultWidget::HandleReturn);
}

void ULDResultWidget::NativeDestruct()
{
	if (ReturnButton)
	{
		ReturnButton->OnClicked.RemoveAll(this);
	}
	OnReturnRequested.Clear();
	Super::NativeDestruct();
}

void ULDResultWidget::HandleReturn()
{
	OnReturnRequested.Broadcast();
}

void ULDResultWidget::UpdateView(const FLDBattleSnapshot& Snapshot, double ServerNow)
{
	if (!ResultText || !Snapshot.IsTerminal())
	{
		return;
	}
	const FString Title = Snapshot.Result == ELDMatchResult::Victory ? TEXT("승리")
	                                                                 : Snapshot.Result == ELDMatchResult::Defeat
	    ? TEXT("패배")
	    : TEXT("매치 종료");
	FString Reason;
	switch (Snapshot.ResultReason)
	{
	case ELDResultReason::EnemyLimit:
		Reason = TEXT("일반 적 수 한도 도달");
		break;
	case ELDResultReason::BossTimeout:
		Reason = TEXT("보스 제한 시간 초과");
		break;
	case ELDResultReason::LoadingTimeout:
		Reason = TEXT("참가자 접속 대기 시간 초과");
		break;
	case ELDResultReason::ParticipantDisconnected:
		Reason = TEXT("참가자 연결 종료");
		break;
	case ELDResultReason::InitializationFailure:
		Reason = TEXT("필수 데이터 또는 게임 초기화 실패");
		break;
	default:
		Reason = TEXT("양쪽 보스와 모든 일반 적 처치");
		break;
	}
	ResultText->SetText(FText::FromString(
	    FString::Printf(TEXT("%s\n\n%s\n\nWAVE %d / %d"), *Title, *Reason, Snapshot.WaveIndex, Snapshot.FinalWave)));
}

void ULDResultWidget::NativeTick(const FGeometry& MyGeometry, float DeltaSeconds)
{
	Super::NativeTick(MyGeometry, DeltaSeconds);
	if (!Canvas)
	{
		return;
	}
	const FVector2D Size = Canvas->GetCachedGeometry().GetLocalSize();
	const double Scale = FMath::Min(Size.X / 1080.0, Size.Y / 2340.0);
	UCanvasPanelSlot* TextSlot = Cast<UCanvasPanelSlot>(ResultText->Slot);
	TextSlot->SetPosition(FVector2D(Size.X * .12, Size.Y * .35));
	TextSlot->SetSize(FVector2D(Size.X * .76, Size.Y * .20));
	UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(ReturnButton->Slot);
	ButtonSlot->SetPosition(FVector2D(Size.X * .27, Size.Y * .57));
	ButtonSlot->SetSize(FVector2D(Size.X * .46, FMath::Max(40.0, Size.Y * .06)));
	for (UTextBlock* Label : {ResultText.Get(), ReturnText.Get()})
	{
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = FMath::Max(12, FMath::RoundToInt(36 * Scale));
		Label->SetFont(Font);
	}
}

bool ULDResultWidget::GetReturnButtonScreenRect(FBox2D& OutRect) const
{
	if (!ReturnButton || !IsInViewport())
	{
		return false;
	}
	const FGeometry Geometry = ReturnButton->GetCachedGeometry();
	FVector2D Min;
	FVector2D Max;
	FVector2D WidgetPosition;
	USlateBlueprintLibrary::LocalToViewport(this, Geometry, FVector2D::ZeroVector, Min, WidgetPosition);
	USlateBlueprintLibrary::LocalToViewport(this, Geometry, Geometry.GetLocalSize(), Max, WidgetPosition);
	OutRect = FBox2D(Min, Max);
	return OutRect.GetArea() > 0;
}
