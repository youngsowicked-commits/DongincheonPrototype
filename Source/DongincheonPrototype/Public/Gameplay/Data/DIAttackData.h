#pragma once

#include "CoreMinimal.h"
#include "DIAttackData.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct DONGINCHEONPROTOTYPE_API FAttackConfig
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Damage = 10.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float KnockbackStrength = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Hit Detection", meta = (ClampMin = "1.0"))
    float HitTraceRadius = 15.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	bool bBreaksGuard = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float LungeStrength = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Assist")
	bool bUseAttackAssist = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Assist", meta = (ClampMin = "0.0"))
	float PreferredTargetDistance = 110.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Assist", meta = (ClampMin = "0.0"))
	float MaxAssistDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Assist", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinAssistForwardDot = 0.35f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack|Assist",meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MaxFacingAssistAngle = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	bool bStopAllMontages = true;
};