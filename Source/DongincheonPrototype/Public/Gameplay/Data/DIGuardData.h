#pragma once

#include "CoreMinimal.h"
#include "DIGuardData.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct FGuardConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<UAnimMontage> GuardMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<UAnimMontage> GuardHitReactMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard")
	TObjectPtr<UAnimMontage> GuardBreakMontage = nullptr;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Guard",meta = (ClampMin = "0.01"))
	float GuardPlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Guard",meta = (ClampMin = "0.01"))
	float GuardBreakPlayRate = 1.0f;
};