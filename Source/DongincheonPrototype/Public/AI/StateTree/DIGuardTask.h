#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIGuardTask.generated.h"

class APawn;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIGuardTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Guard", meta = (ClampMin = "0.0"))
	float GuardDuration = 0.9f;

	float ElapsedTime = 0.0f;
};

USTRUCT(meta = (DisplayName = "DI Guard"))
struct DONGINCHEONPROTOTYPE_API FDIGuardTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FDIGuardTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

	virtual void ExitState(FStateTreeExecutionContext& Context,const FStateTreeTransitionResult& Transition) const override;
};
