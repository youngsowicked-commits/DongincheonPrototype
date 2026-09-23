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
	
	UHealthComponent* OwnerHealth = OwnerCharacter->FindComponentByClass<UHealthComponent>();
	
	if (!IsValid(OwnerHealth) || OwnerHealth->IsDead())
	{
		ClearLockOn();
		return;
	}
	
	AActor* BestTarget = FindBestLockOnTarget();
	
	if (!IsValid(BestTarget))
	{
		return;
	}
	
	CurrentTarget = BestTarget;
	
	OnLockOnTargetChanged.Broadcast(CurrentTarget.Get());
	
	if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
	}
	
	SetComponentTickEnabled(true);
}

void UTargetingComponent::ClearLockOn()
{
	CurrentTarget.Reset();
	
	OnLockOnTargetChanged.Broadcast(nullptr);
	
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

void UTargetingComponent::SwitchTarget(float Direction)
{
    if (!IsValid(OwnerCharacter))
    {
        return;
    }

    if (!CurrentTarget.IsValid())
    {
        return;
    }

    if (FMath::IsNearlyZero(Direction))
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());

    if (!IsValid(PlayerController))
    {
        return;
    }

    AActor* CurrentActor = CurrentTarget.Get();

    if (!IsValidLockOnTarget(CurrentActor))
    {
        ClearLockOn();
        return;
    }

    FVector CameraLocation;
    FRotator CameraRotation;

    PlayerController->GetPlayerViewPoint(CameraLocation,CameraRotation);

    const FVector CameraForward = CameraRotation.Vector();
    const FVector CameraRight = CameraRotation.RotateVector(FVector::RightVector);

    FVector CurrentDirection = CurrentActor->GetActorLocation() - CameraLocation;

    if (!CurrentDirection.Normalize())
    {
        return;
    }

    const float CurrentAngle = FMath::Atan2(FVector::DotProduct(CameraRight, CurrentDirection),
        FVector::DotProduct(CameraForward, CurrentDirection));

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);

    TArray<FOverlapResult> Overlaps;

    const bool bFoundAny = World->OverlapMultiByObjectType(
        Overlaps,
        OwnerCharacter->GetActorLocation(),
        FQuat::Identity,
        ObjectQueryParams,
        FCollisionShape::MakeSphere(LockOnRange),
        QueryParams
    );

    if (!bFoundAny)
    {
        return;
    }

    AActor* BestTarget = nullptr;
    float BestAngularDistance = BIG_NUMBER;

    TSet<AActor*> ProcessedActors;

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();

        if (!IsValid(Candidate))
        {
            continue;
        }

        if (Candidate == CurrentActor)
        {
            continue;
        }

        if (ProcessedActors.Contains(Candidate))
        {
            continue;
        }

        ProcessedActors.Add(Candidate);

        if (!IsValidLockOnTarget(Candidate))
        {
            continue;
        }

        const float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(),Candidate->GetActorLocation());

        if (Distance > LockOnRange)
        {
            continue;
        }

        FVector DirectionToTarget = Candidate->GetActorLocation() - CameraLocation;

        if (!DirectionToTarget.Normalize())
        {
            continue;
        }

        const float CameraDot = FVector::DotProduct(CameraForward,DirectionToTarget);

        if (CameraDot <= MinCameraDot)
        {
            continue;
        }

        const float CandidateAngle = FMath::Atan2(FVector::DotProduct(CameraRight, DirectionToTarget),
            FVector::DotProduct(CameraForward, DirectionToTarget));

        float DeltaAngle = FMath::FindDeltaAngleRadians(CurrentAngle,CandidateAngle);

        float AngularDistance = 0.0f;

        if (Direction > 0.0f)
        {
            // 오른쪽 방향. 끝까지 갔으면 반대편으로 순환.
            if (DeltaAngle <= KINDA_SMALL_NUMBER)
            {
                DeltaAngle += 2.0f * PI;
            }

            AngularDistance = DeltaAngle;
        }
        else
        {
            // 왼쪽 방향. 끝까지 갔으면 반대편으로 순환.
            if (DeltaAngle >= -KINDA_SMALL_NUMBER)
            {
                DeltaAngle -= 2.0f * PI;
            }

            AngularDistance = -DeltaAngle;
        }

        if (AngularDistance < BestAngularDistance)
        {
            BestAngularDistance = AngularDistance;
            BestTarget = Candidate;
        }
    }

    if (!IsValid(BestTarget))
    {
        return;
    }

    CurrentTarget = BestTarget;

    OnLockOnTargetChanged.Broadcast(CurrentTarget.Get());
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
	
	UHealthComponent* OwnerHealth = OwnerCharacter->FindComponentByClass<UHealthComponent>();
	
	if (!IsValid(OwnerHealth) || OwnerHealth->IsDead())
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
	
	UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
	
	if (!IsValid(Movement) || Movement->MovementMode == MOVE_None)
	{
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

