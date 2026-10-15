#include "Cinematic/DIQTECinematicFlowActor.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "MovieSceneObjectBindingID.h"
#include "TimerManager.h"
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

void ADIQTECinematicFlowActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndQTESlowMotion();
	
	UnlockQTECameraLook();

	Super::EndPlay(EndPlayReason);
}

bool ADIQTECinematicFlowActor::SetQTECinematicActors(
	AActor* InPlayer, AActor* InBoss)
{
	if (!IsValid(InPlayer) ||
		!IsValid(InBoss) ||
		InPlayer == InBoss)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[QTE] Invalid Player/Boss binding"));
		return false;
	}

	QTEPlayerActor = InPlayer;
	QTEBossActor = InBoss;

	return true;
}

bool ADIQTECinematicFlowActor::PrepareSequenceActor(
	ALevelSequenceActor* InSequenceActor,
	ULevelSequence* InSequence)
{
	if (!IsValid(InSequenceActor) || !IsValid(InSequence))
	{
		return false;
	}

	// 메인 Setup Sequence는 QTE Runtime Binding 대상이 아니다.
	// 태그 조회 자체를 하지 않고 그대로 재생한다.
	if (InSequence == Sequence.Get())
	{
		return true;
	}

	const FName PlayerTag(TEXT("QTE_Player"));
	const FName BossTag(TEXT("QTE_Boss"));

	const bool bHasPlayerTag =
		InSequenceActor->FindNamedBinding(PlayerTag).IsValid();

	const bool bHasBossTag =
		InSequenceActor->FindNamedBinding(BossTag).IsValid();

	// 각 QTE 연출 Sequence에는 두 바인딩이 필수.
	if (!bHasPlayerTag || !bHasBossTag ||
		!QTEPlayerActor.IsValid() ||
		!QTEBossActor.IsValid())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[QTE] Missing actor or binding tag: %s"),
			*GetNameSafe(InSequence));
		return false;
	}

	TArray<AActor*> PlayerActors;
	PlayerActors.Add(QTEPlayerActor.Get());

	TArray<AActor*> BossActors;
	BossActors.Add(QTEBossActor.Get());
	
	UE_LOG(LogTemp, Warning,
	TEXT("[QTE_DEBUG] BINDING | Sequence=%s | Player=%s | Boss=%s"),
	*GetNameSafe(InSequence),
	*GetNameSafe(QTEPlayerActor.Get()),
	*GetNameSafe(QTEBossActor.Get()));

	InSequenceActor->SetBindingByTag(
		PlayerTag, PlayerActors, false);

	InSequenceActor->SetBindingByTag(
		BossTag, BossActors, false);
	
	const FName FlowTag(TEXT("QTE_Flow"));

	if (!InSequenceActor->FindNamedBinding(FlowTag).IsValid())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[QTE] Missing QTE_Flow tag: %s"),
			*GetNameSafe(InSequence));

		return false;
	}

	TArray<AActor*> FlowActors;
	FlowActors.Add(this);

	InSequenceActor->SetBindingByTag(
		FlowTag,
		FlowActors,
		false);

	UE_LOG(LogTemp, Warning,
		TEXT("[QTE_DEBUG] FLOW BOUND | Actor=%s | Sequence=%s"),
		*GetName(),
		*GetNameSafe(InSequence));

	return true;
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

void ADIQTECinematicFlowActor::NotifyQTEResolved(
	FName StepId, bool bSucceeded)
{
	const FDIQTECinematicStep* Step = GetCurrentStep();

	if (!Step ||
		Step->StepId != StepId ||
		!bQTEWindowOpened ||
		bQTEBranchReached ||
		bCurrentQTEResolved)
	{
		return;
	}

	// 판정 결과만 저장한다.
	// 실제 애니메이션 전환은 QTE_Branch 키에서 수행한다.
	bCurrentQTEResolved = true;
	bCurrentQTESucceeded = bSucceeded;

	UE_LOG(LogTemp, Log,
		TEXT("[QTE] Resolved: %s | Success=%d"),
		*StepId.ToString(), bSucceeded);
}

void ADIQTECinematicFlowActor::HandleQTEWindowStart()
{
	UE_LOG(LogTemp, Warning,
	TEXT("[QTE_DEBUG] WINDOW EVENT RECEIVED | Step=%s | State=%d"),
	*GetCurrentQTEStepId().ToString(),
	static_cast<int32>(FlowState));
	
    if (FlowState != EDIQTECinematicFlowState::PlayingResult ||
        bQTEWindowOpened || bQTEBranchReached)
    {
        return;
    }

    const FDIQTECinematicStep* Step = GetCurrentStep();

    if (!Step)
    {
        return;
    }

    bQTEWindowOpened = true;

    SetActiveSequencePlayRate(QTESlowPlayRate);

    // 기존 Boss Encounter가 QTEComponent와 HUD를 시작
    OnQTERequested.Broadcast(Step->StepId);

    UE_LOG(LogTemp, Log,
        TEXT("[QTE] Window START: %s"),
        *Step->StepId.ToString());
}

