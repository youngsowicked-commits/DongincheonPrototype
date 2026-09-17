// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"

#include "Components/HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/DongincheonCharacter.h"
#include "Character/DongincheonEnemyBase.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginAttack(float DamageAmount, float KnockbackStrength)
{
	ActiveDamageAmount = FMath::Max(0.0f,DamageAmount);
	ActiveKnockbackStrength = FMath::Max(0.0f, KnockbackStrength);
	
	HitActorThisAttack.Reset();
	
	bAttackActive = true;
}

void UCombatComponent::EndAttack()
{
	bAttackActive = false;
	
	ActiveDamageAmount = 0.0f;
	ActiveKnockbackStrength = 0.0f;
	
	HitActorThisAttack.Reset();
}

// Hit Detection
void UCombatComponent::ResetAttackHitActors()
{
	HitActorThisAttack.Reset();
}



bool UCombatComponent::CollectSocketHitActors(FName SocketName, float TraceRadius, TArray<AActor*>& OutHitActors,
	bool bDrawDebug) const
{
	OutHitActors.Reset();
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	
	if (!IsValid(OwnerCharacter))
	{
		return false;
	}
	
	if (SocketName.IsNone())
	{
		return false;
	}
	
	if (TraceRadius <=0.0f)
	{
		return false;
	}
	
	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	
	if (!IsValid(Mesh))
	{
		return false;
	}
	
	if (!Mesh->DoesSocketExist(SocketName))
	{
		UE_LOG(LogTemp,Warning,TEXT("CombatComponent: Socket/Bone '%s' does not exist on %s"),*SocketName.ToString(),
		*OwnerCharacter->GetName());
		
		return false;
	}
	
	UWorld* World = GetWorld();
	
	if (!IsValid(World))
	{
		return false;
	}
	
	const FVector HitLocation = Mesh->GetSocketLocation(SocketName);
	
	FCollisionObjectQueryParams ObjectQueryParams;
	
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CombatSocketHitTrace),false,OwnerCharacter);
	
	const FCollisionShape HitShape = FCollisionShape::MakeSphere(TraceRadius);
	
	TArray<FOverlapResult> OverlapResults;
	
	World->OverlapMultiByObjectType(OverlapResults,HitLocation,FQuat::Identity,ObjectQueryParams,HitShape,QueryParams);
	
	for (const FOverlapResult& Overlap : OverlapResults)
	{
		AActor* HitActor = Overlap.GetActor();
		
		if (!IsValidCombatTarget(HitActor))
		{
			continue;
		}
		
		if (OutHitActors.Contains(HitActor))
		{
			continue;
		}
		
		OutHitActors.Add(HitActor);
	}
	
	if (bDrawDebug)
	{
		DrawDebugSphere(World,HitLocation,TraceRadius,16,OutHitActors.IsEmpty() ? FColor::Red : FColor::Green, false,
			1.0f,0,1.5f);
	}
	
	return !OutHitActors.IsEmpty();
}


// Target Validation
bool UCombatComponent::IsValidCombatTarget(AActor* Target) const
{
	AActor* Owner = GetOwner();
	
	if (!IsValid(Owner))
	{
		return false;
	}
	
	if (!IsValid(Target))
	{
		return false;
	}
	
	if (Target == Owner)
	{
		return false;
	}
	
	const UHealthComponent* TargetHealth = Target->FindComponentByClass<UHealthComponent>();
	
	if (!IsValid(TargetHealth))
	{
		return false;
	}
	
	if (TargetHealth->IsDead())
	{
		return false;
	}
	
	return true;
}

