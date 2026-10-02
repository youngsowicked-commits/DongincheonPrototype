#include "Components/DICharacterAudioComponent.h"

#include "Audio/DICharacterAudioProfile.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Components/HealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogDIAudio, Log, All);

UDICharacterAudioComponent::UDICharacterAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDICharacterAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner)) return;

	if (UCombatComponent* CombatComponent = Owner->FindComponentByClass<UCombatComponent>())
	{
		CombatComponent->OnImpactConfirmed.AddUniqueDynamic(this,&UDICharacterAudioComponent::HandleImpactConfirmed);
	}

	if (UHealthComponent* HealthComponent = Owner->FindComponentByClass<UHealthComponent>())
	{
		HealthComponent->OnDamaged.AddUniqueDynamic(this,&UDICharacterAudioComponent::HandleHealthDamaged);
		HealthComponent->OnDeath.AddUniqueDynamic(this,&UDICharacterAudioComponent::HandleHealthDeath);
	}
}

void UDICharacterAudioComponent::HandleImpactConfirmed(AActor* HitActor, FVector HitLocation, FName HitSocketName,
	float AppliedDamage, EDICombatImpactResult ImpactResult)
{
	if (!IsValid(HitActor) || ImpactResult == EDICombatImpactResult::None) return;
	if (ImpactResult == EDICombatImpactResult::Hit && AppliedDamage <= 0.0f) return;

	FGameplayTag EventTag;

	switch (ImpactResult)
	{
	case EDICombatImpactResult::Hit:
		EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Audio.Combat.Impact.Light")));
		break;

	case EDICombatImpactResult::GuardHit:
		EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Audio.Combat.Impact.Guard")));
		break;

	case EDICombatImpactResult::GuardBreak:
		EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Audio.Combat.Impact.GuardBreak")));
		break;

	default:
		return;
	}

	FDIAudioEventContext Context;
	Context.Instigator = GetOwner();
	Context.Target = HitActor;
	Context.Location = HitLocation;
	Context.SocketName = HitSocketName;

	PlayAudioEvent(EventTag, Context);
}

void UDICharacterAudioComponent::HandleHealthDamaged(float DamageAmount,AActor* DamageCauser)
{
	if (DamageAmount <= 0.0f) return;

	AActor* Owner = GetOwner();
	if (!IsValid(Owner)) return;

	FDIAudioEventContext Context;
	Context.Instigator = DamageCauser;
	Context.Target = Owner;
	Context.Location = Owner->GetActorLocation();

	static const FGameplayTag HitTag =
		FGameplayTag::RequestGameplayTag(FName(TEXT("Audio.Character.Hit")));

	PlayAudioEvent(HitTag,Context);
}

void UDICharacterAudioComponent::HandleHealthDeath(AActor* DamageCauser)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner)) return;

	FDIAudioEventContext Context;
	Context.Instigator = DamageCauser;
	Context.Target = Owner;
	Context.Location = Owner->GetActorLocation();

	static const FGameplayTag DeathTag =
		FGameplayTag::RequestGameplayTag(FName(TEXT("Audio.Character.Death")));

	PlayAudioEvent(DeathTag,Context);
}

