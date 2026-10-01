#include "Animation/AnimNotify_DIComboChainPoint.h"

#include "Character/DongincheonCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_DIComboChainPoint::Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp,Animation,EventReference);

	if (!IsValid(MeshComp)) return;

	ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(MeshComp->GetOwner());
	if (!IsValid(Player)) return;

	Player->TryAdvanceQueuedCombo();
}