#include "Gameplay/Flow/DIGameFlowDirector.h"

#include "Character/DongincheonCharacter.h"
#include "Components/InteractionComponent.h"
#include "Components/TargetingComponent.h"
#include "Player/DIPlayerController.h"
#include "UI/Widgets/DIHUDRootWidget.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/Battle/DIBattleEncounter.h"

DEFINE_LOG_CATEGORY_STATIC(LogDIGameFlow, Log, All);

ADIGameFlowDirector::ADIGameFlowDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ADIGameFlowDirector::BeginPlay()
{
	Super::BeginPlay();

	// Tutorial Encounter 등록 + 초기 비활성
	for (ADIBattleEncounter* Encounter : TutorialEncounters)
	{
		BindEncounter(Encounter);
		DisableEncounterTrigger(Encounter);
	}

	// Main Encounter
	BindEncounter(MainEncounter);
	DisableEncounterTrigger(MainEncounter);

	// Boss Encounter
	BindEncounter(BossEncounter);
	DisableEncounterTrigger(BossEncounter);

	if (bAutoStartFlow)
	{
		SetPhase(EDIGameFlowPhase::Intro);
	}
}

void ADIGameFlowDirector::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	CleanupTutorial();
	Super::EndPlay(EndPlayReason);
}

void ADIGameFlowDirector::BindEncounter(ADIBattleEncounter* Encounter)
{
	if (!IsValid(Encounter))
	{
		return;
	}

	Encounter->OnEncounterCompletedDelegate.AddUniqueDynamic(this,&ADIGameFlowDirector::HandleEncounterCompleted);

	Encounter->OnCombatStartedDelegate.AddUniqueDynamic(this,&ADIGameFlowDirector::HandleTutorialCombatStarted);
}

void ADIGameFlowDirector::DisableEncounterTrigger(ADIBattleEncounter* Encounter)
{
	if (!IsValid(Encounter))
	{
		return;
	}

	Encounter->SetEncounterTriggerEnabled(false);
}

void ADIGameFlowDirector::SetPhase(EDIGameFlowPhase NewPhase)
{
	if (CurrentPhase == NewPhase)
	{
		return;
	}

	const EDIGameFlowPhase PreviousPhase = CurrentPhase;

	if (PreviousPhase == EDIGameFlowPhase::Tutorial &&
		NewPhase != EDIGameFlowPhase::Tutorial)
	{
		CleanupTutorial();
	}

	CurrentPhase = NewPhase;

	UE_LOG(
		LogDIGameFlow,
		Log,
		TEXT("FLOW | %d -> %d"),
		static_cast<int32>(PreviousPhase),
		static_cast<int32>(CurrentPhase));

	OnFlowPhaseChanged(
		PreviousPhase,
		CurrentPhase);

	switch (CurrentPhase)
	{
	case EDIGameFlowPhase::Intro:
		OnIntroRequested();
		break;

	case EDIGameFlowPhase::Chase:
		OnChaseStarted();
		break;

	case EDIGameFlowPhase::Tutorial:
		CurrentTutorialIndex = 0;

		OnTutorialStarted();

		StartCurrentTutorialEncounter();
		break;

	case EDIGameFlowPhase::MainEncounter:
		OnMainEncounterStarted();

		if (IsValid(MainEncounter))
		{
			MainEncounter->SetEncounterTriggerEnabled(true);
		}
		else
		{
			SetPhase(EDIGameFlowPhase::BossEncounter);
		}
		break;

	case EDIGameFlowPhase::BossEncounter:
		OnBossEncounterStarted();

		if (IsValid(BossEncounter))
		{
			BossEncounter->SetEncounterTriggerEnabled(true);
		}
		else
		{
			// Boss가 없으면 Ending으로 fail-safe
			SetPhase(
				EDIGameFlowPhase::Ending);
		}
		break;

	case EDIGameFlowPhase::Ending:
		{
			// Outro 중 전투 모드는 유지하되 입력만 제한.
			if (ADongincheonCharacter* Player =
				Cast<ADongincheonCharacter>(
					UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				Player->SetPresentationInputLocked(true);
			}

			OnEndingRequested();
			break;
		}

	case EDIGameFlowPhase::FreeRoam:
		{
			if (ADongincheonCharacter* Player =
				Cast<ADongincheonCharacter>(
					UGameplayStatics::GetPlayerCharacter(this, 0)))
			{
				// Ending이 모두 끝난 후에만 Battle Mode 종료.
				Player->SetCombatInputEnabled(false);
				Player->SetPresentationInputLocked(false);

				if (UInteractionComponent* Interaction =
					Player->FindComponentByClass<UInteractionComponent>())
				{
					Interaction->SetInteractionEnabled(true);
				}
			}

			OnFreeRoamStarted();
			break;
		}
	}
}

void ADIGameFlowDirector::NotifyIntroFinished()
{
	if (CurrentPhase != EDIGameFlowPhase::Intro)
	{
		return;
	}

	SetPhase(
		EDIGameFlowPhase::Chase);
}

void ADIGameFlowDirector::NotifyChaseFinished()
{
	if (CurrentPhase != EDIGameFlowPhase::Chase)
	{
		return;
	}

	// 여기서부터 Ending 종료까지 연속 Battle Mode 유지.
	if (ADongincheonCharacter* Player =
		Cast<ADongincheonCharacter>(
			UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		Player->SetCombatInputEnabled(true);

		if (UInteractionComponent* Interaction =
			Player->FindComponentByClass<UInteractionComponent>())
		{
			Interaction->SetInteractionEnabled(false);
		}
	}

	if (!TutorialEncounters.IsEmpty())
	{
		SetPhase(EDIGameFlowPhase::Tutorial);
	}
	else
	{
		SetPhase(EDIGameFlowPhase::MainEncounter);
	}
}

void ADIGameFlowDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bTutorialMovementSampling)
    {
        UpdateTutorialMovement(DeltaSeconds);
    }
}

