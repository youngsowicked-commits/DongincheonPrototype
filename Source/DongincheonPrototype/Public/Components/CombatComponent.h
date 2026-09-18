// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AActor;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams
(FOnCombatHitConfirmed, AActor*, HitActor, FVector, HitLocation, FName, HitSocketName,float,AppliedDamage);

UENUM(BlueprintType)
enum class EGuardResult : uint8
{
	NotBlocked,
	Blocked,
	GuardBroken
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGuardHit, float, BlockedDamage, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGuardBroken, float, BlockedDamage, AActor*, DamageCauser);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DONGINCHEONPROTOTYPE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();
	
	// Attack Lifecycle

	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void BeginAttack(float DamageAmount, float KnockbackStrength, bool bBreakGuard = false);
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void EndAttack();
	
	UFUNCTION(BlueprintPure, Category = "Combat|Attack")
	bool IsAttackActive() const
	{
		return bAttackActive;
	}
	
	UFUNCTION(BlueprintPure, Category = "Combat|Attack")
	bool DoesActiveAttackBreakGuard() const
	{
		return bActiveAttackBreakGuard;
	}
	
	//Guard LifeCycle
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Guard")
	void BeginGuard();
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Guard")
	void EndGuard();
	
	UFUNCTION(BlueprintPure, Category = "Combat|Guard")
	bool IsGuarding() const
	{
		return bGuardActive;
	}
	
	UFUNCTION(BlueprintPure, Category = "Combat|Guard")
	bool IsGuardBroken() const
	{
		return bGuardBroken;
	}
	
	UFUNCTION(BlueprintCallable, Category = "Combat|Guard")
	EGuardResult TryBlockDamage(float IncomingDamage, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = "Combat|Guard")
	void RecoverFromGuardBreak();
	
	// Hit Detection
	/*Debug / inspection용 Socket Hit Test.
	*
	* 실제 Damage / Knockback은 발생시키지 않는다.
	* Duplicate Hit Set도 변경하지 않는다.
	*
	* 현재 BP AN_AttackHit 검증용으로도 계속 사용할 수 있다.
	*/
	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Detection")
	bool PerformSocketHitTrace(FName SocketName,float TraceRadius,TArray<AActor*>& OutHitActors, bool bDrawDebug);
	
	/*실제 공격 Hit 처리
	 * BeginAttack() 이후 호출해야함
	 * Socket Detection
	 * → Target Validation
	 * → Duplicate Check
	 * → Damage
	 * → Knockback
	 * → OnHitConfirmed
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Detection")
	bool ProcessSocketHit(FName SocketName);
	
	/*
	 * 현재 Attack에서 이미 맞은 Actor 기록을 초기화
	 * BeginAttack()에서도 자동 Reset
	 * 기존 BP와의 Migration 호환을 위해 유지
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Hit Detection")
	void ResetAttackHitActors();
	
	//기존 Damage API
	UFUNCTION(BlueprintCallable, Category = "Combat|Damage")
	float DealDamage(AActor* Target, float DamageAmount);
	
	//Evnets
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnCombatHitConfirmed OnHitConfirmed;
	
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnGuardHit OnGuardHit;
	
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnGuardBroken OnGuardBroken;

	
private:
	// Internal Hit Detection
	bool CollectSocketHitActors(FName SocketName, float TraceRadius, TArray<AActor*>& OutHitActors, bool bDrawDebug) const;
	
	bool IsValidCombatTarget(AActor* Target) const;
	
	void ApplyKnockback(AActor* Target, float KnockbackStrength) const;
	
	// Guard Settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Guard", meta = (AllowPrivateAccess = "true",
		ClampMin = "-1.0", ClampMax = "1.0"))
	float GuardFrontDotThreshold = 0.35f;
	
	
	//Runtime Guard State
	UPROPERTY(Transient)
	bool bGuardActive = false;
	
	UPROPERTY(Transient)
	bool bGuardBroken = false;
	
	// Damage Settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Damage", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UDamageType> DamageTypeClass;
	
	// Hit Detection Settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hit Detection",meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float DefaultHitTraceRadius = 15.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDrawHitDebug = false;
	
	// Runtime Attack State
	UPROPERTY(Transient)
	bool bAttackActive = false;
	
	UPROPERTY(Transient)
	float ActiveDamageAmount = 0.0f;
	
	UPROPERTY(Transient)
	float ActiveKnockbackStrength = 0.0f;
	
	UPROPERTY(Transient)
	bool bActiveAttackBreakGuard = false;
	
	TSet<TWeakObjectPtr<AActor>> HitActorThisAttack;
};
