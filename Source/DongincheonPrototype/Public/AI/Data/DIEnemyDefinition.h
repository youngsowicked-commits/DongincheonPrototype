// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gameplay/Data/DIHitReactData.h"
#include "Gameplay/Data/DIDeathData.h"
#include "Gameplay/Data/DIAttackData.h"
#include "Gameplay/Data/DIGuardData.h"
#include "DIEnemyDefinition.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct DONGINCHEONPROTOTYPE_API FDIAttackPattern
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack Pattern")
	TArray<int32> AttackIndices;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Attack Pattern", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
};
UCLASS(BlueprintType)
class DONGINCHEONPROTOTYPE_API UDIEnemyDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Attack")
	TArray<FAttackConfig> Attacks;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Attack")
	TArray<FDIAttackPattern> AttackPatterns;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Hit React")
	FHitReactConfig HitReact;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Guard")
	FGuardConfig Guard;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Death")
	FDeathConfig Death;
};
