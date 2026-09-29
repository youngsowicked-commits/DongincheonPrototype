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
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginAttack(float DamageAmount,float KnockbackStrength,bool bBreakGuard,float HitTraceRadius,
	float ImpactFreezeDuration)
{
	ActiveDamageAmount = FMath::Max(0.0f, DamageAmount);
	ActiveKnockbackStrength = FMath::Max(0.0f, KnockbackStrength);
	ActiveHitTraceRadius = HitTraceRadius > 0.0f ? HitTraceRadius : DefaultHitTraceRadius;
	ActiveImpactFreezeDuration = FMath::Max(0.0f, ImpactFreezeDuration);
	bActiveAttackBreakGuard = bBreakGuard;

	HitActorThisAttack.Reset();

	bAttackActive = true;
}

void UCombatComponent::EndAttack()
{
	bAttackActive = false;
	
	ActiveDamageAmount = 0.0f;
	ActiveKnockbackStrength = 0.0f;
	ActiveHitTraceRadius = 0.0f;
	ActiveImpactFreezeDuration = 0.0f;
	bActiveAttackBreakGuard = false;
	
	HitActorThisAttack.Reset();
}

// Guard LifeCycle
void UCombatComponent::BeginGuard()
{
	if (bGuardBroken)
	{
		return;
	}
	
	bGuardActive = true;
}

void UCombatComponent::EndGuard()
{
	bGuardActive = false;
	
}

void UCombatComponent::RecoverFromGuardBreak()
{
	bGuardActive = false;
	bGuardBroken = false;
}

EGuardResult UCombatComponent::TryBlockDamage(float IncomingDamage, AActor* DamageCauser)
{
	if (!bGuardActive || bGuardBroken || IncomingDamage <= 0.0f || !IsValid(DamageCauser))
	{
		return EGuardResult::NotBlocked;
	}
	
	AActor* Owner = GetOwner();
	
	if (!IsValid(Owner))
	{
		return EGuardResult::NotBlocked;
	}
	
	FVector DirectionToAttacker = DamageCauser->GetActorLocation() - Owner->GetActorLocation();
	
	DirectionToAttacker.Z = 0.0f;
	
	if (!DirectionToAttacker.Normalize())
	{
		return  EGuardResult::NotBlocked;
	}
	
	FVector OwnerForward = Owner->GetActorForwardVector();
	OwnerForward.Z = 0.0f;
	
	if (!OwnerForward.Normalize())
	{
		return EGuardResult::NotBlocked;
	}
	
	const float FrontDot = FVector::DotProduct(OwnerForward, DirectionToAttacker);
	
	if (FrontDot < GuardFrontDotThreshold)
	{
		return EGuardResult::NotBlocked;
	}
	
	bool bIncomingBreakGuard = false;
	
	if (const UCombatComponent* AttackerCombat = DamageCauser->FindComponentByClass<UCombatComponent>())
	{
		bIncomingBreakGuard = AttackerCombat->DoesActiveAttackBreakGuard();
	}
	
	if (bIncomingBreakGuard)
	{
		bGuardActive = false;
		bGuardBroken = true;
		
		OnGuardBroken.Broadcast(IncomingDamage,DamageCauser);
		
		return EGuardResult::GuardBroken;
	}
	
	OnGuardHit.Broadcast(IncomingDamage,DamageCauser);
	
	return EGuardResult::Blocked;
}

// Hit Detection
void UCombatComponent::ResetAttackHitActors()
{
	HitActorThisAttack.Reset();
}



bool UCombatComponent::CollectSocketHitActors(FName SocketName,float TraceRadius,TArray<AActor*>& OutHitActors,
    bool bDrawDebug) const
{
    OutHitActors.Reset();

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

    if (!IsValid(OwnerCharacter) || SocketName.IsNone() || TraceRadius <= 0.0f)
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
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("CombatComponent: Socket/Bone '%s' does not exist on %s"),
            *SocketName.ToString(),
            *OwnerCharacter->GetName());

        return false;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return false;
    }

    const FVector SocketLocation = Mesh->GetSocketLocation(SocketName);

    FVector TraceStart = OwnerCharacter->GetActorLocation();
    TraceStart.Z = SocketLocation.Z;

    TraceStart += OwnerCharacter->GetActorForwardVector() * 20.0f;

    const FVector TraceEnd = SocketLocation;

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CombatSocketHitTrace),false,OwnerCharacter);

    const FCollisionShape HitShape = FCollisionShape::MakeSphere(TraceRadius);

    TArray<FHitResult> HitResults;

    World->SweepMultiByObjectType(HitResults,TraceStart,TraceEnd,FQuat::Identity,ObjectQueryParams,
        HitShape,QueryParams);

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();

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
        const FColor DebugColor = OutHitActors.IsEmpty() ? FColor::Red : FColor::Green;

        DrawDebugLine(World,TraceStart,TraceEnd,DebugColor,false,1.0f,0,2.0f);

        DrawDebugSphere(World,TraceStart,TraceRadius,16,DebugColor,false,1.0f,0,
            1.5f);

        DrawDebugSphere(World,TraceEnd,TraceRadius,16,DebugColor,false,1.0f,0,
            1.5f);
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

