#include "Cinematic/DIQTECinematicFlowActor.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"

ADIQTECinematicFlowActor::ADIQTECinematicFlowActor()
{
	// QTE Flow의 Presentation 상태는 외부 Owner가 관리하는 것을 기본값으로 한다.
	// Boss Mid-Fight의 경우 ADIBattleEncounter가 담당한다.
	bManagePresentationState = false;
	bRestoreGameplayOnFinish = false;
}

void ADIQTECinematicFlowActor::LockQTECameraLook()
{
	if (bQTECameraLookLocked)
	{
		return;
	}

	APlayerController* PC =
		UGameplayStatics::GetPlayerController(this, 0);

	if (!IsValid(PC))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[QTE CAMERA] PlayerController not found")
		);
		return;
	}

	PC->SetIgnoreLookInput(true);

	LookLockedController = PC;
	bQTECameraLookLocked = true;

	UE_LOG(LogTemp, Log, TEXT("[QTE CAMERA] LOOK LOCKED"));
}

void ADIQTECinematicFlowActor::UnlockQTECameraLook()
{
	if (!bQTECameraLookLocked)
	{
		return;
	}

	if (APlayerController* PC = LookLockedController.Get())
	{
		PC->SetIgnoreLookInput(false);
	}

	LookLockedController.Reset();
	bQTECameraLookLocked = false;

	UE_LOG(LogTemp, Log, TEXT("[QTE CAMERA] LOOK UNLOCKED"));
}

void ADIQTECinematicFlowActor::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	UnlockQTECameraLook();

	Super::EndPlay(EndPlayReason);
}

void ADIQTECinematicFlowActor::StartQTECinematicFlow()
{
	if (FlowState != EDIQTECinematicFlowState::Inactive && FlowState != EDIQTECinematicFlowState::Completed)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[DIQTECinematicFlowActor] Flow already active: %s"),
			*GetName()
		);

		return;
	}

	if (Steps.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[DIQTECinematicFlowActor] No QTE steps configured: %s"),
			*GetName()
		);

		return;
	}

	TSet<FName> StepIds;

	for (const FDIQTECinematicStep& Step : Steps)
	{
		if (Step.StepId.IsNone())
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[DIQTECinematicFlowActor] StepId is None: %s"),
				*GetName()
			);

			return;
		}

		if (StepIds.Contains(Step.StepId))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[DIQTECinematicFlowActor] Duplicate StepId '%s': %s"),
				*Step.StepId.ToString(),
				*GetName()
			);

			return;
		}

		StepIds.Add(Step.StepId);
	}

	LockQTECameraLook();

	CurrentStepIndex = 0;

	bCurrentQTEResolved = false;
	bCurrentQTESucceeded = false;
	bCurrentQTESequenceFinished = false;

	if (IsValid(Sequence))
	{
		FlowState = EDIQTECinematicFlowState::PlayingSetup;
		PlayCinematic();

		if (!IsPlayingCinematic())
		{
			UnlockQTECameraLook();

			FlowState = EDIQTECinematicFlowState::Inactive;
			CurrentStepIndex = INDEX_NONE;
		}

		return;
	}

	BeginCurrentStep();
}

void ADIQTECinematicFlowActor::NotifyQTEResolved(FName StepId,bool bSucceeded)
{
	const FDIQTECinematicStep* CurrentStep = GetCurrentStep();

	if (!CurrentStep)
	{
		return;
	}

	if (CurrentStep->StepId != StepId)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[DIQTECinematicFlowActor] Received result for wrong StepId. "
				"Expected '%s', received '%s'."
			),
			*CurrentStep->StepId.ToString(),
			*StepId.ToString()
		);

		return;
	}

	if (bCurrentQTEResolved)
	{
		return;
	}

	if (FlowState != EDIQTECinematicFlowState::PlayingQTESequence && FlowState != EDIQTECinematicFlowState::WaitingForQTEResult)
	{
		return;
	}

	bCurrentQTEResolved = true;
	bCurrentQTESucceeded = bSucceeded;

	if (bCurrentQTESequenceFinished)
	{
		PlayCurrentResultSequence();
	}
}

