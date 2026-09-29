#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIStandoffTask.generated.h"

class AActor;
class APawn;

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;


UENUM()
enum class EDIStandoffHoverIntent : uint8
{
    Hold,
    StrafeLeft,
    StrafeRight,
    StepIn,
    StepOut
};


USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIStandoffTaskInstanceData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<APawn> Pawn = nullptr;

    UPROPERTY(EditAnywhere, Category = "Context")
    TObjectPtr<AActor> Target = nullptr;

    // Runtime Only
    UPROPERTY(Transient)
    bool bResettingSpacing = false;

    UPROPERTY(Transient)
    bool bSpacingResetComplete = false;

    UPROPERTY(Transient)
    float HoldElapsedTime = 0.0f;

    UPROPERTY(Transient)
    float CurrentHoldDuration = 0.0f;

    UPROPERTY(Transient)
    float HoverActionElapsedTime = 0.0f;

    UPROPERTY(Transient)
    float CurrentHoverActionDuration = 0.0f;

    UPROPERTY(Transient)
    EDIStandoffHoverIntent CurrentHoverIntent =
        EDIStandoffHoverIntent::Hold;

    UPROPERTY(Transient)
    float PreviousMaxWalkSpeed = 0.0f;

    UPROPERTY(Transient)
    bool bWalkSpeedOverridden = false;
};


USTRUCT(meta = (DisplayName = "DI Standoff"))
struct DONGINCHEONPROTOTYPE_API FDIStandoffTask
    : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()

    using FInstanceDataType = FDIStandoffTaskInstanceData;

    FDIStandoffTask()
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

    virtual void ExitState(
        FStateTreeExecutionContext& Context,
        const FStateTreeTransitionResult& Transition) const override;
};