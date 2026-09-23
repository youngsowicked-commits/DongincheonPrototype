// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DongincheonCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/TargetingComponent.h"
#include "Components/HealthComponent.h"
#include "Components/DIGrabComponent.h"
#include "Components/DIHeatActionComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
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
	GrabComponent = CreateDefaultSubobject<UDIGrabComponent>(TEXT("GrabComponent"));
	HeatActionComponent = CreateDefaultSubobject<UDIHeatActionComponent>(TEXT("HeatActionComponent"));
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
	
	if (IsValid(GrabComponent))
	{
		GrabComponent->OnGrabStateChanged.AddDynamic(this,&ADongincheonCharacter::HandleGrabStateChanged);
		GrabComponent->OnGrabAttackReceived.AddDynamic(this,&ADongincheonCharacter::HandleGrabAttackReceived);
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
	
	if (IsValid(MoveAction))
	{
		EnhancedInput->BindAction(MoveAction,ETriggerEvent::Triggered,this,&ADongincheonCharacter::HandleMoveInputStartedOrTriggered);
		EnhancedInput->BindAction(MoveAction,ETriggerEvent::Completed,this,&ADongincheonCharacter::HandleMoveInputCompleted);
		EnhancedInput->BindAction(MoveAction,ETriggerEvent::Canceled,this,&ADongincheonCharacter::HandleMoveInputCompleted);
	}
	
	if (IsValid(DodgeAction))
	{
		EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Started,this,&ADongincheonCharacter::HandleDodgeInput);
	}
	
	if (IsValid(GuardAction))
	{
		EnhancedInput->BindAction(GuardAction,ETriggerEvent::Started,this,&ADongincheonCharacter::HandleGuardStarted);
		EnhancedInput->BindAction(GuardAction,ETriggerEvent::Completed,this,&ADongincheonCharacter::HandleGuardEnded);
		EnhancedInput->BindAction(GuardAction,ETriggerEvent::Canceled,this,&ADongincheonCharacter::HandleGuardEnded);
	}
	
	if (IsValid(AttackAction))
	{
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &ADongincheonCharacter::HandleAttackInput);
	}
	
	if (IsValid(HeavyAttackAction))
	{
		EnhancedInput->BindAction(HeavyAttackAction,ETriggerEvent::Started,this,&ADongincheonCharacter::HandleHeavyAttackInput);
	}
	
	if (IsValid(GrabAction))
	{
		EnhancedInput->BindAction(GrabAction,ETriggerEvent::Started,this,&ADongincheonCharacter::HandleGrabInput);
	}
	
	if (IsValid(LockOnAction))
	{
		EnhancedInput->BindAction(LockOnAction,ETriggerEvent::Started,this,&ADongincheonCharacter::HandleLockOnStarted);
		EnhancedInput->BindAction(LockOnAction,ETriggerEvent::Completed,this,&ADongincheonCharacter::HandleLockOnEnded);
		EnhancedInput->BindAction(LockOnAction,ETriggerEvent::Canceled,this,&ADongincheonCharacter::HandleLockOnEnded);
	}
	
	if (IsValid(LockOnSwitchAction))
	{
		EnhancedInput->BindAction(LockOnSwitchAction,ETriggerEvent::Triggered,this,&ADongincheonCharacter::HandleLockOnSwitch);
	}
}

void ADongincheonCharacter::HandleLockOnSwitch(const FInputActionValue& Value)
{
	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!IsValid(TargetingComponent))
	{
		return;
	}

	if (!TargetingComponent->IsLockedOn())
	{
		return;
	}

	const float Direction = Value.Get<float>();

	if (FMath::IsNearlyZero(Direction))
	{
		return;
	}

	TargetingComponent->SwitchTarget(Direction);
}

void ADongincheonCharacter::HandleLockOnStarted(const FInputActionValue& Value)
{
	(void)Value;
	
	if (IsGameplayInputLocked())
	{
		return;
	}
	
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}
	
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

void ADongincheonCharacter::HandleMoveInputStartedOrTriggered(const FInputActionValue& Value)
{
	if (IsGameplayInputLocked())
	{
		CachedMoveInput = FVector2D::ZeroVector;
		return;
	}
	
	CachedMoveInput = Value.Get<FVector2D>();
}

void ADongincheonCharacter::HandleMoveInputCompleted(const FInputActionValue& Value)
{
	(void)Value;
	
	CachedMoveInput = FVector2D::ZeroVector;
}


// Runtime Query
bool ADongincheonCharacter::IsMovementInputAllowed() const
{
	if (IsGameplayInputLocked()) return false;
	if (!IsValid(HealthComponent) || HealthComponent->IsDead()) return false;

	if (IsValid(GrabComponent))
	{
		if (GrabComponent->IsBeingGrabbed()) return false;
		if (GrabComponent->IsGrabbing() && !GrabComponent->IsHoldingGrab()) return false;
	}

	const bool bGuardLocked = IsValid(CombatComponent) && (CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken());

	return !bPlayerAttackActive && !bPlayerDodging && !bGuardLocked && !bPlayerHitReacting && !bPlayerDeathStarted;
}

void ADongincheonCharacter::SetPresentationInputLocked(bool blocked)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("PLAYER PRESENTATION LOCK REQUEST | Player=%s | Current=%s | Requested=%s"),
		*GetNameSafe(this),
		bPresentationInputLocked ? TEXT("TRUE") : TEXT("FALSE"),
		blocked ? TEXT("TRUE") : TEXT("FALSE"));
	
	if (bPresentationInputLocked == blocked)
	{
		return;
	}
	
	bPresentationInputLocked = blocked;
	
	if (!blocked)
	{
		return;
	}
	
	//진행중이던 일반 Gameplay Action 정리
	CancelPlayerAttack(0.05f);
	CancelPlayerDodge(0.05f);
	CancelPlayerGrab(0.05f);
	
	bGuardInputHeld = false;
	
	if (IsValid(CombatComponent) && CombatComponent->IsGuarding())
	{
		StopPlayerGuard(0.05f);
	}
	
	CachedMoveInput = FVector2D::ZeroVector;
	
	if (IsValid(TargetingComponent))
	{
		TargetingComponent->ClearLockOn();
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
}