void ADIGameFlowDirector::NotifyTutorialEncounterReady(
    ADIBattleEncounter* Encounter)
{
    if (CurrentPhase != EDIGameFlowPhase::Tutorial ||
        CurrentTutorialIndex != 0 ||
        !TutorialEncounters.IsValidIndex(0) ||
        TutorialEncounters[0] != Encounter ||
        !IsValid(Encounter) ||
        TutorialStage != INDEX_NONE)
    {
        return;
    }

	Encounter->SetTutorialEnemyDamageLimited(true);
	StartTutorialMovement();
}

void ADIGameFlowDirector::StartTutorialMovement()
{
    ADongincheonCharacter* Player =
        Cast<ADongincheonCharacter>(
            UGameplayStatics::GetPlayerCharacter(this, 0));

    ADIPlayerController* PC =
        Cast<ADIPlayerController>(
            UGameplayStatics::GetPlayerController(this, 0));

    if (!IsValid(Player) || !IsValid(PC))
    {
        UE_LOG(LogDIGameFlow, Error,
            TEXT("TUTORIAL MOVEMENT | Player or PC missing"));
        return;
    }

    UnbindTutorialAction();

    TutorialActionPlayer = Player;

    TutorialMovedCm = 0.0f;
    TutorialCameraDegrees = 0.0f;
    TutorialUIRefreshElapsed = 0.0f;

    TutorialLastLocation = Player->GetActorLocation();
    TutorialLastControlYaw = PC->GetControlRotation().Yaw;

    // Encounter Ready 이후 이동 조작 허용
    Player->SetPresentationInputLocked(false);

    bTutorialMovementSampling = true;
    SetActorTickEnabled(true);

    SetTutorialStage(0);

    UE_LOG(LogDIGameFlow, Log,
        TEXT("TUTORIAL MOVEMENT START | Index=0"));
}

