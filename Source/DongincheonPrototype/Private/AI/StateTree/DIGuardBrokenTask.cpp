#include "AI/StateTree/DIGuardBrokenTask.h"

#include "Character/DongincheonEnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FDIGuardBrokenTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	(void)Transition;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	InstanceData.ElapsedTime = 0.0f;

	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

	if (!IsValid(Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	Enemy->StopGuard();
	
	if (!Enemy->StartGuardBreakReaction())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("DI GuardBroken Task: GuardBreak Montage failed | Enemy=%s"),
			*Enemy->GetName());
	}

	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FDIGuardBrokenTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	FInstanceDataType& InstanceData =
		Context.GetInstanceData(*this);

	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

	if (!IsValid(Enemy))
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.ElapsedTime += DeltaTime;

	if (InstanceData.ElapsedTime < InstanceData.RecoveryDuration)
	{
		return EStateTreeRunStatus::Running;
	}

	Enemy->RecoverFromGuardBreak();

	return EStateTreeRunStatus::Succeeded;
}