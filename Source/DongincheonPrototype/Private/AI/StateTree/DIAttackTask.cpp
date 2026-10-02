#include "AI/StateTree/DIAttackTask.h"

#include "AI/Data/DIEnemyDefinition.h"
#include "Character/DongincheonEnemyBase.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"


namespace 
{
	bool IsValidAttackPattern(const UDIEnemyDefinition* Definition, const FDIAttackPattern& Pattern)
	{
		if (!IsValid(Definition))
		{
		 return false;
		}
		
		if (Pattern.Weight <= 0.0f)
		{
			return false;
		}
		
		if (Pattern.AttackIndices.Num() <= 0)
		{
			return false;
		}
		
		for (const int32 AttackIndex : Pattern.AttackIndices)
		{
			if (!Definition->Attacks.IsValidIndex(AttackIndex))
			{
				return false;
			}
		}
		
		return true;
	}
	
	const FDIAttackPattern* SelectWeightedAttackPattern(const UDIEnemyDefinition* Definition,const TArray<FDIAttackPattern>& Patterns)
	{
		if (!IsValid(Definition))
		{
			return nullptr;
		}

		float TotalWeight = 0.0f;

		for (const FDIAttackPattern& Pattern : Patterns)
		{
			if (!IsValidAttackPattern(Definition, Pattern))
			{
				continue;
			}

			TotalWeight += Pattern.Weight;
		}

		if (TotalWeight <= KINDA_SMALL_NUMBER)
		{
			return nullptr;
		}

		float RandomValue = FMath::FRandRange(0.0f, TotalWeight);

		const FDIAttackPattern* LastValidPattern = nullptr;

		for (const FDIAttackPattern& Pattern : Patterns)
		{
			if (!IsValidAttackPattern(Definition, Pattern))
			{
				continue;
			}

			LastValidPattern = &Pattern;

			RandomValue -= Pattern.Weight;

			if (RandomValue <= 0.0f)
			{
				return &Pattern;
			}
		}

		return LastValidPattern;
	}
}

EStateTreeRunStatus FDIAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	InstanceData.ActiveAttackSequece.Reset();
	InstanceData.CurrentSequenceIndex = INDEX_NONE;
	InstanceData.bIsReAiming = false;
	InstanceData.bIsFinalReAiming = false;
	InstanceData.ReAimElapsedTime = 0.0f;
	
	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);
	
	if (!IsValid(Enemy))
	{
		UE_LOG(LogTemp,Error,TEXT("DI Attack Task : Invalid Enemy Pawn"));
		
		return EStateTreeRunStatus::Failed;
	}
	
	if (!IsValid(InstanceData.Target))
	{
		UE_LOG(LogTemp,Warning,TEXT("DI Attack Task: Invalid Target on %s"), *Enemy->GetName());
		
		return EStateTreeRunStatus::Failed;
	}
	
	//높이차이는 의미가 없으므로 XY평면거리만 사용.
	const float DistanceToTarget = FVector::Dist2D(Enemy->GetActorLocation(),InstanceData.Target->GetActorLocation());

	if (DistanceToTarget > InstanceData.MaxAttackDistance)
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("DI Attack Task: Target out of range | Enemy=%s | Distance=%.1f | Max=%.1f"),
			*Enemy->GetName(),
			DistanceToTarget,
			InstanceData.MaxAttackDistance);

		return EStateTreeRunStatus::Failed;
	}
	
	// 현재 Boss Phase에 맞는 Attack Pattern을 선택한다.
	// Phase 2 Pattern이 없거나 모두 Invalid이면 기존 Phase 1 Pattern으로 안전하게 fallback한다.
	if (IsValid(Enemy->EnemyDefinition))
	{
		const UDIEnemyDefinition* Definition = Enemy->EnemyDefinition;

		const FDIAttackPattern* SelectedPattern = nullptr;

		if (Enemy->IsPhase2Active())
		{
			SelectedPattern = SelectWeightedAttackPattern(Definition,Definition->Phase2AttackPatterns);
		}

		if (!SelectedPattern)
		{
			SelectedPattern = SelectWeightedAttackPattern(Definition,Definition->AttackPatterns);
		}

		if (SelectedPattern)
		{
			InstanceData.ActiveAttackSequece = SelectedPattern->AttackIndices;

			UE_LOG(
				LogTemp,
				Log,
				TEXT("DI Attack Task: Pattern Selected | Enemy=%s | Phase2=%s | Hits=%d"),
				*Enemy->GetName(),
				Enemy->IsPhase2Active() ? TEXT("TRUE") : TEXT("FALSE"),
				InstanceData.ActiveAttackSequece.Num());
		}
	}
	
	//패턴이 없거나 모두 .Invalid/Weight 0이면 기존 단타방식으로
	if (InstanceData.ActiveAttackSequece.Num() <= 0)
	{
		InstanceData.ActiveAttackSequece.Add(InstanceData.AttackIndex);
	}
	
	InstanceData.CurrentSequenceIndex = 0;
	
	const int32 FirstAttackIndex = InstanceData.ActiveAttackSequece[InstanceData.CurrentSequenceIndex];
	
	UE_LOG(
	LogTemp,
	Log,
	TEXT("DI Attack Task: Pattern Started | Enemy=%s | Distance=%.1f | Max=%.1f | Hits=%d | FirstAttack=%d"),
	*Enemy->GetName(),
	DistanceToTarget,
	InstanceData.MaxAttackDistance,
	InstanceData.ActiveAttackSequece.Num(),
	FirstAttackIndex);
	