void ADIGameFlowDirector::UpdateTutorialMovement(
    float DeltaSeconds)
{
    if (CurrentPhase != EDIGameFlowPhase::Tutorial ||
        CurrentTutorialIndex != 0 ||
        TutorialStage != 0)
    {
        bTutorialMovementSampling = false;
        SetActorTickEnabled(false);
        return;
    }

    ADongincheonCharacter* Player =
        TutorialActionPlayer.Get();

    ADIPlayerController* PC =
        Cast<ADIPlayerController>(
            UGameplayStatics::GetPlayerController(this, 0));

    if (!IsValid(Player) || !IsValid(PC))
    {
        return;
    }

    const FVector CurrentLocation = Player->GetActorLocation();
    const float CurrentYaw = PC->GetControlRotation().Yaw;

    // 실제 WASD 조작으로 발생한 이동만 인정
    if (Player->HasTutorialMovementInput())
    {
        const float StepCm = FVector::Dist2D(
            TutorialLastLocation, CurrentLocation);

        // 순간이동이나 비정상적인 프레임 이동 제외
        const float MaxStepCm =
            FMath::Max(100.0f, 1200.0f * DeltaSeconds);

        if (StepCm <= MaxStepCm)
        {
            TutorialMovedCm = FMath::Min(
                TutorialRequiredMoveCm,
                TutorialMovedCm + StepCm);
        }
    }

    // 실제 마우스 입력이 있는 프레임만 카메라 회전 집계
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    PC->GetInputMouseDelta(MouseX, MouseY);

    if (!Player->IsGameplayInputLocked() &&
        (FMath::Abs(MouseX) + FMath::Abs(MouseY)) > 0.01f)
    {
        const float DeltaYaw = FMath::Abs(
            FMath::FindDeltaAngleDegrees(
                TutorialLastControlYaw, CurrentYaw));

        TutorialCameraDegrees = FMath::Min(
            TutorialRequiredCameraDegrees,
            TutorialCameraDegrees + FMath::Min(DeltaYaw, 45.0f));
    }

    TutorialLastLocation = CurrentLocation;
    TutorialLastControlYaw = CurrentYaw;

    TutorialUIRefreshElapsed += DeltaSeconds;

    if (TutorialUIRefreshElapsed >= 0.1f)
    {
        TutorialUIRefreshElapsed = 0.0f;
        RefreshTutorialProgress();
    }

    if (TutorialMovedCm < TutorialRequiredMoveCm ||
        TutorialCameraDegrees < TutorialRequiredCameraDegrees)
    {
        return;
    }

    bTutorialMovementSampling = false;
    SetActorTickEnabled(false);

    RefreshTutorialProgress();

    UE_LOG(LogDIGameFlow, Log,
        TEXT("TUTORIAL MOVEMENT COMPLETE | "
             "Move=%.1f | Camera=%.1f"),
        TutorialMovedCm,
        TutorialCameraDegrees);

	// AI Combat는 이미 진행 중.
	// 이동 완료 시 공격 튜토리얼로만 전환한다.

	TutorialFirstCount = 0;
	TutorialSecondCount = 0;

	if (IsValid(Player))
	{
		Player->OnTutorialAction.AddUniqueDynamic(
			this,
			&ADIGameFlowDirector::HandleTutorialAction);
	}

	SetTutorialStage(1);
}

void ADIGameFlowDirector::StartCurrentTutorialEncounter()
{
	if (CurrentPhase != EDIGameFlowPhase::Tutorial)
	{
		return;
	}

	if (!TutorialEncounters.IsValidIndex(
		CurrentTutorialIndex))
	{
		SetPhase(
			EDIGameFlowPhase::MainEncounter);

		return;
	}

	ADIBattleEncounter* Encounter =
		TutorialEncounters[CurrentTutorialIndex];

	if (!IsValid(Encounter))
	{
		++CurrentTutorialIndex;
		StartCurrentTutorialEncounter();
		return;
	}

	if (CurrentTutorialIndex == 1)
	{
		// Tutorial 02: Trigger 없이 적 4명 전투 즉시 시작
		Encounter->StartEncounterFromFlow();
	}
	else
	{
		// Tutorial 01: 기존 Trigger로 적 2명 전투 시작
		Encounter->SetEncounterTriggerEnabled(true);
	}

	UE_LOG(
		LogDIGameFlow,
		Log,
		TEXT("FLOW | Tutorial Encounter Enabled | Index=%d | Encounter=%s"),
		CurrentTutorialIndex,
		*GetNameSafe(Encounter));
}

void ADIGameFlowDirector::HandleTutorialCombatStarted(
	ADIBattleEncounter* Encounter)
{
	if (CurrentPhase != EDIGameFlowPhase::Tutorial ||
		!IsValid(Encounter) ||
		!TutorialEncounters.IsValidIndex(CurrentTutorialIndex) ||
		TutorialEncounters[CurrentTutorialIndex] != Encounter)
	{
		return;
	}

	if (CurrentTutorialIndex > 1)
	{
		return;
	}
	
	if (CurrentTutorialIndex == 1)
	{
		Encounter->SetTutorialEnemyDamageLimited(true);
	}

	// Tutorial 01: AI 전투는 정상 시작하되
	// 이동 / 카메라 학습을 반드시 먼저 실행한다.
	if (CurrentTutorialIndex == 0)
	{
		// BP OnEncounterReady 연결이 누락되었더라도
		// 공격 페이지로 바로 넘어가지 않도록 보장
		if (TutorialStage == INDEX_NONE)
		{
			Encounter->SetTutorialEnemyDamageLimited(true);
			StartTutorialMovement();
		}

		// 이동 / 카메라 완료 전에는 공격 페이지로 전환하지 않음
		if (TutorialStage == 0)
		{
			UE_LOG(LogDIGameFlow, Log,
				TEXT("TUTORIAL 01 | AI Active, Movement Tutorial Running"));

			return;
		}
	}

	bTutorialMovementSampling = false;
	SetActorTickEnabled(false);

	UnbindTutorialAction();

	TutorialFirstCount = 0;
	TutorialSecondCount = 0;

	ADongincheonCharacter* Player =
		Cast<ADongincheonCharacter>(
			UGameplayStatics::GetPlayerCharacter(this, 0));

	if (IsValid(Player))
	{
		TutorialActionPlayer = Player;

		Player->OnTutorialAction.AddUniqueDynamic(
			this,
			&ADIGameFlowDirector::HandleTutorialAction);
	}

	// Tutorial 01 전투 시작 -> 공격 화면
	// Tutorial 02 전투 시작 -> 방어 화면
	SetTutorialStage(CurrentTutorialIndex == 0 ? 1 : 0);

	UE_LOG(LogDIGameFlow, Log,
		TEXT("TUTORIAL COMBAT UI START | Index=%d | Stage=%d"),
		CurrentTutorialIndex,
		TutorialStage);
}

