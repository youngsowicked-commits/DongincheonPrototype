// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DongincheonCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/TargetingComponent.h"
#include "Components/HealthComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "TimerManager.h"

// Sets default values
ADongincheonCharacter::ADongincheonCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("Targeting"));
}

//BeginPlay
void ADongincheonCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsValid(HealthComponent))
	{
		HealthComponent->OnDamaged.AddUniqueDynamic(this, &ADongincheonCharacter::HandleHealthDamaged);
		HealthComponent->OnDeath.AddUniqueDynamic(this, &ADongincheonCharacter::HandleHealthDeath);
	}
	
	if (IsValid(CombatComponent))
	{
		CombatComponent->OnHitConfirmed.AddUniqueDynamic(this,&ADongincheonCharacter::HandleCombatHitConfirmed);
	}
}

//Input
void ADongincheonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	
	if (!IsValid(EnhancedInput))
	{
		return;
	}
	
	if (IsValid(AttackAction))
	{
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &ADongincheonCharacter::HandleAttackInput);
	}
	
	if (IsValid(LockOnAction))
	{
		EnhancedInput->BindAction(LockOnAction,ETriggerEvent::Started,this,&ADongincheonCharacter::HandleLockOnStarted);
		EnhancedInput->BindAction(LockOnAction,ETriggerEvent::Completed,this,&ADongincheonCharacter::HandleLockOnEnded);
		EnhancedInput->BindAction(LockOnAction,ETriggerEvent::Canceled,this,&ADongincheonCharacter::HandleLockOnEnded);
	}
}

void ADongincheonCharacter::HandleLockOnStarted(const FInputActionValue& Value)
{
	(void)Value;
	
	if (IsValid(TargetingComponent))
	{
		TargetingComponent->TryLockOn();
	}
}

void ADongincheonCharacter::HandleLockOnEnded(const FInputActionValue& Value)
{
	(void)Value;
	
	if (IsValid(TargetingComponent))
	{
		TargetingComponent->ClearLockOn();
	}
}

// Runtime Query
bool ADongincheonCharacter::IsMovementInputAllowed() const
{
	if (!IsValid(HealthComponent))
	{
		return false;
	}
	
	if (HealthComponent->IsDead())
	{
		return false;
	}
	
	return !bPlayerAttackActive && !bPlayerHitReacting && !bPlayerDeathStarted;
}

//Attack Input
void ADongincheonCharacter::HandleAttackInput()
{
	if (!IsValid(HealthComponent) || !IsValid(CombatComponent))
	{
		return;
	}
	
	if (HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting)
	{
		return;
	}
	
	if (ComboAttacks.IsEmpty())
	{
		return;
	}
	
	//새 Combo 시작
	if (!bPlayerAttackActive)
	{
		bPlayerAttackActive = true;
		bComboQueued = false;
		ActiveComboIndex = 0;
		
		StartPlayerComboAttack();
		return;
	}
	
	//현재 Montage가 끝났을 떄 다음 타로 연결,
	if (ActiveComboIndex + 1 < ComboAttacks.Num())
	{
		bComboQueued = true;
	}
}

//Attack LifeCycle
void ADongincheonCharacter::StartPlayerComboAttack()
{
	if (!ComboAttacks.IsValidIndex(ActiveComboIndex))
	{
		ResetPlayerAttackState();
		return;
	}
	
	if (!IsValid(CombatComponent) || !IsValid(GetMesh()))
	{
		ResetPlayerAttackState();
		return;
	}
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (!IsValid(AnimInstance))
	{
		ResetPlayerAttackState();
		return;
	}
	
	const FAttackConfig& Attack = ComboAttacks[ActiveComboIndex];
	
	if (!IsValid(Attack.Montage))
	{
		ResetPlayerAttackState();
		return;
	}
	
	const float SafePlayRate = Attack.PlayRate > 0.0f ? Attack.PlayRate : 1.0f;
	
	CombatComponent->BeginAttack(Attack.Damage,Attack.KnockbackStrength);
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		
		if (Attack.LungeStrength > 0.0f)
		{
			const FVector LungeVelocity = GetActorForwardVector() * Attack.LungeStrength;
			
			Movement->AddImpulse(LungeVelocity, true);
		}
	}
	
	const float MontageLength = AnimInstance->Montage_Play(Attack.Montage,SafePlayRate,EMontagePlayReturnType::MontageLength,
		0.0f,Attack.bStopAllMontages);
	
	if (MontageLength <= 0.0)
	{
		ResetPlayerAttackState();
		return;
	}
	
	ActiveAttackMontage = Attack.Montage;
	
	FOnMontageEnded MontageEndedDelegate;
	
	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleAttackMontageEnded);
	
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveAttackMontage);
}

