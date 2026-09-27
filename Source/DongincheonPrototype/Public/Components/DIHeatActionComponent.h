#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Gameplay/Data/DIHeatActionData.h"
#include "DIHeatActionComponent.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EDIHeatActionState : uint8
{
	None,
	Executing,
	BeingVictim
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FDIHeatActionStateChangedSignature,
	EDIHeatActionState,PreviousState,
	EDIHeatActionState,NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FDIHeatChangedSignature,
	float,CurrentHeat,
	float,MaxHeat);

UCLASS(ClassGroup=(DI),meta=(BlueprintSpawnableComponent))
class DONGINCHEONPROTOTYPE_API UDIHeatActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDIHeatActionComponent();

	AActor* FindBestHeatActionTarget(const FGameplayTag& ActionTag,const FHeatActionConfig& Config) const;
	bool CanStartHeatAction(AActor* Target,const FGameplayTag& ActionTag,const FHeatActionConfig& Config) const;
	void AddHeat(float Amount);
	bool HasEnoughHeat(float Cost) const;
	
	bool BeginHeatAction(AActor* Target,const FGameplayTag& ActionTag,const FHeatActionConfig& Config);
	
	void ProcessHeatActionHit();
	void CompleteHeatAction();
	void CancelHeatAction();

	UFUNCTION(BlueprintPure,Category = "Heat Action")
	bool IsExecuting() const;
	
	UFUNCTION(BlueprintPure,Category = "Heat Action")
	bool IsBeingVictim() const;

	UFUNCTION(BlueprintPure,Category = "Heat Action")
	AActor* GetTargetActor() const;
	
	UFUNCTION(BlueprintPure,Category = "Heat Action")
	AActor* GetSourceActor() const;

	UFUNCTION(BlueprintPure,Category = "Heat Action")
	EDIHeatActionState GetHeatActionState() const;

	UFUNCTION(BlueprintPure,Category = "Heat Action")
	FGameplayTag GetActiveActionTag() const;
	
	UFUNCTION(BlueprintPure,Category = "Heat Action|Gauge")
	float GetCurrentHeat() const;

	UFUNCTION(BlueprintPure,Category = "Heat Action|Gauge")
	float GetMaxHeat() const;

	UFUNCTION(BlueprintPure,Category = "Heat Action|Gauge")
	float GetHeatNormalized() const;

	const FHeatActionConfig& GetActiveConfig() const;

	UPROPERTY(BlueprintAssignable,Category = "Heat Action")
	FDIHeatActionStateChangedSignature OnHeatActionStateChanged;
	
	UPROPERTY(BlueprintAssignable,Category = "Heat Action|Gauge")
	FDIHeatChangedSignature OnHeatChanged;

private:
	bool IsValidHeatActionTarget(AActor* Target,const FGameplayTag& ActionTag,const FHeatActionConfig& Config) const;
	bool CanAcceptHeatAction(AActor* Source,const FGameplayTag& ActionTag,const FHeatActionConfig& Config) const;
	bool AcceptHeatAction(AActor* Source,const FGameplayTag& ActionTag,const FHeatActionConfig& Config);
	void ReleaseTarget();
	void SetHeatActionState(EDIHeatActionState NewState);
	void ClearHeatActionState();
	bool ConsumeHeat(float Amount);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> SourceActor;
	
	UPROPERTY(Transient)
	FGameplayTag ActiveActionTag;
	
	UPROPERTY(Transient)
	FHeatActionConfig ActiveConfig;

	UPROPERTY(Transient)
	EDIHeatActionState HeatActionState = EDIHeatActionState::None;

	bool bHitProcessed = false;
	
	UPROPERTY(EditDefaultsOnly,Category = "Heat Action|Gauge",meta = (ClampMin = "0.0"))
	float MaxHeat = 100.0f;

	UPROPERTY(VisibleInstanceOnly,Category = "Heat Action|Gauge")
	float CurrentHeat = 0.0f;
};