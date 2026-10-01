#include "AI/StateTree/DIStandoffTask.h"

#include "AIController.h"
#include "Character/DongincheonEnemyBase.h"
#include "AI/Data/DIEnemyDefinition.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTreeExecutionContext.h"


EStateTreeRunStatus FDIStandoffTask::EnterState(FStateTreeExecutionContext& Context,const FStateTreeTransitionResult& Transition) const
{
    (void)Transition;

    FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

    InstanceData.bResettingSpacing = false;
    InstanceData.bSpacingResetComplete = false;

    InstanceData.HoldElapsedTime = 0.0f;

    InstanceData.HoverActionElapsedTime = 0.0f;
    InstanceData.CurrentHoverActionDuration = 0.0f;
    InstanceData.CurrentHoverIntent = EDIStandoffHoverIntent::Hold;

    InstanceData.PreviousMaxWalkSpeed = 0.0f;
    InstanceData.bWalkSpeedOverridden = false;


    ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

    if (!IsValid(Enemy))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("DI Standoff Task: Invalid Enemy Pawn"));

        return EStateTreeRunStatus::Failed;
    }

    if (!IsValid(InstanceData.Target))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("DI Standoff Task: Invalid Target | Enemy=%s"),
            *Enemy->GetName());

        return EStateTreeRunStatus::Failed;
    }
    
    const UDIEnemyDefinition* EnemyDefinition = Enemy->EnemyDefinition;

    if (!IsValid(EnemyDefinition))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("DI Standoff Task: Invalid EnemyDefinition | Enemy=%s"),
            *Enemy->GetName());

        return EStateTreeRunStatus::Failed;
    }

    const FDIStandoffConfig& StandoffConfig = EnemyDefinition->Standoff;

    AAIController* AIController = Cast<AAIController>(Enemy->GetController());

    if (!IsValid(AIController))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("DI Standoff Task: Invalid AIController | Enemy=%s"),
            *Enemy->GetName());

        return EStateTreeRunStatus::Failed;
    }

    // Standoff에서는 PathFollowing 이동을 끊고
    // 직접 Combat Hover를 소유한다.
    AIController->StopMovement();
    AIController->SetFocus(InstanceData.Target);

    // Standoff 전용 느린 전투 이동 속도
    if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
    {
        InstanceData.PreviousMaxWalkSpeed = Movement->MaxWalkSpeed;
        InstanceData.bWalkSpeedOverridden = true;

        Movement->MaxWalkSpeed = FMath::Max(0.0f, StandoffConfig.StandoffWalkSpeed);
    }
    
    // 이번 Standoff 전체 대치 시간 랜덤 결정
    const float MinHold = FMath::Min(StandoffConfig.MinHoldDuration, StandoffConfig.MaxHoldDuration);
    const float MaxHold = FMath::Max(StandoffConfig.MinHoldDuration, StandoffConfig.MaxHoldDuration);
    InstanceData.CurrentHoldDuration = FMath::FRandRange(MinHold, MaxHold);
    
    const float DistanceToTarget = FVector::Dist2D(Enemy->GetActorLocation(),InstanceData.Target->GetActorLocation());
    
    // 너무 가까운 상태로 들어왔을 때만 최초 Backpedal
    if (DistanceToTarget < StandoffConfig.MinStandoffDistance)
    {
        InstanceData.bResettingSpacing = true;

        UE_LOG(
            LogTemp,
            Log,
            TEXT(
                "[STANDOFF] ENTER RESET | Enemy=%s | Distance=%.1f | Hold=%.2f"),
            *Enemy->GetName(),
            DistanceToTarget,
            InstanceData.CurrentHoldDuration);
    }
    else
    {
        InstanceData.bSpacingResetComplete = true;

        UE_LOG(
            LogTemp,
            Log,
            TEXT(
                "[STANDOFF] ENTER HOVER | Enemy=%s | Distance=%.1f | Hold=%.2f"),
            *Enemy->GetName(),
            DistanceToTarget,
            InstanceData.CurrentHoldDuration);
    }


    return EStateTreeRunStatus::Running;
}


