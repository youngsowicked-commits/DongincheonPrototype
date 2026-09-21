#include "Components/QTEComponent.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogDIQTE, Log, All);

UQTEComponent::UQTEComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UQTEComponent::StartQTE(const FQTEConfig& Config)
{
    if (bQTEActive)
    {
        UE_LOG(
            LogDIQTE,
            Warning,
            TEXT("StartQTE FAILED | Another QTE is already active"));

        return false;
    }

    if (Config.QTEId.IsNone())
    {
        UE_LOG(
            LogDIQTE,
            Error,
            TEXT("StartQTE FAILED | QTEId is None"));

        return false;
    }

    if (Config.Steps.IsEmpty())
    {
        UE_LOG(
            LogDIQTE,
            Error,
            TEXT("StartQTE FAILED | Id=%s | Steps are empty"),
            *Config.QTEId.ToString());

        return false;
    }

    for (int32 Index = 0; Index < Config.Steps.Num(); ++Index)
    {
        const FQTEStep& Step = Config.Steps[Index];

        if (Step.InputType == EQTEInputType::None)
        {
            UE_LOG(
                LogDIQTE,
                Error,
                TEXT("StartQTE FAILED | Id=%s | Step=%d | InputType=None"),
                *Config.QTEId.ToString(),
                Index);

            return false;
        }

        if (Step.TimeLimit <= 0.0f)
        {
            UE_LOG(
                LogDIQTE,
                Error,
                TEXT("StartQTE FAILED | Id=%s | Step=%d | TimeLimit=%.2f"),
                *Config.QTEId.ToString(),
                Index,
                Step.TimeLimit);

            return false;
        }
    }

    ActiveConfig = Config;

    CurrentInputIndex = 0;
    bQTEActive = true;

    StartCurrentStepTimer();

    OnQTEStarted.Broadcast(ActiveConfig.QTEId,ActiveConfig.Steps.Num());

    return true;
}

void UQTEComponent::SubmitInput(EQTEInputType InputType)
{
    if (!bQTEActive)
    {
        return;
    }

    if (!ActiveConfig.Steps.IsValidIndex(CurrentInputIndex))
    {
        CompleteQTE(EQTEResult::Failed);
        return;
    }

    const EQTEInputType ExpectedInput = ActiveConfig.Steps[CurrentInputIndex].InputType;

    if (InputType != ExpectedInput)
    {
        CompleteQTE(EQTEResult::Failed);
        return;
    }

    ++CurrentInputIndex;

    OnQTEProgress.Broadcast(ActiveConfig.QTEId,CurrentInputIndex,ActiveConfig.Steps.Num());

    if (CurrentInputIndex >= ActiveConfig.Steps.Num())
    {
        CompleteQTE(EQTEResult::Success);
        return;
    }

    StartCurrentStepTimer();
}

void UQTEComponent::FailQTE()
{
    if (!bQTEActive)
    {
        return;
    }

    CompleteQTE(EQTEResult::Failed);
}

void UQTEComponent::CancelQTE()
{
    if (!bQTEActive)
    {
        return;
    }

    CompleteQTE(EQTEResult::Cancelled);
}

void UQTEComponent::HandleTimeout()
{
    if (!bQTEActive)
    {
        return;
    }

    CompleteQTE(EQTEResult::Failed);
}

void UQTEComponent::StartCurrentStepTimer()
{
    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return;
    }

    World->GetTimerManager().ClearTimer(QTETimeoutHandle);

    if (!bQTEActive)
    {
        return;
    }

    if (!ActiveConfig.Steps.IsValidIndex(CurrentInputIndex))
    {
        return;
    }

    const float StepTimeLimit = ActiveConfig.Steps[CurrentInputIndex].TimeLimit;

    UE_LOG(
    LogTemp,
    Warning,
    TEXT("QTE STEP TIMER START | Id=%s | Step=%d/%d | Expected=%d | Limit=%.2f"),
    *ActiveConfig.QTEId.ToString(),
    CurrentInputIndex + 1,
    ActiveConfig.Steps.Num(),
    static_cast<int32>(ActiveConfig.Steps[CurrentInputIndex].InputType),
    StepTimeLimit);
    
    if (StepTimeLimit <= 0.0f)
    {
        return;
    }

    World->GetTimerManager().SetTimer(QTETimeoutHandle,this,&UQTEComponent::HandleTimeout,StepTimeLimit,false);
}

void UQTEComponent::CompleteQTE(EQTEResult Result)
{
    if (!bQTEActive)
    {
        return;
    }

    const FName CompletedQTEId = ActiveConfig.QTEId;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(QTETimeoutHandle);
    }

    bQTEActive = false;
    ActiveConfig = FQTEConfig();
    CurrentInputIndex = 0;

    OnQTECompleted.Broadcast(
        CompletedQTEId,
        Result);
}

EQTEInputType UQTEComponent::GetExpectedInput() const
{
    if (!bQTEActive)
    {
        return EQTEInputType::None;
    }

    if (!ActiveConfig.Steps.IsValidIndex(CurrentInputIndex))
    {
        return EQTEInputType::None;
    }

    return ActiveConfig.Steps[CurrentInputIndex].InputType;
}

float UQTEComponent::GetTimeLimit() const
{
    if (!bQTEActive)
    {
        return 0.0f;
    }

    if (!ActiveConfig.Steps.IsValidIndex(CurrentInputIndex))
    {
        return 0.0f;
    }

    return ActiveConfig.Steps[CurrentInputIndex].TimeLimit;
}

float UQTEComponent::GetRemainingTime() const
{
    if (!bQTEActive)
    {
        return 0.0f;
    }

    if (!ActiveConfig.Steps.IsValidIndex(CurrentInputIndex))
    {
        return 0.0f;
    }

    if (ActiveConfig.Steps[CurrentInputIndex].TimeLimit <= 0.0f)
    {
        return 0.0f;
    }

    const UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return 0.0f;
    }

    const float RemainingTime = World->GetTimerManager().GetTimerRemaining(QTETimeoutHandle);

    return FMath::Max(0.0f, RemainingTime);
}

float UQTEComponent::GetRemainingTimeNormalized() const
{
    const float TimeLimit = GetTimeLimit();

    if (TimeLimit <= 0.0f)
    {
        return 0.0f;
    }

    return FMath::Clamp(GetRemainingTime() / TimeLimit,0.0f,1.0f);
}

void UQTEComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(QTETimeoutHandle);
    }

    bQTEActive = false;
    ActiveConfig = FQTEConfig();
    CurrentInputIndex = 0;

    Super::EndPlay(EndPlayReason);
}
