#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DIHealthBarWidgetBase.generated.h"

class UHealthComponent;
class UProgressBar;

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

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<UHealthComponent> HealthSource;

	UFUNCTION()
	void HandleHealthChanged(
		float OldHealth,
		float NewHealth,
		float MaxHealth);

	UFUNCTION()
	void HandleMaxHealthChanged(
		float OldMaxHealth,
		float NewMaxHealth,
		float CurrentHealth);

	void BindHealthSource();
	void UnbindHealthSource();
	void RefreshHealthBar();
};