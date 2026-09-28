// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DongincheonEnemyBase.h"
#include "AIController.h"
#include "TimerManager.h"
#include "Components/HealthComponent.h"
#include "Components/CombatComponent.h"
#include "Components/DIGrabComponent.h"
#include "Components/DIHeatActionComponent.h"
#include "Components/CapsuleComponent.h"
#include "AI/DongincheonAIController.h"
#include "AI/Data/DIEnemyDefinition.h"
#include "AI/DongincheonAIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ADongincheonEnemyBase::ADongincheonEnemyBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	GrabComponent = CreateDefaultSubobject<UDIGrabComponent>(TEXT("Grab"));
	HeatActionComponent = CreateDefaultSubobject<UDIHeatActionComponent>(TEXT("HeatAction"));
	
	bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}

}

float ADongincheonEnemyBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
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
			return 0.0f;
			
		case EGuardResult::GuardBroken:
			return 0.0f;
			
		case EGuardResult::NotBlocked:
		default:
			break;
		}
	}
	
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

bool ADongincheonEnemyBase::StartGuard()
{
	UE_LOG(LogTemp, Warning, TEXT("[GUARD] StartGuard CALLED"));
	
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AActor* FocusActor = AIController->GetFocusActor();

		float TargetYaw = GetActorRotation().Yaw;

		if (IsValid(FocusActor))
		{
			TargetYaw =
				(FocusActor->GetActorLocation() - GetActorLocation())
				.Rotation()
				.Yaw;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[FACING] Guard | Focus=%s | ActorYaw=%.1f | ControlYaw=%.1f | TargetYaw=%.1f"),
			*GetNameSafe(FocusActor),
			GetActorRotation().Yaw,
			AIController->GetControlRotation().Yaw,
			TargetYaw);
	}
	
	if (!IsValid(EnemyDefinition) || !IsValid(CombatComponent))
	{
		return false;
	}

	if (HealthComponent && HealthComponent->IsDead())
	{
		return false;
	}

	if (CombatComponent->IsGuardBroken())
	{
		return false;
	}

	UAnimMontage* GuardMontage = EnemyDefinition->Guard.GuardMontage;

	if (!IsValid(GuardMontage) || !IsValid(GetMesh()))
	{
		return false;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return false;
	}

	const float SafePlayRate = EnemyDefinition->Guard.GuardPlayRate > 0.0f ? EnemyDefinition->Guard.GuardPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GuardMontage,SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		return false;
	}

	CombatComponent->BeginGuard();
	
	UE_LOG(
	LogTemp,
	Warning,
	TEXT("[GUARD] MontageResult=%.2f IsGuarding=%s"),
	MontageResult,
	CombatComponent->IsGuarding() ? TEXT("TRUE") : TEXT("FALSE")
);

	return CombatComponent->IsGuarding();
}

void ADongincheonEnemyBase::StopGuard()
{
	if (IsValid(CombatComponent))
	{
		CombatComponent->EndGuard();
	}

	if (!IsValid(EnemyDefinition) || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimMontage* GuardMontage = EnemyDefinition->Guard.GuardMontage;

	if (!IsValid(GuardMontage))
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(0.1f,GuardMontage);
	}
}

void ADongincheonEnemyBase::RecoverFromGuardBreak()
{
	if (!IsValid(CombatComponent))
	{
		return;
	}

	CombatComponent->RecoverFromGuardBreak();
}

bool ADongincheonEnemyBase::IsGuardActive() const
{
	return IsValid(CombatComponent) && CombatComponent->IsGuarding();
}

