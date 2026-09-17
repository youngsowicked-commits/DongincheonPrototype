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
	
	const FDIAttackPattern* SelectWeightedAttackPattern(const UDIEnemyDefinition* Definition)
	{
		if (!IsValid(Definition))
		{
			return nullptr;
		}
		
		float TotalWeight = 0.0f;
		
		for (const FDIAttackPattern& Pattern : Definition->AttackPatterns)
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
		
		for (const FDIAttackPattern& Pattern : Definition->AttackPatterns)
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

EStateTreeRunStatus FDIAttackTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	InstanceData.ActiveAttackSequece.Reset();
	InstanceData.CurrentSequenceIndex = INDEX_NONE;
	
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
	const float DistanceToTarget = FVector::Dist2D(Enemy->GetActorLocation(), InstanceData.Target->GetActorLocation());
	
	if (DistanceToTarget > InstanceData.MaxAttackDistance)
	{
		UE_LOG(LogTemp, Verbose, TEXT("DI Attack Task: Target out of range | Enemy=%s | Distance=%.1f | Max=%.1f"),
			*Enemy->GetName(), DistanceToTarget, InstanceData.MaxAttackDistance);
		
		return EStateTreeRunStatus::Failed;
	}
	
	//AttackPatterns가 존재하고 유효하면 Pattern을 랜덤 선택
	if (IsValid(Enemy->EnemyDefinition))
	{
		const FDIAttackPattern* SelectedPattern = SelectWeightedAttackPattern(Enemy->EnemyDefinition);
		
		if (SelectedPattern)
		{
			InstanceData.ActiveAttackSequece = SelectedPattern->AttackIndices;
		}
	}
	
	//패턴이 없거나 모두 .Invalid/Weight 0이면 기존 단타방식으로
	if (InstanceData.ActiveAttackSequece.Num() <= 0)
	{
		InstanceData.ActiveAttackSequece.Add(InstanceData.AttackIndex);
	}
	
	InstanceData.CurrentSequenceIndex = 0;
	
	const int32 FirstAttackIndex = InstanceData.ActiveAttackSequece[InstanceData.CurrentSequenceIndex];
	
	UE_LOG(LogTemp,Log,TEXT("DI Attack Tast: Pattern Started | Enemy=%s | Hits=%d | FirstAttack=%d"),
		*Enemy->GetName(), InstanceData.ActiveAttackSequece.Num(),FirstAttackIndex);
	
	if (!Enemy->StartAttack(InstanceData.AttackIndex))
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
	
	if (Enemy->IsAttackActive())
	{
		return EStateTreeRunStatus::Running;
	}
	
	//현재 한 타 종료, ActiveAttackIndex를 먼저 정리
	Enemy->FinishAttack();
	
	++InstanceData.CurrentSequenceIndex;
	
	//Pattern의 마지막 공격까지 모두 끝남.
	if (!InstanceData.ActiveAttackSequece.IsValidIndex(InstanceData.CurrentSequenceIndex))
	{
		UE_LOG(LogTemp,Log,TEXT("DI Attack Tast: Pattern Completed | Enemy%s"), *Enemy->GetName());
		
		InstanceData.ActiveAttackSequece.Reset();
		InstanceData.CurrentSequenceIndex = INDEX_NONE;
		
		return EStateTreeRunStatus::Succeeded;
	}
	
	//다음 콤보 어택 시작
	const int32 NextAttackIndex = InstanceData.ActiveAttackSequece[InstanceData.CurrentSequenceIndex];
	
	UE_LOG(LogTemp, Verbose, TEXT("DI Attack Task: Combo Next | Enemy%s | Step=%d | AttackIndex=%d"),
		*Enemy->GetName(),InstanceData.CurrentSequenceIndex,NextAttackIndex);
	
	if (!Enemy->StartAttack(NextAttackIndex))
	{
		UE_LOG(LogTemp,Warning,TEXT("DI Attack Task: Combo StartAttack failed | Enemy=%s | AttackIndex=%d"),
			*Enemy->GetName(),NextAttackIndex);
		
		InstanceData.ActiveAttackSequece.Reset();
		InstanceData.CurrentSequenceIndex = INDEX_NONE;
		
		return EStateTreeRunStatus::Failed;
	}
	
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
}
