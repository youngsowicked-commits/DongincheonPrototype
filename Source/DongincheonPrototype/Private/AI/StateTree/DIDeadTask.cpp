#include "AI/StateTree/DIDeadTask.h"

#include "Character/DongincheonEnemyBase.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FDIDeadTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);
	
	if (!IsValid(Enemy))
	{
		UE_LOG(LogTemp, Error, TEXT("DI Dead Task : Invalid Enemy Pawn"));
		
		return EStateTreeRunStatus::Failed;
	}
	
	if (!Enemy->StartDeath())
	{
		UE_LOG(LogTemp,Warning,TEXT("DI Dead Task : StartDeath failed on %s"), *Enemy->GetName());
	}
	
	//Dead는 Terminal State
	//Dead Montage가 끝나도 AI는 절대 다시 활성화 하지 않는다.
	return EStateTreeRunStatus::Running;
}