bool ADongincheonEnemyBase::StartGuardBreakReaction()
{
	if (!IsValid(EnemyDefinition) || !IsValid(GetMesh()))
	{
		return false;
	}

	UAnimMontage* GuardBreakMontage = EnemyDefinition->Guard.GuardBreakMontage;

	if (!IsValid(GuardBreakMontage))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("GuardBreak failed: GuardBreakMontage is null on %s"),
			*GetName());

		return false;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return false;
	}

	const float SafePlayRate = EnemyDefinition->Guard.GuardBreakPlayRate > 0.0f ? EnemyDefinition->Guard.GuardBreakPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GuardBreakMontage,SafePlayRate);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[GUARD_BREAK] MontageResult=%.2f"),
		MontageResult);

	return MontageResult > 0.0f;
}

bool ADongincheonEnemyBase::StartAttack(int32 AttackIndex)
{
	if (!EnemyDefinition)
	{
		UE_LOG(LogTemp,Warning,TEXT("Attack failed : EnemyDefinition is null on %s"), *GetName());
		
		return false;
	}
	
	if (!EnemyDefinition->Attacks.IsValidIndex(AttackIndex))
	{
		UE_LOG(LogTemp,Warning,TEXT("Attack failed : Invalid AttackIndex %d on %s"), AttackIndex, *GetName());
		
		return false;
	}
	
	if (HealthComponent && HealthComponent->IsDead())
	{
		return false;
	}
	
	if (!CombatComponent)
	{
		UE_LOG(LogTemp,Warning,TEXT("Attack failed : CombatComponent is null on %s"), *GetName());
		
		return false;
	}
	
	if (CombatComponent->IsAttackActive())
	{
		UE_LOG(LogTemp,Warning,TEXT("Attack failed : Attack already active on %s"), *GetName());
		
		return false;
	}
	
	const FAttackConfig& AttackConfig = EnemyDefinition->Attacks[AttackIndex];
	
	if (!AttackConfig.Montage)
	{
		UE_LOG(LogTemp,Warning,TEXT("Attack failed : Monatage is null at index %d on %s"), AttackIndex, *GetName());
		
		return false;
	}
	
	USkeletalMeshComponent* MeshComponent = GetMesh();
	
	if (!MeshComponent)
	{
		return false;
	}
	
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	
	if (!AnimInstance)
	{
		return false;
	}
	
	if (ADongincheonAIController* AIController = Cast<ADongincheonAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		
		if (AttackConfig.LungeStrength > 0.0f)
		{
			const FVector LungeVelocity = GetActorForwardVector() * AttackConfig.LungeStrength;
			
			Movement->AddImpulse(LungeVelocity, true);
		}
	}
	
	CombatComponent->BeginAttack(AttackConfig.Damage, AttackConfig.KnockbackStrength, AttackConfig.bBreaksGuard);
	
	const float MontageResult = AnimInstance->Montage_Play(AttackConfig.Montage, AttackConfig.PlayRate,
		EMontagePlayReturnType::MontageLength, 0.0f, AttackConfig.bStopAllMontages);
	
	if (MontageResult <= 0.0f)
	{
		CombatComponent->EndAttack();
		
		UE_LOG(LogTemp,Warning,TEXT("Attack failed : Montage_Play returned 0 on %s"), *GetName());
		
		return false;
	}
	
	ActiveAttackIndex = AttackIndex;
	
	return true;
}

bool ADongincheonEnemyBase::IsAttackActive() const
{
	if (!EnemyDefinition || !CombatComponent || !CombatComponent->IsAttackActive())
	{
		return false;
	}
	
	if (!EnemyDefinition->Attacks.IsValidIndex(ActiveAttackIndex))
	{
		return false;
	}
	
	const UAnimMontage* AttackMontage = EnemyDefinition->Attacks[ActiveAttackIndex].Montage;
	
	if (!AttackMontage)
	{
		return false;
	}
	
	const USkeletalMeshComponent* MeshComponent = GetMesh();
	
	if (!MeshComponent)
	{
		return false;
	}
	
	const UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	
	if (!AnimInstance)
	{
		return false;
	}
	
	return AnimInstance->Montage_IsActive(AttackMontage);
}