void ADongincheonCharacter::SetInteractionInputLocked(bool bLocked)
{
	bInteractionInputLocked = bLocked;
}

//Attack Input
void ADongincheonCharacter::HandleAttackInput()
{
	if (bPresentationInputLocked)
	{
		OnQTEInputPressed.Broadcast(EQTEInputType::Light);
		return;
	}

	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!IsValid(HealthComponent) || !IsValid(CombatComponent) || !IsValid(GrabComponent))
	{
		return;
	}

	if (HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging ||
		CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken())
	{
		return;
	}
	
	if (GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	if (GrabComponent->IsHoldingGrab())
	{
		StartPlayerGrabAttack();
		return;
	}

	if (GrabComponent->IsGrabbing())
	{
		return;
	}

	// 새 Light Combo 시작
	if (!bPlayerAttackActive)
	{
		if (ComboAttacks.IsEmpty())
		{
			return;
		}

		bPlayerAttackActive = true;

		ActiveAttackMode = EPlayerAttackMode::LightCombo;
		QueuedAttackType = EQueuedAttackType::None;

		ActiveComboIndex = 0;
		ActiveHeavyBranchIndex = INDEX_NONE;
		ActiveHeavyComboIndex = INDEX_NONE;

		StartPlayerComboAttack();
		return;
	}

	// Heavy 체인 중에는 LMB로 다시 Light로 돌아가지 않음
	if (ActiveAttackMode != EPlayerAttackMode::LightCombo)
	{
		return;
	}

	// 이미 Heavy가 예약돼 있으면 Light가 덮어쓰지 않음
	if (QueuedAttackType == EQueuedAttackType::Heavy)
	{
		return;
	}

	if (ActiveComboIndex + 1 < ComboAttacks.Num())
	{
		QueuedAttackType = EQueuedAttackType::Light;
	}
}

AActor* ADongincheonCharacter::ResolveContextualHeatActionTarget(const FHeatActionConfig*& OutConfig) const
{
	OutConfig = nullptr;

	if (!IsValid(HeatActionComponent))
	{
		return nullptr;
	}

	if (IsValid(GrabComponent) && GrabComponent->IsHoldingGrab())
	{
		AActor* Target = GrabComponent->GetGrabbedActor();

		if (!HeatActionComponent->CanStartHeatAction(Target,GrabHeatActionConfig))
		{
			return nullptr;
		}

		OutConfig = &GrabHeatActionConfig;
		return Target;
	}

	if (IsValid(GrabComponent) && GrabComponent->IsGrabbing())
	{
		return nullptr;
	}

	AActor* Target = HeatActionComponent->FindBestHeatActionTarget(NormalHeatActionConfig);

	if (!HeatActionComponent->CanStartHeatAction(Target,NormalHeatActionConfig))
	{
		return nullptr;
	}

	OutConfig = &NormalHeatActionConfig;
	return Target;
}

void ADongincheonCharacter::HandleHeavyAttackInput()
{
	if (bPresentationInputLocked)
	{
		OnQTEInputPressed.Broadcast(EQTEInputType::Heavy);
		return;
	}

	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!IsValid(HealthComponent) || !IsValid(CombatComponent))
	{
		return;
	}

	if (HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging ||
		CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken())
	{
		return;
	}

	// 아무 Light도 안 친 상태에서 RMB
	if (!bPlayerAttackActive)
	{
		if (NeutralHeavyCombo.IsEmpty())
		{
			return;
		}

		bPlayerAttackActive = true;

		ActiveAttackMode = EPlayerAttackMode::Heavy;
		QueuedAttackType = EQueuedAttackType::None;

		ActiveComboIndex = 0;
		ActiveHeavyBranchIndex = INDEX_NONE;
		ActiveHeavyComboIndex = 0;

		StartPlayerHeavyComboAttack();
		return;
	}

	// Light 도중 RMB -> 현재 Light 단계의 Heavy Branch 예약
	if (ActiveAttackMode == EPlayerAttackMode::LightCombo)
	{
		if (!HeavyBranches.IsValidIndex(ActiveComboIndex))
		{
			return;
		}

		if (HeavyBranches[ActiveComboIndex].Attacks.IsEmpty())
		{
			return;
		}

		QueuedAttackType = EQueuedAttackType::Heavy;
		return;
	}

	// 이미 Heavy Branch 안에 들어온 뒤 RMB -> 다음 Heavy
	if (ActiveAttackMode == EPlayerAttackMode::Heavy)
	{
		const TArray<FAttackConfig>* HeavyCombo = GetActiveHeavyCombo();

		if (!HeavyCombo)
		{
			return;
		}

		if (ActiveHeavyComboIndex + 1 < HeavyCombo->Num())
		{
			QueuedAttackType = EQueuedAttackType::Heavy;
		}
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

	StartPlayerAttack(ComboAttacks[ActiveComboIndex]);
}

void ADongincheonCharacter::StartPlayerHeavyComboAttack()
{
	const TArray<FAttackConfig>* HeavyCombo = GetActiveHeavyCombo();

	if (!HeavyCombo || !HeavyCombo->IsValidIndex(ActiveHeavyComboIndex))
	{
		ResetPlayerAttackState();
		return;
	}

	StartPlayerAttack((*HeavyCombo)[ActiveHeavyComboIndex]);
}

