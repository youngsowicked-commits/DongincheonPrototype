#pragma once

#include "CoreMinimal.h"
#include "DIDeathData.generated.h"


class UAnimMontage;

USTRUCT(BlueprintType)
struct DONGINCHEONPROTOTYPE_API FDeathConfig
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	TObjectPtr<UAnimMontage> Montage = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	bool bStopAllMontage = true;
};
