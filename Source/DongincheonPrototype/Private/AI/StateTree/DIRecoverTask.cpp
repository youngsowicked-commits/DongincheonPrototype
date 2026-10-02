#include "AI/StateTree/DIRecoverTask.h"

#include "AIController.h"
#include "Character/DongincheonEnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FDIRecoverTask::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    (void)Transition;

    FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
    InstanceData.ElapsedTime = 0.0f;

    ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

    if (!IsValid(Enemy) || !IsValid(InstanceData.Target))
    {
        return EStateTreeRunStatus::Failed;
    }

    if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
    {
        AIController->StopMovement();
        AIController->SetFocus(InstanceData.Target);
    }

    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }

    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FDIRecoverTask::Tick(
    FStateTreeExecutionContext& Context,
    const float DeltaTime) const
{
    FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

    ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

    if (!IsValid(Enemy) || !IsValid(InstanceData.Target))
    {
        return EStateTreeRunStatus::Failed;
    }

    InstanceData.ElapsedTime += DeltaTime;

    FVector ToTarget = InstanceData.Target->GetActorLocation() - Enemy->GetActorLocation();
    ToTarget.Z = 0.0f;

    if (!ToTarget.IsNearlyZero())
    {
        const float CurrentYaw = Enemy->GetActorRotation().Yaw;
        const float TargetYaw = ToTarget.Rotation().Yaw;
        const float NewYaw = FMath::FixedTurn(CurrentYaw, TargetYaw, InstanceData.RotationSpeed * DeltaTime);

        Enemy->SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
    }

    if (InstanceData.ElapsedTime >= InstanceData.RecoveryDuration)
    {
        return EStateTreeRunStatus::Succeeded;
    }

    return EStateTreeRunStatus::Running;
}