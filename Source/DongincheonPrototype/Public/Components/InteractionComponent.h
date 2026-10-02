// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableChanged,AActor*,NewInteractable);

UCLASS( ClassGroup=(Interaction), BlueprintType,Blueprintable,meta=(BlueprintSpawnableComponent) )
class DONGINCHEONPROTOTYPE_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractionComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool Interact();
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool CancelCurrentInteraction();
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void NotifyInteractionEnded(AActor* Interactable);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsInteracting() const
	{
		return IsValid(ActiveInteractable.Get());
	}
	
	UFUNCTION(BlueprintCallable, Category = "Interaction|Availability")
	void SetInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Interaction|Availability")
	bool IsInteractionEnabled() const
	{
		return bInteractionEnabled;
	}
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FText GetCurrentInteractionText() const;
	
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableChanged OnInteractableChanged;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionRadius = 185.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bDebugInteraction = false;

protected:
	virtual void BeginPlay() override;
	
	
private:
	void UpdateCurrentInteractable();
	
	void DrawInteractionDebug(AActor* SelectedTarget) const;
	
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentInteractable = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveInteractable = nullptr;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Interaction|Runtime")
	bool bInteractionEnabled = true;
	
	FTimerHandle InteractionUpdateTimerHandle;
	
	bool TryInteract(AActor* Target);
	AActor* FindBestInteractable() const;
};
