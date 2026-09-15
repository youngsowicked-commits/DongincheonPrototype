// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotify_AttackHit.h"

#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotify_AttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!IsValid(MeshComp))
	{
		UE_LOG(LogTemp, Error, TEXT("ATTACK HIT TEST: MeshComp INVALID"));
		return;
	}
	
	AActor* Owner = MeshComp->GetOwner();
	
	if (!IsValid(Owner))
	{
		UE_LOG(LogTemp, Error, TEXT("ATTACK HIT TEST: Owner INVALID"));
		return;
	}
	
	UE_LOG(
	   LogTemp,
	   Warning,
	   TEXT("ATTACK HIT TEST: Notify FIRED | Owner=%s | Socket=%s"),
	   *Owner->GetName(),
	   *HitSocketName.ToString()
   );
	
	if (HitSocketName.IsNone())
	{
		UE_LOG(LogTemp,Warning,TEXT("AnimNotify_AttackHit : HitSocketName is None. Owner=%s Animation=%s"),
			*Owner->GetName(),IsValid(Animation) ? *Animation->GetName() : TEXT("None"));
		
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ATTACK HIT TEST: HitSocketName NONE")
		);
		return;
	}
	
	UCombatComponent* CombatComponent = Owner->FindComponentByClass<UCombatComponent>();
	
	if (!IsValid(CombatComponent))
	{
		UE_LOG(LogTemp,Warning,TEXT("Animnotify_AttackHit: CombatComponent not found on %s"), *Owner->GetName());
		
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ATTACK HIT TEST: CombatComponent NOT FOUND | Owner=%s"),
			*Owner->GetName()
		);
		
		return;
	}
	
	UE_LOG(
	   LogTemp,
	   Warning,
	   TEXT("ATTACK HIT TEST: CombatComponent FOUND | AttackActive=%s"),
	   CombatComponent->IsAttackActive()
		   ? TEXT("TRUE")
		   : TEXT("FALSE")
   );
	
	const bool bHit = CombatComponent->ProcessSocketHit(HitSocketName);
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("ATTACK HIT TEST: ProcessSocketHit = %s"),
		bHit ? TEXT("TRUE") : TEXT("FALSE")
	);
}

