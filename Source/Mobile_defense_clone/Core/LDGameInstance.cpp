#include "Core/LDGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/PendingNetGame.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void ULDGameInstance::Init()
{
	Super::Init();
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ULDGameInstance::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ULDGameInstance::HandleTravelFailure);
	}
}

void ULDGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	FTSTicker::GetCoreTicker().RemoveTicker(ReturnTicker);
	ReturnTicker.Reset();
	bReturnScheduled = false;
	bTravelPending = false;
	Super::Shutdown();
}

bool ULDGameInstance::NormalizeJoinAddress(const FString& Input, FString& OutAddress)
{
	OutAddress.Reset();
	const FString Address = Input.TrimStartAndEnd();
	if (Address.IsEmpty() || Address.Len() > 21)
	{
		return false;
	}
	FString Host = Address;
	FString Port;
	const bool bHasPort = Address.Split(TEXT(":"), &Host, &Port);
	const auto ParseUnsigned = [](const FString& Text, int32 MaxDigits, int32 MaxValue, int32& OutValue)
	{
		OutValue = 0;
		if (Text.IsEmpty() || Text.Len() > MaxDigits)
		{
			return false;
		}
		for (const TCHAR Character : Text)
		{
			if (Character < TEXT('0') || Character > TEXT('9'))
			{
				return false;
			}
			OutValue = OutValue * 10 + Character - TEXT('0');
		}
		return OutValue <= MaxValue;
	};
	TArray<FString> Octets;
	Host.ParseIntoArray(Octets, TEXT("."), false);
	if (Octets.Num() != 4)
	{
		return false;
	}
	int32 Values[4];
	for (int32 Index = 0; Index < 4; ++Index)
	{
		if (!ParseUnsigned(Octets[Index], 3, 255, Values[Index]))
		{
			return false;
		}
	}
	int32 PortValue = 7777;
	if ((bHasPort && (!ParseUnsigned(Port, 5, 65535, PortValue) || PortValue == 0)) || Values[0] == 0 ||
	    Values[0] >= 224)
	{
		return false;
	}
	OutAddress = FString::Printf(TEXT("%d.%d.%d.%d:%d"), Values[0], Values[1], Values[2], Values[3], PortValue);
	return true;
}

bool ULDGameInstance::TryBeginMatchTravel()
{
	if (bTravelPending || bReturnScheduled)
	{
		return false;
	}
	bTravelPending = true;
	EntryMessage = FText::GetEmpty();
	return true;
}

bool ULDGameInstance::RequestEntryReturn(const FText& Reason)
{
	if (bReturnScheduled)
	{
		return false;
	}
	EntryMessage = Reason;
	bTravelPending = true;
	bReturnScheduled = true;
	// Engine failure delegates can fire during Browse/TickWorldTravel. Travel on the next core tick,
	// outside that stack and outside a soon-to-be-destroyed world's timer manager.
	ReturnTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(
	    this,
	    [this](float)
	    {
		    ReturnTicker.Reset();
		    if (UWorld* World = GetWorld())
		    {
			    UGameplayStatics::OpenLevel(World, TEXT("/Game/LD/Maps/L_P0Entry"), true);
		    }
		    else
		    {
			    bReturnScheduled = false;
			    bTravelPending = false;
		    }
		    return false;
	    }));
	return true;
}

void ULDGameInstance::NotifyEntryReady()
{
	FTSTicker::GetCoreTicker().RemoveTicker(ReturnTicker);
	ReturnTicker.Reset();
	bReturnScheduled = false;
	bTravelPending = false;
}

bool ULDGameInstance::IsTravelPending() const
{
	return bTravelPending;
}

const FText& ULDGameInstance::GetEntryMessage() const
{
	return EntryMessage;
}

void ULDGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type,
                                           const FString& Error)
{
	const FWorldContext* Context = GetWorldContext();
	const bool bOwnPending = Context && Context->PendingNetGame && Context->PendingNetGame->NetDriver == Driver;
	if ((!World || World->GetGameInstance() != this) && !bOwnPending)
	{
		return;
	}
	// On a listen host, GameMode owns participant departure and the Aborted result screen.
	if (World && World->GetNetMode() == NM_ListenServer && !bOwnPending)
	{
		return;
	}
	RequestEntryReturn(
	    NSLOCTEXT("LD", "NetworkReturn", "연결이 종료되었거나 참가하지 못했습니다. 주소와 호스트 상태를 확인하세요."));
}

void ULDGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error)
{
	if (World && World->GetGameInstance() == this)
	{
		RequestEntryReturn(
		    NSLOCTEXT("LD", "TravelReturn", "전장을 열지 못했습니다. 패키지의 맵과 연결 주소를 확인하세요."));
	}
}
