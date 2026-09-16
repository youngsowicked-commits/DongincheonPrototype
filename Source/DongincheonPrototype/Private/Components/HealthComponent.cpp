// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HealthComponent.h"

#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	CurrentHealth = MaxHealth;
	bIsDead = false;
	

}

float UHealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float UHealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}

float UHealthComponent::GetHealthNormalized() const
{
	if (MaxHealth <= 0.0)
	{
		return 0.0f;
	}
	
	return CurrentHealth / MaxHealth;
}

bool UHealthComponent::IsDead() const
{
	return bIsDead;
}

void UHealthComponent::Heal(float HealAmount)
{
	if (bIsDead || HealAmount <= 0.0f)
	{
		return;
	}
	
	const float OldHealth = CurrentHealth;
	
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth);
	
	if (!FMath::IsNearlyEqual(OldHealth, CurrentHealth))
	{
		OnHealthChanged.Broadcast(OldHealth,CurrentHealth,MaxHealth);
	}
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth, bool bAdjustCurrentHealth)
{
	NewMaxHealth = FMath::Max(NewMaxHealth, 1.0f);
	
	if (FMath::IsNearlyEqual(MaxHealth, NewMaxHealth))
	{
		return;
	}
	
	const float OldMaxHealth = MaxHealth;
	const float OldHealth = CurrentHealth;
	
	MaxHealth = NewMaxHealth;
	
	if (!bIsDead)
	{
		if (bAdjustCurrentHealth)
		{
			const float MaxHealthDifference = MaxHealth - OldMaxHealth;
			
			CurrentHealth = FMath::Clamp(CurrentHealth + MaxHealthDifference, 0.0f,MaxHealth);
		}
		else
		{
			CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
		}
	}
	
	OnMaxHealthChanged.Broadcast(OldMaxHealth, MaxHealth, CurrentHealth);

	if (!FMath::IsNearlyEqual(OldHealth, CurrentHealth))
	{
		OnHealthChanged.Broadcast(OldHealth,CurrentHealth,MaxHealth);
	}
}

void UHealthComponent::ResetHealth()
{
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &UHealthComponent::HandleTakeAnyDamage);
	}
}

void UHealthComponent::HandleTakeAnyDamage(AActor* DamageActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (DamageActor != GetOwner())
	{
		return;
	}
	
	if (bIsDead || Damage <= 0.0f)
	{
		return;
	}
	
	const float OldHealth = CurrentHealth;
	
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	
	UE_LOG(LogTemp, Warning, TEXT("[NativeHealth] Owner=%s Damage=%.1f HP=%.1f/%.1f"),
	*GetNameSafe(GetOwner()),Damage,CurrentHealth,MaxHealth);
	
	if (FMath::IsNearlyEqual(OldHealth, CurrentHealth))
	{
		return;
	}
	
	const bool bDiedThisHit = CurrentHealth <= 0.0f;
	
	if (bDiedThisHit)
	{
		bIsDead = true;
	}
	
	OnHealthChanged.Broadcast(OldHealth,CurrentHealth,MaxHealth);
	
	if (bDiedThisHit)
	{
		OnDeath.Broadcast(DamageCauser);
	}
	else
	{
		OnDamaged.Broadcast(Damage, DamageCauser);
	}
}

