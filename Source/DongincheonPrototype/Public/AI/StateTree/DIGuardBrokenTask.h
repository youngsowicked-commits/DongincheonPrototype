#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIGuardBrokenTask.generated.h"

class APawn;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIGuardBrokenTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Guard", meta = (ClampMin = "0.0"))
	float RecoveryDuration = 0.6f;

	float ElapsedTime = 0.0f;
};

USTRUCT(meta = (DisplayName = "DI Guard Broken"))
struct DONGINCHEONPROTOTYPE_API FDIGuardBrokenTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FDIGuardBrokenTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};