// Tutorial Gameplay Action 수신
void ADIGameFlowDirector::HandleTutorialAction(
	EDIPlayerTutorialAction Action)
{
	if (CurrentPhase != EDIGameFlowPhase::Tutorial ||
		!TutorialActionPlayer.IsValid())
	{
		return;
	}

	bool bChanged = false;

	// Tutorial 01: 기본 공격 6회, 강공격 4회
	if (CurrentTutorialIndex == 0 && TutorialStage == 1)
	{
		if (Action == EDIPlayerTutorialAction::LightHit &&
			TutorialFirstCount < 6)
		{
			++TutorialFirstCount;
			bChanged = true;
		}
		else if (Action == EDIPlayerTutorialAction::HeavyHit &&
				 TutorialSecondCount < 4)
		{
			++TutorialSecondCount;
			bChanged = true;
		}
	}
	else if (CurrentTutorialIndex == 0 && TutorialStage == 2)
	{
		if (Action == EDIPlayerTutorialAction::LightComboChained &&
			TutorialComboCount < 3)
		{
			++TutorialComboCount;
			bChanged = true;
		}
		else if (Action == EDIPlayerTutorialAction::LightToHeavyBranched &&
				 TutorialHeavyBranchCount < 2)
		{
			++TutorialHeavyBranchCount;
			bChanged = true;
		}
		else if (Action == EDIPlayerTutorialAction::GrabSucceeded &&
				 TutorialGrabCount < 2)
		{
			++TutorialGrabCount;
			bChanged = true;
		}
		else if (Action == EDIPlayerTutorialAction::GrabAttackHit &&
				 TutorialGrabCount >= 1 &&
				 TutorialGrabAttackCount < 2)
		{
			++TutorialGrabAttackCount;
			bChanged = true;
		}
	}
	// Tutorial 02: 회피 4회, 가드 성공 3회
	else if (CurrentTutorialIndex == 1 && TutorialStage == 0)
	{
		if (Action == EDIPlayerTutorialAction::DodgePerformed &&
			TutorialFirstCount < 4)
		{
			++TutorialFirstCount;
			bChanged = true;
		}
		else if (Action == EDIPlayerTutorialAction::GuardBlocked &&
				 TutorialSecondCount < 3)
		{
			++TutorialSecondCount;
			bChanged = true;
		}
	}
	
	// Tutorial 02: 록온 및 마우스 휠 대상 변경
	else if (CurrentTutorialIndex == 1 && TutorialStage == 1)
	{
		if (Action == EDIPlayerTutorialAction::LockOnAcquired &&
			TutorialLockOnCount < 1)
		{
			TutorialLockOnCount = 1;
			bChanged = true;
		}
		else if (Action == EDIPlayerTutorialAction::LockOnSwitched &&
				 TutorialLockOnCount >= 1 &&
				 TutorialSwitchCount < 1)
		{
			TutorialSwitchCount = 1;
			bChanged = true;
		}
	}
	
	if (!bChanged)
	{
		return;
	}

	UE_LOG(LogDIGameFlow, Log,
		TEXT("TUTORIAL ACTION | Index=%d | Stage=%d | "
			 "Action=%d | Count=%d,%d"),
		CurrentTutorialIndex,
		TutorialStage,
		static_cast<int32>(Action),
		TutorialFirstCount,
		TutorialSecondCount);

	RefreshTutorialProgress();
	TryFinishComboGrabTutorial();
	TryFinishSpecialTutorial();

	if (CurrentTutorialIndex == 0 &&
		TutorialStage == 1 &&
		TutorialFirstCount >= 6 &&
		TutorialSecondCount >= 4)
	{
		SetTutorialStage(2);
	}
	else if (CurrentTutorialIndex == 1 &&
			 TutorialStage == 0 &&
			 TutorialFirstCount >= 4 &&
			 TutorialSecondCount >= 3)
	{
		SetTutorialStage(1);
	}
}