void ADongincheonEnemyBase::FinishAttack()
{
	if (CombatComponent)
	{
		CombatComponent->EndAttack();
	}
	
	ActiveAttackIndex = INDEX_NONE;
}

void ADongincheonEnemyBase::CancelAttack(float BlendOutTime)
{
	if (EnemyDefinition && EnemyDefinition->Attacks.IsValidIndex(ActiveAttackIndex))
	{
		UAnimMontage* AttackMontage = EnemyDefinition->Attacks[ActiveAttackIndex].Montage;
		
		if (AttackMontage)
		{
			if (USkeletalMeshComponent* MeshComponent = GetMesh())
			{
				if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
				{
					if (AnimInstance->Montage_IsActive(AttackMontage))
					{
						AnimInstance->Montage_Stop(BlendOutTime, AttackMontage);
					}
				}
				
			}
		}
	}
	
	if (CombatComponent)
	{
		CombatComponent->EndAttack();
	}
	
	ActiveAttackIndex = INDEX_NONE;
}

void ADongincheonEnemyBase::SetPhase2Active(bool bActive)
{
	if (bPhase2Active == bActive)
	{
		return;
	}

	bPhase2Active = bActive;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("01B BOSS PHASE | Enemy=%s | Phase2=%s"),
		*GetNameSafe(this),
		bPhase2Active ? TEXT("TRUE") : TEXT("FALSE"));
}

bool ADongincheonEnemyBase::StartHitReact()
{
	if (!EnemyDefinition)
	{
		UE_LOG(LogTemp,Warning,TEXT("HitReact failed : EnemyDefinition is null on %s"), *GetName());
		
		return false;
	}
	
	if (HealthComponent && HealthComponent->IsDead())
	{
		return false;
	}
	
	UAnimMontage* HitReactMontage = EnemyDefinition->HitReact.Montage;
	
	if (!HitReactMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("HitReact failed : Montage is null on %s"), *GetName());
		
		return false;
	}
	
	USkeletalMeshComponent* MeshComponent = GetMesh();
	
	if (!MeshComponent)
	{
		return false;
	}
	
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	
	if (!AnimInstance)
	{
		return false;
	}
	
	//StateTree의 기존 MoveTo / Path Following을 즉시 중단.
	/*if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	
	// 남아 있는 실제 Character Velocity 제거.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	*/
	
	const float MontageResult = AnimInstance->Montage_Play(
	HitReactMontage, EnemyDefinition->HitReact.PlayRate, EMontagePlayReturnType::MontageLength, 0.0f,
	EnemyDefinition->HitReact.bStopAllMontage
	);
	
	return MontageResult > 0.0f;
}

bool ADongincheonEnemyBase::IsHitReactActive() const
{
	if (!EnemyDefinition || !EnemyDefinition->HitReact.Montage)
	{
		return false;
	}
	
	const USkeletalMeshComponent* MeshComponent = GetMesh();
	
	if (!MeshComponent)
	{
		return false;
	}
	
	const UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	
	if (!AnimInstance)
	{
		return false;
	}
	
	return AnimInstance->Montage_IsActive(EnemyDefinition->HitReact.Montage);
}

void ADongincheonEnemyBase::StopHitReact(float BlendOutTime)
{
	if (!EnemyDefinition || !EnemyDefinition->HitReact.Montage)
	{
		return;;
	}
	
	USkeletalMeshComponent* MeshComponent = GetMesh();
	
	if (!MeshComponent)
	{
		return;
	}
	
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	
	if (!AnimInstance)
	{
		return;
	}
	
	AnimInstance->Montage_Stop(BlendOutTime, EnemyDefinition->HitReact.Montage);
}

