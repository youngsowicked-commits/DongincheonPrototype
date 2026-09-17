#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIAttackTask.generated.h"

class AActor;
class APawn;

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIAttackTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Target = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0"))
	int32 AttackIndex = 0;
	
	/*
	* Approach MoveTo의 Acceptance Radius와는 역할이 다름.
	* Approach Radius:
	* "이 정도면 접근 이동을 끝내도 된다."
	* 
	* MaxAttackDistance:
	* "실제로 지금 공격을 시작해도 된다."
	*
	* 플레이어가 Approach 종료 직후 뒤로 빠지는 상황을 잡기 위한
	* Attack 진입 직전 최종 검증값.
	*/
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.0"))
	float MaxAttackDistance = 200.0f;
	
	//Runtime Only
	UPROPERTY(Transient)
	TArray<int32> ActiveAttackSequece;
	
	UPROPERTY(Transient)
	int32 CurrentSequenceIndex = INDEX_NONE;
};

USTRUCT(meta = (DisplayName = "DI Attack"))
struct DONGINCHEONPROTOTYPE_API FDIAttackTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FDIAttackTaskInstanceData;
	
	FDIAttackTask()
	{
		bShouldCallTick = true;
	}
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};