void ADIGameFlowDirector::HandleTutorialHeatActionEnded(
	EDIHeatActionPresentationEndReason EndReason,
	AActor* Target,
	FGameplayTag ActionTag)
{
	(void)Target;
	(void)ActionTag;

	if (CurrentPhase != EDIGameFlowPhase::Tutorial ||
		CurrentTutorialIndex != 1 ||
		TutorialStage != 1 ||
		EndReason != EDIHeatActionPresentationEndReason::Completed)
	{
		return;
	}

	if (TutorialHeatCount < 1)
	{
		TutorialHeatCount = 1;
		RefreshTutorialProgress();
	}

	TryFinishSpecialTutorial();
}

void ADIGameFlowDirector::TryFinishSpecialTutorial()
{
	if (CurrentPhase != EDIGameFlowPhase::Tutorial ||
		CurrentTutorialIndex != 1 ||
		TutorialStage != 1 ||
		bTutorialSpecialCompleted)
	{
		return;
	}

	if (TutorialLockOnCount < 1 ||
		TutorialSwitchCount < 1 ||
		TutorialHeatCount < 1)
	{
		return;
	}

	bTutorialSpecialCompleted = true;

	// Tutorial 01에서 시작한 동일 Encounter의 적
	if (TutorialEncounters.IsValidIndex(1) &&
	IsValid(TutorialEncounters[1]))
	{
		TutorialEncounters[1]
			->SetTutorialEnemyDamageLimited(false);
	}

	ShowTutorialFightInstruction();

	UE_LOG(LogDIGameFlow, Log,
		TEXT("ALL TUTORIALS COMPLETE | Real combat started"));
}

void ADIGameFlowDirector::ShowTutorialFightInstruction()
{
	ADIPlayerController* PC =
		Cast<ADIPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0));

	if (!IsValid(PC))
	{
		return;
	}

	PC->SetTutorialProgress(FText::GetEmpty());

	PC->ShowTutorialPrompt(
		NSLOCTEXT("DITutorial", "FightTitle",
			"실전 전투"),
		NSLOCTEXT("DITutorial", "FightText",
			"이제 적을 쓰러뜨려라!"));

	GetWorldTimerManager().ClearTimer(TutorialFightPromptTimer);

	GetWorldTimerManager().SetTimer(
		TutorialFightPromptTimer,
		this,
		&ADIGameFlowDirector::HideTutorialFightInstruction,
		2.5f,
		false);
}

void ADIGameFlowDirector::HideTutorialFightInstruction()
{
	if (CurrentPhase != EDIGameFlowPhase::Tutorial ||
		!bTutorialSpecialCompleted)
	{
		return;
	}

	if (ADIPlayerController* PC =
		Cast<ADIPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->HideTutorialPrompt();
		PC->SetTutorialProgress(FText::GetEmpty());
	}
}

void ADIGameFlowDirector::SetTutorialStage(int32 NewStage)
{
    if (TutorialStage == NewStage)
    {
        return;
    }

    TutorialStage = NewStage;
	
	// Tutorial 01: 콤보 / 잡기 연습 시작
	// 네 가지 완료 조건을 달성할 때까지 0.01 데미지 유지
	if (CurrentTutorialIndex == 0 && TutorialStage == 2)
	{
		TutorialComboCount = 0;
		TutorialHeavyBranchCount = 0;
		TutorialGrabCount = 0;
		TutorialGrabAttackCount = 0;
		bTutorialComboGrabCompleted = false;
	}

    // Tutorial 02 특수 액션 페이지 진입
    if (CurrentTutorialIndex == 1 && TutorialStage == 1)
    {
    	TutorialLockOnCount = 0;
    	TutorialSwitchCount = 0;
    	TutorialHeatCount = 0;
    	bTutorialSpecialCompleted = false;
    	
        if (ADongincheonCharacter* Player =
            Cast<ADongincheonCharacter>(
                UGameplayStatics::GetPlayerCharacter(this, 0)))
        {
        	if (UTargetingComponent* Targeting =
	Player->FindComponentByClass<UTargetingComponent>())
        	{
        		// 방어 연습 중 이미 록온한 상태라면 인정
        		if (Targeting->IsLockedOn() &&
					IsValid(Targeting->GetLockOnTarget()))
        		{
        			TutorialLockOnCount = 1;
        		}
        	}
        	
            Player->SetTutorialInfiniteHeatEnabled(true);
        	Player->OnHeatActionPresentationEnded.AddUniqueDynamic(
	this,
	&ADIGameFlowDirector::HandleTutorialHeatActionEnded);
        }

        UE_LOG(LogDIGameFlow, Log,
            TEXT("TUTORIAL INFINITE HEAT | ON"));
    }

    ShowTutorialPage();
    RefreshTutorialProgress();

    UE_LOG(LogDIGameFlow, Log,
        TEXT("TUTORIAL PAGE SHOW | Index=%d | Stage=%d"),
        CurrentTutorialIndex,
        TutorialStage);
}

