// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnInteractableChanged,
	AActor*,
	NewInteractable
);

UCLASS( 
	ClassGroup=(Interaction), 
	BlueprintType,
	Blueprintable,
	meta=(BlueprintSpawnableComponent) 
	)
class DONGINCHEONPROTOTYPE_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractionComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool Interact();
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	FText GetCurrentInteractionText() const;
	
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableChanged OnInteractableChanged;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionRadius = 250.0f;

protected:
	virtual void BeginPlay() override;
	
	
private:
	void UpdateCurrentInteractable();
	
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentInteractable = nullptr;
	
	FTimerHandle InteractionUpdateTimerHandle;
	
	bool TryInteract(AActor* Target);
	AActor* FindBestInteractable() const;
};
