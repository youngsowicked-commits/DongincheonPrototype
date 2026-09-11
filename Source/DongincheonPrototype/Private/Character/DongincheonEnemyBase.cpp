// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DongincheonEnemyBase.h"
#include "Components/HealthComponent.h"
#include "Components/CombatComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ADongincheonEnemyBase::ADongincheonEnemyBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));

}

// Called when the game starts or when spawned
void ADongincheonEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (HealthComponent)
	{
		HealthComponent->OnDamaged.AddUniqueDynamic(this,&ADongincheonEnemyBase::HandleHealthDamaged);
		HealthComponent->OnDeath.AddUniqueDynamic(this,&ADongincheonEnemyBase::OnDeathPresentation);
	}
	
}

void ADongincheonEnemyBase::HandleHealthDamaged(float DamageAmount, AActor* DamageCauser)
{
	OnDamagePresentation(DamageAmount, DamageCauser);
}

void ADongincheonEnemyBase::HandleHealthDeath(AActor* DamageCauser)
{
	OnDeathPresentation(DamageCauser);
}

// Called every frame
void ADongincheonEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ADongincheonEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

