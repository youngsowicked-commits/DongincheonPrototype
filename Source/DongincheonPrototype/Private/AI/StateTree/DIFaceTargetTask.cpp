#include "AI/StateTree/DIFaceTargetTask.h"

#include "AI/DongincheonAIController.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FDIFaceTargetTask::EnterState(
    FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
    (void)Transition;

    FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

    if (!IsValid(InstanceData.Pawn))
    {
        return EStateTreeRunStatus::Failed;
    }

    ADongincheonAIController* AIController = Cast<ADongincheonAIController>(InstanceData.Pawn->GetController());

    if (!IsValid(AIController))
    {
        return EStateTreeRunStatus::Failed;
    }

    if (!IsValid(AIController->GetCombatTarget()))
    {
        return EStateTreeRunStatus::Failed;
    }

    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FDIFaceTargetTask::Tick(FStateTreeExecutionContext& Context,float DeltaTime) const
{
    FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

    APawn* Pawn = InstanceData.Pawn;

    if (!IsValid(Pawn))
    {
        return EStateTreeRunStatus::Failed;
    }

    ADongincheonAIController* AIController = Cast<ADongincheonAIController>(Pawn->GetController());

    if (!IsValid(AIController))
    {
        return EStateTreeRunStatus::Failed;
    }

    AActor* CombatTarget = AIController->GetCombatTarget();

    if (!IsValid(CombatTarget))
    {
        return EStateTreeRunStatus::Failed;
    }

    FVector ToTarget = CombatTarget->GetActorLocation() - Pawn->GetActorLocation();

    ToTarget.Z = 0.0f;

    if (ToTarget.IsNearlyZero())
    {
        return EStateTreeRunStatus::Succeeded;
    }

    const float TargetYaw = ToTarget.Rotation().Yaw;

    const FRotator CurrentRotation = Pawn->GetActorRotation();

    const float YawDifference = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw,TargetYaw));

    if (YawDifference <= InstanceData.AcceptanceAngle)
    {
        return EStateTreeRunStatus::Succeeded;
    }

    const FRotator TargetRotation(0.0f,TargetYaw,0.0f);

    const FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation,TargetRotation,DeltaTime,InstanceData.RotationSpeed);

    Pawn->SetActorRotation(NewRotation);

    return EStateTreeRunStatus::Running;
}