bool ADongincheonEnemyBase::StartDeath()
{
	if (IsValid(GrabComponent))
	{
		GrabComponent->ForceRelease();
		GrabComponent->SetCanBeGrabbed(false);
	}
	
	if (IsValid(HeatActionComponent))
	{
		HeatActionComponent->CancelHeatAction();
	}
	
	// Death가 GrabBreak보다 최우선이다.
	// GrabBreak 연출 중 죽었다면 탈출 Montage를 즉시 중단하고
	// 이후 GrabBreak.End로 Combat 복귀하지 못하도록 상태를 정리한다.
	if (bGrabBreakFreeActive)
	{
		bGrabBreakFreeActive = false;

		if (IsValid(EnemyDefinition) && IsValid(EnemyDefinition->GrabResponse.BreakFreeMontage))
		{
			if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
			{
				AnimInstance->Montage_Stop(0.05f,EnemyDefinition->GrabResponse.BreakFreeMontage);
			}
		}
	}
	
	//Death Gameplay State는 Presentation보다 먼저 보장한다.
	//Death Montage가 없거나 재생 실패해도 AI가 계속 움직이면 안 된다.
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	
	
	if (!EnemyDefinition)
	{
		UE_LOG(LogTemp,Warning,TEXT("Death failed : EnemyDefinition is null on %s"), *GetName());
		
		return false;
	}
	
	if (!EnemyDefinition->Death.Montage)
	{
		UE_LOG(LogTemp,Warning,TEXT("Death failed : Montage is null on %s"), *GetName());
		
		return false;
	}
	
	USkeletalMeshComponent* MeshComponent = GetMesh();
	
	if (!MeshComponent)
	{
		return false;
	}
	
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	
	if (!AnimInstance)
	{
		return false;
	}
	
	
	const float MontageResult = AnimInstance->Montage_Play(EnemyDefinition->Death.Montage,EnemyDefinition->Death.PlayRate,
		EMontagePlayReturnType::MontageLength, 0.0f, EnemyDefinition->Death.bStopAllMontage);
	
	if (MontageResult <= 0.0f)
	{
		UE_LOG(LogTemp,Warning,TEXT("Death failed : Montage_Play returned 0 on %s"), *GetName());
		
		return false;
	}
	
	return true;
}

bool ADongincheonEnemyBase::InDeathActive() const
{
	if (!EnemyDefinition || !EnemyDefinition->Death.Montage)
	{
		return false;
	}
	
	const USkeletalMeshComponent* MeshComponent = GetMesh();
	
	if (!MeshComponent)
	{
		return false;
	}
	
	const UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	
	if (!AnimInstance)
	{
		return false;
	}
	
	return AnimInstance->Montage_IsActive(EnemyDefinition->Death.Montage);
	
}

void ADongincheonEnemyBase::FinalizeDeath()
{
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

// Called when the game starts or when spawned
void ADongincheonEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp,Warning,TEXT("01B HEALTH CHECK: Enemy=%s | MemberHealth=%s | Ptr=%p"),*GetName(),
		*GetNameSafe(HealthComponent),HealthComponent.Get());

	TArray<UHealthComponent*> HealthComponents;
	GetComponents<UHealthComponent>(HealthComponents);

	UE_LOG(LogTemp,Warning,TEXT("01B HEALTH CHECK: UHealthComponent Count=%d"),HealthComponents.Num());

	for (UHealthComponent* Component : HealthComponents)
	{
		UE_LOG(LogTemp,Warning,TEXT("01B HEALTH CHECK: Found=%s | Ptr=%p"),*GetNameSafe(Component),Component);
	}

	if (HealthComponent)
	{
		HealthComponent->OnDamaged.AddUniqueDynamic(this, &ADongincheonEnemyBase::HandleHealthDamaged);

		HealthComponent->OnDeath.AddUniqueDynamic(this, &ADongincheonEnemyBase::HandleHealthDeath);
	}
	
	if (CombatComponent)
	{
		CombatComponent->OnGuardHit.AddUniqueDynamic(this, &ADongincheonEnemyBase::HandleGuardHit);
		CombatComponent->OnGuardBroken.AddUniqueDynamic(this, &ADongincheonEnemyBase::HandleGuardBroken);
	}
	
	if (GrabComponent)
	{
		GrabComponent->OnGrabStateChanged.AddUniqueDynamic(this,&ADongincheonEnemyBase::HandleGrabStateChanged);
	}
	
	if (HeatActionComponent)
	{
		HeatActionComponent->OnHeatActionStateChanged.AddUniqueDynamic(this,&ADongincheonEnemyBase::HandleHeatActionStateChanged);
	}
}

