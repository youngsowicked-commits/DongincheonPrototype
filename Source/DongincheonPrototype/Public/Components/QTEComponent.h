#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "QTEComponent.generated.h"

UENUM(BlueprintType)
enum class EQTEInputType : uint8
{
    None,
    Light,
    Heavy,
    Dodge
};

UENUM(BlueprintType)
enum class EQTEResult : uint8
{
    Success,
    Failed,
    Cancelled
};

USTRUCT(BlueprintType)
struct DONGINCHEONPROTOTYPE_API FQTEStep
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QTE")
    EQTEInputType InputType = EQTEInputType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QTE",
        meta = (ClampMin = "0.0"))
    float TimeLimit = 1.0f;
};

USTRUCT(BlueprintType)
struct DONGINCHEONPROTOTYPE_API FQTEConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QTE")
    FName QTEId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QTE")
    TArray<FQTEStep> Steps;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FQTEStartedSignature,FName, QTEId,int32, RequiredInputCount);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FQTEProgressSignature,FName, QTEId,int32, CurrentInputCount,int32, RequiredInputCount);

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
    void SubmitInput(EQTEInputType InputType);

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
    int32 GetCurrentInputCount() const
    {
        return CurrentInputIndex;
    }

    UFUNCTION(BlueprintPure, Category = "QTE")
    int32 GetRequiredInputCount() const
    {
        return ActiveConfig.Steps.Num();
    }

    UFUNCTION(BlueprintPure, Category = "QTE")
    EQTEInputType GetExpectedInput() const;

    UFUNCTION(BlueprintPure, Category = "QTE")
    float GetTimeLimit() const;

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
    void StartCurrentStepTimer();

private:
    bool bQTEActive = false;

    FQTEConfig ActiveConfig;

    int32 CurrentInputIndex = 0;

    FTimerHandle QTETimeoutHandle;
};