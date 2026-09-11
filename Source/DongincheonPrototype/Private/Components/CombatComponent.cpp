// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"

#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

}

float UCombatComponent::DealDamage(AActor* Target, float DamageAmount)
{
	AActor* Owner = GetOwner();
	
	if (!IsValid(Target) || !IsValid(Owner) || Target == Owner || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	
	AController* InstigatorController = nullptr;
	
	if (const APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		InstigatorController = OwnerPawn->GetController();
	}
	
	TSubclassOf<UDamageType> DamageType = DamageTypeClass;
	
	if (!DamageType)
	{
		DamageType = UDamageType::StaticClass();
	}
	
	return UGameplayStatics::ApplyDamage(
		Target, DamageAmount, InstigatorController, Owner, DamageType);
}

