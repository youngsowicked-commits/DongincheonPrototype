#include "AI/StateTree/DIHitReactTask.h"

#include "Character/DongincheonEnemyBase.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FDIHitReactTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);
	
	if (!IsValid(Enemy))
	{
		UE_LOG(LogTemp,Error,TEXT("DI HitReact Task : Invalid Enemy Pwan"));
		
		return EStateTreeRunStatus::Failed;
	}
	
	if (!Enemy->StartHitReact())
	{
		UE_LOG(LogTemp,Warning,TEXT("DI HitReact Task : StartHitReact failed on %s"), *Enemy->GetName());
		
		return EStateTreeRunStatus::Failed;
	}
	
	return  EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FDIHitReactTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);
	
	if (!IsValid(Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}
	
	if (Enemy->IsHitReactActive())
	{
		return EStateTreeRunStatus::Running;
	}
	
	return EStateTreeRunStatus::Succeeded;
}

void FDIHitReactTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);
	
	if (!IsValid(Enemy))
	{
		return;
	}
	
	if (Enemy->IsHitReactActive())
	{
		Enemy->StopHitReact();
	}
	
}
