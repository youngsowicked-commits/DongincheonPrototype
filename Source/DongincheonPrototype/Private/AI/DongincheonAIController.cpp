// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/DongincheonAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "NativeGameplayTags.h"
#include "StateTreeEvents.h"
#include "Kismet/GameplayStatics.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_HitReact,"StateTreeEvent.Enemy.HitReact");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_Dead,"StateTreeEvent.Enemy.Dead");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_Presentation_Begin,"StateTreeEvent.Enemy.Presentation.Begin");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_Presentation_End,"StateTreeEvent.Enemy.Presentation.End");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_GuardBroken, "StateTreeEvent.Enemy.GuardBroken");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_Grabbed_Begin,"StateTreeEvent.Enemy.Grabbed.Begin");

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_Grabbed_End,"StateTreeEvent.Enemy.Grabbed.End");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_HeatActionVictim_Begin,"StateTreeEvent.Enemy.HeatActionVictim.Begin");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StateTreeEvent_Enemy_HeatActionVictim_End,"StateTreeEvent.Enemy.HeatActionVictim.End");


ADongincheonAIController::ADongincheonAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	
	BrainComponent = StateTreeComponent;
	
	//중요: 동적 Spawn에서는 BeginPlay 시점에 아직 Pawn을 Possess하지 않았을수 있으므로 StateTree 자동 시작을 막는다.
	//실제 시작은 OnPossess에서 한다.
	StateTreeComponent->SetStartLogicAutomatically(false);
}

void ADongincheonAIController::SendHitReactEvent()
{
	if (!StateTreeComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("01B: StateTreeComponent NULL"));

		return;
	}
	
	const EStateTreeRunStatus RunStatus =
	StateTreeComponent->GetStateTreeRunStatus();

	const TCHAR* StatusText = TEXT("Unknown");

	switch (RunStatus)
	{
	case EStateTreeRunStatus::Running:
		StatusText = TEXT("Running");
		break;

	case EStateTreeRunStatus::Stopped:
		StatusText = TEXT("Stopped");
		break;

	case EStateTreeRunStatus::Succeeded:
		StatusText = TEXT("Succeeded");
		break;

	case EStateTreeRunStatus::Failed:
		StatusText = TEXT("Failed");
		break;

	case EStateTreeRunStatus::Unset:
		StatusText = TEXT("Unset");
		break;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B: HitReact Event - StateTree Status = %s"),
		StatusText);

	StateTreeComponent->SendStateTreeEvent(
		FStateTreeEvent(TAG_StateTreeEvent_Enemy_HitReact));

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B: HitReact Event SENT"));
}

void ADongincheonAIController::SendDeadEvent()
{
	if (!StateTreeComponent)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("01B DEATH: StateTreeComponent NULL"));
		
		return;
	}
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B DEATH: SendDeadEvent | Status=%d"),
		static_cast<int32>(StateTreeComponent->GetStateTreeRunStatus()));
	
	StateTreeComponent->SendStateTreeEvent(FStateTreeEvent(TAG_StateTreeEvent_Enemy_Dead));
	
	UE_LOG(
	   LogTemp,
	   Warning,
	   TEXT("01B DEATH: Dead Event SENT"));
}

void ADongincheonAIController::SendGuardBrokenEvent()
{
	if (!StateTreeComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("01B: StateTreeComponent NULL"));

		return;
	}
	
	const EStateTreeRunStatus RunStatus =
	StateTreeComponent->GetStateTreeRunStatus();

	const TCHAR* StatusText = TEXT("Unknown");

	switch (RunStatus)
	{
	case EStateTreeRunStatus::Running:
		StatusText = TEXT("Running");
		break;

	case EStateTreeRunStatus::Stopped:
		StatusText = TEXT("Stopped");
		break;

	case EStateTreeRunStatus::Succeeded:
		StatusText = TEXT("Succeeded");
		break;

	case EStateTreeRunStatus::Failed:
		StatusText = TEXT("Failed");
		break;

	case EStateTreeRunStatus::Unset:
		StatusText = TEXT("Unset");
		break;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B: GuardBreak - StateTree Status = %s"),
		StatusText);

	StateTreeComponent->SendStateTreeEvent(
		FStateTreeEvent(TAG_StateTreeEvent_Enemy_GuardBroken));

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B: GuardBreak Event SENT"));
}

void ADongincheonAIController::SendGrabbedEvent()
{
	if (!StateTreeComponent)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("01B GRAB: StateTreeComponent NULL"));

		return;
	}

	StateTreeComponent->SendStateTreeEvent(
		FStateTreeEvent(TAG_StateTreeEvent_Enemy_Grabbed_Begin));

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B GRAB: Grabbed BEGIN Event SENT | Pawn=%s"),
		*GetNameSafe(GetPawn()));
}

void ADongincheonAIController::SendGrabReleasedEvent()
{
	if (!StateTreeComponent)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("01B GRAB: StateTreeComponent NULL"));

		return;
	}

	StateTreeComponent->SendStateTreeEvent(FStateTreeEvent(TAG_StateTreeEvent_Enemy_Grabbed_End));

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B GRAB: Grabbed END Event SENT | Pawn=%s"),
		*GetNameSafe(GetPawn()));
}

