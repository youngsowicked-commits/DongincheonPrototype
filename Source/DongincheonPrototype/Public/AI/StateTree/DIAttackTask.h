#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIAttackTask.generated.h"

class APawn;

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIAttackTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0"))
	int32 AttackIndex = 0;
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