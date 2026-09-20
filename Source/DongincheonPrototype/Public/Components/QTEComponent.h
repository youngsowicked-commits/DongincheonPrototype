#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "QTEComponent.generated.h"

UENUM(BlueprintType)
enum class EQTEResult : uint8
{
    Success,
    Failed,
    Cancelled
};

USTRUCT(BlueprintType)
struct DONGINCHEONPROTOTYPE_API FQTEConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QTE")
    FName QTEId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QTE", meta = (ClampMin = "1"))
    int32 RequiredPressCount = 1;

    // 0 = 시간 제한 없음
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "QTE",meta = (ClampMin = "0.0"))
    float TimeLimit = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FQTEStartedSignature,FName, QTEId,int32, RequiredPressCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FQTEProgressSignature,FName, QTEId,int32, CurrentPressCount,
    int32, RequiredPressCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FQTECompletedSignature,FName, QTEId,EQTEResult, Result);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DONGINCHEONPROTOTYPE_API UQTEComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UQTEComponent();

    UFUNCTION(BlueprintCallable, Category = "QTE")
    bool StartQTE(const FQTEConfig& Config);

    UFUNCTION(BlueprintCallable, Category = "QTE")
    void SubmitPress();

    UFUNCTION(BlueprintCallable, Category = "QTE")
    void FailQTE();

    UFUNCTION(BlueprintCallable, Category = "QTE")
    void CancelQTE();

    UFUNCTION(BlueprintPure, Category = "QTE")
    bool IsQTEActive() const
    {
        return bQTEActive;
    }

    UFUNCTION(BlueprintPure, Category = "QTE")
    FName GetActiveQTEId() const
    {
        return ActiveConfig.QTEId;
    }

    UFUNCTION(BlueprintPure, Category = "QTE")
    int32 GetCurrentPressCount() const
    {
        return CurrentPressCount;
    }

    UFUNCTION(BlueprintPure, Category = "QTE")
    int32 GetRequiredPressCount() const
    {
        return ActiveConfig.RequiredPressCount;
    }
    
    UFUNCTION(BlueprintPure, Category = "QTE")
    float GetTimeLimit() const
    {
        return ActiveConfig.TimeLimit;
    }

    UFUNCTION(BlueprintPure, Category = "QTE")
    float GetRemainingTime() const;

    UPROPERTY(BlueprintAssignable, Category = "QTE")
    FQTEStartedSignature OnQTEStarted;

    UPROPERTY(BlueprintAssignable, Category = "QTE")
    FQTEProgressSignature OnQTEProgress;

    UPROPERTY(BlueprintAssignable, Category = "QTE")
    FQTECompletedSignature OnQTECompleted;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void CompleteQTE(EQTEResult Result);

    void HandleTimeout();

private:
    bool bQTEActive = false;

    FQTEConfig ActiveConfig;

    int32 CurrentPressCount = 0;

    FTimerHandle QTETimeoutHandle;
};