void UCombatComponent::PauseCurrentImpactMontage(AActor* Actor)
{
	ACharacter* Character = Cast<ACharacter>(Actor);

	if (!IsValid(Character) || !IsValid(Character->GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage();

	if (!IsValid(Montage))
	{
		return;
	}

	for (const FImpactPausedMontageState& PausedState : PausedImpactMontages)
	{
		if (PausedState.AnimInstance.Get() == AnimInstance &&
			PausedState.Montage.Get() == Montage)
		{
			return;
		}
	}

	FImpactPausedMontageState NewState;
	NewState.AnimInstance = AnimInstance;
	NewState.Montage = Montage;

	PausedImpactMontages.Add(NewState);

	AnimInstance->Montage_Pause(Montage);
}

bool UCombatComponent::IsVictimHitReactReady(AActor* Victim) const
{
	if (!IsValid(Victim))
	{
		return false;
	}

	if (const ADongincheonEnemyBase* Enemy =
		Cast<ADongincheonEnemyBase>(Victim))
	{
		return Enemy->IsHitReactActive();
	}

	if (const ADongincheonCharacter* Player =
		Cast<ADongincheonCharacter>(Victim))
	{
		return Player->IsPlayerHitReacting();
	}

	return false;
}

void UCombatComponent::TryPausePendingImpactVictims()
{
	bImpactVictimPauseRetryScheduled = false;

	if (!bImpactFreezeActive)
	{
		PendingImpactVictims.Reset();
		return;
	}

	for (int32 Index = PendingImpactVictims.Num() - 1; Index >= 0; --Index)
	{
		AActor* Victim = PendingImpactVictims[Index].Get();

		if (!IsValid(Victim))
		{
			PendingImpactVictims.RemoveAtSwap(Index);
			continue;
		}

		if (!IsVictimHitReactReady(Victim))
		{
			continue;
		}

		PauseCurrentImpactMontage(Victim);

		PendingImpactVictims.RemoveAtSwap(Index);
	}

	if (!PendingImpactVictims.IsEmpty() &&
		!bImpactVictimPauseRetryScheduled &&
		IsValid(GetWorld()))
	{
		bImpactVictimPauseRetryScheduled = true;

		GetWorld()->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(
				this,
				&UCombatComponent::TryPausePendingImpactVictims));
	}
}

void UCombatComponent::StartImpactFreeze(
	AActor* HitActor,
	float KnockbackStrength)
{
	AActor* Attacker = GetOwner();

	if (!IsValid(Attacker) || !IsValid(HitActor))
	{
		return;
	}

	if (ActiveImpactFreezeDuration <= 0.0f)
	{
		if (KnockbackStrength > 0.0f)
		{
			ApplyKnockback(HitActor, KnockbackStrength);
		}

		return;
	}

	// Knockback은 Impact Hold가 끝날 때까지 보류.
	if (KnockbackStrength > 0.0f)
	{
		FPendingImpactKnockback PendingKnockback;
		PendingKnockback.Target = HitActor;
		PendingKnockback.Strength = KnockbackStrength;

		PendingImpactKnockbacks.Add(PendingKnockback);
	}

	const TWeakObjectPtr<AActor> WeakVictim(HitActor);

	if (!PendingImpactVictims.Contains(WeakVictim))
	{
		PendingImpactVictims.Add(WeakVictim);
	}

	if (!bImpactFreezeActive)
	{
		bImpactFreezeActive = true;

		// 공격자는 정확한 Contact Frame에서 즉시 Hold.
		PauseCurrentImpactMontage(Attacker);

		GetWorld()->GetTimerManager().SetTimer(
			ImpactFreezeTimerHandle,
			this,
			&UCombatComponent::FinishImpactFreeze,
			ActiveImpactFreezeDuration,
			false);
	}

	// Player Victim은 이미 HitReact가 시작됐을 수 있고,
	// Enemy는 StateTree 전환 후 준비될 때까지 다음 Tick에서 재확인.
	TryPausePendingImpactVictims();
}

void UCombatComponent::FinishImpactFreeze()
{
	bImpactFreezeActive = false;
	bImpactVictimPauseRetryScheduled = false;

	PendingImpactVictims.Reset();

	for (const FImpactPausedMontageState& PausedState : PausedImpactMontages)
	{
		UAnimInstance* AnimInstance = PausedState.AnimInstance.Get();
		UAnimMontage* Montage = PausedState.Montage.Get();

		if (!IsValid(AnimInstance) || !IsValid(Montage))
		{
			continue;
		}

		AnimInstance->Montage_Resume(Montage);
	}

	PausedImpactMontages.Reset();

	for (const FPendingImpactKnockback& PendingKnockback :
		PendingImpactKnockbacks)
	{
		AActor* Target = PendingKnockback.Target.Get();

		if (!IsValid(Target) ||
			PendingKnockback.Strength <= 0.0f)
		{
			continue;
		}

		ApplyKnockback(
			Target,
			PendingKnockback.Strength);
	}

	PendingImpactKnockbacks.Reset();
}

bool UCombatComponent::PerformSocketHitTrace(FName SocketName, float TraceRadius, TArray<AActor*>& OutHitActors, bool bDrawDebug)
{
	return CollectSocketHitActors(SocketName, TraceRadius, OutHitActors, bDrawDebug);
}

bool UCombatComponent::ProcessHitActor(
	AActor* HitActor,
	const FVector& HitLocation,
	FName SocketName)
{
	if (!IsValid(HitActor))
	{
		return false;
	}

	ACharacter* OwnerCharacter =
		Cast<ACharacter>(GetOwner());

	if (!IsValid(OwnerCharacter))
	{
		return false;
	}

	if (HitActor == OwnerCharacter)
	{
		return false;
	}

	if (!IsValidCombatTarget(HitActor))
	{
		return false;
	}

	const bool bOwnerIsPlayer =
		OwnerCharacter->IsA<ADongincheonCharacter>();

	const bool bOwnerIsEnemy =
		OwnerCharacter->IsA<ADongincheonEnemyBase>();

	const bool bTargetIsPlayer =
		HitActor->IsA<ADongincheonCharacter>();

	const bool bTargetIsEnemy =
		HitActor->IsA<ADongincheonEnemyBase>();

	const bool bValidCombatTarget =
		(bOwnerIsPlayer && bTargetIsEnemy) ||
		(bOwnerIsEnemy && bTargetIsPlayer);

	if (!bValidCombatTarget)
	{
		return false;
	}

	const TWeakObjectPtr<AActor> HitActorPtr(HitActor);

	if (HitActorThisAttack.Contains(HitActorPtr))
	{
		return false;
	}

	HitActorThisAttack.Add(HitActorPtr);

	float AppliedDamage = 0.0f;

	if (ActiveDamageAmount > 0.0f)
	{
		AppliedDamage =
			DealDamage(HitActor, ActiveDamageAmount);
	}

	if (AppliedDamage > 0.0f)
	{
		StartImpactFreeze(
			HitActor,
			ActiveKnockbackStrength);
	}

	OnHitConfirmed.Broadcast(
		HitActor,
		HitLocation,
		SocketName,
		AppliedDamage);

	return true;
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

	const float TraceRadius = ActiveHitTraceRadius > 0.0f ? ActiveHitTraceRadius : DefaultHitTraceRadius;

	if (!CollectSocketHitActors(SocketName, TraceRadius, HitActors, bDrawHitDebug))
	{
		return false;
	}

	const FVector HitLocation = Mesh->GetSocketLocation(SocketName);

	bool bAnyNewHit = false;

	for (AActor* HitActor : HitActors)
	{
		if (ProcessHitActor(
			HitActor,
			HitLocation,
			SocketName))
		{
			bAnyNewHit = true;
		}
	}

	return bAnyNewHit;
}

bool UCombatComponent::ProcessSocketTrajectoryHit(
    FName SocketName,
    const FVector& TraceStart,
    const FVector& TraceEnd)
{
    if (!bAttackActive ||
        SocketName.IsNone())
    {
        return false;
    }

    ACharacter* OwnerCharacter =
        Cast<ACharacter>(GetOwner());

    if (!IsValid(OwnerCharacter))
    {
        return false;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return false;
    }

    const float TraceRadius =
        ActiveHitTraceRadius > 0.0f
        ? ActiveHitTraceRadius
        : DefaultHitTraceRadius;

    if (TraceRadius <= 0.0f)
    {
        return false;
    }

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(CombatSocketTrajectoryHit),
        false,
        OwnerCharacter);

    const FCollisionShape HitShape =
        FCollisionShape::MakeSphere(TraceRadius);

    TArray<FHitResult> HitResults;

    World->SweepMultiByObjectType(
        HitResults,
        TraceStart,
        TraceEnd,
        FQuat::Identity,
        ObjectQueryParams,
        HitShape,
        QueryParams);

    bool bAnyNewHit = false;

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();

        const FVector HitLocation =
            Hit.ImpactPoint.IsNearlyZero()
            ? TraceEnd
            : Hit.ImpactPoint;

        if (ProcessHitActor(
            HitActor,
            HitLocation,
            SocketName))
        {
            bAnyNewHit = true;
        }
    }

    if (bDrawHitDebug)
    {
        const FColor DebugColor =
            bAnyNewHit
            ? FColor::Green
            : FColor::Red;

        DrawDebugLine(
            World,
            TraceStart,
            TraceEnd,
            DebugColor,
            false,
            0.15f,
            0,
            2.0f);

        DrawDebugSphere(
            World,
            TraceStart,
            TraceRadius,
            12,
            DebugColor,
            false,
            0.15f);

        DrawDebugSphere(
            World,
            TraceEnd,
            TraceRadius,
            12,
            DebugColor,
            false,
            0.15f);
    }

    return bAnyNewHit;
}