// Damage
float UCombatComponent::DealDamage(AActor* Target, float DamageAmount)
{
	AActor* Owner = GetOwner();
	
	if (!IsValidCombatTarget(Target) || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	
	AController* InstigatorController = nullptr;
	
	if (const APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		InstigatorController = OwnerPawn->GetController();
	}
	
	TSubclassOf<UDamageType> DamageType = DamageTypeClass;
	
	if (!DamageType)
	{
		DamageType = UDamageType::StaticClass();
	}
	
	return UGameplayStatics::ApplyDamage(
		Target, DamageAmount, InstigatorController, Owner, DamageType);
}

// Knockback
void UCombatComponent::ApplyKnockback(AActor* Target, float KnockbackStrength) const
{
	if (!IsValid(Target) || KnockbackStrength <= 0.0f)
	{
		return;
	}
	
	ACharacter* TargetCharacter = Cast<ACharacter>(Target);
	
	if (!IsValid(TargetCharacter))
	{
		return;
	}
	
	UCharacterMovementComponent* Movement = TargetCharacter->GetCharacterMovement();
	
	if (!IsValid(Movement))
	{
		return;
	}
	
	const AActor* Owner = GetOwner();
	
	if (!IsValid(Owner))
	{
		return;
	}
	
	FVector KnockbackDirection = TargetCharacter->GetActorLocation() - Owner->GetActorLocation();
	
	KnockbackDirection.Z = 0.0f;
	
	if (!KnockbackDirection.Normalize())
	{
		KnockbackDirection = Owner->GetActorForwardVector();
		
		KnockbackDirection.Z = 0.0f;
		
		KnockbackDirection.Normalize();
	}
	
	Movement->AddImpulse(KnockbackDirection * KnockbackStrength, true);
}


bool UCombatComponent::PerformSocketHitTrace(FName SocketName, float TraceRadius, TArray<AActor*>& OutHitActors, bool bDrawDebug)
{
	return CollectSocketHitActors(SocketName, TraceRadius, OutHitActors, bDrawDebug);
}

bool UCombatComponent::ProcessSocketHit(FName SocketName)
{
	if (!bAttackActive)
	{
		return false;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (!IsValid(OwnerCharacter))
	{
		return false;
	}

	USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();

	if (!IsValid(Mesh))
	{
		return false;
	}

	TArray<AActor*> HitActors;

	if (!CollectSocketHitActors(SocketName, DefaultHitTraceRadius, HitActors, bDrawHitDebug))
	{
		return false;
	}

	const FVector HitLocation =
		Mesh->GetSocketLocation(SocketName);

	bool bAnyNewHit = false;

	for (AActor* HitActor : HitActors)
	{
		if (!IsValid(HitActor))
		{
			continue;
		}
		
		// 자기 자신 타격 방지
		if (HitActor == OwnerCharacter)
		{
			continue;
		}
		
		const bool bOwnerIsPlayer = OwnerCharacter->IsA<ADongincheonCharacter>();
		const bool bOwnerIsEnemy = OwnerCharacter->IsA<ADongincheonEnemyBase>();
		const bool bTargetIsPlayer = HitActor->IsA<ADongincheonCharacter>();
		const bool bTargetIsEnemy = HitActor->IsA<ADongincheonEnemyBase>();
		
		//Player - Enemy끼리만 전투 허용
		const bool bValidCombatTarget = (bOwnerIsPlayer && bTargetIsEnemy) || (bOwnerIsEnemy && bTargetIsPlayer);
		
		if (!bValidCombatTarget)
		{
			continue;
		}
		
		const TWeakObjectPtr<AActor> HitActorPtr(HitActor);

		if (HitActorThisAttack.Contains(HitActorPtr))
		{
			continue;
		}

		HitActorThisAttack.Add(HitActorPtr);

		float AppliedDamage = 0.0f;

		if (ActiveDamageAmount > 0.0f)
		{
			AppliedDamage = DealDamage(HitActor, ActiveDamageAmount);
		}

		if (ActiveKnockbackStrength > 0.0f)
		{
			ApplyKnockback(HitActor, ActiveKnockbackStrength);
		}

		OnHitConfirmed.Broadcast(HitActor, HitLocation, SocketName, AppliedDamage);

		bAnyNewHit = true;
	}

	return bAnyNewHit;
}
