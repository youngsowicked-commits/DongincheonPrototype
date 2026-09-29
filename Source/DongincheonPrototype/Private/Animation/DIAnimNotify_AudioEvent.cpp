#include "Animation/DIAnimNotify_AudioEvent.h"

#include "Components/DICharacterAudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UDIAnimNotify_AudioEvent::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !EventTag.IsValid())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();

	if (!Owner)
	{
		return;
	}

	UDICharacterAudioComponent* AudioComponent =
		Owner->FindComponentByClass<UDICharacterAudioComponent>();

	if (!AudioComponent)
	{
		return;
	}

	FDIAudioEventContext Context;
	Context.Instigator = Owner;
	Context.Location = Owner->GetActorLocation();
	Context.SocketName = SocketName;

	// Socket이 지정되어 있고 실제로 존재한다면
	// Context의 World Location도 정확한 Socket 위치로 기록.
	if (!SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
	{
		Context.Location = MeshComp->GetSocketLocation(SocketName);
	}

	AudioComponent->PlayAudioEvent(EventTag, Context);
}