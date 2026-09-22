// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionComponent.h"

#include "Interface/Interactable.h"
#include "Gameplay/Collision/DICollisionChannels.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"

// Sets default values for this component's properties
UInteractionComponent::UInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

bool UInteractionComponent::Interact()
{
	return TryInteract(CurrentInteractable);
}

FText UInteractionComponent::GetCurrentInteractionText() const
{
	if (!IsValid(CurrentInteractable))
	{
		return FText::GetEmpty();
	}
	
	return IInteractable::Execute_GetInteractionText(CurrentInteractable);
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	UpdateCurrentInteractable();
	
	GetWorld()->GetTimerManager().SetTimer(
	InteractionUpdateTimerHandle,
	this,
	&UInteractionComponent::UpdateCurrentInteractable,
	0.1f,
	true
	);
}

void UInteractionComponent::UpdateCurrentInteractable()
{
	AActor* NewInteractable = FindBestInteractable();
	
	if (bDebugInteraction)
	{
		DrawInteractionDebug(NewInteractable);
	}
	
	if (CurrentInteractable != NewInteractable)
	{
		CurrentInteractable = NewInteractable;
		
		OnInteractableChanged.Broadcast(CurrentInteractable.Get());
	}
}

bool UInteractionComponent::TryInteract(AActor* Target)
{
	if (!IsValid(Target))
	{
		return false;
	}
	if (!Target->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		return false;
	}
	
	IInteractable::Execute_Interact(Target,GetOwner());
	
	return true;
}

AActor* UInteractionComponent::FindBestInteractable() const
{
	AActor* Owner = GetOwner();

	if (!IsValid(Owner) || !IsValid(GetWorld()))
	{
		return nullptr;
	}

	TArray<FOverlapResult> OverlapResults;

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector OwnerForward = Owner->GetActorForwardVector();
	const FCollisionShape SearchShape = FCollisionShape::MakeSphere(InteractionRadius);
	const FCollisionObjectQueryParams ObjectQueryParams(DICollision::Interactable);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		OwnerLocation,
		FQuat::Identity,
		ObjectQueryParams,
		SearchShape,
		QueryParams
	);

	AActor* BestTarget = nullptr;
	float BestDistanceSquared = FMath::Square(InteractionRadius);

	for (const FOverlapResult& Result : OverlapResults)
	{
		UPrimitiveComponent* CandidateComponent = Result.GetComponent();
		AActor* Candidate = Result.GetActor();

		if (!IsValid(CandidateComponent) || !IsValid(Candidate) || Candidate == Owner)
		{
			continue;
		}

		if (!Candidate->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
		{
			continue;
		}

		FVector CandidatePoint = CandidateComponent->GetComponentLocation();
		const float SurfaceDistance = CandidateComponent->GetClosestPointOnCollision(OwnerLocation, CandidatePoint);

		const FVector DirectionPoint = SurfaceDistance > 0.0f
			? CandidatePoint
			: CandidateComponent->GetComponentLocation();

		const FVector ToCandidate = DirectionPoint - OwnerLocation;
		const float DistanceSquared = SurfaceDistance > 0.0f
			? FMath::Square(SurfaceDistance)
			: 0.0f;

		if (DistanceSquared > BestDistanceSquared)
		{
			continue;
		}

		const FVector DirectionToCandidate = ToCandidate.GetSafeNormal();
		const float FacingDot = FVector::DotProduct(OwnerForward, DirectionToCandidate);

		if (FacingDot <= 0.0f)
		{
			continue;
		}

		BestTarget = Candidate;
		BestDistanceSquared = DistanceSquared;
	}

	return BestTarget;
}

void UInteractionComponent::DrawInteractionDebug(AActor* SelectedTarget) const
{
#if ENABLE_DRAW_DEBUG

	AActor* Owner = GetOwner();

	if (!IsValid(Owner) || !IsValid(GetWorld()))
	{
		return;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();

	DrawDebugSphere(GetWorld(), OwnerLocation, InteractionRadius, 24,
		FColor::Cyan, false, 0.11f, 0, 1.0f);

	if (!IsValid(SelectedTarget))
	{
		return;
	}

	DrawDebugLine(GetWorld(), OwnerLocation, SelectedTarget->GetActorLocation(),
		FColor::Green, false, 0.11f, 0, 2.0f);

	DrawDebugString(GetWorld(), SelectedTarget->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f),
		SelectedTarget->GetName(), nullptr, FColor::Green, 0.11f, false);

#endif
}
