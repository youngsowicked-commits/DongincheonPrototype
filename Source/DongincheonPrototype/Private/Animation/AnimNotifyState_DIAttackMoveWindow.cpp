#include "Animation/AnimNotifyState_DIAttackMoveWindow.h"

#include "Character/DongincheonCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_DIAttackMoveWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,
	float TotalDuration,const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp,Animation,TotalDuration,EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(MeshComp->GetOwner());

	if (!IsValid(Player))
	{
		return;
	}

	Player->BeginAttackMoveWindow(TotalDuration);
}

void UAnimNotifyState_DIAttackMoveWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp,Animation,EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(MeshComp->GetOwner());

	if (!IsValid(Player))
	{
		return;
	}

	Player->EndAttackMoveWindow();
}

FString UAnimNotifyState_DIAttackMoveWindow::GetNotifyName_Implementation() const
{
	return TEXT("DI Attack Move Window");
}