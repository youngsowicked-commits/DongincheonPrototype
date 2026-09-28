#include "Actor/InspectableInteractionBase.h"

#include "Character/DongincheonCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InteractionComponent.h"
#include "Gameplay/Collision/DICollisionChannels.h"

AInspectableInteractionBase::AInspectableInteractionBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(SceneRoot);

	InteractionBounds->InitBoxExtent(FVector(75.0f, 75.0f, 75.0f));

	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionObjectType(DICollision::Interactable);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionBounds->SetGenerateOverlapEvents(false);

	InteractionText = NSLOCTEXT(
		"DongincheonInteraction",
		"Inspect",
		"조사하기"
	);
}

void AInspectableInteractionBase::Interact_Implementation(AActor* Interactor)
{
	ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(Interactor);

	if (!IsValid(Player))
	{
		return;
	}

	if (InteractionState != EInspectableInteractionState::Idle)
	{
		return;
	}

	if (Player->IsGameplayInputLocked())
	{
		return;
	}

	ActiveInteractor = Player;
	InteractionState = EInspectableInteractionState::Active;

	Player->SetInteractionInputLocked(true);

	OnInspectionStarted(Player, InspectMessage);
}

bool AInspectableInteractionBase::CancelInteraction_Implementation(AActor* Interactor)
{
	if (InteractionState != EInspectableInteractionState::Active)
	{
		return false;
	}

	if (!IsValid(ActiveInteractor))
	{
		return false;
	}

	if (Interactor != ActiveInteractor)
	{
		return false;
	}

	EndInspection();
	return true;
}

FText AInspectableInteractionBase::GetInteractionText_Implementation() const
{
	return InteractionText;
}

void AInspectableInteractionBase::EndInspection()
{
	if (InteractionState != EInspectableInteractionState::Active)
	{
		return;
	}

	// ActiveInteractor를 지우기 전에 Player 보관
	ADongincheonCharacter* Player = ActiveInteractor.Get();

	// 1. Native Runtime State 먼저 종료
	InteractionState = EInspectableInteractionState::Idle;
	ActiveInteractor = nullptr;

	// 2. Player Input 복구
	if (IsValid(Player))
	{
		Player->SetInteractionInputLocked(false);
	}

	// 3. BP Presentation 종료
	// BP_InspectableBase에서는 여기서 Inspect Widget 제거
	OnInspectionEnded();

	// 4. InteractionComponent에 Active Interaction 종료 통보
	if (IsValid(Player))
	{
		if (UInteractionComponent* Interaction = Player->FindComponentByClass<UInteractionComponent>())
		{
			Interaction->NotifyInteractionEnded(this);
		}
	}
}