void ADIQTECinematicFlowActor::HandleQTEBranch()
{
	UE_LOG(LogTemp, Warning,
	TEXT("[QTE_DEBUG] BRANCH EVENT RECEIVED | Step=%s"),
	*GetCurrentQTEStepId().ToString());
	
    if (FlowState != EDIQTECinematicFlowState::PlayingResult ||
        !bQTEWindowOpened || bQTEBranchReached)
    {
        return;
    }

    // 입력이 없었다면 기존 Encounter의 QTE를 실패 처리하도록 요청
    if (!bCurrentQTEResolved)
    {
        OnQTEBranchDeadline.Broadcast();
    }

    // Blueprint 연결이 빠진 경우를 로그로 드러낸다.
    if (!bCurrentQTEResolved)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[QTE] Deadline did not resolve QTE. Check BP binding."));

        bCurrentQTEResolved = true;
        bCurrentQTESucceeded = false;
    }

    bQTEBranchReached = true;

    PendingFailStartSeconds = GetActiveSequenceTimeSeconds();

    // 분기 이후 원래 재생 속도로 복귀
    SetActiveSequencePlayRate(1.0f);

    if (bCurrentQTESucceeded)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[QTE] SUCCESS - Continue Success Sequence"));
        return;
    }

    // Sequence 평가 중 Stop/교체하지 않고 다음 Tick으로 연기
    bPendingFailSwitch = true;

    GetWorldTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(
            this,
            &ADIQTECinematicFlowActor::ExecuteFailTransition));
}

void ADIQTECinematicFlowActor::ExecuteFailTransition()
{
    const FDIQTECinematicStep* Step = GetCurrentStep();

    if (!bPendingFailSwitch || !Step)
    {
        return;
    }

    bPendingFailSwitch = false;

    if (!IsValid(Step->FailSequence))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[QTE] Missing Fail Sequence: %s"),
            *Step->StepId.ToString());

        CompleteFlow();
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("[QTE] FAIL - Switch at %.3f seconds"),
        PendingFailStartSeconds);

    if (!ReplaceActiveSequenceAtTime(
        Step->FailSequence,
        PendingFailStartSeconds))
    {
        CompleteFlow();
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

	case EDIQTECinematicFlowState::PlayingResult:
		if (bPendingFailSwitch)
		{
			return;
		}

		if (!bQTEBranchReached)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[QTE] Sequence ended before QTE_Branch event"));

			CompleteFlow();
			return;
		}

		AdvanceToNextStep();
		break;

	default:
		UE_LOG(LogTemp, Warning,
			TEXT("[QTE] Unexpected flow state: %d"),
			static_cast<int32>(FlowState));
		break;
	}
}

void ADIQTECinematicFlowActor::BeginCurrentStep()
{
	const FDIQTECinematicStep* Step = GetCurrentStep();

	if (!Step)
	{
		CompleteFlow();
		return;
	}

	bCurrentQTEResolved = false;
	bCurrentQTESucceeded = false;
	bCurrentQTESequenceFinished = false;

	bQTEWindowOpened = false;
	bQTEBranchReached = false;
	bPendingFailSwitch = false;

	if (!IsValid(Step->SuccessSequence))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[QTE] Missing Success Sequence: %s"),
			*Step->StepId.ToString());

		CompleteFlow();
		return;
	}

	FlowState = EDIQTECinematicFlowState::PlayingResult;

	// Success를 기본 애니메이션으로 처음부터 재생
	PlaySequenceAsset(Step->SuccessSequence);

	if (!IsPlayingCinematic())
	{
		CompleteFlow();
	}
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
	EndQTESlowMotion();
	
	FlowState = EDIQTECinematicFlowState::Completed;

	CurrentStepIndex = INDEX_NONE;

	bCurrentQTEResolved = false;
	bCurrentQTESucceeded = false;
	bCurrentQTESequenceFinished = false;
	bQTEWindowOpened = false;
	bQTEBranchReached = false;
	bPendingFailSwitch = false;

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

void ADIQTECinematicFlowActor::BeginQTESlowMotion()
{
	if (bSlowMotionApplied || !IsValid(GetWorld()))
	{
		return;
	}

	PreviousGlobalTimeDilation =
		UGameplayStatics::GetGlobalTimeDilation(this);

	UGameplayStatics::SetGlobalTimeDilation(
		this, QTEInputTimeDilation);

	bSlowMotionApplied = true;
}

void ADIQTECinematicFlowActor::EndQTESlowMotion()
{
	if (!bSlowMotionApplied)
	{
		return;
	}

	if (IsValid(GetWorld()))
	{
		UGameplayStatics::SetGlobalTimeDilation(
			this, PreviousGlobalTimeDilation);
	}

	bSlowMotionApplied = false;
}