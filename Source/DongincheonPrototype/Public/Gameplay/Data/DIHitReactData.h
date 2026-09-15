#pragma once

#include "CoreMinimal.h"
#include "DIHitReactData.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct DONGINCHEONPROTOTYPE_API FHitReactConfig
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit React")
	TObjectPtr<UAnimMontage> Montage = nullptr;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category = "Hit React", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit React")
	bool bStopAllMontage = true;
};