void ADongincheonCharacter::StartPlayerAttack(const FAttackConfig& Attack)
{
	if (!IsValid(CombatComponent) || !IsValid(GetMesh()) || !IsValid(Attack.Montage))
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

	const float SafePlayRate = Attack.PlayRate > 0.0f ? Attack.PlayRate : 1.0f;

	CombatComponent->BeginAttack(Attack.Damage,Attack.KnockbackStrength,Attack.bBreaksGuard);

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();

		if (Attack.LungeStrength > 0.0f)
		{
			Movement->AddImpulse(GetActorForwardVector() * Attack.LungeStrength,true);
		}
	}

	const float MontageLength = AnimInstance->Montage_Play(Attack.Montage,SafePlayRate,EMontagePlayReturnType::MontageLength,
			0.0f,Attack.bStopAllMontages);

	if (MontageLength <= 0.0f)
	{
		ResetPlayerAttackState();
		return;
	}

	ActiveAttackMontage = Attack.Montage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleAttackMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveAttackMontage);
}

const TArray<FAttackConfig>* ADongincheonCharacter::GetActiveHeavyCombo() const
{
	// Light 없이 시작한 RMB 체인
	if (ActiveHeavyBranchIndex == INDEX_NONE)
	{
		return &NeutralHeavyCombo;
	}

	// Light N타 후 들어온 Heavy Branch
	if (!HeavyBranches.IsValidIndex(ActiveHeavyBranchIndex))
	{
		return nullptr;
	}

	return &HeavyBranches[ActiveHeavyBranchIndex].Attacks;
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

    if (bInterrupted || bPlayerHitReacting || bPlayerDeathStarted || (IsValid(HealthComponent) && HealthComponent->IsDead()))
    {
        bPlayerAttackActive = false;
        ActiveAttackMode = EPlayerAttackMode::None;
        QueuedAttackType = EQueuedAttackType::None;

        ActiveComboIndex = 0;
        ActiveHeavyBranchIndex = INDEX_NONE;
        ActiveHeavyComboIndex = INDEX_NONE;
        return;
    }

    // Light Combo가 끝난 경우
    if (ActiveAttackMode == EPlayerAttackMode::LightCombo)
    {
        // RMB가 예약되어 있으면 현재 Light 단계의 Heavy Branch로 진입
        if (QueuedAttackType == EQueuedAttackType::Heavy)
        {
            if (HeavyBranches.IsValidIndex(ActiveComboIndex) && !HeavyBranches[ActiveComboIndex].Attacks.IsEmpty())
            {
                ActiveAttackMode = EPlayerAttackMode::Heavy;

                ActiveHeavyBranchIndex = ActiveComboIndex;
                ActiveHeavyComboIndex = 0;

                QueuedAttackType = EQueuedAttackType::None;

                StartPlayerHeavyComboAttack();
                return;
            }
        }

        // LMB가 예약되어 있으면 다음 Light
        if (QueuedAttackType == EQueuedAttackType::Light && ActiveComboIndex + 1 < ComboAttacks.Num())
        {
            QueuedAttackType = EQueuedAttackType::None;

            ++ActiveComboIndex;

            StartPlayerComboAttack();
            return;
        }
    }

    // Heavy Combo가 끝난 경우
    if (ActiveAttackMode == EPlayerAttackMode::Heavy && QueuedAttackType == EQueuedAttackType::Heavy)
    {
        const TArray<FAttackConfig>* HeavyCombo =
            GetActiveHeavyCombo();

        if (HeavyCombo &&
            ActiveHeavyComboIndex + 1 < HeavyCombo->Num())
        {
            QueuedAttackType = EQueuedAttackType::None;

            ++ActiveHeavyComboIndex;

            StartPlayerHeavyComboAttack();
            return;
        }
    }

    // 더 이상 이어질 공격 없음
    bPlayerAttackActive = false;
    ActiveAttackMode = EPlayerAttackMode::None;
    QueuedAttackType = EQueuedAttackType::None;

    ActiveComboIndex = 0;
    ActiveHeavyBranchIndex = INDEX_NONE;
    ActiveHeavyComboIndex = INDEX_NONE;
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

	ActiveAttackMode = EPlayerAttackMode::None;
	QueuedAttackType = EQueuedAttackType::None;

	ActiveComboIndex = 0;
	ActiveHeavyBranchIndex = INDEX_NONE;
	ActiveHeavyComboIndex = INDEX_NONE;

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

	ActiveAttackMode = EPlayerAttackMode::None;
	QueuedAttackType = EQueuedAttackType::None;

	ActiveComboIndex = 0;
	ActiveHeavyBranchIndex = INDEX_NONE;
	ActiveHeavyComboIndex = INDEX_NONE;
}

//Health
void ADongincheonCharacter::HandleHealthDamaged(float DamageAmount, AActor* DamageCauser)
{
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}

	if (IsValid(GrabComponent) && GrabComponent->IsBeingGrabbed())
	{
		return;
	}
    
	StartPlayerHitReact();
}

void ADongincheonCharacter::HandleHealthDeath(AActor* DamageCauser)
{
	StartPlayerDeath();
}

//Dodge
void ADongincheonCharacter::HandleDodgeInput()
{
	if (bPresentationInputLocked)
	{
		OnQTEInputPressed.Broadcast(EQTEInputType::Dodge);
		return;
	}

	if (IsGameplayInputLocked())
	{
		return;
	}
	
	if (!IsValid(HealthComponent) || HealthComponent->IsDead())
	{
		return;
	}
	
	if (bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging || bPlayerAttackActive || (IsValid(CombatComponent) &&
		 (CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken())))	
	{
		return;
	}
	
	StartPlayerDodge();
}