EStateTreeRunStatus FDIStandoffTask::Tick(FStateTreeExecutionContext& Context,const float DeltaTime) const
{
    FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

    ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);

    if (!IsValid(Enemy) || !IsValid(InstanceData.Target))
    {
        return EStateTreeRunStatus::Failed;
    }
    
    const UDIEnemyDefinition* EnemyDefinition = Enemy->EnemyDefinition;

    if (!IsValid(EnemyDefinition))
    {
        return EStateTreeRunStatus::Failed;
    }

    const FDIStandoffConfig& StandoffConfig = EnemyDefinition->Standoff;


    AAIController* AIController = Cast<AAIController>(Enemy->GetController());

    if (!IsValid(AIController))
    {
        return EStateTreeRunStatus::Failed;
    }


    // Facing
    FVector ToTarget = InstanceData.Target->GetActorLocation() - Enemy->GetActorLocation();

    ToTarget.Z = 0.0f;

    if (ToTarget.IsNearlyZero())
    {
        return EStateTreeRunStatus::Running;
    }

    ToTarget.Normalize();

    const float TargetYaw = ToTarget.Rotation().Yaw;

    const float ActorYawBefore = Enemy->GetActorRotation().Yaw;
    const float ControlYawBefore = AIController->GetControlRotation().Yaw;
    const float YawError = FMath::FindDeltaAngleDegrees(ActorYawBefore, TargetYaw);

    if (FMath::Abs(YawError) > 15.0f)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[STANDOFF_ROT] Actor=%.1f | Control=%.1f | Target=%.1f | Error=%.1f"),
            ActorYawBefore, ControlYawBefore, TargetYaw, YawError);
    }

    AIController->SetFocus(InstanceData.Target);
    AIController->SetControlRotation(FRotator(0.0f,TargetYaw,0.0f));
    Enemy->SetActorRotation(FRotator(0.0f,TargetYaw,0.0f));

    const float DistanceToTarget = FVector::Dist2D(Enemy->GetActorLocation(),InstanceData.Target->GetActorLocation());


    // 1. 최초 Spacing Reset
    //
    // Player를 바라보면서 뒤로 걸어서
    // 전투 대치 거리를 한 번 정리한다.
    if (InstanceData.bResettingSpacing)
    {
        const float DesiredDistance = FMath::Max(StandoffConfig.MinStandoffDistance, StandoffConfig.DesiredStandoffDistance);


        if (DistanceToTarget < DesiredDistance)
        {
            Enemy->AddMovementInput(-ToTarget, StandoffConfig.BackpedalInputScale);

            return EStateTreeRunStatus::Running;
        }


        if (UCharacterMovementComponent* Movement =
            Enemy->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }


        InstanceData.bResettingSpacing = false;
        InstanceData.bSpacingResetComplete = true;

        InstanceData.HoldElapsedTime = 0.0f;
        InstanceData.HoverActionElapsedTime = 0.0f;
        InstanceData.CurrentHoverActionDuration = 0.0f;
        InstanceData.CurrentHoverIntent =
            EDIStandoffHoverIntent::Hold;


        UE_LOG(
            LogTemp,
            Log,
            TEXT(
                "[STANDOFF] RESET COMPLETE | Enemy=%s | Distance=%.1f"),
            *Enemy->GetName(),
            DistanceToTarget);

        return EStateTreeRunStatus::Running;
    }


    // 2. Combat Hover
    if (!InstanceData.bSpacingResetComplete)
    {
        return EStateTreeRunStatus::Failed;
    }


    InstanceData.HoldElapsedTime += DeltaTime;
    InstanceData.HoverActionElapsedTime += DeltaTime;


    // Standoff 전체 시간 종료
    if (InstanceData.HoldElapsedTime >=
        InstanceData.CurrentHoldDuration)
    {
        if (UCharacterMovementComponent* Movement =
            Enemy->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }


        UE_LOG(
            LogTemp,
            Log,
            TEXT(
                "[STANDOFF] COMPLETE | Enemy=%s | Distance=%.1f | Hold=%.2f"),
            *Enemy->GetName(),
            DistanceToTarget,
            InstanceData.HoldElapsedTime);


        return EStateTreeRunStatus::Succeeded;
    }


    // 현재 Hover Intent가 끝났으면
    // 다음 행동을 랜덤 선택
    if (InstanceData.CurrentHoverActionDuration <= 0.0f ||
        InstanceData.HoverActionElapsedTime >=
        InstanceData.CurrentHoverActionDuration)
    {
        TArray<EDIStandoffHoverIntent> AvailableIntents;
        
        TArray<float> AvailableWeights;

        auto AddWeightedIntent = [&AvailableIntents, &AvailableWeights](EDIStandoffHoverIntent Intent, float Weight)
        {
            const float SafeWeight = FMath::Max(0.0f, Weight);

            if (SafeWeight <= 0.0f)
            {
                return;
            }

            AvailableIntents.Add(Intent);
            AvailableWeights.Add(SafeWeight);
        };

        AddWeightedIntent(EDIStandoffHoverIntent::Hold, StandoffConfig.HoldWeight);
        AddWeightedIntent(EDIStandoffHoverIntent::StrafeLeft, StandoffConfig.StrafeLeftWeight);
        AddWeightedIntent(EDIStandoffHoverIntent::StrafeRight, StandoffConfig.StrafeRightWeight);

        if (DistanceToTarget > StandoffConfig.MinHoverDistance)
        {
            AddWeightedIntent(EDIStandoffHoverIntent::StepIn, StandoffConfig.StepInWeight);
        }

        if (DistanceToTarget < StandoffConfig.MaxHoverDistance)
        {
            AddWeightedIntent(EDIStandoffHoverIntent::StepOut, StandoffConfig.StepOutWeight);
        }

        if (AvailableIntents.Num() > 0)
        {
            float TotalWeight = 0.0f;

            for (const float Weight : AvailableWeights)
            {
                TotalWeight += Weight;
            }

            float Roll = FMath::FRandRange(0.0f, TotalWeight);
            int32 SelectedIndex = AvailableIntents.Num() - 1;

            for (int32 Index = 0; Index < AvailableWeights.Num(); ++Index)
            {
                Roll -= AvailableWeights[Index];

                if (Roll <= 0.0f)
                {
                    SelectedIndex = Index;
                    break;
                }
            }

            InstanceData.CurrentHoverIntent = AvailableIntents[SelectedIndex];
        }
        else
        {
            InstanceData.CurrentHoverIntent = EDIStandoffHoverIntent::Hold;
        }
        
        const float MinActionDuration = FMath::Min(StandoffConfig.MinHoverActionDuration, StandoffConfig.MaxHoverActionDuration);
        const float MaxActionDuration = FMath::Max(StandoffConfig.MinHoverActionDuration, StandoffConfig.MaxHoverActionDuration);
        InstanceData.CurrentHoverActionDuration = FMath::FRandRange(MinActionDuration, MaxActionDuration);

        InstanceData.HoverActionElapsedTime = 0.0f;


        UE_LOG(
            LogTemp,
            Verbose,
            TEXT(
                "[STANDOFF] NEW HOVER INTENT | Enemy=%s | Intent=%d | Duration=%.2f | Distance=%.1f"),
            *Enemy->GetName(),
            static_cast<int32>(
                InstanceData.CurrentHoverIntent),
            InstanceData.CurrentHoverActionDuration,
            DistanceToTarget);
    }


    // 3. 현재 Hover Intent 실행
    switch (InstanceData.CurrentHoverIntent)
    {
    case EDIStandoffHoverIntent::Hold:
    {
        // 일부러 아무 이동도 하지 않는다.
        // 상대를 바라보며 한 박자 기다린다.
        break;
    }


    case EDIStandoffHoverIntent::StrafeLeft:
    {
        Enemy->AddMovementInput(-Enemy->GetActorRightVector(), StandoffConfig.HoverInputScale);

        break;
    }


    case EDIStandoffHoverIntent::StrafeRight:
    {
        Enemy->AddMovementInput(Enemy->GetActorRightVector(), StandoffConfig.HoverInputScale);
        
        break;
    }


    case EDIStandoffHoverIntent::StepIn:
    {
        // 실행 중 거리가 너무 가까워지면
        // 더 이상 앞으로 가지 않는다.
        if (DistanceToTarget > StandoffConfig.MinHoverDistance)
        {
            Enemy->AddMovementInput(ToTarget, StandoffConfig.HoverInputScale);
        }

        break;
    }


    case EDIStandoffHoverIntent::StepOut:
    {
        // 실행 중 너무 멀어지면
        // 더 이상 뒤로 가지 않는다.
        if (DistanceToTarget < StandoffConfig.MaxHoverDistance)
        {
            Enemy->AddMovementInput(-ToTarget, StandoffConfig.HoverInputScale);
        }

        break;
    }


    default:
        break;
    }


    return EStateTreeRunStatus::Running;
}


void FDIStandoffTask::ExitState(FStateTreeExecutionContext& Context,const FStateTreeTransitionResult& Transition) const
{
    (void)Transition;

    FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

    ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);


    if (IsValid(Enemy))
    {
        if (AAIController* AIController = Cast<AAIController>(Enemy->GetController()))
        {
            // Standoff가 소유하던 이동만 종료.
            // Combat Focus 자체는 Combat 루프에서 계속 사용한다.
            AIController->StopMovement();
        }


        if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();

            if (InstanceData.bWalkSpeedOverridden)
            {
                Movement->MaxWalkSpeed = InstanceData.PreviousMaxWalkSpeed;
            }
        }
    }


    InstanceData.bResettingSpacing = false;
    InstanceData.bSpacingResetComplete = false;

    InstanceData.HoldElapsedTime = 0.0f;
    InstanceData.CurrentHoldDuration = 0.0f;

    InstanceData.HoverActionElapsedTime = 0.0f;
    InstanceData.CurrentHoverActionDuration = 0.0f;
    InstanceData.CurrentHoverIntent = EDIStandoffHoverIntent::Hold;

    InstanceData.PreviousMaxWalkSpeed = 0.0f;
    InstanceData.bWalkSpeedOverridden = false;
}