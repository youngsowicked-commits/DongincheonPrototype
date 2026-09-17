// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/AnimNotify_DeathEnd.h"

#include "Character/DongincheonCharacter.h"
#include "Character/DongincheonEnemyBase.h"
#include "Components/SkeletalMeshComponent.h"

FString UAnimNotify_DeathEnd::GetNotifyName_Implementation() const
{
	return TEXT("DI Death End Native");
}

void UAnimNotify_DeathEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
const FAnimNotifyEventReference& EventReference)
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
	
	if (ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(Owner))
	{
		Enemy->FinalizeDeath();
		return;
	}
	
	if (ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(Owner))
	{
		Player->FinalizePlayerDeath();
	}
}