void ADIGameFlowDirector::TryFinishComboGrabTutorial()
{
	if (CurrentPhase != EDIGameFlowPhase::Tutorial ||
		CurrentTutorialIndex != 0 ||
		TutorialStage != 2 ||
		bTutorialComboGrabCompleted)
	{
		return;
	}

	if (TutorialComboCount < 3 ||
		TutorialHeavyBranchCount < 2 ||
		TutorialGrabCount < 2 ||
		TutorialGrabAttackCount < 2)
	{
		return;
	}

	bTutorialComboGrabCompleted = true;

	// Tutorial 01의 2명에게 정상 데미지 복구
	if (TutorialEncounters.IsValidIndex(0) &&
		IsValid(TutorialEncounters[0]))
	{
		TutorialEncounters[0]
			->SetTutorialEnemyDamageLimited(false);
	}

	// Tutorial 01 연습 UI만 종료.
	// 적 2명을 처치해야 Tutorial 02로 넘어간다.
	if (ADIPlayerController* PC =
		Cast<ADIPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->SetTutorialProgress(FText::GetEmpty());
		PC->HideTutorialPrompt();
	}

	UE_LOG(LogDIGameFlow, Log,
		TEXT("TUTORIAL 01 TRAINING COMPLETE | Defeat 2 enemies"));
}

void ADIGameFlowDirector::ShowTutorialPage()
{
    ADIPlayerController* PC =
        Cast<ADIPlayerController>(
            UGameplayStatics::GetPlayerController(this, 0));

    if (!IsValid(PC))
    {
        return;
    }

    PC->SetHUDContext(EDIHUDContext::Gameplay);

    UDIHUDRootWidget* HUD = PC->GetHUDRootWidget();

    if (!IsValid(HUD))
    {
        return;
    }

    if (CurrentTutorialIndex == 0)
    {
        switch (TutorialStage)
        {
        // Tutorial 01-0: 이동 / 카메라
        case 0:
            HUD->ShowTutorialPromptRows(
                NSLOCTEXT("DITutorial", "T01MoveTitle",
                    "이동과 카메라"),
                FName(TEXT("WASD")),
                NSLOCTEXT("DITutorial", "T01MoveRow1",
                    "캐릭터 이동"),
                FName(TEXT("Mouse")),
                NSLOCTEXT("DITutorial", "T01MoveRow2",
                    "카메라 회전"));
            break;

        // Tutorial 01-1: 기본 / 강공격
        case 1:
            HUD->ShowTutorialPromptRows(
                NSLOCTEXT("DITutorial", "T01AttackTitle",
                    "기본 전투"),
                FName(TEXT("LMB")),
                NSLOCTEXT("DITutorial", "T01AttackRow1",
                    "기본 공격"),
                FName(TEXT("RMB")),
                NSLOCTEXT("DITutorial", "T01AttackRow2",
                    "강공격"));
            break;

        // Tutorial 01-2: 콤보 / 잡기
        case 2:
            HUD->ShowTutorialPromptRows(
                NSLOCTEXT("DITutorial", "T01ComboTitle",
                    "콤보와 잡기"),
                FName(TEXT("LMB")),
                NSLOCTEXT("DITutorial", "T01ComboRow1",
                    "연속 입력: 기본 콤보\n"
                    "기본 공격 후 우클릭: 강공격 연계"),
                FName(TEXT("Q")),
                NSLOCTEXT("DITutorial", "T01ComboRow2",
                    "적 잡기\n"
                    "잡은 상태에서 좌클릭: 잡기 공격"));
            break;
        }
    }
    else if (CurrentTutorialIndex == 1)
    {
        switch (TutorialStage)
        {
        // Tutorial 02-0: 회피 / 가드
        case 0:
            HUD->ShowTutorialPromptRows(
                NSLOCTEXT("DITutorial", "T02DefenseTitle",
                    "방어"),
                FName(TEXT("E")),
                NSLOCTEXT("DITutorial", "T02DefenseRow1",
                    "회피"),
                FName(TEXT("Shift")),
                NSLOCTEXT("DITutorial", "T02DefenseRow2",
                    "누르고 유지: 가드"));
            break;

        // Tutorial 02-1: 록온 / 히트 액션
        case 1:
            HUD->ShowTutorialPromptRows(
                NSLOCTEXT("DITutorial", "T02HeatTitle",
                    "특수 액션"),
                FName(TEXT("Space")),
                NSLOCTEXT("DITutorial", "T02HeatRow1",
                    "누르고 유지: 록온\n"
                    "유지 중 마우스 휠: 대상 변경"),
                FName(TEXT("RMB")),
                NSLOCTEXT("DITutorial", "T02HeatRow2",
                    "히트 액션 (사용 가능할 때)"));
            break;
        }
    }
}

