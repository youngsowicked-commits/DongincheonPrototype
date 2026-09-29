#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Audio/DIAudioTypes.h"
#include "DICharacterAudioComponent.generated.h"

class UAudioComponent;
class UDICharacterAudioProfile;
class USceneComponent;
class USoundBase;

UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class DONGINCHEONPROTOTYPE_API UDICharacterAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDICharacterAudioComponent();

	UFUNCTION(BlueprintCallable, Category = "Audio")
	UAudioComponent* PlayAudioEvent(FGameplayTag EventTag,const FDIAudioEventContext& Context);

	UFUNCTION(BlueprintPure, Category = "Audio")
	bool HasAudioEvent(FGameplayTag EventTag) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<UDICharacterAudioProfile> AudioProfile = nullptr;

private:
	USoundBase* SelectVariant(const FDIAudioEventDefinition& Definition) const;

	USceneComponent* ResolveOwnerAttachComponent() const;
};