if (!Enemy->StartAttack(FirstAttackIndex))
{
		UE_LOG(LogTemp,Warning,TEXT("DI Attack Task : StartAttack failed | Enemy=%s | AttackIndex=%d"), 
			*Enemy->GetName(), FirstAttackIndex);
		
		InstanceData.ActiveAttackSequece.Reset();
		InstanceData.CurrentSequenceIndex = INDEX_NONE;
		
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
	
	// 마지막 공격 종료 후 Recover 진입 전 Facing 정리
	if (InstanceData.bIsFinalReAiming)
	{
		if (!IsValid(InstanceData.Target))
		{
			InstanceData.bIsFinalReAiming = false;
			InstanceData.ActiveAttackSequece.Reset();
			InstanceData.CurrentSequenceIndex = INDEX_NONE;
			InstanceData.ReAimElapsedTime = 0.0f;

			return EStateTreeRunStatus::Succeeded;
		}

		InstanceData.ReAimElapsedTime += DeltaTime;

		FVector ToTarget = InstanceData.Target->GetActorLocation() - Enemy->GetActorLocation();
		ToTarget.Z = 0.0f;

		bool bFacingTarget = true;

		if (!ToTarget.IsNearlyZero())
		{
			const float TargetYaw = ToTarget.Rotation().Yaw;
			const FRotator CurrentRotation = Enemy->GetActorRotation();
			const float YawDifference = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetYaw));

			bFacingTarget = YawDifference <= InstanceData.ComboReAimAcceptanceAngle;

			if (!bFacingTarget)
			{
				FRotator TargetRotation = CurrentRotation;
				TargetRotation.Yaw = TargetYaw;

				const FRotator NewRotation = FMath::RInterpConstantTo(
					CurrentRotation,
					TargetRotation,
					DeltaTime,
					InstanceData.FinalReAimSpeed);

				Enemy->SetActorRotation(NewRotation);
			}
		}

		const bool bReAimTimedOut = InstanceData.ReAimElapsedTime >= InstanceData.FinalReAimMaxTime;

		if (!bFacingTarget && !bReAimTimedOut)
		{
			return EStateTreeRunStatus::Running;
		}

		InstanceData.bIsFinalReAiming = false;
		InstanceData.ActiveAttackSequece.Reset();
		InstanceData.CurrentSequenceIndex = INDEX_NONE;
		InstanceData.ReAimElapsedTime = 0.0f;

		return EStateTreeRunStatus::Succeeded;
	}

	// 1. 콤보 사이 ReAim 중
	if (InstanceData.bIsReAiming)
    {
    	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[COMBO_REAIM] ACTIVE | Enemy=%s | Step=%d | Time=%.3f"),
		*Enemy->GetName(),
		InstanceData.CurrentSequenceIndex,
		InstanceData.ReAimElapsedTime);

    	
    	
        if (!IsValid(InstanceData.Target))
        {
            return EStateTreeRunStatus::Failed;
        }

        if (!InstanceData.ActiveAttackSequece.IsValidIndex(InstanceData.CurrentSequenceIndex))
        {
            return EStateTreeRunStatus::Failed;
        }

        InstanceData.ReAimElapsedTime += DeltaTime;

        FVector ToTarget = InstanceData.Target->GetActorLocation() - Enemy->GetActorLocation();

        // 높이 차이는 무시
        ToTarget.Z = 0.0f;

        bool bFacingTarget = true;

        if (!ToTarget.IsNearlyZero())
        {
            const float TargetYaw = ToTarget.Rotation().Yaw;

            const FRotator CurrentRotation = Enemy->GetActorRotation();

            const float YawDifference = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw,TargetYaw));
        	
        	UE_LOG(
				LogTemp,
				Warning,
				TEXT("[COMBO_REAIM] ActorYaw=%.1f | TargetYaw=%.1f | Diff=%.1f"),
				CurrentRotation.Yaw,
				TargetYaw,
				YawDifference);

            bFacingTarget = YawDifference <= InstanceData.ComboReAimAcceptanceAngle;

            // 아직 충분히 정면을 못 봤으면 조금씩 회전
            if (!bFacingTarget)
            {
                FRotator TargetRotation = CurrentRotation;

                TargetRotation.Yaw = TargetYaw;

                const FRotator NewRotation = FMath::RInterpConstantTo(CurrentRotation,TargetRotation,DeltaTime,InstanceData.ComboReAimSpeed);

                Enemy->SetActorRotation(NewRotation);
            	UE_LOG(
					LogTemp,
					Warning,
					TEXT("[COMBO_REAIM] ROTATE | NewYaw=%.1f"),
					NewRotation.Yaw);
            }
        }

        const bool bReAimTimedOut = InstanceData.ReAimElapsedTime >= InstanceData.ComboReAimMaxTime;

        // 아직 각도도 안 맞았고 제한시간도 남았다면 다음 프레임에도 계속 ReAim
        if (!bFacingTarget && !bReAimTimedOut)
        {
            return EStateTreeRunStatus::Running;
        }

        // ReAim 완료 → 다음 콤보 공격 시작
        const int32 NextAttackIndex = InstanceData.ActiveAttackSequece[InstanceData.CurrentSequenceIndex];

        InstanceData.bIsReAiming = false;
        InstanceData.ReAimElapsedTime = 0.0f;

        UE_LOG(
            LogTemp,
            Verbose,
            TEXT(
                "DI Attack Task: Combo ReAim Completed | Enemy=%s | Step=%d | AttackIndex=%d"),
            *Enemy->GetName(),
            InstanceData.CurrentSequenceIndex,
            NextAttackIndex);

        if (!Enemy->StartAttack(NextAttackIndex))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT(
                    "DI Attack Task: Combo StartAttack failed | Enemy=%s | AttackIndex=%d"),
                *Enemy->GetName(),
                NextAttackIndex);

            InstanceData.ActiveAttackSequece.Reset();
            InstanceData.CurrentSequenceIndex = INDEX_NONE;
            InstanceData.bIsReAiming = false;
            InstanceData.ReAimElapsedTime = 0.0f;

            return EStateTreeRunStatus::Failed;
        }

        return EStateTreeRunStatus::Running;
    }

    // 2. 현재 공격이 아직 진행 중
    if (Enemy->IsAttackActive())
    {
        return EStateTreeRunStatus::Running;
    }
	
    // 3. 현재 한 타 종료
    Enemy->FinishAttack();

    ++InstanceData.CurrentSequenceIndex;
	
	// 4. Pattern 전체 완료
	if (!InstanceData.ActiveAttackSequece.IsValidIndex(
			InstanceData.CurrentSequenceIndex))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("DI Attack Task: Pattern Completed -> Final ReAim | Enemy=%s"),
			*Enemy->GetName());

		InstanceData.bIsReAiming = false;
		InstanceData.bIsFinalReAiming = true;
		InstanceData.ReAimElapsedTime = 0.0f;

		return EStateTreeRunStatus::Running;
	}

    // 5. 다음 공격이 존재함 : 즉시 StartAttack 하지 않고 ReAim 단계로 진입
    InstanceData.bIsReAiming = true;
    InstanceData.ReAimElapsedTime = 0.0f;

    return EStateTreeRunStatus::Running;
}

void FDIAttackTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InstanceData.Pawn);
	
	if (IsValid(Enemy))
	{
		Enemy->CancelAttack();
	}
	
	InstanceData.ActiveAttackSequece.Reset();
	InstanceData.CurrentSequenceIndex = INDEX_NONE;
	InstanceData.bIsReAiming = false;
	InstanceData.bIsFinalReAiming = false;
	InstanceData.ReAimElapsedTime = 0.0f;
}
