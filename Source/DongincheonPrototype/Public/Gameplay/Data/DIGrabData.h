#pragma once

#include "CoreMinimal.h"
#include "DIGrabData.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct FGrabConfig
{
	GENERATED_BODY()

	// Grabber
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Grabber")
	TObjectPtr<UAnimMontage> GrabStartMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Grabber")
	TObjectPtr<UAnimMontage> GrabHoldMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Grabber")
	TObjectPtr<UAnimMontage> GrabAttackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Grabber")
	TObjectPtr<UAnimMontage> GrabReleaseMontage = nullptr;

	// Being Grabbed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Victim")
	TObjectPtr<UAnimMontage> BeingGrabbedStartMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Victim")
	TObjectPtr<UAnimMontage> BeingGrabbedHoldMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Victim")
	TObjectPtr<UAnimMontage> BeingGrabbedHitReactMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Victim")
	TObjectPtr<UAnimMontage> BeingGrabbedReleaseMontage = nullptr;

	// Tuning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Tuning",meta = (ClampMin = "0.01"))
	float GrabPlayRate = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Tuning",meta = (ClampMin = "0.0"))
	float GrabHoldDistance = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Tuning")
	float GrabVictimYawOffset = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Tuning",meta = (ClampMin = "0.0"))
	float GrabMoveSpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grab|Tuning",meta = (ClampMin = "0.0"))
	float GrabAttackDamage = 10.0f;
};