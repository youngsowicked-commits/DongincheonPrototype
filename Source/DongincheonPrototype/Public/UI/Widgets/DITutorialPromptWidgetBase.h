#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "DITutorialPromptWidgetBase.generated.h"

class UImage;
class UTextBlock;
class UWidget;
class UWidgetAnimation;

UCLASS(Abstract, Blueprintable)
class DONGINCHEONPROTOTYPE_API UDITutorialPromptWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
	void ShowTutorialPrompt(const FText& InTitle, const FText& InDescription);

	UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
	void SetTutorialProgress(const FText& InProgressText);

	UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
	void SetTutorialInputIcon(const FSlateBrush& InBrush);

	UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
	void HideTutorialPrompt();

	UFUNCTION(BlueprintPure, Category = "HUD|Tutorial")
	bool IsTutorialPromptVisible() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> TutorialPromptRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TutorialTitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TutorialDescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TutorialInputIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TutorialProgressText;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> TutorialShowAnimation;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> TutorialHideAnimation;

	UPROPERTY(Transient)
	bool bTutorialPromptVisible = false;

	UFUNCTION()
	void HandleTutorialHideFinished();
};