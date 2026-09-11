// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DongincheonCharacter.h"

#include "Components/CombatComponent.h"
#include "Components/HealthComponent.h"
#include "Components/InteractionComponent.h"

// Sets default values
ADongincheonCharacter::ADongincheonCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));

}


