#include "Gameplay/Flow/DIGameFlowDirector.h"

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

void ADIGameFlowDirector::BindEncounter(
	ADIBattleEncounter* Encounter)
{
	if (!IsValid(Encounter))
	{
		return;
	}

	Encounter->OnEncounterCompletedDelegate.AddUniqueDynamic(
		this,
		&ADIGameFlowDirector::HandleEncounterCompleted);
}

void ADIGameFlowDirector::DisableEncounterTrigger(
	ADIBattleEncounter* Encounter)
{
	if (!IsValid(Encounter))
	{
		return;
	}

	Encounter->SetEncounterTriggerEnabled(false);
}

void ADIGameFlowDirector::SetPhase(
	EDIGameFlowPhase NewPhase)
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
		OnEndingRequested();
		break;

	case EDIGameFlowPhase::FreeRoam:
		OnFreeRoamStarted();
		break;

	case EDIGameFlowPhase::None:
	default:
		break;
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

	if (!TutorialEncounters.IsEmpty())
	{
		SetPhase(
			EDIGameFlowPhase::Tutorial);
	}
	else
	{
		SetPhase(
			EDIGameFlowPhase::MainEncounter);
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

void ADIGameFlowDirector::HandleEncounterCompleted(
	ADIBattleEncounter* Encounter)
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

			if (TutorialEncounters[CurrentTutorialIndex] !=
				Encounter)
			{
				return;
			}

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