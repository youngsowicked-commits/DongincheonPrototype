#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "DITargetAttackingCondition.generated.h"

class AActor;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDITargetAttackingConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Target = nullptr;
};

USTRUCT(meta = (DisplayName = "DI Target Attacking"))
struct DONGINCHEONPROTOTYPE_API FDITargetAttackingCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FDITargetAttackingConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};