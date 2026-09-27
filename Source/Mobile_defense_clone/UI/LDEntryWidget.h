#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LDEntryWidget.generated.h"

class UButton;
class UCanvasPanel;
class UEditableTextBox;
class UTextBlock;

/** Minimal local host/address/join entry for P0 device testing; no matchmaking or saved player data. */
UCLASS()
class MOBILE_DEFENSE_CLONE_API ULDEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	bool GetActionScreenRect(bool bHost, FBox2D& OutRect) const;
	bool GetAddressScreenRect(FBox2D& OutRect) const;
	void SetAddressText(const FString& Text);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;

private:
	friend class FLDP0EntryStyleLifetimeTest;
	void UpdateAddressFontSize(int32 FontSize);
	UTextBlock* AddLabel(FName Name, const FText& Text);
	UButton* AddButton(FName Name, const FText& Text, UTextBlock*& OutLabel);
	static bool GetWidgetRect(const UWidget* Widget, FBox2D& OutRect);
	UFUNCTION()
	void OnHost();
	UFUNCTION()
	void OnJoin();
	UPROPERTY()
	TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY()
	TObjectPtr<UTextBlock> Title;
	UPROPERTY()
	TObjectPtr<UTextBlock> Hint;
	UPROPERTY()
	TObjectPtr<UTextBlock> Feedback;
	UPROPERTY()
	TObjectPtr<UTextBlock> HostLabel;
	UPROPERTY()
	TObjectPtr<UTextBlock> JoinLabel;
	UPROPERTY()
	TObjectPtr<UButton> HostButton;
	UPROPERTY()
	TObjectPtr<UButton> JoinButton;
	UPROPERTY()
	TObjectPtr<UEditableTextBox> Address;
};
