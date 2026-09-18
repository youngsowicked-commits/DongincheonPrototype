#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIPresentationHoldTask.generated.h"

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIPresentationHoldTaskInstanceData 
{
	GENERATED_BODY()
};

USTRUCT(meta = (DisplayName = "DI Presentation Hold"))
struct DONGINCHEONPROTOTYPE_API FDIPresentationHoldTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FDIPresentationHoldTaskInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