// Tutorial 진행도 UI 갱신
void ADIGameFlowDirector::RefreshTutorialProgress()
{
	if (CurrentPhase != EDIGameFlowPhase::Tutorial)
	{
		return;
	}

	ADIPlayerController* PC =
		Cast<ADIPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0));

	if (!IsValid(PC))
	{
		return;
	}

	FText Progress = FText::GetEmpty();

	if (CurrentTutorialIndex == 0)
	{
		if (TutorialStage == 0)
		{
			Progress = FText::Format(
				NSLOCTEXT("DITutorial", "MoveProgress",
					"이동 {0}/{1}cm · 카메라 {2}/{3}°"),
				FText::AsNumber(FMath::RoundToInt(TutorialMovedCm)),
				FText::AsNumber(
					FMath::RoundToInt(TutorialRequiredMoveCm)),
				FText::AsNumber(
					FMath::RoundToInt(TutorialCameraDegrees)),
				FText::AsNumber(
					FMath::RoundToInt(TutorialRequiredCameraDegrees)));
		}
		else if (TutorialStage == 1)
		{
			Progress = FText::Format(
				NSLOCTEXT("DITutorial", "AttackProgress",
					"기본 {0}/6 · 강공격 {1}/4"),
				FText::AsNumber(TutorialFirstCount),
				FText::AsNumber(TutorialSecondCount));
		}
		else if (TutorialStage == 2)
		{
			Progress = FText::Format(
				NSLOCTEXT("DITutorial", "ComboGrabProgress",
					"연타 {0}/3 · 분기 {1}/2\n"
					"잡기 {2}/2 · 잡기 공격 {3}/2"),
				FText::AsNumber(TutorialComboCount),
				FText::AsNumber(TutorialHeavyBranchCount),
				FText::AsNumber(TutorialGrabCount),
				FText::AsNumber(TutorialGrabAttackCount));
		}
	}
	else if (CurrentTutorialIndex == 1 &&
			 TutorialStage == 0)
	{
		Progress = FText::Format(
			NSLOCTEXT("DITutorial", "DefenseProgress",
				"회피 {0}/4 · 가드 {1}/3"),
			FText::AsNumber(TutorialFirstCount),
			FText::AsNumber(TutorialSecondCount));
	}
	else if (CurrentTutorialIndex == 1 &&
		 TutorialStage == 1)
	{
		Progress = FText::Format(
			NSLOCTEXT("DITutorial", "SpecialProgress",
				"록온 {0}/1 · 전환 {1}/1 · 히트 {2}/1"),
			FText::AsNumber(TutorialLockOnCount),
			FText::AsNumber(TutorialSwitchCount),
			FText::AsNumber(TutorialHeatCount));
	}

	PC->SetTutorialProgress(Progress);
}


// Tutorial 이벤트 구독 해제
void ADIGameFlowDirector::UnbindTutorialAction()
{
    if (ADongincheonCharacter* Player =
        TutorialActionPlayer.Get())
    {
        Player->OnTutorialAction.RemoveDynamic(
            this,
            &ADIGameFlowDirector::HandleTutorialAction
        );
    }

    TutorialActionPlayer.Reset();
}

