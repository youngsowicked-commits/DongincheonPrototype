#include "Animation/AnimNotify_GrabCheck.h"

#include "Character/DongincheonCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_GrabCheck::Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp,Animation,EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	ADongincheonCharacter* Character = Cast<ADongincheonCharacter>(MeshComp->GetOwner());

	if (!IsValid(Character))
	{
		return;
	}

	Character->TryCommitPlayerGrab();
}