#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "UI/DIHUDTypes.h"
#include "DIHUDRootWidget.generated.h"

class UDIHealthBarWidgetBase;
class UDIHeatActionComponent;
class UDIQTEPromptWidgetBase;
class UDIBossIntroWidgetBase;
class UDITutorialPromptWidgetBase;
class UDIContextActionHintWidgetBase;
class UHealthComponent;
class UQTEComponent;
class UWidget;

UCLASS(Abstract, Blueprintable)
class DONGINCHEONPROTOTYPE_API UDIHUDRootWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Player ------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetPlayerHealthSource(UHealthComponent* InHealthComponent);
    
    UFUNCTION(BlueprintCallable, Category = "HUD|Heat")
    void SetPlayerHeatSource(UDIHeatActionComponent* InHeatComponent);
    
    // Enemy -------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetEnemyHealthSource(UHealthComponent* InHealthComponent);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ClearEnemyHealthSource();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetEnemyHUDActive(bool bActive);

    UFUNCTION(BlueprintPure, Category = "HUD")
    bool IsEnemyHUDActive() const;
    
    // Boss --------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetBossHealthSource(UHealthComponent* InHealthComponent);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ClearBossHealthSource();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetBossHUDActive(bool bActive);

    UFUNCTION(BlueprintPure, Category = "HUD")
    bool IsBossHUDActive() const;
    
    // Boss Intro ----------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "HUD|BossIntro")
    void ShowBossIntro(const FText& BossRole, const FText& BossName);

    UFUNCTION(BlueprintCallable, Category = "HUD|BossIntro")
    void HideBossIntro();
    
    // QTE ---------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "HUD|QTE")
    void SetQTESource(UQTEComponent* InQTEComponent);

    UFUNCTION(BlueprintCallable, Category = "HUD|QTE")
    void ClearQTESource();
    
    // Tutorial ----------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
    void ShowTutorialPrompt(const FText& Title, const FText& Description);

    UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
    void SetTutorialProgress(const FText& ProgressText);

    UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
    void HideTutorialPrompt();
    
    // Context Action Hint ------------------------------------
    UFUNCTION(BlueprintCallable, Category = "HUD|ContextHint")
    void ShowContextActionHint(const FText& ActionText);

    UFUNCTION(BlueprintCallable, Category = "HUD|ContextHint")
    void SetContextActionIcon(const FSlateBrush& InBrush);

    UFUNCTION(BlueprintCallable, Category = "HUD|ContextHint")
    void HideContextActionHint();
    
    // Global HUD Context ------------------------------------
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetHUDContext(EDIHUDContext NewContext);
    
    UFUNCTION(BlueprintCallable, Category = "HUD|LockOn")
    void SetLockOnTarget(AActor* InTarget);

    UFUNCTION(BlueprintPure, Category = "HUD")
    EDIHUDContext GetHUDContext() const;


protected:
    virtual void NativeConstruct() override;

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    
private:
    // Entire gameplay/combat HUD.
    // Cinematic / Interaction 상태에서는 이 Container 전체를 숨긴다.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CombatHUDContainer;

    // Player HP
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDIHealthBarWidgetBase> PlayerHealthBar;
    
    // Player Lock On
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> LockOnMarker;
    
    // Enemy Name / HP Presentation Container
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> EnemyHUDContainer;

    // Enemy HP
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDIHealthBarWidgetBase> EnemyHealthBar;
    
    // Boss Name / HP / 이후 Boss 관련 Presentation 전체 Container
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> BossHUDContainer;
    
    // Boss HP
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDIHealthBarWidgetBase> BossHealthBar;
    
    // QTE Prompt
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDIQTEPromptWidgetBase> QTEPrompt;
    
    // Tutorial Prompt
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDITutorialPromptWidgetBase> TutorialPrompt;
    
    //Context Prompt
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDIContextActionHintWidgetBase> ContextActionHint;
    
    UPROPERTY(Transient)
    EDIHUDContext CurrentHUDContext = EDIHUDContext::Gameplay;
    
    // Boss Intro Presentation
    // CombatHUDContainer 밖의 독립 레이어.
    // Cinematic Context에서도 표시 가능해야 한다.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDIBossIntroWidgetBase> BossIntro;
    
    UPROPERTY(Transient)
    TObjectPtr<AActor> LockOnTarget;

    UPROPERTY(EditDefaultsOnly, Category = "HUD|LockOn")
    float LockOnMarkerHeightOffset = 90.0f;
    
    UPROPERTY(Transient)
    bool bEnemyHUDActive = false;

    UPROPERTY(Transient)
    bool bBossHUDActive = false;


    void RefreshHUDVisibility();
};