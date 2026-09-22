#pragma once

#include "CoreMinimal.h"
#include "DIHeatActionData.generated.h"

class UAnimMontage;

UENUM(BlueprintType)
enum class EDIHeatActionType : uint8
{
	None,
	Normal,
	Grab
};

USTRUCT(BlueprintType)
struct FHeatActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action")
	EDIHeatActionType Type = EDIHeatActionType::None;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Animation")
	TObjectPtr<UAnimMontage> PlayerMontage = nullptr;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Animation")
	TObjectPtr<UAnimMontage> VictimMontage = nullptr;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Combat",meta = (ClampMin = "0.0"))
	float Damage = 30.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Targeting",meta = (ClampMin = "0.0"))
	float MaxDistance = 150.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Targeting",meta = (ClampMin = "-1.0",ClampMax = "1.0"))
	float MinForwardDot = 0.35f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Alignment",meta = (ClampMin = "0.0"))
	float TargetDistance = 90.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Alignment")
	float VictimYawOffset = 180.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Heat Action|Animation",meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;
};