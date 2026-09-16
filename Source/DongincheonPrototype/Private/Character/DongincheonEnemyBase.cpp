// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DongincheonEnemyBase.h"
#include "AIController.h"
#include "Components/HealthComponent.h"
#include "Components/CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "AI/DongincheonAIController.h"
#include "AI/Data/DIEnemyDefinition.h"
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
	
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	
	CombatComponent->BeginAttack(AttackConfig.Damage, AttackConfig.KnockbackStrength);
	
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
	
	// 기존 AI 이동 요청 정기
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}
	
	// 실제 캐릭터 속도 제거 + 사망 후 이동 완전 차단
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
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
	
	if (HealthComponent)
	{
		HealthComponent->OnDamaged.AddUniqueDynamic(this,&ADongincheonEnemyBase::HandleHealthDamaged);
		HealthComponent->OnDeath.AddUniqueDynamic(this,&ADongincheonEnemyBase::HandleHealthDeath);
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
	if (ADongincheonAIController* AIController = Cast<ADongincheonAIController>(GetController()))
	{
		AIController->SendDeadEvent();
	}
	
	OnDeathPresentation(DamageCauser);
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