void ADongincheonCharacter::HandleGrabInput()
{
	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!IsValid(HealthComponent) || !IsValid(CombatComponent) || !IsValid(GrabComponent))
	{
		return;
	}

	if (HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging ||
		bPlayerAttackActive || CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken())
	{
		return;
	}

	if (GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	if (GrabComponent->IsHoldingGrab())
	{
		StartPlayerGrabRelease();
		return;
	}

	if (GrabComponent->IsGrabbing())
	{
		return;
	}

	if (IsValid(ActiveGrabStartMontage))
	{
		return;
	}

	StartPlayerGrabAttempt();
}

AActor* ADongincheonCharacter::FindBestGrabTarget() const
{
    if (!IsValid(GrabComponent) || !IsValid(GetWorld()) || GrabSearchRadius <= 0.0f)
    {
        return nullptr;
    }

    TArray<FOverlapResult> Overlaps;

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerGrabSearch),false,this);

    const bool bFoundAny = GetWorld()->OverlapMultiByObjectType(Overlaps,GetActorLocation(),FQuat::Identity,
            ObjectQueryParams,FCollisionShape::MakeSphere(GrabSearchRadius),QueryParams);

    if (!bFoundAny)
    {
        return nullptr;
    }

    AActor* BestTarget = nullptr;
    float BestDistanceSquared = TNumericLimits<float>::Max();

    const FVector PlayerLocation = GetActorLocation();
    const FVector PlayerForward = GetActorForwardVector().GetSafeNormal2D();

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();

        if (!IsValid(Candidate) || Candidate == this)
        {
            continue;
        }

        if (!GrabComponent->CanStartGrab(Candidate))
        {
            continue;
        }

        const FVector ToCandidate = Candidate->GetActorLocation() - PlayerLocation;

        const FVector Direction = ToCandidate.GetSafeNormal2D();

        if (Direction.IsNearlyZero())
        {
            continue;
        }

        const float ForwardDot = FVector::DotProduct(PlayerForward, Direction);

        if (ForwardDot < GrabMinForwardDot)
        {
            continue;
        }

        const float DistanceSquared = ToCandidate.SizeSquared2D();

        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

void ADongincheonCharacter::StartPlayerGrabAttempt()
{
	if (!IsValid(GrabComponent) || !IsValid(GetMesh()) || !IsValid(GrabConfig.GrabStartMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;
	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.GrabStartMontage,SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		return;
	}

	ActiveGrabStartMontage = GrabConfig.GrabStartMontage;

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleGrabStartMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveGrabStartMontage);
}

void ADongincheonCharacter::TryCommitPlayerGrab()
{
	if (!IsValid(GrabComponent) || !IsValid(ActiveGrabStartMontage))
	{
		return;
	}

	if (GrabComponent->IsGrabbing() || GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	AActor* Target = FindBestGrabTarget();

	if (!IsValid(Target))
	{
		return;
	}

	GrabComponent->BeginGrab(Target);
}

void ADongincheonCharacter::HandleGrabStartMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveGrabStartMontage)
	{
		return;
	}

	ActiveGrabStartMontage = nullptr;

	if (!IsValid(GrabComponent))
	{
		return;
	}

	if (bInterrupted)
	{
		if (GrabComponent->IsGrabbing())
		{
			GrabComponent->ForceRelease();
		}

		return;
	}

	if (GrabComponent->GetGrabState() != EDIGrabState::Starting)
	{
		return;
	}

	GrabComponent->CompleteGrabStart(GrabConfig.GrabHoldDistance,GrabConfig.GrabVictimYawOffset);

	if (GrabComponent->IsHoldingGrab())
	{
		PlayPlayerGrabHold();
	}
}


void ADongincheonCharacter::PlayPlayerGrabHold()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsHoldingGrab() || !IsValid(GetMesh()) || !IsValid(GrabConfig.GrabHoldMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;
	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.GrabHoldMontage,SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		return;
	}

	ActiveGrabHoldMontage = GrabConfig.GrabHoldMontage;
}

void ADongincheonCharacter::StopPlayerGrabHold(float BlendOutTime)
{
	UAnimMontage* MontageToStop = ActiveGrabHoldMontage;
	ActiveGrabHoldMontage = nullptr;

	if (!IsValid(MontageToStop) || !IsValid(GetMesh()))
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(FMath::Max(0.0f,BlendOutTime),MontageToStop);
	}
}

void ADongincheonCharacter::StartPlayerGrabAttack()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsHoldingGrab() || !IsValid(GetMesh()) || !IsValid(GrabConfig.GrabAttackMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	if (!GrabComponent->BeginGrabAttack(GrabConfig.GrabAttackDamage))
	{
		return;
	}

	StopPlayerGrabHold(0.05f);
	
	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.GrabAttackMontage,SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		GrabComponent->EndGrabAttack();
		PlayPlayerGrabHold();
		return;
	}

	ActiveGrabAttackMontage = GrabConfig.GrabAttackMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleGrabAttackMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveGrabAttackMontage);
}

void ADongincheonCharacter::HandleGrabAttackMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveGrabAttackMontage)
	{
		return;
	}

	ActiveGrabAttackMontage = nullptr;

	if (!IsValid(GrabComponent))
	{
		return;
	}

	if (GrabComponent->IsHoldingGrab())
	{
		PlayPlayerGrabHold();
	}
}