void ADongincheonEnemyBase::HandleHealthDamaged(float DamageAmount, AActor* DamageCauser)
{
	if (ADongincheonAIController* AIController = Cast<ADongincheonAIController>(GetController()))
	{
		// Damage가 들어온 순간 기존 AI Path/Chase 이동을 먼저 제거한다.
		// 이후 UCombatComponent가 Knockback Impulse를 적용할 수 있도록
		// HitReact 진입 전에만 수행한다.
		UE_LOG(LogTemp, Warning, TEXT("01B: HitReact AIController Cast SUCCESS"));
		AIController->StopMovement();
		
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
		
		AIController->SendHitReactEvent();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("01B: HitReact AIController Cast FAILED"));
	}
	
	OnDamagePresentation(DamageAmount, DamageCauser);
}

void ADongincheonEnemyBase::HandleHealthDeath(AActor* DamageCauser)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B DEATH: HandleHealthDeath | Enemy=%s"),
		*GetNameSafe(this));
	
	if (ADongincheonAIController* AIController = Cast<ADongincheonAIController>(GetController()))
	{
		AIController->SendDeadEvent();
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("01B DEATH: AIController Cast FAILED"));
	}
	
	OnDeathPresentation(DamageCauser);
}

void ADongincheonEnemyBase::HandleGuardHit(float BlockedDamage, AActor* DamageCauser)
{
	(void)BlockedDamage;
	(void)DamageCauser;

	if (!IsValid(EnemyDefinition) || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimMontage* GuardHitMontage = EnemyDefinition->Guard.GuardHitReactMontage;

	if (!IsValid(GuardHitMontage))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GUARD_HIT] GuardHitReactMontage is null | Enemy=%s"),
			*GetName());

		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const float MontageResult = AnimInstance->Montage_Play(GuardHitMontage,1.0f);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[GUARD_HIT] MontageResult=%.2f | Enemy=%s"),
		MontageResult,
		*GetName());
}

void ADongincheonEnemyBase::HandleGuardBroken(float BlockedDamage, AActor* DamageCauser)
{
	(void)BlockedDamage;
	(void)DamageCauser;

	if (ADongincheonAIController* AIController = Cast<ADongincheonAIController>(GetController()))
	{
		AIController->SendGuardBrokenEvent();
	}
}