void ADongincheonCharacter::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveAttackMontage)
	{
		return;
	}
	
	ActiveAttackMontage = nullptr;
	
	ClearHitStop();
	
	if (IsValid(CombatComponent))
	{
		CombatComponent->EndAttack();
	}
	
	if (IsValid(CombatComponent))
	{
		CombatComponent->EndAttack();
	}
	
	if (bInterrupted || bPlayerHitReacting || bPlayerDeathStarted || (IsValid(HealthComponent) && HealthComponent->IsDead()))
	{
		bPlayerAttackActive = false;
		bComboQueued = false;
		ActiveComboIndex = 0;
		return;
	}
	
	if (bComboQueued && ActiveComboIndex + 1 < ComboAttacks.Num())
	{
		bComboQueued = false;
		++ActiveComboIndex;
		
		StartPlayerComboAttack();
		return;
	}
	
	bPlayerAttackActive = false;
	bComboQueued = false;
	ActiveComboIndex = 0;
}

void ADongincheonCharacter::CancelPlayerAttack(float BlendOutTime)
{
	UAnimMontage* MontageToStop = ActiveAttackMontage;
	
	ActiveAttackMontage = nullptr;
	
	ClearHitStop();
	
	if (IsValid(CombatComponent))
	{
		CombatComponent->EndAttack();
	}
	
	bPlayerAttackActive = false;
	bComboQueued = false;
	ActiveComboIndex = 0;
	
	if (!IsValid(MontageToStop) || !IsValid(GetMesh()))
	{
		return;
	}
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (!IsValid(AnimInstance))
	{
		return;
	}
	
	AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), MontageToStop);
}

void ADongincheonCharacter::ResetPlayerAttackState()
{
	ActiveAttackMontage = nullptr;
	
	ClearHitStop();
	
	if (IsValid(CombatComponent))
	{
		CombatComponent->EndAttack();
	}
	
	bPlayerAttackActive = false;
	bComboQueued = false;
	ActiveComboIndex = 0;
}

//Health
void ADongincheonCharacter::HandleHealthDamaged(float DamageAmount, AActor* DamageCauser)
{
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}
	
	StartPlayerHitReact();
}

void ADongincheonCharacter::HandleHealthDeath(AActor* DamageCauser)
{
	StartPlayerDeath();
}

//Hit React
void ADongincheonCharacter::StartPlayerHitReact()
{
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}
	
	CancelPlayerAttack(0.05f);
	
	if (!IsValid(GetMesh()))
	{
		return;
	}
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (!IsValid(AnimInstance))
	{
		return;
	}
	
	//기존 HitReact가 재생중이면서 새 HitReact가 소유권을 가져간다.
	if (IsValid(ActiveHitReactMontage))
	{
		UAnimMontage* PreviousHitReact = ActiveHitReactMontage;
		
		ActiveHitReactMontage = nullptr;
		
		AnimInstance->Montage_Stop(0.05f, PreviousHitReact);
	}
	
	bPlayerHitReacting = true;
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	
	if (!IsValid(HitReactMontage))
	{
		FinishPlayerHitReact();
		return;
	}
	
	const float SafePlayRate = HitReactPlayRate > 0.0f ? HitReactPlayRate : 1.0f;
	const float MontageLength = AnimInstance->Montage_Play(HitReactMontage, SafePlayRate, EMontagePlayReturnType::MontageLength,
		0.0f,bHitReactStopAllMontages);
	
	if (MontageLength <= 0.0f)
	{
		FinishPlayerHitReact();
		return;
	}
	
	ActiveHitReactMontage = HitReactMontage;
	
	FOnMontageEnded MontageEndedDelegate;
	
	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleHitReactMontageEnded);
	
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveHitReactMontage);
}

void ADongincheonCharacter::HandleHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveHitReactMontage)
	{
		return;
	}
	
	ActiveHitReactMontage = nullptr;
	
	FinishPlayerHitReact();
}

