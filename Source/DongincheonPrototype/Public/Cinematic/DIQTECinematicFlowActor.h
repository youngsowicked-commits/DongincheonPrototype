#pragma once

#include "CoreMinimal.h"
#include "Cinematic/DICinematicFlowActor.h"
#include "Cinematic/DICinematicTypes.h"
#include "DIQTECinematicFlowActor.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FDIQTERequestedSignature,
	FName,
	StepId
);

UCLASS()
class DONGINCHEONPROTOTYPE_API ADIQTECinematicFlowActor
	: public ADICinematicFlowActor
{
	GENERATED_BODY()

public:
	ADIQTECinematicFlowActor();

	UFUNCTION(BlueprintCallable, Category = "QTE|Cinematic")
	void StartQTECinematicFlow();

	UFUNCTION(BlueprintCallable, Category = "QTE|Cinematic")
	void NotifyQTEResolved(FName StepId, bool bSucceeded);

	UFUNCTION(BlueprintPure, Category = "QTE|Cinematic")
	EDIQTECinematicFlowState GetQTECinematicFlowState() const;

	UFUNCTION(BlueprintPure, Category = "QTE|Cinematic")
	FName GetCurrentQTEStepId() const;

	UPROPERTY(BlueprintAssignable, Category = "QTE|Cinematic")
	FDIQTERequestedSignature OnQTERequested;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void HandleSequenceSegmentFinished() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QTE|Cinematic")
	TArray<FDIQTECinematicStep> Steps;

private:
	void LockQTECameraLook();
	void UnlockQTECameraLook();

	TWeakObjectPtr<APlayerController> LookLockedController;
	bool bQTECameraLookLocked = false;
	
	void BeginCurrentStep();
	void PlayCurrentResultSequence();
	void AdvanceToNextStep();
	void CompleteFlow();

	const FDIQTECinematicStep* GetCurrentStep() const;

	UPROPERTY(Transient)
	int32 CurrentStepIndex = INDEX_NONE;

	UPROPERTY(Transient)
	EDIQTECinematicFlowState FlowState =
		EDIQTECinematicFlowState::Inactive;

	UPROPERTY(Transient)
	bool bCurrentQTEResolved = false;

	UPROPERTY(Transient)
	bool bCurrentQTESucceeded = false;

	UPROPERTY(Transient)
	bool bCurrentQTESequenceFinished = false;
};