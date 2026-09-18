#include "AI/StateTree/DIPresentationHoldTask.h"

EStateTreeRunStatus FDIPresentationHoldTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FDIPresentationHoldTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return EStateTreeRunStatus::Running;
}
