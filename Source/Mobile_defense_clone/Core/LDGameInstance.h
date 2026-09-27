#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Engine/GameInstance.h"
#include "LDGameInstance.generated.h"

/** Local travel and error presentation only; no match, economy or combat source state lives here. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;
	static bool NormalizeJoinAddress(const FString& Input, FString& OutAddress);
	bool TryBeginMatchTravel();
	bool RequestEntryReturn(const FText& Reason);
	void NotifyEntryReady();
	bool IsTravelPending() const;
	const FText& GetEntryMessage() const;

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error);
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
	FTSTicker::FDelegateHandle ReturnTicker;
	FText EntryMessage;
	bool bTravelPending = false;
	bool bReturnScheduled = false;
};
