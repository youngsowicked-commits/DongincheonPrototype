// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class AActor;
class AController;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChangedSignature,float,OldHealth,float,NewHealth,float,MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMaxHealthChangedSignature,float,OldMaxHealth,float,NewMaxHealth,float,CurrentHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDeathSignature, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamagedSignature,float,DamageAmount,AActor*,DamageCauser);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )


class DONGINCHEONPROTOTYPE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetDamageEnabled(bool bEnabled);
	
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDamageEnabled() const;
	
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const;
	
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const;
	
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthNormalized() const;
	
	UFUNCTION(BlueprintPure,Category = "Health")
	bool IsDead() const; 
	
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float HealAmount);
	
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth, bool bAdjustCurrentHealth = true);
	
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnMaxHealthChangedSignature OnMaxHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthDeathSignature OnDeath;
	
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDamagedSignature OnDamaged;
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "1.0" , UIMin = "1.0" , AllowPrivateAccess = "true"))
	float MaxHealth = 100.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth = 0.0f;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Health", meta = (AllowPrivateAccess = "true"))
	bool bDamageEnalbe = true;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Health", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;
	
	UFUNCTION()
	void HandleTakeAnyDamage(
		AActor* DamageActor,
		float Damage,
		const UDamageType* DamageType,
		AController* InstigatedBy,
		AActor* DamageCauser);
};
