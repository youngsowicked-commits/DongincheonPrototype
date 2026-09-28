// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionComponent.h"

#include "Interface/Interactable.h"
#include "Gameplay/Collision/DICollisionChannels.h"
#include "Components/PrimitiveComponent.h"
#include "Character/DongincheonCharacter.h"
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

void UInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	if (bInteractionEnabled == bEnabled)
	{
		return;
	}

	if (!bEnabled)
	{
		// 먼저 Availability를 닫아 Cancel 과정에서 후보가 다시 잡히지 않게 한다.
		bInteractionEnabled = false;

		// 실행 중 Interaction이 있으면 대상에게 정상 종료 요청.
		CancelCurrentInteraction();

		ActiveInteractable = nullptr;
		CurrentInteractable = nullptr;

		OnInteractableChanged.Broadcast(nullptr);
		return;
	}

	bInteractionEnabled = true;

	UpdateCurrentInteractable();
}

bool UInteractionComponent::Interact()
{
	if (!bInteractionEnabled)
	{
		return false;
	}

	// 이미 NPC / Shop / Inspectable 등이 실행 중이면
	// 새로운 Interaction 시작 금지.
	if (IsValid(ActiveInteractable.Get()))
	{
		return false;
	}

	ADongincheonCharacter* Player =
		Cast<ADongincheonCharacter>(GetOwner());

	if (IsValid(Player) && Player->IsGameplayInputLocked())
	{
		return false;
	}

	return TryInteract(CurrentInteractable.Get());
}

bool UInteractionComponent::CancelCurrentInteraction()
{
	AActor* Target = ActiveInteractable.Get();

	if (!IsValid(Target))
	{
		return false;
	}

	if (!Target->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();

	if (!IsValid(OwnerActor))
	{
		return false;
	}

	const bool bCancelled = IInteractable::Execute_CancelInteraction(Target,OwnerActor);

	if (!bCancelled)
	{
		return false;
	}

	// 대상 쪽에서 먼저 NotifyInteractionEnded를 호출하지 않았다면
	// Component가 여기서 정리.
	if (ActiveInteractable.Get() == Target)
	{
		NotifyInteractionEnded(Target);
	}

	return true;
}

void UInteractionComponent::NotifyInteractionEnded(AActor* Interactable)
{
	if (ActiveInteractable.Get() != Interactable)
	{
		return;
	}

	ActiveInteractable = nullptr;

	// Combat 등으로 Interaction 전체가 Disable된 상태면
	// Prompt를 다시 찾지 않는다.
	if (!bInteractionEnabled)
	{
		CurrentInteractable = nullptr;
		OnInteractableChanged.Broadcast(nullptr);
		return;
	}

	// 정상 종료라면 주변 후보를 다시 검사.
	UpdateCurrentInteractable();
}

FText UInteractionComponent::GetCurrentInteractionText() const
{
	if (!bInteractionEnabled)
	{
		return FText::GetEmpty();
	}

	if (IsValid(ActiveInteractable.Get()))
	{
		return FText::GetEmpty();
	}

	if (!IsValid(CurrentInteractable.Get()))
	{
		return FText::GetEmpty();
	}

	return IInteractable::Execute_GetInteractionText(CurrentInteractable.Get());
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
	if (!bInteractionEnabled)
	{
		return;
	}

	// Interaction 실행 중에는 새로운 후보 Scan 금지.
	if (IsValid(ActiveInteractable.Get()))
	{
		if (IsValid(CurrentInteractable.Get()))
		{
			CurrentInteractable = nullptr;
			OnInteractableChanged.Broadcast(nullptr);
		}

		return;
	}

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
	if (!bInteractionEnabled)
	{
		return false;
	}

	if (IsValid(ActiveInteractable.Get()))
	{
		return false;
	}

	if (!IsValid(Target))
	{
		return false;
	}

	if (!Target->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		return false;
	}

	// 후보 → 실행 중 대상으로 승격.
	ActiveInteractable = Target;

	// Interaction 시작 순간 World Prompt 제거.
	CurrentInteractable = nullptr;
	OnInteractableChanged.Broadcast(nullptr);

	IInteractable::Execute_Interact(Target, GetOwner());

	return true;
}

AActor* UInteractionComponent::FindBestInteractable() const
{
	// Interaction 자체가 비활성화됐거나,
	// 이미 NPC / Shop / Inspectable Interaction이 실행 중이면
	// 새로운 후보를 찾지 않는다.
	if (!bInteractionEnabled || IsValid(ActiveInteractable.Get()))
	{
		return nullptr;
	}

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
