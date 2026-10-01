#include "Animation/AnimNotify_DIAudioEvent.h"

#include "Components/DICharacterAudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UAnimNotify_DIAudioEvent::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	if (!EventTag.IsValid())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();

	if (!Owner)
	{
		return;
	}
	
	UDICharacterAudioComponent* AudioComponent = Owner->FindComponentByClass<UDICharacterAudioComponent>();

	if (!AudioComponent)
	{
		return;
	}
	
	FDIAudioEventContext Context;
	Context.Instigator = Owner;
	Context.Location = Owner->GetActorLocation();
	Context.SocketName = SocketName;

	if (!SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
	{
		Context.Location = MeshComp->GetSocketLocation(SocketName);
	}

	AudioComponent->PlayAudioEvent(EventTag, Context);
}