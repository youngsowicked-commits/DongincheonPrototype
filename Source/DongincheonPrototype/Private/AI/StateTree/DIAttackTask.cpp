#include "AI/StateTree/DIAttackTask.h"

#include "Character/DongincheonEnemyBase.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FDIAttackTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);
	
	if (!IsValid(Enemy))
	{
		UE_LOG(LogTemp,Error,TEXT("DI Attack Task : Invalid Enemy Pawn"));
		
		return EStateTreeRunStatus::Failed;
	}
	
	if (!Enemy->StartAttack(InstanceData.AttackIndex))
	{
		UE_LOG(LogTemp,Warning,TEXT("DI Attack Task : StartAttack failed on %s"), *Enemy->GetName());
		
		return EStateTreeRunStatus::Failed;
	}
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FDIAttackTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);
	
	if (!IsValid(Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}
	
	if (Enemy->IsAttackActive())
	{
		return EStateTreeRunStatus::Running;
	}
	
	Enemy->FinishAttack();
	
	return EStateTreeRunStatus::Succeeded;
}

void FDIAttackTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);
	
	if (!IsValid(Enemy))
	{
		return;
	}
	
	Enemy->CancelAttack();
}
