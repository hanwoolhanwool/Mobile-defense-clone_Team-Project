#include "UI/LDEntryWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/SafeZone.h"
#include "Components/TextBlock.h"
#include "Core/LDEntryPlayerController.h"

TSharedRef<SWidget> ULDEntryWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this);
	}
	if (!WidgetTree->RootWidget)
	{
		UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EntryBackground"));
		Background->SetBrushColor(FLinearColor(.035f, .045f, .06f, 1));
		Background->SetPadding(FMargin(0));
		USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>(USafeZone::StaticClass(), TEXT("EntrySafeZone"));
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EntryCanvas"));
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Background->AddChild(Safe);
		Safe->AddChild(Canvas);
		WidgetTree->RootWidget = Background;
		Title = AddLabel(TEXT("Title"), NSLOCTEXT("LD", "EntryTitle", "P0 협동 디펜스"));
		Hint =
		    AddLabel(TEXT("Hint"), NSLOCTEXT("LD", "EntryHint", "한 명이 호스트를 시작한 뒤\n같은 네트워크의 다른 기기에서 참가하세요."));
		Feedback = AddLabel(TEXT("Feedback"), FText::GetEmpty());
		UTextBlock* Label = nullptr;
		HostButton = AddButton(TEXT("Host"), NSLOCTEXT("LD", "HostMatch", "호스트 시작"), Label);
		HostLabel = Label;
		JoinButton = AddButton(TEXT("Join"), NSLOCTEXT("LD", "JoinMatch", "주소로 참가"), Label);
		JoinLabel = Label;
		Address = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("HostAddress"));
		Address->SetHintText(NSLOCTEXT("LD", "HostAddressHint", "호스트 IPv4 주소 (예: 192.168.0.10:7777)"));
		Address->SetText(FText::FromString(TEXT("127.0.0.1:7777")));
		Address->KeyboardType = EVirtualKeyboardType::Default;
		Address->VirtualKeyboardTrigger = EVirtualKeyboardTrigger::OnAllFocusEvents;
		Canvas->AddChildToCanvas(Address);
	}
	return Super::RebuildWidget();
}

UTextBlock* ULDEntryWidget::AddLabel(FName Name, const FText& Text)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Label->SetText(Text);
	Label->SetJustification(ETextJustify::Center);
	Label->SetAutoWrapText(true);
	Label->SetVisibility(ESlateVisibility::HitTestInvisible);
	Canvas->AddChildToCanvas(Label);
	return Label;
}

UButton* ULDEntryWidget::AddButton(FName Name, const FText& Text, UTextBlock*& OutLabel)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	OutLabel->SetText(Text);
	OutLabel->SetJustification(ETextJustify::Center);
	Button->AddChild(OutLabel);
	Button->SetTouchMethod(EButtonTouchMethod::PreciseTap);
	Canvas->AddChildToCanvas(Button);
	return Button;
}

void ULDEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HostButton->OnClicked.AddUniqueDynamic(this, &ULDEntryWidget::OnHost);
	JoinButton->OnClicked.AddUniqueDynamic(this, &ULDEntryWidget::OnJoin);
}

void ULDEntryWidget::NativeDestruct()
{
	if (HostButton)
	{
		HostButton->OnClicked.RemoveAll(this);
		JoinButton->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void ULDEntryWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	const ALDEntryPlayerController* Controller = Cast<ALDEntryPlayerController>(GetOwningPlayer());
	if (!Controller || !Canvas)
	{
		return;
	}
	const FVector2D Size = Canvas->GetCachedGeometry().GetLocalSize();
	const double Scale = FMath::Min(Size.X / 1080.0, Size.Y / 1600.0);
	if (Scale <= 0)
	{
		return;
	}
	const FVector2D Offset = (Size - FVector2D(1080, 1600) * Scale) * .5;
	const auto Place = [Scale, Offset](UWidget* Widget, const FVector4& Rect)
	{
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot);
		CanvasSlot->SetPosition(Offset + FVector2D(Rect.X, Rect.Y) * Scale);
		CanvasSlot->SetSize(FVector2D(Rect.Z, Rect.W) * Scale);
	};
	Place(Title, FVector4(100, 210, 880, 120));
	Place(Hint, FVector4(100, 370, 880, 190));
	Place(HostButton, FVector4(140, 600, 800, 150));
	Place(Address, FVector4(140, 890, 800, 130));
	Place(JoinButton, FVector4(140, 1070, 800, 150));
	Place(Feedback, FVector4(100, 1280, 880, 260));
	for (UTextBlock* Label : {Title.Get(), Hint.Get(), Feedback.Get(), HostLabel.Get(), JoinLabel.Get()})
	{
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = FMath::Max(12, FMath::RoundToInt((Label == Title ? 50 : 32) * Scale));
		Label->SetFont(Font);
	}
	FEditableTextBoxStyle AddressStyle = Address->GetWidgetStyle();
	AddressStyle.TextStyle.Font.Size = FMath::Max(12, FMath::RoundToInt(30 * Scale));
	Address->SetWidgetStyle(AddressStyle);
	const bool bEnabled = !Controller->IsEntryBusy();
	HostButton->SetIsEnabled(bEnabled);
	JoinButton->SetIsEnabled(bEnabled);
	Address->SetIsEnabled(bEnabled);
	Feedback->SetText(Controller->GetEntryFeedback());
}

bool ULDEntryWidget::GetWidgetRect(const UWidget* Widget, FBox2D& OutRect)
{
	if (!Widget)
	{
		return false;
	}
	const FGeometry Geometry = Widget->GetCachedGeometry();
	FVector2D Min;
	FVector2D Max;
	FVector2D WidgetPosition;
	USlateBlueprintLibrary::LocalToViewport(Widget, Geometry, FVector2D::ZeroVector, Min, WidgetPosition);
	USlateBlueprintLibrary::LocalToViewport(Widget, Geometry, Geometry.GetLocalSize(), Max, WidgetPosition);
	OutRect = FBox2D(Min, Max);
	return OutRect.GetArea() > 0;
}

bool ULDEntryWidget::GetActionScreenRect(bool bHost, FBox2D& OutRect) const
{
	return GetWidgetRect(bHost ? HostButton.Get() : JoinButton.Get(), OutRect);
}

bool ULDEntryWidget::GetAddressScreenRect(FBox2D& OutRect) const
{
	return GetWidgetRect(Address, OutRect);
}

void ULDEntryWidget::SetAddressText(const FString& Text)
{
	if (Address)
	{
		Address->SetText(FText::FromString(Text));
	}
}

void ULDEntryWidget::OnHost()
{
	if (ALDEntryPlayerController* Controller = Cast<ALDEntryPlayerController>(GetOwningPlayer()))
	{
		Controller->RequestHost();
	}
}

void ULDEntryWidget::OnJoin()
{
	if (ALDEntryPlayerController* Controller = Cast<ALDEntryPlayerController>(GetOwningPlayer()))
	{
		Controller->RequestJoin(Address->GetText().ToString());
	}
}
