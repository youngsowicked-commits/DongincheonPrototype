// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Gameplay/Data/DIAttackData.h"
#include "Gameplay/Data/DIGuardData.h"
#include "Gameplay/Data/DIGrabData.h"
#include "Player/DIPlayerController.h"
#include "Components/QTEComponent.h"
#include "Gameplay/Data/DICombatTypes.h"
#include "DongincheonCharacter.generated.h"

enum class EDIGrabState : uint8;
class UHealthComponent;
class UTargetingComponent;
class UCombatComponent;
class UInteractionComponent;
class UInputAction;
class UDIGrabComponent;
class UDIHeatActionComponent;
class UDIHeatActionDefinition;
class UAnimMontage;
class UDICharacterAudioComponent;
class UCameraShakeBase;
class UAnimNotify_DIComboChainPoint;
class UDICombatVFXComponent;

struct FInputActionValue;

UENUM(BlueprintType)
enum class EPlayerDodgeDirection : uint8
{
	Backward,
	Left,
	Right
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerQTEInputSignature,EQTEInputType,InputType);
enum class EPlayerAttackMode : uint8
{
	None,
	LightCombo,
	Heavy
};

enum class EQueuedAttackType : uint8
{
	None,
	Light,
	Heavy
};

USTRUCT(BlueprintType)
struct FPlayerHeavyComboBranch
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TArray<FAttackConfig> Attacks;
};