UAudioComponent* UDICharacterAudioComponent::PlayAudioEvent(FGameplayTag EventTag, const FDIAudioEventContext& Context)
{
	if (!EventTag.IsValid())
	{
		UE_LOG(
			LogDIAudio,
			Warning,
			TEXT("[%s] Invalid Audio Event Tag."),
			*GetNameSafe(GetOwner()));

		return nullptr;
	}

	if (!AudioProfile)
	{
		UE_LOG(
			LogDIAudio,
			Warning,
			TEXT("[%s] AudioProfile is not assigned."),
			*GetNameSafe(GetOwner()));

		return nullptr;
	}

	const FDIAudioEventDefinition* Definition = AudioProfile->FindEvent(EventTag);

	if (!Definition)
	{
		UE_LOG(
			LogDIAudio,
			Warning,
			TEXT("[%s] Audio Event not found: %s"),
			*GetNameSafe(GetOwner()),
			*EventTag.ToString());

		return nullptr;
	}

	USoundBase* SelectedSound = SelectVariant(*Definition);

	if (!SelectedSound)
	{
		UE_LOG(
			LogDIAudio,
			Warning,
			TEXT("[%s] Audio Event has no valid sound variant: %s"),
			*GetNameSafe(GetOwner()),
			*EventTag.ToString());

		return nullptr;
	}

	const float PitchMin = FMath::Max(0.01f, FMath::Min(Definition->PitchRange.X, Definition->PitchRange.Y));

	const float PitchMax = FMath::Max(PitchMin, FMath::Max(Definition->PitchRange.X, Definition->PitchRange.Y));

	const float Pitch = FMath::FRandRange(PitchMin, PitchMax);

	const float Volume = FMath::Max(0.0f, Definition->VolumeMultiplier);

	switch (Definition->PlaybackMode)
	{
	case EDIAudioPlaybackMode::OwnerAttached:
		{
			USceneComponent* AttachComponent =
				ResolveOwnerAttachComponent();

			if (!AttachComponent)
			{
				UE_LOG(
					LogDIAudio,
					Warning,
					TEXT("[%s] No valid attach component for Audio Event: %s"),
					*GetNameSafe(GetOwner()),
					*EventTag.ToString());

				return nullptr;
			}

			UAudioComponent* SpawnedAudio = UGameplayStatics::SpawnSoundAttached(
				SelectedSound,
				AttachComponent,
				Context.SocketName,
				FVector::ZeroVector,
				EAttachLocation::KeepRelativeOffset,
				true,
				Volume,
				Pitch,
				0.0f,
				Definition->AttenuationOverride,
				Definition->ConcurrencyOverride,
				true);
			

			return SpawnedAudio;
		}

	case EDIAudioPlaybackMode::ContextLocation:
		{
			return UGameplayStatics::SpawnSoundAtLocation(
				this,
				SelectedSound,
				Context.Location,
				FRotator::ZeroRotator,
				Volume,
				Pitch,
				0.0f,
				Definition->AttenuationOverride,
				Definition->ConcurrencyOverride,
				true);
		}

	default:
		break;
	}

	return nullptr;
}

bool UDICharacterAudioComponent::HasAudioEvent(FGameplayTag EventTag) const
{
	if (!AudioProfile || !EventTag.IsValid())
	{
		return false;
	}

	return AudioProfile->FindEvent(EventTag) != nullptr;
}

USoundBase* UDICharacterAudioComponent::SelectVariant(const FDIAudioEventDefinition& Definition) const
{
	int32 ValidVariantCount = 0;

	for (USoundBase* Variant : Definition.Variants)
	{
		if (IsValid(Variant))
		{
			++ValidVariantCount;
		}
	}

	if (ValidVariantCount <= 0)
	{
		return nullptr;
	}

	int32 SelectedValidIndex = FMath::RandRange(0, ValidVariantCount - 1);

	for (USoundBase* Variant : Definition.Variants)
	{
		if (!IsValid(Variant))
		{
			continue;
		}

		if (SelectedValidIndex == 0)
		{
			return Variant;
		}

		--SelectedValidIndex;
	}

	return nullptr;
}

USceneComponent* UDICharacterAudioComponent::ResolveOwnerAttachComponent() const
{
	AActor* Owner = GetOwner();

	if (!Owner)
	{
		return nullptr;
	}

	// Character라면 Mesh 기준.
	// SocketName을 지정했을 때 손/머리 등의 Socket을 사용할 수 있다.
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			return Mesh;
		}
	}

	// Character가 아니면 RootComponent fallback.
	return Owner->GetRootComponent();
}