void ADongincheonEnemyBase::HandleGrabStateChanged(EDIGrabState PreviousState,EDIGrabState NewState)
{
    ADongincheonAIController* AIController = Cast<ADongincheonAIController>(GetController());

    // Grab Victim 진입
    if (NewState == EDIGrabState::BeingGrabbed)
    {
        // Boss / BreakFree 대상은 정상 Grab AI 상태에 들어가지 않는다.
    	// 새 Grab이 시작될 때 이전 BreakFree 판정값을 초기화한다.
    	bGrabBreakFreeActive = false;

    	if (IsValid(EnemyDefinition))
    	{
    		const float BreakFreeChance =
				FMath::Clamp(EnemyDefinition->GrabResponse.BreakFreeChance, 0.0f, 1.0f);

    		// Grab이 성립할 때 딱 한 번만 BreakFree 확률을 판정한다.
    		if (BreakFreeChance > 0.0f &&
				FMath::FRand() <= BreakFreeChance)
    		{
    			bGrabBreakFreeActive = true;

    			// BeginGrab()의 양방향 관계 설정이 끝난 다음 Tick에 안전하게 해제한다.
    			GetWorldTimerManager().SetTimerForNextTick(
					this,
					&ADongincheonEnemyBase::ResolveGrabPolicyAfterGrabStarted);

    			return;
    		}
    	}

        // 여기부터는 실제로 잡힌 상태를 유지하는 일반 Enemy만 처리한다.
        if (IsValid(AIController))
        {
            AIController->StopMovement();
        }

        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }

        if (IsAttackActive())
        {
            CancelAttack(0.05f);
        }

        if (IsGuardActive())
        {
            StopGuard();
        }

        // Heat Action Victim이 더 높은 우선순위를 가진다.
        if (IsValid(HeatActionComponent) && HeatActionComponent->IsBeingVictim())
        {
            return;
        }

        if (IsValid(AIController))
        {
            AIController->SendGrabbedEvent();
        }

        return;
    }

    // Grab Victim 이탈
    if (PreviousState == EDIGrabState::BeingGrabbed)
    {
    	// BreakFree로 발생한 Release는 일반 Grab 종료로 처리하지 않는다.
    	// ResolveGrabPolicyAfterGrabStarted()가 ForceRelease 완료 후 값을 정리한다.
    	if (bGrabBreakFreeActive)
    	{
    		return;
    	}

        // 죽은 Enemy는 Combat으로 복귀하지 않는다.
        if (IsValid(HealthComponent) && HealthComponent->IsDead())
        {
            return;
        }

        // Heat Action Victim 상태가 남아있으면 Combat으로 복귀하지 않는다.
        if (IsValid(HeatActionComponent) && HeatActionComponent->IsBeingVictim())
        {
            return;
        }

        if (IsValid(AIController))
        {
            AIController->SendGrabReleasedEvent();
        }
    }
}

void ADongincheonEnemyBase::ResolveGrabPolicyAfterGrabStarted()
{
    // 다음 Tick 전에 Grab이 이미 끝났다면 BreakFree 처리도 종료한다.
    if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed())
    {
        bGrabBreakFreeActive = false;
        return;
    }

    if (!bGrabBreakFreeActive)
    {
        return;
    }

    ADongincheonAIController* AIController = Cast<ADongincheonAIController>(GetController());

    UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

    UAnimMontage* BreakFreeMontage = IsValid(EnemyDefinition) ? EnemyDefinition->GrabResponse.BreakFreeMontage.Get()
            : nullptr;

    // 실제 Grab 관계는 우선 즉시 해제한다.
    // bGrabBreakFreeActive는 ForceRelease() 도중 발생하는
    // BeingGrabbed -> None Delegate를 구분하기 위해 아직 true로 유지한다.
    GrabComponent->ForceRelease();

    // BreakFree Montage가 설정되지 않았다면
    // AI Presentation 상태에는 들어가지 않고 Grab 해제만 완료한다.
    if (!IsValid(AnimInstance) || !IsValid(BreakFreeMontage))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("01B GRAB BREAK: Montage INVALID | Enemy=%s | Montage=%s"),
            *GetNameSafe(this),
            *GetNameSafe(BreakFreeMontage));

        bGrabBreakFreeActive = false;
        return;
    }

    // 여기부터 실제 BreakFree Presentation 시작.
    if (IsValid(AIController))
    {
        AIController->StopMovement();
    }

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }

    if (IsAttackActive())
    {
        CancelAttack(0.05f);
    }

    if (IsGuardActive())
    {
        StopGuard();
    }

    // 탈출 모션 중 다시 Grab되는 것을 방지한다.
    GrabComponent->SetCanBeGrabbed(false);

    const float MontageDuration = AnimInstance->Montage_Play(BreakFreeMontage);

    if (MontageDuration <= 0.0f)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("01B GRAB BREAK: Montage_Play FAILED | Enemy=%s"),
            *GetNameSafe(this));

        GrabComponent->SetCanBeGrabbed(true);
        bGrabBreakFreeActive = false;
        return;
    }

    // 실제 Montage가 재생된 경우에만 StateTree GrabBreak 상태로 진입한다.
    if (IsValid(AIController))
    {
        AIController->SendGrabBreakEvent();
    }

    FOnMontageEnded MontageEndedDelegate;MontageEndedDelegate.BindUObject(this,
    	&ADongincheonEnemyBase::HandleGrabBreakMontageEnded);

    AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,BreakFreeMontage);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("01B GRAB BREAK: Montage START | Enemy=%s | Montage=%s"),
        *GetNameSafe(this),
        *GetNameSafe(BreakFreeMontage));
}

