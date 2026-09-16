// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Gameplay/Data/DIAttackData.h"
#include "DongincheonCharacter.generated.h"

class UHealthComponent;
class UTargetingComponent;
class UCombatComponent;
class UInteractionComponent;
class UInputAction;
class UAnimMontage;
class UAnimInstance;
class UCameraShakeBase;

struct FInputActionValue;

UCLASS()
class DONGINCHEONPROTOTYPE_API ADongincheonCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADongincheonCharacter();
	
	//Player Runtime Query
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	bool IsPlayerAttacking() const
	{
		return  bPlayerAttackActive;
	}
	
	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	bool IsPlayerHitReacting() const
	{
		return bPlayerHitReacting;
	}
	
	UFUNCTION(BlueprintPure, Category = "Player|Movement")
	bool IsMovementInputAllowed() const;
	
	UFUNCTION(BlueprintCallable, Category = "Player|Death")
	void FinalizePlayerDeath();

protected:
	virtual void BeginPlay() override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	//Input
	void HandleLockOnStarted(const FInputActionValue& Value);
	void HandleLockOnEnded(const FInputActionValue& Value);
	
	//Health
	UFUNCTION()
	void HandleHealthDamaged(float DamageAmount, AActor* DamageCauser);
	
	UFUNCTION()
	void HandleHealthDeath(AActor* DamageCauser);
	
	//Attack
	void HandleAttackInput();
	
	void StartPlayerComboAttack();
	
	void HandleAttackMontageEnded(UAnimMontage* Montage,bool bInterrupted);
	
	void CancelPlayerAttack(float BlendOutTime = 0.0f);
	
	void ResetPlayerAttackState();
	
	//Hit React
	
	void StartPlayerHitReact();
	
	void HandleHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	void FinishPlayerHitReact();
	
	//Death
	void StartPlayerDeath();
	
	void HandleDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	//Hit Feedback
	UFUNCTION()
	void HandleCombatHitConfirmed(AActor* HitActor, FVector HitLocation, FName HitSocketName, float AppliedDamage);
	
	void StartHitStop();
	
	void ResumeHitStop();
	
	void ClearHitStop();
	

protected:
	//Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTargetingComponent> TargetingComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatComponent> CombatComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractionComponent> InteractionComponent;
	
	//Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> AttackAction;
	
	//LockOn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LockOnAction;
	
	//Attack Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Attack")
	TArray<FAttackConfig> ComboAttacks;
	
	//Hit React Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|HitReact")
	TObjectPtr<UAnimMontage> HitReactMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|HitReact")
	float HitReactPlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|HitReact")
	bool bHitReactStopAllMontages = true;
	
	//Death Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Death")
	TObjectPtr<UAnimMontage> DeathMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Death", meta = (ClampMin = "0.01"))
	float DeathPlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Death")
	bool bDeathStopAllMontages = true;
	
	//Hit Feedback Content / Tuning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Feedback")
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Player|Combat|Feedback", meta = (ClampMin = "0.0"))
	float HitCameraShakeScale = 0.4f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Player|Combat|Feedback", meta = (ClampMin = "0.0"))
	float HitStopDuration = 0.1f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Player|Combat|Feedback", meta = (ClampMin = "0.0"))
	float FinisherHitStopDuration = 0.3f;
	
	//Runtime State
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerAttackActive = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bComboQueued = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	int32 ActiveComboIndex = 0;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerHitReacting = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerDeathStarted = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerDeathFinalized = false;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveHitReactMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDeathMontage;
	
	bool bHitStopActive = false;
	
	FTimerHandle HitStopTimerHandle;
};
