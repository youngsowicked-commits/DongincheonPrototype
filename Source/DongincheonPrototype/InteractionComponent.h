// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"


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
	
	UFUNCTION(BlueprintCallable,Category = "Interaction")
	bool TryInteract(AActor* Target);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	AActor* FindBestInteractable() const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionRadius = 250.0f;
};
