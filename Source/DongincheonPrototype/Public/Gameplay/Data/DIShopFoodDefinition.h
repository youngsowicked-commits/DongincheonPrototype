#pragma once

#include "CoreMinimal.h"
#include "DIShopFoodDefinition.generated.h"

USTRUCT(BlueprintType)
struct FDIShopFoodDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	FName FoodId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Food")
	float HealAmount = 0.0f;
};