#include "AI/StateTree/DIGuardTask.h"
#include "AI/DongincheonAIController.h"

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
	
	InstanceData.ElapsedTime = 0.0f;

	if (!Enemy->StartGuard())
	{
		return EStateTreeRunStatus::Failed;
	}
	
	if (ADongincheonAIController* AIController = Cast<ADongincheonAIController>(Enemy->GetController()))
	{
		AIController->MarkGuardUsed();
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FDIGuardTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

	if (!IsValid(Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.ElapsedTime += DeltaTime;

	if (InstanceData.ElapsedTime >= InstanceData.GuardDuration)
	{
		return EStateTreeRunStatus::Succeeded;
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