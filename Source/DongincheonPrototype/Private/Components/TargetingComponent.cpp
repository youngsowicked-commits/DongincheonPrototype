#include "Components/TargetingComponent.h"

#include "Components/HealthComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Actor.h"

UTargetingComponent::UTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTargetingComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	
	SetComponentTickEnabled(false);
}

void UTargetingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UpdateLockOn(DeltaTime);
}

void UTargetingComponent::TryLockOn()
{
	if (!IsValid(OwnerCharacter))
	{
		return;
	}
	
	AActor* BestTarget = FindBestLockOnTarget();
	
	if (!IsValid(BestTarget))
	{
		return;
	}
	
	CurrentTarget = BestTarget;
	
	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
	}
	
	SetComponentTickEnabled(true);
}

void UTargetingComponent::ClearLockOn()
{
	CurrentTarget.Reset();
	
	if (IsValid(OwnerCharacter))
	{
		if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
		{
			Movement->bOrientRotationToMovement = true;
		}
	}
	
	SetComponentTickEnabled(false);
}

bool UTargetingComponent::IsValidLockOnTarget(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}
	
	UHealthComponent* TargetHealth = TargetActor->FindComponentByClass<UHealthComponent>();
	
	if (!IsValid(TargetHealth))
	{
		return false;
	}
	
	return !TargetHealth->IsDead();
}

bool UTargetingComponent::IsLockedOn() const
{
	return CurrentTarget.IsValid();
}

AActor* UTargetingComponent::GetLockOnTarget() const
{
	return CurrentTarget.Get();
}

AActor* UTargetingComponent::FindBestLockOnTarget() const
{
	if (!IsValid(OwnerCharacter))
	{
		return nullptr;
	}
	
	UWorld* World = GetWorld();
	
	if (!IsValid(World))
	{
		return nullptr;
	}
	
	APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
	
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}
	
	FVector CameraLocation;
	FRotator CameraRotation;
	
	PlayerController->GetPlayerViewPoint(CameraLocation,CameraRotation);
	
	const FVector CameraForward = CameraRotation.Vector();
	
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	
	TArray<FOverlapResult> Overlaps;
	
	const bool bFoundAny = World->OverlapMultiByObjectType(Overlaps,OwnerCharacter->GetActorLocation(),FQuat::Identity,
		ObjectQueryParams,FCollisionShape::MakeSphere(LockOnRange),QueryParams);
	
	if (!bFoundAny)
	{
		return nullptr;
	}
	
	AActor* BestTarget = nullptr;
	float ClosestDistance = LockOnRange;
	
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		
		if (!IsValidLockOnTarget(Candidate))
		{
			continue;
		}
		
		FVector DirectionToTarget = Candidate->GetActorLocation() - CameraLocation;
		
		if (!DirectionToTarget.Normalize())
		{
			continue;
		}
		
		const float CamearaDot = FVector::DotProduct(CameraForward, DirectionToTarget);
		
		if (CamearaDot <= MinCameraDot)
		{
			continue;
		}
		
		const float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(),Candidate->GetActorLocation());
		
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			BestTarget = Candidate;
		}
	}
	
	return BestTarget;
}

void UTargetingComponent::UpdateLockOn(float DeltaTime)
{
	if (!IsValid(OwnerCharacter))
	{
		ClearLockOn();
		return;
	}
	
	AActor* Target = CurrentTarget.Get();
	
	if (!IsValidLockOnTarget(Target))
	{
		ClearLockOn();
		return;
	}
	
	const float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(),Target->GetActorLocation());
	
	if (Distance > LockOnBreakRange)
	{
		ClearLockOn();
		return;
	}
	
	const FVector Direction = Target->GetActorLocation() - OwnerCharacter->GetActorLocation();
	
	if (Direction.IsNearlyZero())
	{
		return;
	}
	
	const FRotator LookAtRotation = Direction.Rotation();
	const FRotator TargetRotation(0.0f,LookAtRotation.Yaw,0.0f);
	const FRotator NewRotation = FMath::RInterpTo(OwnerCharacter->GetActorRotation(),TargetRotation,DeltaTime,LookOnRotationSpeed);
	
	OwnerCharacter->SetActorRotation(NewRotation);
}

