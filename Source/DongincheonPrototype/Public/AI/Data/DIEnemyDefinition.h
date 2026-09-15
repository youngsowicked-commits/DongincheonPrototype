// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Gameplay/Data/DIHitReactData.h"
#include "Gameplay/Data/DIDeathData.h"
#include "Gameplay/Data/DIAttackData.h"
#include "DIEnemyDefinition.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class DONGINCHEONPROTOTYPE_API UDIEnemyDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Attack")
	TArray<FAttackConfig> Attacks;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Hit React")
	FHitReactConfig HitReact;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Death")
	FDeathConfig Death;
};
