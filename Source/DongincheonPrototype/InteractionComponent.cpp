// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionComponent.h"

#include "Interactable.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UInteractionComponent::UInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
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
	
	return false;
}

AActor* UInteractionComponent::FindBestInteractable() const
{
	AActor* Owner = GetOwner();
	
	if (!IsValid(Owner))
	{
		return nullptr;
	}
	
	TArray<AActor*> InteractableActors;
	
	UGameplayStatics::GetAllActorsWithInterface(
		GetWorld(),
		UInteractable::StaticClass(),
		InteractableActors
		);
	
	AActor* BestTarget = nullptr;
	float BestDistanceSquared = FMath::Square(InteractionRadius);
	
	const FVector OwnerLoaction = Owner->GetActorLocation();
	const FVector OwnerForward = Owner->GetActorForwardVector();
	
	for (AActor* Candidate : InteractableActors)
	{
		if (!IsValid(Candidate) || Candidate == Owner)
		{
			continue;
		}
		
		const FVector ToCandidate = Candidate->GetActorLocation() - OwnerLoaction;
		
		const float DistanceSquared = ToCandidate.SizeSquared();
		
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

