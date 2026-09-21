#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "DIGuardCooldownCondition.generated.h"

class APawn;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIGuardCooldownConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Guard", meta = (ClampMin = "0.0"))
	float CooldownDuration = 5.0f;
};

USTRUCT(meta = (DisplayName = "DI Guard Cooldown"))
struct DONGINCHEONPROTOTYPE_API FDIGuardCooldownCondition
	: public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FDIGuardCooldownConditionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};