EDIQTECinematicFlowState ADIQTECinematicFlowActor::GetQTECinematicFlowState() const
{
	return FlowState;
}

FName ADIQTECinematicFlowActor::GetCurrentQTEStepId() const
{
	const FDIQTECinematicStep* CurrentStep = GetCurrentStep();

	return CurrentStep ? CurrentStep->StepId : NAME_None;
}

void ADIQTECinematicFlowActor::HandleSequenceSegmentFinished()
{
	switch (FlowState)
	{
	case EDIQTECinematicFlowState::PlayingSetup:
		BeginCurrentStep();
		break;

	case EDIQTECinematicFlowState::PlayingQTESequence:
		bCurrentQTESequenceFinished = true;

		if (bCurrentQTEResolved)
		{
			PlayCurrentResultSequence();
		}
		else
		{
			FlowState = EDIQTECinematicFlowState::WaitingForQTEResult;
		}
		break;

	case EDIQTECinematicFlowState::PlayingResult:
		AdvanceToNextStep();
		break;

	default:
		UE_LOG(
			LogTemp,
			Warning,
			TEXT(
				"[DIQTECinematicFlowActor] Sequence finished in unexpected state: %d"
			),
			static_cast<int32>(FlowState)
		);
		break;
	}
}

void ADIQTECinematicFlowActor::BeginCurrentStep()
{
	const FDIQTECinematicStep* CurrentStep = GetCurrentStep();

	if (!CurrentStep)
	{
		CompleteFlow();
		return;
	}

	bCurrentQTEResolved = false;
	bCurrentQTESucceeded = false;
	bCurrentQTESequenceFinished = false;

	if (IsValid(CurrentStep->QTESequence))
	{
		FlowState = EDIQTECinematicFlowState::PlayingQTESequence;

		PlaySequenceAsset(CurrentStep->QTESequence);

		bCurrentQTESequenceFinished = !IsPlayingCinematic();
	}
	else
	{
		bCurrentQTESequenceFinished = true;

		FlowState = EDIQTECinematicFlowState::WaitingForQTEResult;
	}

	OnQTERequested.Broadcast(CurrentStep->StepId);
}

void ADIQTECinematicFlowActor::PlayCurrentResultSequence()
{
	const FDIQTECinematicStep* CurrentStep = GetCurrentStep();

	if (!CurrentStep)
	{
		CompleteFlow();
		return;
	}

	ULevelSequence* ResultSequence = bCurrentQTESucceeded ? CurrentStep->SuccessSequence.Get() : CurrentStep->FailSequence.Get();

	if (!IsValid(ResultSequence))
	{
		AdvanceToNextStep();
		return;
	}

	FlowState =
		EDIQTECinematicFlowState::PlayingResult;

	PlaySequenceAsset(ResultSequence);

	if (!IsPlayingCinematic())
	{
		AdvanceToNextStep();
	}
}

void ADIQTECinematicFlowActor::AdvanceToNextStep()
{
	++CurrentStepIndex;

	if (!Steps.IsValidIndex(CurrentStepIndex))
	{
		CompleteFlow();
		return;
	}

	BeginCurrentStep();
}

void ADIQTECinematicFlowActor::CompleteFlow()
{
	FlowState = EDIQTECinematicFlowState::Completed;

	CurrentStepIndex = INDEX_NONE;

	bCurrentQTEResolved = false;
	bCurrentQTESucceeded = false;
	bCurrentQTESequenceFinished = false;

	// 전체 QTE Flow 완료 시 Camera Look 복구
	UnlockQTECameraLook();

	// 기존 Encounter의 Combat Resume 이벤트 유지
	OnCinematicFinished.Broadcast();
}

const FDIQTECinematicStep* ADIQTECinematicFlowActor::GetCurrentStep() const
{
	if (!Steps.IsValidIndex(CurrentStepIndex))
	{
		return nullptr;
	}

	return &Steps[CurrentStepIndex];
}