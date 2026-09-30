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

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[AUDIO NOTIFY] FIRED | Mesh=%s | Anim=%s | Tag=%s"),
		*GetNameSafe(MeshComp),
		*GetNameSafe(Animation),
		*EventTag.ToString());

	if (!MeshComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AUDIO NOTIFY] FAIL | MeshComp=NULL"));
		return;
	}

	if (!EventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[AUDIO NOTIFY] FAIL | EventTag INVALID"));
		return;
	}

	AActor* Owner = MeshComp->GetOwner();

	if (!Owner)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AUDIO NOTIFY] FAIL | Owner=NULL"));
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[AUDIO NOTIFY] Owner=%s"),
		*GetNameSafe(Owner));

	UDICharacterAudioComponent* AudioComponent =
		Owner->FindComponentByClass<UDICharacterAudioComponent>();

	if (!AudioComponent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[AUDIO NOTIFY] FAIL | AudioComponent NOT FOUND | Owner=%s"),
			*GetNameSafe(Owner));

		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[AUDIO NOTIFY] AudioComponent FOUND=%s"),
		*GetNameSafe(AudioComponent));

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