void ADongincheonCharacter::StartPlayerGrabRelease()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsHoldingGrab())
	{
		return;
	}

	if (!GrabComponent->BeginReleaseGrab())
	{
		return;
	}
	
	StopPlayerGrabHold(0.05f);

	if (!IsValid(GrabConfig.GrabReleaseMontage) || !IsValid(GetMesh()))
	{
		GrabComponent->CompleteReleaseGrab();
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		GrabComponent->CompleteReleaseGrab();
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.GrabReleaseMontage,SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		GrabComponent->CompleteReleaseGrab();
		return;
	}

	ActiveGrabReleaseMontage = GrabConfig.GrabReleaseMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleGrabReleaseMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveGrabReleaseMontage);
}

void ADongincheonCharacter::HandleGrabReleaseMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveGrabReleaseMontage)
	{
		return;
	}

	ActiveGrabReleaseMontage = nullptr;

	if (!IsValid(GrabComponent))
	{
		return;
	}

	if (bInterrupted)
	{
		GrabComponent->ForceRelease();
		return;
	}

	GrabComponent->CompleteReleaseGrab();
}

void ADongincheonCharacter::HandleGrabStateChanged(EDIGrabState PreviousState,EDIGrabState NewState)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (PreviousState == EDIGrabState::None && NewState == EDIGrabState::Starting)
		{
			PreGrabWalkSpeed = Movement->MaxWalkSpeed;
			bGrabWalkSpeedOverridden = true;
			Movement->StopMovementImmediately();
		}

		if (NewState == EDIGrabState::Holding && bGrabWalkSpeedOverridden)
		{
			Movement->MaxWalkSpeed = FMath::Max(0.0f,GrabConfig.GrabMoveSpeed);
		}

		if (NewState == EDIGrabState::Attacking || NewState == EDIGrabState::Releasing)
		{
			Movement->StopMovementImmediately();
		}

		if (NewState == EDIGrabState::BeingGrabbed)
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}

		if (PreviousState == EDIGrabState::BeingGrabbed && NewState == EDIGrabState::None)
		{
			if (!bPlayerDeathStarted && IsValid(HealthComponent) && !HealthComponent->IsDead())
			{
				Movement->SetMovementMode(MOVE_Walking);
			}
		}

		if (NewState == EDIGrabState::None && bGrabWalkSpeedOverridden)
		{
			Movement->MaxWalkSpeed = PreGrabWalkSpeed;
			PreGrabWalkSpeed = 0.0f;
			bGrabWalkSpeedOverridden = false;
		}
	}
	
	if (NewState == EDIGrabState::BeingGrabbed)
	{
		StartPlayerBeingGrabbed();
		return;
	}

	if (PreviousState == EDIGrabState::BeingGrabbed && NewState == EDIGrabState::None)
	{
		if (IsValid(GetMesh()))
		{
			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				if (IsValid(ActiveBeingGrabbedStartMontage))
				{
					AnimInstance->Montage_Stop(0.15f,ActiveBeingGrabbedStartMontage);
				}

				if (IsValid(ActiveBeingGrabbedHoldMontage))
				{
					AnimInstance->Montage_Stop(0.15f,ActiveBeingGrabbedHoldMontage);
				}
				
				if (IsValid(ActiveBeingGrabbedHitReactMontage))
				{
					AnimInstance->Montage_Stop(0.15f,ActiveBeingGrabbedHitReactMontage);
				}
			}
		}

		ActiveBeingGrabbedStartMontage = nullptr;
		ActiveBeingGrabbedHoldMontage = nullptr;
		ActiveBeingGrabbedHitReactMontage = nullptr;
	}
}

void ADongincheonCharacter::StartPlayerBeingGrabbed()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed() || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	if (!IsValid(GrabConfig.BeingGrabbedStartMontage))
	{
		PlayPlayerBeingGrabbedHold();
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.BeingGrabbedStartMontage,SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		PlayPlayerBeingGrabbedHold();
		return;
	}

	ActiveBeingGrabbedStartMontage = GrabConfig.BeingGrabbedStartMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleBeingGrabbedStartMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveBeingGrabbedStartMontage);
}

void ADongincheonCharacter::HandleBeingGrabbedStartMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveBeingGrabbedStartMontage)
	{
		return;
	}

	ActiveBeingGrabbedStartMontage = nullptr;

	if (bInterrupted)
	{
		return;
	}

	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	PlayPlayerBeingGrabbedHold();
}

void ADongincheonCharacter::PlayPlayerBeingGrabbedHold()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed() || !IsValid(GetMesh()) ||
		!IsValid(GrabConfig.BeingGrabbedHoldMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.BeingGrabbedHoldMontage,SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		return;
	}

	ActiveBeingGrabbedHoldMontage = GrabConfig.BeingGrabbedHoldMontage;
}

void ADongincheonCharacter::HandleGrabAttackReceived(float Damage,AActor* DamageCauser)
{
	if (Damage <= 0.0f || !IsValid(DamageCauser) || !IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}

	StartPlayerBeingGrabbedHitReact();
}

void ADongincheonCharacter::StartPlayerBeingGrabbedHitReact()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed() || !IsValid(GetMesh()) 
		|| !IsValid(GrabConfig.BeingGrabbedHitReactMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	if (IsValid(ActiveBeingGrabbedHitReactMontage) && AnimInstance->Montage_IsPlaying(ActiveBeingGrabbedHitReactMontage))
	{
		return;
	}

	if (IsValid(ActiveBeingGrabbedHoldMontage))
	{
		AnimInstance->Montage_Stop(0.05f,ActiveBeingGrabbedHoldMontage);

		ActiveBeingGrabbedHoldMontage = nullptr;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.BeingGrabbedHitReactMontage,SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		PlayPlayerBeingGrabbedHold();
		return;
	}

	ActiveBeingGrabbedHitReactMontage = GrabConfig.BeingGrabbedHitReactMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleBeingGrabbedHitReactMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveBeingGrabbedHitReactMontage);
}

