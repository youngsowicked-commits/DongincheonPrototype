// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/DongincheonAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "NativeGameplayTags.h"
#include "StateTreeEvents.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_HitReact,"StateTreeEvent.Enemy.HitReact");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_Dead,"StateTreeEvent.Enemy.Dead");


ADongincheonAIController::ADongincheonAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	
	BrainComponent = StateTreeComponent;
}

void ADongincheonAIController::SendHitReactEvent()
{
	if (!StateTreeComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("01B: StateTreeComponent NULL"));

		return;
	}
	
	const EStateTreeRunStatus RunStatus =
	StateTreeComponent->GetStateTreeRunStatus();

	const TCHAR* StatusText = TEXT("Unknown");

	switch (RunStatus)
	{
	case EStateTreeRunStatus::Running:
		StatusText = TEXT("Running");
		break;

	case EStateTreeRunStatus::Stopped:
		StatusText = TEXT("Stopped");
		break;

	case EStateTreeRunStatus::Succeeded:
		StatusText = TEXT("Succeeded");
		break;

	case EStateTreeRunStatus::Failed:
		StatusText = TEXT("Failed");
		break;

	case EStateTreeRunStatus::Unset:
		StatusText = TEXT("Unset");
		break;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B: HitReact Event - StateTree Status = %s"),
		StatusText);

	StateTreeComponent->SendStateTreeEvent(
		FStateTreeEvent(TAG_StateTreeEvent_Enemy_HitReact));

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B: HitReact Event SENT"));
}

void ADongincheonAIController::SendDeadEvent()
{
	if (!StateTreeComponent)
	{
		return;
	}
	
	StateTreeComponent->SendStateTreeEvent(FStateTreeEvent(TAG_StateTreeEvent_Enemy_Dead));
}
