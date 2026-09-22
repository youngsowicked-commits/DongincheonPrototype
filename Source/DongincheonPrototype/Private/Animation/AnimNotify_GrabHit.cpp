#include "Animation/AnimNotify_GrabHit.h"

#include "Components/DIGrabComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAnimNotify_GrabHit::Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp,Animation,EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();

	if (!IsValid(OwnerActor))
	{
		return;
	}

	if (UDIGrabComponent* GrabComponent = OwnerActor->FindComponentByClass<UDIGrabComponent>())
	{
		GrabComponent->ProcessGrabAttackHit();
	}
}