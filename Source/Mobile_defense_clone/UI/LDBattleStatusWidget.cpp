#include "UI/LDBattleStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SafeZone.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> ULDBattleStatusWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this);
	}
	if (!WidgetTree->RootWidget)
	{
		USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("BattleSafeZone"));
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BattleCanvas"));
		Safe->AddChild(Canvas);
		WidgetTree->RootWidget = Safe;
		Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WaveStatus"));
		BossStatus = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BossStatus"));
		for (UTextBlock* Label : {Status.Get(), BossStatus.Get()})
		{
			Label->SetJustification(ETextJustify::Center);
			Canvas->AddChildToCanvas(Label);
		}
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return Super::RebuildWidget();
}

void ULDBattleStatusWidget::UpdateView(const FLDBattleSnapshot& Snapshot, double ServerNow)
{
	if (!Status || !BossStatus)
	{
		return;
	}
	const double Deadline = Snapshot.Phase == ELDMatchPhase::Loading     ? Snapshot.LoadingDeadlineServerSeconds
	                        : Snapshot.Phase == ELDMatchPhase::Preparing ? Snapshot.PreparationEndServerSeconds
	                        : Snapshot.BossDeadlineServerSeconds > 0     ? Snapshot.BossDeadlineServerSeconds
	                                                                     : Snapshot.WaveEndServerSeconds;
	const int32 Seconds = Snapshot.IsTerminal() ? 0 : FMath::CeilToInt(FMath::Max(0.0, Deadline - ServerNow));
	const FString Phase = Snapshot.Phase == ELDMatchPhase::Loading ? TEXT("접속 대기")
	                                                               : Snapshot.Phase == ELDMatchPhase::Preparing
	    ? TEXT("전투 준비")
	    : Snapshot.IsTerminal()
	    ? TEXT("전투 종료")
	    : TEXT("2인 협동");
	Status->SetText(FText::FromString(
	    FString::Printf(TEXT("WAVE %d / %d\n%02d:%02d\n%s\n일반 적 %d / %d"), Snapshot.WaveIndex, Snapshot.FinalWave,
	                         Seconds / 60, Seconds % 60, *Phase, Snapshot.ActiveEnemyCount, Snapshot.MaxEnemyCount)));
	Status->SetColorAndOpacity(Snapshot.ActiveEnemyCount >= Snapshot.MaxEnemyCount - 10
	                               ? FSlateColor(FLinearColor(1, .35f, .2f))
	                               : FSlateColor(FLinearColor::White));
	FString BossText;
	for (const FLDBossSnapshot& Boss : Snapshot.Bosses)
	{
		if (!BossText.IsEmpty())
		{
			BossText += TEXT("   |   ");
		}
		BossText += FString::Printf(TEXT("보스 %d  %.0f/%.0f"), Boss.RouteIndex + 1, Boss.HP, Boss.MaxHP);
	}
	BossStatus->SetText(FText::FromString(BossText));
}

void ULDBattleStatusWidget::NativeTick(const FGeometry& MyGeometry, float DeltaSeconds)
{
	Super::NativeTick(MyGeometry, DeltaSeconds);
	if (!Canvas)
	{
		return;
	}
	const FVector2D Size = Canvas->GetCachedGeometry().GetLocalSize();
	const double Scale = FMath::Min(Size.X / 1080.0, Size.Y / 2340.0);
	const double OffsetX = (Size.X - 1080.0 * Scale) * .5;
	for (UTextBlock* Label : {Status.Get(), BossStatus.Get()})
	{
		const bool bBoss = Label == BossStatus;
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Label->Slot);
		CanvasSlot->SetPosition(FVector2D(OffsetX + (bBoss ? 180 : 240) * Scale, (bBoss ? 476 : 228) * Scale));
		CanvasSlot->SetSize(FVector2D((bBoss ? 720 : 600) * Scale, (bBoss ? 42 : 232) * Scale));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = FMath::Max(bBoss ? 8 : 10, FMath::RoundToInt((bBoss ? 22 : 34) * Scale));
		Label->SetFont(Font);
	}
}
