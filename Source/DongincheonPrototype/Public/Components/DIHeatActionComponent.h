#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

UCLASS(ClassGroup=(DI),meta=(BlueprintSpawnableComponent))
class DONGINCHEONPROTOTYPE_API UDIHeatActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDIHeatActionComponent();

	AActor* FindBestHeatActionTarget(const FHeatActionConfig& Config) const;
	bool CanStartHeatAction(AActor* Target,const FHeatActionConfig& Config) const;
	bool BeginHeatAction(AActor* Target,const FHeatActionConfig& Config);

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
	EDIHeatActionType GetActiveType() const;

	const FHeatActionConfig& GetActiveConfig() const;

	UPROPERTY(BlueprintAssignable,Category = "Heat Action")
	FDIHeatActionStateChangedSignature OnHeatActionStateChanged;

private:
	bool IsValidHeatActionTarget(AActor* Target,const FHeatActionConfig& Config) const;
	bool CanAcceptHeatAction(AActor* Source,const FHeatActionConfig& Config) const;
	bool AcceptHeatAction(AActor* Source,const FHeatActionConfig& Config);
	void ReleaseTarget();
	void SetHeatActionState(EDIHeatActionState NewState);
	void ClearHeatActionState();

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> SourceActor;
	
	UPROPERTY(Transient)
	FHeatActionConfig ActiveConfig;

	UPROPERTY(Transient)
	EDIHeatActionState HeatActionState = EDIHeatActionState::None;

	bool bHitProcessed = false;
};