void ADongincheonCharacter::HandleBeingGrabbedHitReactMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveBeingGrabbedHitReactMontage)
	{
		return;
	}

	ActiveBeingGrabbedHitReactMontage = nullptr;

	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}

	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	PlayPlayerBeingGrabbedHold();
}

void ADongincheonCharacter::CancelPlayerGrab(float BlendOutTime)
{
	StopPlayerGrabHold(BlendOutTime);
	
	if (IsValid(GrabComponent))
	{
		GrabComponent->ForceRelease();
	}

	if (!IsValid(GetMesh()))
	{
		ActiveGrabStartMontage = nullptr;
		ActiveGrabAttackMontage = nullptr;
		ActiveGrabReleaseMontage = nullptr;
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		ActiveGrabStartMontage = nullptr;
		ActiveGrabAttackMontage = nullptr;
		ActiveGrabReleaseMontage = nullptr;
		return;
	}

	const float SafeBlendOutTime = FMath::Max(0.0f,BlendOutTime);

	if (IsValid(ActiveGrabStartMontage))
	{
		UAnimMontage* MontageToStop = ActiveGrabStartMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate,MontageToStop);

		ActiveGrabStartMontage = nullptr;

		AnimInstance->Montage_Stop(SafeBlendOutTime,MontageToStop);
	}

	if (IsValid(ActiveGrabAttackMontage))
	{
		UAnimMontage* MontageToStop = ActiveGrabAttackMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate,MontageToStop);

		ActiveGrabAttackMontage = nullptr;

		AnimInstance->Montage_Stop(SafeBlendOutTime,MontageToStop);
	}

	if (IsValid(ActiveGrabReleaseMontage))
	{
		UAnimMontage* MontageToStop = ActiveGrabReleaseMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate,MontageToStop);

		ActiveGrabReleaseMontage = nullptr;

		AnimInstance->Montage_Stop(SafeBlendOutTime,MontageToStop);
	}
}

void ADongincheonCharacter::StartPlayerDodge()
{
	if (!IsValid(GetMesh()))
	{
		return;
	}
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (!IsValid(AnimInstance))
	{
		return;
	}
	
	const EPlayerDodgeDirection DodgeDirection = ResolvePlayerDodgeDirection();
	
	UAnimMontage* MontageToPlay = GetDodgeMontage(DodgeDirection);
	
	if (!IsValid(MontageToPlay))
	{
		return;
	}
	
	const FVector DodgeWorldDirection = GetDodgeWorldDirection(DodgeDirection);
	
	if (DodgeWorldDirection.IsNearlyZero())
	{
		return;
	}
	
	bPlayerDodging = true;
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		
		if (DodgeStrength > 0.0f)
		{
			Movement->AddImpulse(DodgeWorldDirection * DodgeStrength, true);
		}
	}
	
	const float SafePlayRate = DodgePlayRate > 0.0f ? DodgePlayRate : 1.0f;
	const float MontageLength = AnimInstance->Montage_Play(MontageToPlay, SafePlayRate, EMontagePlayReturnType::MontageLength,
		0.0f,bDodgeStopAllMontages);
	
	if (MontageLength <= 0.0f)
	{
		ResetPlayerDodgeState();
		return;
	}
	
	ActiveDodgeMontage = MontageToPlay;
	
	FOnMontageEnded MontageEndedDelegate;
	
	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleDodgeMontageEnded);
	
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveDodgeMontage);
}

EPlayerDodgeDirection ADongincheonCharacter::ResolvePlayerDodgeDirection() const
{
	//IA_Move기본입력: X = Left, Right , Y = Forward, Backward
	if (CachedMoveInput.X < -0.1f)
	{
		return EPlayerDodgeDirection::Left;
	}
	
	if (CachedMoveInput.X > 0.1f)
	{
		return EPlayerDodgeDirection::Right;
	}
	
	//무입력 , W, S 모두 Backstep
	return EPlayerDodgeDirection::Backward;
}

UAnimMontage* ADongincheonCharacter::GetDodgeMontage(EPlayerDodgeDirection Direction) const
{
	switch (Direction)
	{
	case EPlayerDodgeDirection::Backward:
		return DodgeBackwardMontage;
		
	case EPlayerDodgeDirection::Left:
		return DodgeLeftMontage;
		
	case EPlayerDodgeDirection::Right:
		return DodgeRightMontage;
		
	default:
		return nullptr;
	}
}

FVector ADongincheonCharacter::GetDodgeWorldDirection(EPlayerDodgeDirection Direction) const
{
	switch (Direction)
	{
	case EPlayerDodgeDirection::Backward:
		return -GetActorForwardVector().GetSafeNormal2D();
		
	case EPlayerDodgeDirection::Left:
		return -GetActorRightVector().GetSafeNormal2D();
		
	case EPlayerDodgeDirection::Right:
		return GetActorRightVector().GetSafeNormal2D();
		
	default:
		return FVector::ZeroVector;
	}
}

void ADongincheonCharacter::HandleDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)bInterrupted;
	
	if (!IsValid(Montage) || Montage != ActiveDodgeMontage)
	{
		return;
	}
	
	ActiveDodgeMontage = nullptr;
	
	ResetPlayerDodgeState();
}

void ADongincheonCharacter::CancelPlayerDodge(float BlendOutTIme)
{
	UAnimMontage* MontageToStop = ActiveDodgeMontage;
	
	ActiveDodgeMontage = nullptr;
	bPlayerDodging = false;
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	
	if (!IsValid(MontageToStop) || !IsValid(GetMesh()))
	{
		return;
	}
	
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(FMath::Max(0.0f,BlendOutTIme),MontageToStop);
	}
}

