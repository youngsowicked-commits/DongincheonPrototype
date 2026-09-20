#include "Components/QTEComponent.h"

#include "Engine/World.h"

UQTEComponent::UQTEComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UQTEComponent::StartQTE(const FQTEConfig& Config)
{
    if (bQTEActive)
    {
        return false;
    }

    ActiveConfig = Config;

    ActiveConfig.RequiredPressCount = FMath::Max(1, Config.RequiredPressCount);

    ActiveConfig.TimeLimit = FMath::Max(0.0f, Config.TimeLimit);

    CurrentPressCount = 0;
    bQTEActive = true;

    if (ActiveConfig.TimeLimit > 0.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(QTETimeoutHandle,this,&UQTEComponent::HandleTimeout,
                ActiveConfig.TimeLimit,false);
        }
    }

    OnQTEStarted.Broadcast(ActiveConfig.QTEId,ActiveConfig.RequiredPressCount);

    return true;
}

void UQTEComponent::SubmitPress()
{
    if (!bQTEActive)
    {
        return;
    }

    ++CurrentPressCount;

    CurrentPressCount = FMath::Min(CurrentPressCount,ActiveConfig.RequiredPressCount);

    OnQTEProgress.Broadcast(ActiveConfig.QTEId,CurrentPressCount,ActiveConfig.RequiredPressCount);

    if (CurrentPressCount >= ActiveConfig.RequiredPressCount)
    {
        CompleteQTE(EQTEResult::Success);
    }
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

    OnQTECompleted.Broadcast(CompletedQTEId,Result);

    ActiveConfig = FQTEConfig();
    CurrentPressCount = 0;
}

float UQTEComponent::GetRemainingTime() const
{
    if (!bQTEActive)
    {
        return 0.0f;
    }

    if (ActiveConfig.TimeLimit <= 0.0f)
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

void UQTEComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(QTETimeoutHandle);
    }

    bQTEActive = false;

    Super::EndPlay(EndPlayReason);
}
