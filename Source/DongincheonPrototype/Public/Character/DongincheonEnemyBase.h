// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DongincheonEnemyBase.generated.h"

class UHealthComponent;
class UCombatComponent;
class UDIEnemyDefinition;
class AActor;

UCLASS()
class DONGINCHEONPROTOTYPE_API ADongincheonEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ADongincheonEnemyBase();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatComponent> CombatComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Data")
	TObjectPtr<UDIEnemyDefinition> EnemyDefinition = nullptr;
	
	bool StartAttack(int32 AttackIndex);
	bool IsAttackActive() const;
	void FinishAttack();
	void CancelAttack(float BlendOutTime = 0.1f);
	
	bool StartHitReact();
	bool IsHitReactActive() const;
	void StopHitReact(float BlendOutTime = 0.1f);
	
	bool StartDeath();
	bool InDeathActive() const;
	
	void FinalizeDeath();
	
private:
	UPROPERTY(Transient)
	int32 ActiveAttackIndex = INDEX_NONE;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void HandleHealthDamaged(float DamageAmount, AActor* DamageCauser);
	
	UFUNCTION()
	void HandleHealthDeath(AActor* DamageCauser);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Presentation")
	void OnDamagePresentation(float DamageAmount, AActor* DamageCauser);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Presentation")
	void OnDeathPresentation(AActor* DamageCauser);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