void ADIGameFlowDirector::CleanupTutorial()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TutorialFightPromptTimer);
	}
	
	bTutorialMovementSampling = false;
	SetActorTickEnabled(false);

	ADongincheonCharacter* Player =
		TutorialActionPlayer.Get();

	if (!IsValid(Player))
	{
		Player = Cast<ADongincheonCharacter>(
			UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	if (IsValid(Player))
	{
		Player->OnHeatActionPresentationEnded.RemoveDynamic(
	this,
	&ADIGameFlowDirector::HandleTutorialHeatActionEnded);
		Player->SetTutorialInfiniteHeatEnabled(false);
	}

	if (TutorialEncounters.IsValidIndex(CurrentTutorialIndex) &&
	IsValid(TutorialEncounters[CurrentTutorialIndex]))
	{
		TutorialEncounters[CurrentTutorialIndex]
			->SetTutorialEnemyDamageLimited(false);
	}
	
	UnbindTutorialAction();

	if (ADIPlayerController* PC =
		Cast<ADIPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0)))
	{
		PC->SetTutorialProgress(FText::GetEmpty());
		PC->HideTutorialPrompt();
	}

	TutorialStage = INDEX_NONE;
	TutorialFirstCount = 0;
	TutorialSecondCount = 0;
	TutorialComboCount = 0;
	TutorialHeavyBranchCount = 0;
	TutorialGrabCount = 0;
	TutorialGrabAttackCount = 0;
	bTutorialComboGrabCompleted = false;
	TutorialLockOnCount = 0;
	TutorialSwitchCount = 0;
	TutorialHeatCount = 0;
	bTutorialSpecialCompleted = false;
	TutorialMovedCm = 0.0f;
	TutorialCameraDegrees = 0.0f;
	TutorialUIRefreshElapsed = 0.0f;

	UE_LOG(LogDIGameFlow, Log,
		TEXT("TUTORIAL CLEANUP | Infinite Heat OFF"));
}

void ADIGameFlowDirector::HandleEncounterCompleted(ADIBattleEncounter* Encounter)
{
	if (!IsValid(Encounter))
	{
		return;
	}

	switch (CurrentPhase)
	{
	case EDIGameFlowPhase::Tutorial:
		{
			if (!TutorialEncounters.IsValidIndex(CurrentTutorialIndex) ||
				TutorialEncounters[CurrentTutorialIndex] != Encounter)
			{
				return;
			}

			// Tutorial 01: 2명 처치 완료
			if (CurrentTutorialIndex == 0)
			{
				if (!bTutorialComboGrabCompleted)
				{
					UE_LOG(LogDIGameFlow, Error,
						TEXT("TUTORIAL 01 | Enemies defeated before training complete"));
					return;
				}

				// 기존 Tutorial 01 UI / 이벤트 정리
				CleanupTutorial();

				CurrentTutorialIndex = 1;

				if (!TutorialEncounters.IsValidIndex(1) ||
					!IsValid(TutorialEncounters[1]))
				{
					UE_LOG(LogDIGameFlow, Error,
						TEXT("TUTORIAL 02 Encounter missing! Check TutorialEncounters[1]"));
					return;
				}

				// Tutorial 01 완료 BP 이벤트 종료 후
				// 다음 프레임에 Tutorial 02 직접 시작
				GetWorldTimerManager().SetTimerForNextTick(
					this,
					&ADIGameFlowDirector::StartCurrentTutorialEncounter);

				UE_LOG(LogDIGameFlow, Log,
					TEXT("TUTORIAL 01 COMPLETE | Starting Tutorial 02 next tick"));

				break;
			}

			// Tutorial 02: 4명 처치 완료
			if (CurrentTutorialIndex == 1)
			{
				if (!bTutorialSpecialCompleted)
				{
					UE_LOG(LogDIGameFlow, Error,
						TEXT("TUTORIAL 02 | Enemies defeated before training complete"));
					return;
				}

				// SetPhase 내부에서 CleanupTutorial 실행
				SetPhase(EDIGameFlowPhase::MainEncounter);

				break;
			}

			break;
		}

	case EDIGameFlowPhase::MainEncounter:
		{
			if (Encounter == MainEncounter)
			{
				SetPhase(
					EDIGameFlowPhase::BossEncounter);
			}

			break;
		}

	case EDIGameFlowPhase::BossEncounter:
		{
			if (Encounter == BossEncounter)
			{
				SetPhase(
					EDIGameFlowPhase::Ending);
			}

			break;
		}

	default:
		break;
	}
}

void ADIGameFlowDirector::NotifyEndingFinished()
{
	if (CurrentPhase != EDIGameFlowPhase::Ending)
	{
		return;
	}

	SetPhase(EDIGameFlowPhase::FreeRoam);
}