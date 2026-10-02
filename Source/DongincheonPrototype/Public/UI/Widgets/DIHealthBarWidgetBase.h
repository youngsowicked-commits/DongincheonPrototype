#pragma once

#include "CoreMinimal.h"
#include "Components/HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "DIHealthBarWidgetBase.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UDIHeatActionComponent;

UCLASS(Abstract, Blueprintable)
class DONGINCHEONPROTOTYPE_API UDIHealthBarWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD|Health")
	void SetHealthSource(UHealthComponent* InHealthComponent);

	UFUNCTION(BlueprintCallable, Category = "HUD|Health")
	void ClearHealthSource();

	UFUNCTION(BlueprintPure, Category = "HUD|Health")
	UHealthComponent* GetHealthSource() const;
	
	UFUNCTION(BlueprintCallable, Category = "HUD|Heat")
	void SetHeatSource(UDIHeatActionComponent* InHeatComponent);

	UFUNCTION(BlueprintCallable, Category = "HUD|Heat")
	void ClearHeatSource();

	UFUNCTION(BlueprintPure, Category = "HUD|Heat")
	UDIHeatActionComponent* GetHeatSource() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HPMainImage;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HPMainMID;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HPDamageLagImage;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HeatMainImage;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HeatMainMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HPDamageLagMID;

	UPROPERTY(Transient)
	TObjectPtr<UHealthComponent> HealthSource;
	
	UPROPERTY(Transient)
	TObjectPtr<UDIHeatActionComponent> HeatSource;

	UFUNCTION()
	void HandleHealthChanged(float OldHealth,float NewHealth,float MaxHealth);

	UFUNCTION()
	void HandleMaxHealthChanged(float OldMaxHealth,float NewMaxHealth,float CurrentHealth);
	
	UFUNCTION()
	void HandleHeatChanged(float CurrentHeat, float MaxHeat);
	
	float HeatCurrentPercent = 0.0f;
	float HeatStartPercent = 0.0f;
	float HeatTargetPercent = 0.0f;
	float HeatAnimationElapsed = 0.0f;

	bool bHeatAnimating = false;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Heat", meta = (ClampMin = "0.01"))
	float HeatFillDuration = 0.22f;
	
	float DamageLagCurrentPercent = 1.0f;
	float DamageLagStartPercent = 1.0f;
	float DamageLagTargetPercent = 1.0f;
	float DamageLagDelayRemaining = 0.0f;
	float DamageLagAnimationElapsed = 0.0f;

	bool bDamageLagAnimating = false;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health|DamageLag")
	float DamageLagDelay = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Health|DamageLag")
	float DamageLagDuration = 0.35f;

	void BindHealthSource();
	void UnbindHealthSource();
	void RefreshHealthBar();
	
	void BindHeatSource();
	void UnbindHeatSource();
	void RefreshHeatBar();

	void EnsureDynamicMaterial();
};