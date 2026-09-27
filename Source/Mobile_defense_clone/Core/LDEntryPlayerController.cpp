#include "Core/LDEntryPlayerController.h"

#include "Core/LDGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "UI/LDEntryWidget.h"

void ALDEntryPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		if (ULDGameInstance* Instance = GetGameInstance<ULDGameInstance>())
		{
			Instance->NotifyEntryReady();
		}
		bShowMouseCursor = true;
		FInputModeUIOnly Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		EnsureEntryWidget();
	}
}

void ALDEntryPlayerController::EnsureEntryWidget()
{
	if (EntryWidget && !EntryWidget->IsInViewport())
	{
		EntryWidget = nullptr;
	}
	if (IsLocalController() && !EntryWidget)
	{
		EntryWidget = CreateWidget<ULDEntryWidget>(this, ULDEntryWidget::StaticClass());
		if (EntryWidget)
		{
			EntryWidget->AddToViewport(100);
		}
	}
}

void ALDEntryPlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);
	EnsureEntryWidget();
}

void ALDEntryPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (EntryWidget)
	{
		EntryWidget->RemoveFromParent();
		EntryWidget = nullptr;
	}
	Super::EndPlay(Reason);
}

bool ALDEntryPlayerController::RequestHost()
{
	ULDGameInstance* Instance = GetGameInstance<ULDGameInstance>();
	if (!IsLocalController() || !Instance || !Instance->TryBeginMatchTravel())
	{
		return false;
	}
	LocalFeedback = FText::GetEmpty();
	UGameplayStatics::OpenLevel(this, TEXT("/Game/LD/Maps/L_P0"), true, TEXT("listen"));
	return true;
}

bool ALDEntryPlayerController::RequestJoin(const FString& Address)
{
	FString Normalized;
	if (!ULDGameInstance::NormalizeJoinAddress(Address, Normalized))
	{
		LocalFeedback = NSLOCTEXT("LD", "InvalidJoinAddress", "IPv4 주소를 입력하세요. 예: 192.168.0.10:7777");
		return false;
	}
	ULDGameInstance* Instance = GetGameInstance<ULDGameInstance>();
	if (!IsLocalController() || !Instance || !Instance->TryBeginMatchTravel())
	{
		return false;
	}
	LocalFeedback = FText::GetEmpty();
	ClientTravel(Normalized, TRAVEL_Absolute);
	return true;
}

bool ALDEntryPlayerController::IsEntryBusy() const
{
	const ULDGameInstance* Instance = GetGameInstance<ULDGameInstance>();
	return !Instance || Instance->IsTravelPending();
}

FText ALDEntryPlayerController::GetEntryFeedback() const
{
	if (!LocalFeedback.IsEmpty())
	{
		return LocalFeedback;
	}
	const ULDGameInstance* Instance = GetGameInstance<ULDGameInstance>();
	if (!Instance)
	{
		return NSLOCTEXT("LD", "EntryMissingInstance", "프로젝트의 GameInstance 설정을 확인하세요.");
	}
	return Instance->IsTravelPending()             ? NSLOCTEXT("LD", "EntryConnecting", "전장에 연결하고 있습니다…")
	                                               : Instance->GetEntryMessage();
}

bool ALDEntryPlayerController::GetEntryActionScreenRect(bool bHost, FBox2D& OutRect) const
{
	return EntryWidget && EntryWidget->GetActionScreenRect(bHost, OutRect);
}

void ALDEntryPlayerController::SetJoinAddressText(const FString& Text)
{
	if (EntryWidget && !IsEntryBusy())
	{
		EntryWidget->SetAddressText(Text);
	}
}
