#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "DITutorialPromptWidgetBase.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
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
	void ShowTutorialPromptRows(const FText& Title,FName Row1IconId,const FText& Row1Text,FName Row2IconId,const FText& Row2Text);

	UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
	void SetTutorialProgress(const FText& InProgressText);
	
	// Tutorial Two-Row Presentation

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TutorialRow1Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TutorialRow1Text;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TutorialRow2Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TutorialRow2Text;

	// IconId -> Texture (WBP Class Defaults에서 지정)
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Tutorial|Icons")
	TMap<FName, TObjectPtr<UTexture2D>> TutorialIconTextures;

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

	// 첫 번째 조작키 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TutorialKeyIconTop;

	// 두 번째 조작키 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TutorialKeyIconBottom;

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