// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DongincheonAIController.generated.h"

class UStateTreeAIComponent;


/**
 * 
 */
UCLASS()
class DONGINCHEONPROTOTYPE_API ADongincheonAIController : public AAIController
{
	GENERATED_BODY()
	
	
public:
	ADongincheonAIController();
	
	void SendHitReactEvent();
	void SendDeadEvent();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
};
