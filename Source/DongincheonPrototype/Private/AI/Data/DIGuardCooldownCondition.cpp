#include "AI/Data/DIGuardCooldownCondition.h"

#include "Character/DongincheonEnemyBase.h"
#include "AI/Data/DIEnemyDefinition.h"
#include "AI/DongincheonAIController.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

bool FDIGuardCooldownCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

	if (!IsValid(Enemy) || !IsValid(Enemy->EnemyDefinition))
	{
		return false;
	}

	ADongincheonAIController* AIController = Cast<ADongincheonAIController>(Enemy->GetController());

	if (!IsValid(AIController))
	{
		return false;
	}

	return AIController->CanUseGuard(Enemy->EnemyDefinition->GuardBehavior.Cooldown);
}