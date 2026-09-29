#include "AI/Data/DITargetAttackingCondition.h"

#include "Components/CombatComponent.h"
#include "StateTreeExecutionContext.h"

bool FDITargetAttackingCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!IsValid(InstanceData.Target))
	{
		return false;
	}

	const UCombatComponent* TargetCombatComponent = InstanceData.Target->FindComponentByClass<UCombatComponent>();

	if (!IsValid(TargetCombatComponent))
	{
		return false;
	}

	return TargetCombatComponent->IsAttackActive();
}