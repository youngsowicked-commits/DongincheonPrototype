#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIHitReactTask.generated.h"

class APawn;

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIHitReactTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
};

USTRUCT(meta = (DisplayName = "DI Hit React"))
struct DONGINCHEONPROTOTYPE_API FDIHitReactTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FDIHitReactTaskInstanceData;
	
	FDIHitReactTask()
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


