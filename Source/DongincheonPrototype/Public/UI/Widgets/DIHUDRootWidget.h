#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/DIHUDTypes.h"
#include "DIHUDRootWidget.generated.h"

class UDIHealthBarWidgetBase;
class UHealthComponent;
class UWidget;

UCLASS(Abstract, Blueprintable)
class DONGINCHEONPROTOTYPE_API UDIHUDRootWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Player ------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetPlayerHealthSource(UHealthComponent* InHealthComponent);


    // Boss --------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetBossHealthSource(UHealthComponent* InHealthComponent);

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void ClearBossHealthSource();

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetBossHUDActive(bool bActive);

    UFUNCTION(BlueprintPure, Category = "HUD")
    bool IsBossHUDActive() const;


    // Global HUD Context ------------------------------------

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetHUDContext(EDIHUDContext NewContext);

    UFUNCTION(BlueprintPure, Category = "HUD")
    EDIHUDContext GetHUDContext() const;


protected:
    virtual void NativeConstruct() override;


private:
    // Entire gameplay/combat HUD.
    // Cinematic / Interaction 상태에서는 이 Container 전체를 숨긴다.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> CombatHUDContainer;


    // Player HP
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDIHealthBarWidgetBase> PlayerHealthBar;


    // Boss Name / HP / 이후 Boss 관련 Presentation 전체 Container
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> BossHUDContainer;


    // Boss HP
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UDIHealthBarWidgetBase> BossHealthBar;


    UPROPERTY(Transient)
    EDIHUDContext CurrentHUDContext = EDIHUDContext::Gameplay;

    UPROPERTY(Transient)
    bool bBossHUDActive = false;


    void RefreshHUDVisibility();
};