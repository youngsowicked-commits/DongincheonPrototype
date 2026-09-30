#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DIBossIntroWidgetBase.generated.h"

class UWidget;
class UTextBlock;
class UWidgetAnimation;

UCLASS(Abstract, Blueprintable)
class DONGINCHEONPROTOTYPE_API UDIBossIntroWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD|BossIntro")
	void ShowBossIntro(
		const FText& InBossRole,
		const FText& InBossName);

	UFUNCTION(BlueprintCallable, Category = "HUD|BossIntro")
	void HideBossIntro();

	UFUNCTION(BlueprintPure, Category = "HUD|BossIntro")
	bool IsBossIntroPlaying() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> BossIntroRoot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossRoleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	/*
	 * WBP에서 만드는 등장 → Hold → 퇴장 애니메이션.
	 * 이름은 반드시 BossIntroAnimation.
	 */
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> BossIntroAnimation;

	UPROPERTY(Transient)
	bool bBossIntroPlaying = false;

	UFUNCTION()
	void HandleBossIntroAnimationFinished();
};