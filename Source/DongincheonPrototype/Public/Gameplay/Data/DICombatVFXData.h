#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DICombatVFXData.generated.h"

class UNiagaraSystem;

UCLASS(BlueprintType)
class DONGINCHEONPROTOTYPE_API UDICombatVFXData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat VFX|Impact")
	TObjectPtr<UNiagaraSystem> NormalHit = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat VFX|Impact")
	TObjectPtr<UNiagaraSystem> GuardHit = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat VFX|Impact")
	TObjectPtr<UNiagaraSystem> GuardBreak = nullptr;
};