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
class UCameraShakeBase;

struct FInputActionValue;

UENUM(BlueprintType)
enum class EPlayerDodgeDirection : uint8
{
	Backward,
	Left,
	Right
};

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
	
	UFUNCTION(BlueprintCallable, Category = "Player|Input")
	void SetPrentationInputLocked(bool blocked);
	
	UFUNCTION(BlueprintCallable, Category = "Player|Input")
	void SetInteractionInputLocked(bool bLocked);
	
	UFUNCTION(BlueprintPure, Category = "Player|Input")
	bool IsGameplayInputLocked() const
	{
		return bInteractionInputLocked || bPresentationInputLocked;
	}
	
	void FinalizePlayerDeath();
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	//Input
	void HandleLockOnStarted(const FInputActionValue& Value);
	void HandleLockOnEnded(const FInputActionValue& Value);
	
	void HandleMoveInputStartedOrTriggered(const FInputActionValue& Value);
	void HandleMoveInputCompleted(const FInputActionValue& Value);
	
	void HandleDodgeInput();
	
	//Guard
	void HandleGuardStarted();
	void HandleGuardEnded();
	
	//Dodge
	void StartPlayerDodge();
	
	EPlayerDodgeDirection ResolvePlayerDodgeDirection() const;
	
	UAnimMontage* GetDodgeMontage(EPlayerDodgeDirection Direction) const;
	
	FVector GetDodgeWorldDirection(EPlayerDodgeDirection Direction) const;
	
	void HandleDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	void CancelPlayerDodge(float BlendOutTime = 0.0f);
	
	void ResetPlayerDodgeState();
	
	//Guard
	void StartPlayerGuard();
	
	void StopPlayerGuard(float BlendOutTime = 0.1f);
	
	void HandleGuardMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	void StartPlayerGuardHitReact();
	
	void HandleGuardHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	void StartPlayerGuardBreak();
	
	void HandleGuardBreakMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	void FinishPlayerGuardBreak();
	
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
	
	//Move
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> MoveAction;
	
	//Dodge
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> DodgeAction;
	
	//Guard
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> GuardAction;
	
	//LockOn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LockOnAction;
	

	//Dodge Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeBackwardMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeLeftMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeRightMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge", meta = (ClampMin = "0.01"))
	float DodgePlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge", meta = (ClampMin = "0.0"))
	float DodgeStrength = 650.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge")
	bool bDodgeStopAllMontages = true;
	
	//Guard Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Guard")
	TObjectPtr<UAnimMontage> GuardMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Guard")
	TObjectPtr<UAnimMontage> GuardHitReactMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Guard")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Guard",
		meta = (ClampMin = "0.01"))
	float GuardBreakPlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Guard", meta = (ClampMin = "0.01"))
	float GuardPlayRate = 1.0f;
	
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
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Input|Runtime")
	bool bPresentationInputLocked = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Input|Runtime")
	bool bInteractionInputLocked = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerAttackActive = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bComboQueued = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	int32 ActiveComboIndex = 0;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerDodging = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bGuardInputHeld = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerHitReacting = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerDeathStarted = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerDeathFinalized = false;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDodgeMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveGuardMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveGuardHitReactMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveGuardBreakMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveHitReactMontage;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDeathMontage;
	
	FVector2D CachedMoveInput = FVector2D::ZeroVector;
	
	bool bHitStopActive = false;
	
	FTimerHandle HitStopTimerHandle;
};
