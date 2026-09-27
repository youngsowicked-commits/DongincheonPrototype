#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Gameplay/Data/DIHeatActionData.h"
#include "DIHeatActionDefinition.generated.h"

UENUM(BlueprintType)
enum class EDIHeatActionTargetSource : uint8
{
	NearbyTarget,
	CurrentGrabTarget
};

UCLASS(BlueprintType)
class DONGINCHEONPROTOTYPE_API UDIHeatActionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Identity")
	FGameplayTag ActionTag;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Context")
	FGameplayTagQuery ActivationQuery;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Selection")
	int32 Priority = 0;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Targeting")
	EDIHeatActionTargetSource TargetSource = EDIHeatActionTargetSource::NearbyTarget;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action")
	FHeatActionConfig Config;
};