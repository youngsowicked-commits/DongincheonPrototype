#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TargetingComponent.generated.h"

class AActor;
class ACharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DONGINCHEONPROTOTYPE_API UTargetingComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UTargetingComponent();
	
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void TryLockOn();
	void ClearLockOn();
	
	UFUNCTION(BlueprintPure, Category="Targeting")
	bool IsValidLockOnTarget(AActor* TargetActor) const;
	
	UFUNCTION(BlueprintPure, Category="Targeting")
	bool IsLockedOn() const;
	
	UFUNCTION(BlueprintPure, Category="Targeting")
	AActor* GetLockOnTarget() const;
	
private:
	AActor* FindBestLockOnTarget() const;
	void UpdateLockOn(float DeltaTime);
	
private:
	UPROPERTY(EditDefaultsOnly, Category="Targeting", meta = (ClampMin="0.0"))
	float LockOnRange = 1000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Targeting", meta = (ClampMin="0.0"))
	float LockOnBreakRange = 1200.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Targeting", meta = (ClampMin="0.0"))
	float LookOnRotationSpeed = 12.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Targeting", meta = (ClampMin="-1.0", ClampMax="1.0"))
	float MinCameraDot = 0.3f;
	
	TWeakObjectPtr<AActor> CurrentTarget;
	
	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;
};