void ADongincheonCharacter::FinishPlayerHitReact()
{
	bPlayerHitReacting = false;
	
	if (bPlayerDeathStarted || !IsValid(HealthComponent) || HealthComponent->IsDead())
	{
		return;
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
}

//Death
void ADongincheonCharacter::StartPlayerDeath()
{
	if (bPlayerDeathStarted)
	{
		return;
	}
	
	bPlayerDeathStarted = true;
	bPlayerHitReacting = false;
	
	CancelPlayerAttack(0.0f);
	
	if (!IsValid(GetMesh()))
	{
		FinalizePlayerDeath();
		return;
	}
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (!IsValid(AnimInstance))
	{
		FinalizePlayerDeath();
		return;
	}
	
	if (IsValid(ActiveHitReactMontage))
	{
		UAnimMontage* PreviousHitReact = ActiveHitReactMontage;
		
		ActiveHitReactMontage = nullptr;
		
		AnimInstance->Montage_Stop(0.0f, PreviousHitReact);
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	
	if (!IsValid(DeathMontage))
	{
		FinalizePlayerDeath();
		return;
	}
	
	const float SafePlayRate = DeathPlayRate > 0.0f ? DeathPlayRate : 1.0f;
	
	const float MontageLength = AnimInstance->Montage_Play(DeathMontage, SafePlayRate, EMontagePlayReturnType::MontageLength,
		0.0f, bDeathStopAllMontages);
	
	if (MontageLength <= 0.0f)
	{
		FinalizePlayerDeath();
		return;
	}
	
	ActiveDeathMontage = DeathMontage;
	
	//Auto Blend Out이 켜진 Montage에 대한 fallback, DeathEnd Native Notify는 다음 단계에서 별도로 추가
	FOnMontageEnded MontageEndedDelegate;
	
	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleDeathMontageEnded);
	
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveDeathMontage);
}

void ADongincheonCharacter::HandleDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveDeathMontage)
	{
		return;
	}
	
	ActiveDeathMontage = nullptr;
	
	FinalizePlayerDeath();
}

void ADongincheonCharacter::FinalizePlayerDeath()
{
	if (bPlayerDeathFinalized)
	{
		return;	
	}
	
	bPlayerDeathFinalized = true;
	
	ClearHitStop();
	
	if (IsValid(CombatComponent))
	{
		CombatComponent->EndAttack();
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

//Hit Confirm Feedback
void ADongincheonCharacter::HandleCombatHitConfirmed(AActor* HitActor, FVector HitLocation, FName HitSocketName,
	float AppliedDamage)
{
	if (AppliedDamage <= 0.0f)
	{
		return;
	}
	
	//동일 타격에서 여러 Actor가 잡혀도 HitStop이 중복 시작되지 않게 한다.
	if (bHitStopActive)
	{
		return;
	}
	
	if (HitCameraShakeClass)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
			{
				CameraManager->StartCameraShake(HitCameraShakeClass,HitCameraShakeScale,ECameraShakePlaySpace::CameraLocal,
					FRotator::ZeroRotator);
			}
		}
	}
	
	StartHitStop();
}

void ADongincheonCharacter::StartHitStop()
{
	if (!IsValid(ActiveAttackMontage) || !IsValid(GetMesh()))
	{
		return;
	}
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (!IsValid(AnimInstance))
	{
		return;
	}
	
	const bool bIsFinisher = ComboAttacks.Num() > 0 && ActiveComboIndex == ComboAttacks.Num() - 1;
	const float Duration = bIsFinisher ? FinisherHitStopDuration : HitStopDuration;
	
	if (Duration <= 0.0f)
	{
		return;
	}
	
	bHitStopActive = true;
	
	AnimInstance->Montage_Pause(ActiveAttackMontage);
	
	GetWorldTimerManager().SetTimer(HitStopTimerHandle,this,&ADongincheonCharacter::ResumeHitStop,Duration,false);
}

void ADongincheonCharacter::ResumeHitStop()
{
	if (IsValid(ActiveAttackMontage) && IsValid(GetMesh()))
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Resume(ActiveAttackMontage);
		}
	}
	
	bHitStopActive = false;
}

void ADongincheonCharacter::ClearHitStop()
{
	GetWorldTimerManager().ClearTimer(HitStopTimerHandle);
	
	bHitStopActive = false;
}


