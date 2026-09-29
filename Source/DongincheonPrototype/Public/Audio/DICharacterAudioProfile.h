#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Audio/DIAudioTypes.h"
#include "DICharacterAudioProfile.generated.h"

UCLASS(BlueprintType)
class DONGINCHEONPROTOTYPE_API UDICharacterAudioProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TMap<FGameplayTag, FDIAudioEventDefinition> Events;

	const FDIAudioEventDefinition* FindEvent(const FGameplayTag& EventTag) const;
};