#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LDEntryPlayerController.generated.h"

class ULDEntryWidget;

UCLASS()
class MOBILE_DEFENSE_CLONE_API ALDEntryPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	bool RequestHost();
	bool RequestJoin(const FString& Address);
	bool IsEntryBusy() const;
	FText GetEntryFeedback() const;
	bool GetEntryActionScreenRect(bool bHost, FBox2D& OutRect) const;
	// Changes the editable widget only; joining still goes through the same button intent and parser.
	void SetJoinAddressText(const FString& Text);
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void PlayerTick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void EnsureEntryWidget();
	UPROPERTY()
	TObjectPtr<ULDEntryWidget> EntryWidget;
	FText LocalFeedback;
};
