#include "AI/Data/DIGuardCooldownCondition.h"

#include "AI/DongincheonAIController.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

bool FDIGuardCooldownCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.Pawn))
	{
		return false;
	}

	const ADongincheonAIController* AIController = Cast<ADongincheonAIController>(
			InstanceData.Pawn->GetController());

	if (!IsValid(AIController))
	{
		return false;
	}

	return AIController->CanUseGuard(InstanceData.CooldownDuration);
}