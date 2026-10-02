#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIRecoverTask.generated.h"

class AActor;
class APawn;

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIRecoverTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, Category = "Recover", meta = (ClampMin = "0.0"))
	float RecoveryDuration = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Recover|Facing", meta = (ClampMin = "1.0"))
	float RotationSpeed = 420.0f;

	UPROPERTY(Transient)
	float ElapsedTime = 0.0f;
};

USTRUCT(meta = (DisplayName = "DI Recover"))
struct DONGINCHEONPROTOTYPE_API FDIRecoverTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FDIRecoverTaskInstanceData;

	FDIRecoverTask()
	{
		bShouldCallTick = true;
	}

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;
};