void ADongincheonAIController::SendHeatActionVictimEvent()
{
	if (!StateTreeComponent)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("01B HEAT ACTION: StateTreeComponent NULL"));

		return;
	}

	StateTreeComponent->SendStateTreeEvent(
		FStateTreeEvent(TAG_StateTreeEvent_Enemy_HeatActionVictim_Begin));

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B HEAT ACTION: Victim BEGIN Event SENT | Pawn=%s"),
		*GetNameSafe(GetPawn()));
}

void ADongincheonAIController::SendHeatActionVictimReleasedEvent()
{
	if (!StateTreeComponent)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("01B HEAT ACTION: StateTreeComponent NULL"));

		return;
	}

	StateTreeComponent->SendStateTreeEvent(
		FStateTreeEvent(TAG_StateTreeEvent_Enemy_HeatActionVictim_End));

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B HEAT ACTION: Victim END Event SENT | Pawn=%s"),
		*GetNameSafe(GetPawn()));
}

void ADongincheonAIController::MarkGuardUsed()
{
	const UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	LastGuardTime = World->GetTimeSeconds();
}

bool ADongincheonAIController::CanUseGuard(float CooldownDuration) const
{
	if (CooldownDuration <= 0.0f)
	{
		return true;
	}

	if (LastGuardTime < 0.0)
	{
		return true;
	}

	const UWorld* World = GetWorld();

	if (!IsValid(World))
	{
		return false;
	}

	const double CurrentTime = World->GetTimeSeconds();
	const double ElapsedTime = CurrentTime - LastGuardTime;

	return ElapsedTime >= static_cast<double>(CooldownDuration);
}


void ADongincheonAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	CombatTarget.Reset();
	LastGuardTime = -1.0;
	
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp,Error,TEXT("01B : OnPossess failed - Pawn is invalid"));
		
		return;
	}
	
	if (!StateTreeComponent)
	{
		UE_LOG(LogTemp,Error,TEXT("01B: OnPossess failed - StateTreeComponent i NULL"));
		
		return;
	}
	
	UE_LOG(LogTemp,Warning,TEXT("01B: OnPossess | Controller=%s | Pawn=%s"), *GetNameSafe(this), *GetNameSafe(InPawn));
	
	if (const ADongincheonEnemyBase* Enemy = Cast<ADongincheonEnemyBase>(InPawn))
	{
		if (!Enemy->ShouldAutoStartStateTreeOnPossess())
		{
			UE_LOG(LogTemp,Warning,TEXT("01B: StateTree start DEFERRED | Pawn=%s"), *GetNameSafe(InPawn));
			
			return;
		}
	}
	
	StateTreeComponent->StartLogic();
}

void ADongincheonAIController::StartStateTreeLogic()
{
	if (!StateTreeComponent)
	{
		UE_LOG(LogTemp,Error,TEXT("01B: StartStateTreeLogic failed - StateTreeComponent is NULL"));
		
		return;
	}
	
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	if (IsValid(PlayerPawn))
	{
		CombatTarget = PlayerPawn;
		
		SetFocus(PlayerPawn);

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("01B: Combat Focus SET | Pawn=%s | Target=%s"),
			*GetNameSafe(GetPawn()),
			*GetNameSafe(CombatTarget.Get()));
	}
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B MANUAL START BEFORE | Pawn=%s | IsRunning=%s | Status=%d"),
		*GetNameSafe(GetPawn()),
		StateTreeComponent->IsRunning() ? TEXT("TRUE") : TEXT("FALSE"),
		static_cast<int32>(StateTreeComponent->GetStateTreeRunStatus()));

	StateTreeComponent->StartLogic();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("01B MANUAL START AFTER | Pawn=%s | IsRunning=%s | Status=%d"),
		*GetNameSafe(GetPawn()),
		StateTreeComponent->IsRunning() ? TEXT("TRUE") : TEXT("FALSE"),
		static_cast<int32>(StateTreeComponent->GetStateTreeRunStatus()));
	/*
	if (StateTreeComponent->IsRunning())
	{
		return;
	}
	*/
	//UE_LOG(LogTemp,Warning,TEXT("01B: StateTree StartLogic called | Pawn=%s"), *GetNameSafe(GetPawn()));
}

void ADongincheonAIController::EnterPresentationState()
{
	if (!StateTreeComponent || !StateTreeComponent->IsRunning())
	{
		return;
	}
	
	StateTreeComponent->SendStateTreeEvent(FStateTreeEvent(TAG_StateTreeEvent_Enemy_Presentation_Begin));
	
	UE_LOG(LogTemp,Warning,TEXT("01B: Presentation BEGIN event set | Pawn=%s"), *GetNameSafe(GetPawn()));
}

void ADongincheonAIController::ExitPresentationState()
{
	if (!StateTreeComponent || !StateTreeComponent->IsRunning())
	{
		return;
	}
	
	StateTreeComponent->SendStateTreeEvent(FStateTreeEvent(TAG_StateTreeEvent_Enemy_Presentation_End));
	
	UE_LOG(LogTemp,Warning,TEXT("01B: Presentation END event sent | Pawn=%s"), *GetNameSafe(GetPawn()));
}

