// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AActor;
class UDamageType;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DONGINCHEONPROTOTYPE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Damage")
	float DealDamage(AActor* Target, float DamageAmount);
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UDamageType> DamageTypeClass;
		
};
