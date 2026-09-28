#include "Animation/AnimNotify_DIHeatActionHit.h"

#include "Components/DIHeatActionComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_DIHeatActionHit::Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp,Animation,EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();

	if (!IsValid(Owner))
	{
		return;
	}

	UDIHeatActionComponent* HeatActionComponent =
		Owner->FindComponentByClass<UDIHeatActionComponent>();

	if (!IsValid(HeatActionComponent) || !HeatActionComponent->IsExecuting())
	{
		return;
	}

	HeatActionComponent->ProcessHeatActionHit();
}