void ADongincheonCharacter::ResetPlayerDodgeState()
{
	ActiveDodgeMontage = nullptr;
	bPlayerDodging = false;
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
}

//Guard
void ADongincheonCharacter::HandleGuardStarted()
{
	if (bPresentationInputLocked)
	{
		OnQTEInputPressed.Broadcast(EQTEInputType::Guard);
		return;
	}
	
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || !IsValid(CombatComponent))
	{
		return;
	}

	bGuardInputHeld = true;

	if (bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging || bPlayerAttackActive ||
		CombatComponent->IsGuardBroken())
	{
		return;
	}

	if (IsValid(ActiveGuardHitReactMontage))
	{
		CombatComponent->BeginGuard();
		return;
	}

	if (CombatComponent->IsGuarding())
	{
		return;
	}

	StartPlayerGuard();
}

void ADongincheonCharacter::HandleGuardEnded()
{
	//shift를 뗐음
	bGuardInputHeld = false;
	
	
	if (!IsValid(CombatComponent))
	{
		return;
	}
	
	//가드 히트 리액션 재생중이면 애니메이션 끝까지 재생후 추가공격 가드하지 않음
	if (IsValid(ActiveGuardHitReactMontage))
	{
		CombatComponent->EndGuard();
		return;
	}
	
	if (!CombatComponent->IsGuarding())
	{
		return;
	}
	
	StopPlayerGuard(0.1f);
}

void ADongincheonCharacter::StartPlayerGuard()
{
	if (!IsValid(CombatComponent) || CombatComponent->IsGuardBroken() || !IsValid(GetMesh()) 
		|| !IsValid(GuardConfig.GuardMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const float SafePlayRate = GuardConfig.GuardPlayRate > 0.0f ? GuardConfig.GuardPlayRate : 1.0f;

	const float MontageLength =
		AnimInstance->Montage_Play(GuardConfig.GuardMontage,SafePlayRate);

	if (MontageLength <= 0.0f)
	{
		return;
	}

	CombatComponent->BeginGuard();

	ActiveGuardMontage = GuardConfig.GuardMontage;

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleGuardMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveGuardMontage);
}

void ADongincheonCharacter::StopPlayerGuard(float BlendOutTime)
{
	UAnimMontage* MontageToStop = ActiveGuardMontage;
	ActiveGuardMontage = nullptr;

	if (IsValid(CombatComponent))
	{
		CombatComponent->EndGuard();
	}

	if (!IsValid(MontageToStop) || !IsValid(GetMesh()))
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), MontageToStop);
	}
}

void ADongincheonCharacter::HandleGuardMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)bInterrupted;

	if (!IsValid(Montage) || Montage != ActiveGuardMontage)
	{
		return;
	}

	ActiveGuardMontage = nullptr;

	if (IsValid(CombatComponent))
	{
		CombatComponent->EndGuard();
	}
}

void ADongincheonCharacter::StartPlayerGuardHitReact()
{
	if (!IsValid(GetMesh()) || !IsValid(GuardConfig.GuardHitReactMontage))
	{
		return;
	}
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (!IsValid(AnimInstance))
	{
		return;
	}
	
	//기존 가드 루프를 끊고 액티브 가드 몽타주를 먼저 비워서 스탑으로 발생하는 몽타주엔디드가 가드 상태를 해제하지 못하게 한다.
	if (IsValid(ActiveGuardMontage))
	{
		UAnimMontage* GuardLoopToStop = ActiveGuardMontage;
		ActiveGuardMontage = nullptr;
		
		AnimInstance->Montage_Stop(0.05f, GuardLoopToStop);
	}
	
	//이미 Guard Hit Reaction이 재생중이면 같은 Reaction 중복 안시킴
	if (IsValid(ActiveGuardHitReactMontage))
	{
		return;
	}
	
	const float MontageLength = AnimInstance->Montage_Play(GuardConfig.GuardHitReactMontage, 1.0f);
	
	if (MontageLength <= 0.0f)
	{
		//재생을 실패했는데 Guard키를 계속 누르고 있다면 Guard Loop를 복구
		if (bGuardInputHeld)
		{
			StartPlayerGuard();
		}
		
		return;
	}
	
	ActiveGuardHitReactMontage = GuardConfig.GuardHitReactMontage;
	
	FOnMontageEnded MontageEndedDelegate;
	
	MontageEndedDelegate.BindUObject(this,&ADongincheonCharacter::HandleGuardHitReactMontageEnded);
	
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,ActiveGuardHitReactMontage);
}

void ADongincheonCharacter::HandleGuardHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)bInterrupted;
	
	if (!IsValid(Montage) || Montage != ActiveGuardHitReactMontage)
	{
		return;
	}
	
	ActiveGuardHitReactMontage = nullptr;
	
	// Death -> 일반 HitReact로 넘어가는 중이면 Guard로 다시 복귀하면 안됨
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting)
	{
		return;
	}
	
	//Guard키를 계속 누르고 있으면 Gurad Loop로 복귀
	if (bGuardInputHeld)
	{
		StartPlayerGuard();
		return;
	}
	
	//Guard키를 이미 똈다면 일반 상태로 복귀
	if (IsValid(CombatComponent))
	{
		CombatComponent->EndGuard();
	}
}

