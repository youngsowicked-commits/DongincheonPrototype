#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/CombatComponent.h"
#include "DICombatVFXComponent.generated.h"

class UDICombatVFXData;
class UCombatComponent;
class UNiagaraSystem;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class DONGINCHEONPROTOTYPE_API UDICombatVFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDICombatVFXComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat VFX")
	TObjectPtr<UDICombatVFXData> CombatVFXData = nullptr;

private:
	UFUNCTION()
	void HandleImpactConfirmed(AActor* HitActor,FVector HitLocation,FName HitSocketName,float AppliedDamage,EDICombatImpactResult ImpactResult);
	
	UNiagaraSystem* ResolveImpactSystem(EDICombatImpactResult ImpactResult) const;

	UPROPERTY(Transient)
	TObjectPtr<UCombatComponent> CombatComponent = nullptr;
};