#include "Gameplay/Flow/DIGameFlowDirector.h"

#include "Character/DongincheonCharacter.h"
#include "Components/InteractionComponent.h"
#include "Player/DIPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Gameplay/Battle/DIBattleEncounter.h"

DEFINE_LOG_CATEGORY_STATIC(LogDIGameFlow, Log, All);

ADIGameFlowDirector::ADIGameFlowDirector()
{
	PrimaryActorTick.bCanEverTick = false;
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

	const EDIGameFlowPhase PreviousPhase =
		CurrentPhase;

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
			// Main Encounter가 없으면 Boss로 fail-safe
			SetPhase(
				EDIGameFlowPhase::BossEncounter);
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

	Encounter->SetEncounterTriggerEnabled(true);

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
    // Tutorial 단계에서만 작동
    if (CurrentPhase != EDIGameFlowPhase::Tutorial)
    {
        return;
    }

    if (!IsValid(Encounter))
    {
        return;
    }

    // 현재 Tutorial 배열 인덱스 확인
    if (!TutorialEncounters.IsValidIndex(CurrentTutorialIndex))
    {
        return;
    }

    // 실제 현재 Tutorial Encounter인지 검사
    if (TutorialEncounters[CurrentTutorialIndex] != Encounter)
    {
        return;
    }

    // 기존 PlayerController 가져오기
    ADIPlayerController* PC =
        Cast<ADIPlayerController>(
            UGameplayStatics::GetPlayerController(this, 0));

    if (!IsValid(PC))
    {
        UE_LOG(
            LogDIGameFlow,
            Warning,
            TEXT("TUTORIAL UI | PlayerController not found")
        );

        return;
    }

    // Tutorial 중에도 기존 Combat HUD 유지
    PC->SetHUDContext(EDIHUDContext::Gameplay);

    // 이전 진행 문구 초기화
    PC->SetTutorialProgress(FText::GetEmpty());

    switch (CurrentTutorialIndex)
    {
    case 0:
        // Tutorial 01: 기본 공격 / 콤보 / 잡기
        PC->ShowTutorialPrompt(
            NSLOCTEXT(
                "DITutorial",
                "Tutorial01Title",
                "기본 전투"
            ),
            NSLOCTEXT(
                "DITutorial",
                "Tutorial01Description",
                "LMB : 기본 공격 / 연속 입력으로 콤보\n"
                "RMB : 강공격 / LMB 후 RMB로 파생 공격\n"
                "Q : 잡기 / 적에게 접근하여 잡기 입력\n"
                "잡은 상태에서 LMB : 잡기 공격"
            )
        );
        break;

    case 1:
        // Tutorial 02: 회피 / 가드 / 록온 / 히트 액션
        PC->ShowTutorialPrompt(
            NSLOCTEXT(
                "DITutorial",
                "Tutorial02Title",
                "전투 응용"
            ),
            NSLOCTEXT(
                "DITutorial",
                "Tutorial02Description",
                "E : 회피\n"
                "Shift : 가드\n"
                "Space : 록온\n"
                "히트 액션 사용 가능 시 RMB"
            )
        );
        break;

    default:
        return;
    }

    UE_LOG(
        LogDIGameFlow,
        Log,
        TEXT("TUTORIAL UI SHOW | Index=%d | Encounter=%s"),
        CurrentTutorialIndex,
        *GetNameSafe(Encounter)
    );
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
			if (!TutorialEncounters.IsValidIndex(
				CurrentTutorialIndex))
			{
				return;
			}

			if (TutorialEncounters[CurrentTutorialIndex] != Encounter)
			{
				return;
			}
			
			if (ADIPlayerController* PC = Cast<ADIPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
			{
				PC->HideTutorialPrompt();
			}

			UE_LOG(
				LogDIGameFlow,
				Log,
				TEXT("TUTORIAL UI HIDE | Index=%d"),
				CurrentTutorialIndex
			);

			++CurrentTutorialIndex;

			if (TutorialEncounters.IsValidIndex(
				CurrentTutorialIndex))
			{
				StartCurrentTutorialEncounter();
			}
			else
			{
				SetPhase(
					EDIGameFlowPhase::MainEncounter);
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

	SetPhase(
		EDIGameFlowPhase::FreeRoam);
}