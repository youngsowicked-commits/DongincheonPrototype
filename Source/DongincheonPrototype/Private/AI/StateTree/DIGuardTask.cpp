#include "AI/StateTree/DIGuardTask.h"

#include "Character/DongincheonEnemyBase.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FDIGuardTask::EnterState(FStateTreeExecutionContext& Context, 
	const FStateTreeTransitionResult& Transition) const
{
	(void)Transition;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

	if (!IsValid(Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!Enemy->StartGuard())
	{
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FDIGuardTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	(void)DeltaTime;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

	if (!IsValid(Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

void FDIGuardTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	(void)Transition;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

	if (!IsValid(Enemy))
	{
		return;
	}

	Enemy->StopGuard();
}