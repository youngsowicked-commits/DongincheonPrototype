#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIDeadTask.generated.h"

class APawn;

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIDeadTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
};

USTRUCT(meta = (DisplayName = "DI Dead"))
struct DONGINCHEONPROTOTYPE_API FDIDeadTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FDIDeadTaskInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};