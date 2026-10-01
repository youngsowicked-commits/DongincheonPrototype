#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "DIContextActionHintWidgetBase.generated.h"

class UImage;
class UTextBlock;
class UWidget;
class UWidgetAnimation;

UCLASS(Abstract, Blueprintable)
class DONGINCHEONPROTOTYPE_API UDIContextActionHintWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD|ContextHint")
	void ShowContextActionHint(const FText& ActionText);

	UFUNCTION(BlueprintCallable, Category = "HUD|ContextHint")
	void SetContextActionIcon(const FSlateBrush& InBrush);

	UFUNCTION(BlueprintCallable, Category = "HUD|ContextHint")
	void HideContextActionHint();

	UFUNCTION(BlueprintPure, Category = "HUD|ContextHint")
	bool IsContextActionHintVisible() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ContextActionHintRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ContextActionIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ContextActionText;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ContextHintShowAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ContextHintHideAnimation;

	UPROPERTY(Transient)
	bool bContextActionHintVisible = false;

	UFUNCTION()
	void HandleContextHintHideFinished();
};