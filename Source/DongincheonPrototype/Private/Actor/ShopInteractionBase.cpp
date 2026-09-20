#include "Actor/ShopInteractionBase.h"

#include "Components/SceneComponent.h"

AShopInteractionBase::AShopInteractionBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractionPoint = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionPoint"));
	InteractionPoint->SetupAttachment(SceneRoot);

	InteractionText = NSLOCTEXT(
		"DongincheonInteraction",
		"ShopOrder",
		"주문하기"
	);
}

void AShopInteractionBase::Interact_Implementation(AActor* Interactor)
{
	// Shop의 실제 Content Flow는 Blueprint Child에서 구현한다.
	// Camera / Menu / Heal은 아직 여기서 소유하지 않는다.
}

FText AShopInteractionBase::GetInteractionText_Implementation() const
{
	return InteractionText;
}

FTransform AShopInteractionBase::GetInteractionPointTransform() const
{
	if (!IsValid(InteractionPoint))
	{
		return GetActorTransform();
	}

	return InteractionPoint->GetComponentTransform();
}