void ADongincheonEnemyBase::HandleGrabBreakMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
	if (!bGrabBreakFreeActive)
	{
		return;
	}

	bGrabBreakFreeActive = false;

	const bool bIsDead  =IsValid(HealthComponent) && HealthComponent->IsDead();

	// 죽은 Enemy는 다시 Grab 가능 상태로 돌리지 않는다.
	if (IsValid(GrabComponent) && !bIsDead)
	{
		GrabComponent->SetCanBeGrabbed(true);
	}

	if (bIsDead)
	{
		return;
	}

	// Heat Action이 더 높은 우선순위를 차지하고 있다면
	// GrabBreak.End로 Combat 상태를 덮어쓰지 않는다.
	if (IsValid(HeatActionComponent) &&
		HeatActionComponent->IsBeingVictim())
	{
		return;
	}

	if (ADongincheonAIController* AIController = Cast<ADongincheonAIController>(GetController()))
	{
		AIController->SendGrabBreakFinishedEvent();
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B GRAB BREAK: Montage END | Enemy=%s | Interrupted=%s"),
		*GetNameSafe(this),
		bInterrupted ? TEXT("TRUE") : TEXT("FALSE"));
}

void ADongincheonEnemyBase::HandleHeatActionStateChanged(EDIHeatActionState PreviousState,EDIHeatActionState NewState)
{
	ADongincheonAIController* AIController = Cast<ADongincheonAIController>(GetController());

	// Heat Action Victim 진입.
	if (NewState == EDIHeatActionState::BeingVictim)
	{
		if (IsValid(AIController))
		{
			AIController->StopMovement();
		}

		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}

		if (IsAttackActive())
		{
			CancelAttack(0.05f);
		}

		if (IsGuardActive())
		{
			StopGuard();
		}
		
		// Heat Action은 GrabBreak보다 우선순위가 높다.
		// GrabBreak Montage가 실행 중이라면 중단하고 Heat Action Victim으로 넘긴다.
		if (bGrabBreakFreeActive && IsValid(EnemyDefinition) && IsValid(EnemyDefinition->GrabResponse.BreakFreeMontage))
		{
			if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
			{
				AnimInstance->Montage_Stop(0.05f,EnemyDefinition->GrabResponse.BreakFreeMontage);
			}
		}

		if (IsValid(AIController))
		{
			AIController->SendHeatActionVictimEvent();
		}

		return;
	}

	// Heat Action Victim 이탈.
	if (PreviousState == EDIHeatActionState::BeingVictim)
	{
		// Heat Action으로 죽었다면 AI를 다시 깨우지 않는다.
		if (IsValid(HealthComponent) && HealthComponent->IsDead())
		{
			return;
		}

		// Grab이 아직 유지 중이면 Combat이 아니라 BeingGrabbed로 돌아간다.
		if (IsValid(GrabComponent) && GrabComponent->IsBeingGrabbed())
		{
			if (IsValid(AIController))
			{
				AIController->SendGrabbedEvent();
			}

			return;
		}

		if (IsValid(AIController))
		{
			AIController->SendHeatActionVictimReleasedEvent();
		}
	}
}

// Called every frame
void ADongincheonEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ADongincheonEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