void ADongincheonCharacter::StartPlayerGuardBreak()
{
	if (!IsValid(CombatComponent) || !CombatComponent->IsGuardBroken() || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		CombatComponent->RecoverFromGuardBreak();
		return;
	}

	if (IsValid(ActiveGuardMontage))
	{
		UAnimMontage* MontageToStop = ActiveGuardMontage;
		ActiveGuardMontage = nullptr;
		AnimInstance->Montage_Stop(0.05f, MontageToStop);
	}

	if (IsValid(ActiveGuardHitReactMontage))
	{
		UAnimMontage* MontageToStop = ActiveGuardHitReactMontage;
		ActiveGuardHitReactMontage = nullptr;
		AnimInstance->Montage_Stop(0.05f, MontageToStop);
	}

	if (!IsValid(GuardConfig.GuardBreakMontage))
	{
		FinishPlayerGuardBreak();
		return;
	}

	const float SafePlayRate = GuardConfig.GuardBreakPlayRate > 0.0f ? GuardConfig.GuardBreakPlayRate : 1.0f;

	const float MontageLength = AnimInstance->Montage_Play( GuardConfig.GuardBreakMontage,SafePlayRate);

	if (MontageLength <= 0.0f)
	{
		FinishPlayerGuardBreak();
		return;
	}

	ActiveGuardBreakMontage = GuardConfig.GuardBreakMontage;

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleGuardBreakMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveGuardBreakMontage);
}

void ADongincheonCharacter::HandleGuardBreakMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)bInterrupted;

	if (!IsValid(Montage) || Montage != ActiveGuardBreakMontage)
	{
		return;
	}

	ActiveGuardBreakMontage = nullptr;

	FinishPlayerGuardBreak();
}

void ADongincheonCharacter::FinishPlayerGuardBreak()
{
	if (!IsValid(CombatComponent))
	{
		return;
	}

	CombatComponent->RecoverFromGuardBreak();

	if (bPlayerDeathStarted || !IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerHitReacting)
	{
		return;
	}

	if (bGuardInputHeld)
	{
		StartPlayerGuard();
	}
}


//Hit React
void ADongincheonCharacter::StartPlayerHitReact()
{
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}
	
	CancelPlayerAttack(0.05f);
	CancelPlayerDodge(0.05f);
	CancelPlayerGrab(0.05f);
	StopPlayerGuard(0.05f);
	
	if (IsValid(ActiveGuardBreakMontage) && IsValid(GetMesh()))
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			UAnimMontage* GuardBreakToStop = ActiveGuardBreakMontage;

			FOnMontageEnded EmptyEndDelegate;
			AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate,GuardBreakToStop);

			ActiveGuardBreakMontage = nullptr;

			AnimInstance->Montage_Stop(0.05f,GuardBreakToStop);
		}

		if (IsValid(CombatComponent))
		{
			CombatComponent->RecoverFromGuardBreak();
		}
	}
	
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
		
		//이전 HitReact 재생의 EndDelegate가 새 HitReact 상태를 종료시키지 못하도록 먼저 해제
		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate,PreviousHitReact);
		
		
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
	
	if (bInterrupted)
	{
		// 같은 HitReact Montage의 새 재생이 이미 진행 중이라면
		// 이전 재생의 종료 Callback이므로 현재 상태를 종료하지 않는다.
		if (IsValid(GetMesh()))
		{
			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				if (AnimInstance->Montage_IsPlaying(ActiveHitReactMontage))
				{
					return;
				}
			}
		}
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
	CancelPlayerDodge(0.0f);
	CancelPlayerGrab(0.05f);
	StopPlayerGuard(0.0f);
	
	bGuardInputHeld = false;

	if (IsValid(CombatComponent))
	{
		CombatComponent->RecoverFromGuardBreak();
	}
	
	if (IsValid(TargetingComponent))
	{
		TargetingComponent->ClearLockOn();
	}
	
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
	
	if (IsValid(ActiveGuardHitReactMontage))
	{
		UAnimMontage* GuardHitToStop = ActiveGuardHitReactMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate,GuardHitToStop);

		ActiveGuardHitReactMontage = nullptr;

		AnimInstance->Montage_Stop(0.0f,GuardHitToStop);
	}

	if (IsValid(ActiveGuardBreakMontage))
	{
		UAnimMontage* GuardBreakToStop = ActiveGuardBreakMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate,GuardBreakToStop);

		ActiveGuardBreakMontage = nullptr;

		AnimInstance->Montage_Stop(0.0f,GuardBreakToStop);
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

float ADongincheonCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	
	if (IsValid(CombatComponent))
	{
		const EGuardResult GuardResult = CombatComponent->TryBlockDamage(DamageAmount, DamageCauser);

		switch (GuardResult)
		{
		case EGuardResult::Blocked:
			{
				if (GuardConfig.GuardHitRecoilStrength > 0.0f)
				{
					FVector RecoilDirection = -GetActorForwardVector();

					if (IsValid(DamageCauser))
					{
						RecoilDirection =
							(GetActorLocation() - DamageCauser->GetActorLocation())
							.GetSafeNormal2D();
					}

					if (UCharacterMovementComponent* Movement =
						GetCharacterMovement())
					{
						Movement->AddImpulse(
							RecoilDirection * GuardConfig.GuardHitRecoilStrength,
							true);
					}
				}

				StartPlayerGuardHitReact();
				return 0.0f;
			}

		case EGuardResult::GuardBroken:
				StartPlayerGuardBreak();
				return 0.0f;

		case EGuardResult::NotBlocked:
				default:
				break;
		}
	}
	
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}


//Hit Confirm Feedback
void ADongincheonCharacter::HandleCombatHitConfirmed(AActor* HitActor, FVector HitLocation, FName HitSocketName,
	float AppliedDamage)
{
	if (AppliedDamage <= 0.0f)
	{
		return;
	}
	
	if (IsValid(HitActor))
	{
		if (ADIPlayerController* DIPlayerController = Cast<ADIPlayerController>(GetController()))
		{
			DIPlayerController->ShowEnemyHUD(HitActor);
		}
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


