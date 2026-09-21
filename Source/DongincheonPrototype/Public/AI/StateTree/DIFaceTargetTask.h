#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIFaceTargetTask.generated.h"

class APawn;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIFaceTargetTaskInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	// 초당 회전 각도
	UPROPERTY(EditAnywhere, Category = "Facing", meta = (ClampMin = "1.0"))
	float RotationSpeed = 540.0f;

	// 이 각도 이하까지 맞으면 Facing 완료
	UPROPERTY(EditAnywhere, Category = "Facing", meta = (ClampMin = "0.1"))
	float AcceptanceAngle = 5.0f;
};

USTRUCT(meta = (DisplayName = "DI Face Target"))
struct DONGINCHEONPROTOTYPE_API FDIFaceTargetTask
	: public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FDIFaceTargetTaskInstanceData;

	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,const FStateTreeTransitionResult& Transition) const override;

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context,float DeltaTime) const override;
};