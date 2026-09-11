// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/DongincheonAIController.h"

#include "Components/StateTreeAIComponent.h"

ADongincheonAIController::ADongincheonAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	
	BrainComponent = StateTreeComponent;
}