UCLASS()
class DONGINCHEONPROTOTYPE_API ADongincheonCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADongincheonCharacter();
	
	UPROPERTY(BlueprintAssignable, Category = "Player|QTE")
	FPlayerQTEInputSignature OnQTEInputPressed;
	
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
	void SetPresentationInputLocked(bool blocked);
	
	UFUNCTION(BlueprintCallable, Category = "Player|Input")
	void SetInteractionInputLocked(bool bLocked);
	
	UFUNCTION(BlueprintPure, Category = "Player|Input")
	bool IsGameplayInputLocked() const;
	
	UFUNCTION(BlueprintCallable, Category = "Player|Combat")
	void SetCombatInputEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Player|Combat")
	bool IsCombatInputEnabled() const;
	
	void FinalizePlayerDeath();
	
	void BeginAttackMoveWindow(float MoveWindowDuration);
	void EndAttackMoveWindow();
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	//Input
	void HandleLockOnStarted(const FInputActionValue& Value);
	void HandleLockOnEnded(const FInputActionValue& Value);
	
	void HandleLockOnSwitch(const FInputActionValue& Value);
	
	void HandleMoveInputStartedOrTriggered(const FInputActionValue& Value);
	void HandleMoveInputCompleted(const FInputActionValue& Value);
	
	void RefreshMovementSpeed();
	
	void HandleRunStarted();
	void HandleRunEnded();
	
	void HandleDodgeInput();
	
	//Grab
	friend class UAnimNotify_GrabCheck;

	void HandleGrabInput();
	AActor* FindBestGrabTarget() const;
	void StartPlayerGrabAttempt();
	void TryCommitPlayerGrab();
	void HandleGrabStartMontageEnded(UAnimMontage* Montage,bool bInterrupted);
	
	void PlayPlayerGrabHold();
	void StopPlayerGrabHold(float BlendOutTime);
	
	void StartPlayerGrabAttack();
	void HandleGrabAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	void StartPlayerGrabRelease();
	void HandleGrabReleaseMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	UFUNCTION()
	void HandleGrabStateChanged(EDIGrabState PreviousState, EDIGrabState NewState);

	void StartPlayerBeingGrabbed();
	void HandleBeingGrabbedStartMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void PlayPlayerBeingGrabbedHold();
	
	UFUNCTION()
	void HandleGrabAttackReceived(float Damage, AActor* DamageCauser);
	
	UFUNCTION()
	void HandleGrabAttackHitConfirmed(AActor* HitActor,float AppliedDamage);

	void StartPlayerBeingGrabbedHitReact();
	void HandleBeingGrabbedHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
	void CancelPlayerGrab(float BlendOutTime);
	
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
	
	void HandleHeavyAttackInput();
	
	void StartPlayerComboAttack();
	
	void StartPlayerHeavyComboAttack();
	
	void StartPlayerAttack(const FAttackConfig& Attack);
	
	friend class UAnimNotify_DIComboChainPoint;
	bool TryAdvanceQueuedCombo();
	
	void StartAttackAssistMove(AActor* Target, const FAttackConfig& Attack);
	
	void UpdateAttackAssistMove(float DeltaTime);
	
	void StopAttackAssistMove();
	
	const TArray<FAttackConfig>* GetActiveHeavyCombo() const;
	
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
	void HandleCombatHitConfirmed(AActor* HitActor, FVector HitLocation, FName HitSocketName, float AppliedDamage,
		EDICombatImpactResult ImpactResult);
	
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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<UDIGrabComponent> GrabComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Components")
	TObjectPtr<UDIHeatActionComponent> HeatActionComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractionComponent> InteractionComponent;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = "Components",meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDICharacterAudioComponent> CharacterAudioComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDICombatVFXComponent> CombatVFXComponent;
	
	//Input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> AttackAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> HeavyAttackAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> GrabAction;
	
	
	
	//Move
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Player|Movement",meta=(ClampMin="0.0"))
	float ExplorationWalkSpeed = 200.0f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Player|Movement",meta=(ClampMin="0.0"))
	float ExplorationRunSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Player|Movement",meta=(ClampMin="0.0"))
	float CombatMoveSpeed = 500.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Player|Movement",meta=(ClampMin="0.0"))
	float LockOnMoveSpeed = 240.0f;
	
	//Run
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Player|Input")
	TObjectPtr<UInputAction> RunAction;
	
	//Dodge
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> DodgeAction;
	
	//Guard
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input")
	TObjectPtr<UInputAction> GuardAction;
	
	//LockOn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LockOnAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LockOnSwitchAction;

	//Dodge Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeBackwardMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeLeftMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge")
	TObjectPtr<UAnimMontage> DodgeRightMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge", meta = (ClampMin = "0.01"))
	float DodgePlayRate = 1.4f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge", meta = (ClampMin = "0.0"))
	float DodgeStrength = 850.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Dodge")
	bool bDodgeStopAllMontages = true;
	
	//Guard Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Guard")
	FGuardConfig GuardConfig;
	
	//Grab Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Grab")
	FGrabConfig GrabConfig;
	
	//Heat Action Content
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Player|Combat|HeatAction")
	TArray<TObjectPtr<UDIHeatActionDefinition>> HeatActionDefinitions;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Player|Combat|HeatAction",meta = (ClampMin = "0.0"))
	float HeatGainPerHit = 10.0f;

	AActor* ResolveContextualHeatActionTarget(const UDIHeatActionDefinition*& OutDefinition) const;
	
	bool TryStartContextualHeatAction();
	void StartPlayerHeatAction(AActor* Target,const UDIHeatActionDefinition& Definition);
	void HandleHeatActionMontageEnded(UAnimMontage* Montage,bool bInterrupted);
	void CancelPlayerHeatAction(float BlendOutTime = 0.0f);
	void StopHeatActionVictimMontage(float BlendOutTime);
	
	//Attack Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Attack")
	TArray<FAttackConfig> ComboAttacks;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Attack Assist", meta = (ClampMin = "100.0"))
	float AttackAssistMaxSpeed = 950.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Attack Assist", meta = (ClampMin = "0.0"))
	float AttackFacingFollowSpeed = 720.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Attack Assist", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float AttackAssistMinSpeedScale = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Attack Assist", meta = (ClampMin = "0.5", ClampMax = "4.0"))
	float AttackAssistEaseExponent = 1.5f;
	
	// RMB → RMB → RMB...
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Attack")
	TArray<FAttackConfig> NeutralHeavyCombo;

	// [0] = Light 1타 후 Heavy Chain
	// [1] = Light 2타 후 Heavy Chain
	// [2] = Light 3타 후 Heavy Chain ...
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Attack")
	TArray<FPlayerHeavyComboBranch> HeavyBranches;
	
	//Grab Content
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Grab", meta = (ClampMin = "0.0"))
	float GrabSearchRadius = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|Combat|Grab",  meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float GrabMinForwardDot = 0.35f;
	
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
	bool bCombatInputEnabled = false;
	
	bool bRunInputHeld = false;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Player|Combat|Runtime")
	bool bPlayerAttackActive = false;
	
	TWeakObjectPtr<AActor> AttackAssistMoveTarget;

	FVector AttackMoveFallbackDirection = FVector::ZeroVector;

	float AttackAssistPreferredDistance = 0.0f;
	float AttackAssistMaxDistance = 0.0f;

	float AttackMoveBaseRemainingDistance = 0.0f;
	float AttackMoveInitialDistance = 0.0f;
	float AttackMoveBaseSpeed = 0.0f;
	
	float AttackMoveWindowTimeRemaining = 0.0f;
	
	float AttackFacingStartYaw = 0.0f;
	float AttackFacingMaxAngle = 0.0f;
	
	bool bAttackMoveWindowRequired = false;
	bool bAttackMoveWindowOpen = false;
	
	EPlayerAttackMode ActiveAttackMode = EPlayerAttackMode::None;
	
	EQueuedAttackType QueuedAttackType = EQueuedAttackType::None;

	int32 ActiveHeavyBranchIndex = INDEX_NONE;
	int32 ActiveHeavyComboIndex = INDEX_NONE;
	
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
	TObjectPtr<UAnimMontage> ActiveGrabStartMontage = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveGrabHoldMontage = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveGrabAttackMontage = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveGrabReleaseMontage = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveHeatActionMontage = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveHeatActionVictimMontage = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> ActiveHeatActionVictimCharacter;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveBeingGrabbedStartMontage = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveBeingGrabbedHoldMontage = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveBeingGrabbedHitReactMontage = nullptr;
	
	float PreGrabWalkSpeed = 0.0f;
	bool bGrabWalkSpeedOverridden = false;
	
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveDeathMontage;
	
	FVector2D CachedMoveInput = FVector2D::ZeroVector;
	
	bool bHitStopActive = false;
	
	FTimerHandle HitStopTimerHandle;
};
