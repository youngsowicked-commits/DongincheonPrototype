#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DIHeatActionPromptWidgetBase.generated.h"

class UWidget;
class UWidgetAnimation;

UCLASS(Abstract, Blueprintable)
class DONGINCHEONPROTOTYPE_API UDIHeatActionPromptWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD|HeatAction")
	void ShowHeatActionPrompt();

	UFUNCTION(BlueprintCallable, Category = "HUD|HeatAction")
	void HideHeatActionPrompt();

	UFUNCTION(BlueprintPure, Category = "HUD|HeatAction")
	bool IsHeatActionPromptVisible() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> HeatActionPromptRoot;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> HeatActionPromptShowAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> HeatActionPromptHideAnimation;

	UPROPERTY(Transient)
	bool bHeatActionPromptVisible = false;

	UFUNCTION()
	void HandleHeatActionPromptHideFinished();
};