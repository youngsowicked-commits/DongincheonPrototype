// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotify_AttackHit.h"

#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_AttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!IsValid(MeshComp))
	{
		return;
	}
	
	AActor* Owner = MeshComp->GetOwner();
	
	if (!IsValid(Owner))
	{
		return;
	}
	
	if (HitSocketName.IsNone())
	{
		UE_LOG(LogTemp,Warning,TEXT("AnimNotify_AttackHit : HitSocketName is None. Owner=%s Animation=%s"),
			*Owner->GetName(),IsValid(Animation) ? *Animation->GetName() : TEXT("None"));
		
		return;
	}
	
	UCombatComponent* CombatComponent = Owner->FindComponentByClass<UCombatComponent>();
	
	if (!IsValid(CombatComponent))
	{
		UE_LOG(LogTemp,Warning,TEXT("Animnotify_AttackHit: CombatComponent not found on %s"), *Owner->GetName());
		
		return;
	}
	
	CombatComponent->ProcessSocketHit(HitSocketName);
}

