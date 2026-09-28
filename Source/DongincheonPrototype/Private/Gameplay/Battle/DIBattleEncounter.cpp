#include "Gameplay/Battle/DIBattleEncounter.h"

#include "Character/DongincheonCharacter.h"
#include "Character/DongincheonEnemyBase.h"
#include "Player/DIPlayerController.h"
#include "UI/DIHUDTypes.h"
#include "Components/QTEComponent.h"
#include "Components/HealthComponent.h"
#include "Components/InteractionComponent.h"
#include "AI/DongincheonAIController.h"
#include "Components/BoxComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/FastReferenceCollector.h"
#include "WorldPartition/ContentBundle/ContentBundleLog.h"

DEFINE_LOG_CATEGORY_STATIC(LogDIBattleEncounter, Log, All);

ADIBattleEncounter::ADIBattleEncounter()
{
	PrimaryActorTick.bCanEverTick = false;

	BattleTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("BattleTrigger"));

	QTEComponent = CreateDefaultSubobject<UQTEComponent>(TEXT("QTE"));

	SetRootComponent(BattleTrigger);

	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	BattleTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	BattleTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	BattleTrigger->SetGenerateOverlapEvents(true);
}

void ADIBattleEncounter::BeginPlay()
{
	Super::BeginPlay();

	BattleTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADIBattleEncounter::HandleTriggerBeginOverlap);

	if (IsValid(QTEComponent))
	{
		QTEComponent->OnQTECompleted.AddUniqueDynamic(this, &ADIBattleEncounter::HandleQTECompleted);
	}

	//게임 시작시 전투 Blocker 비활성화
	SetBattleBlockerEnabled(false);
}

void ADIBattleEncounter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Encounter가 제거되거나 World가 종료될 때 남아 있는 Presentation Input Lock을 정리한다.
	ReleasePresentationPlayerLock();

	if (IsValid(QTEComponent) && QTEComponent->IsQTEActive())
	{
		QTEComponent->CancelQTE();
	}

	if (ADIPlayerController* PlayerController = Cast<ADIPlayerController>
	(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->ClearQTESource();
	}
	
	Super::EndPlay(EndPlayReason);
}

void ADIBattleEncounter::HandleEnemyHealthChanged(float OldHealth, float NewHealth, float MaxHealth)
{
	if (bCompleted || MaxHealth <= 0.0f || NewHealth <= 0.0f)
	{
		return;
	}

	const float OldNormalized = OldHealth / MaxHealth;
	const float NewNormalized = NewHealth / MaxHealth;

	for (int32 Index = 0; Index < HealthTriggers.Num(); ++Index)
	{
		if (TriggeredHealthTriggerIndices.Contains(Index))
		{
			continue;
		}

		const FDIBattleHealthTrigger& Trigger = HealthTriggers[Index];

		if (Trigger.TriggerId.IsNone())
		{
			continue;
		}

		if (OldNormalized > Trigger.HealthThreshold && NewNormalized <= Trigger.HealthThreshold)
		{
			TriggeredHealthTriggerIndices.Add(Index);

			UE_LOG(LogDIBattleEncounter, Log, TEXT("HEALTH TRIGGER: %s | Id=%s | HP=%.1f/%.1f | Normalized=%.2f"),
			       *GetName(), *Trigger.TriggerId.ToString(), NewHealth, MaxHealth, NewNormalized);

			if (Trigger.TriggerId == MidFightTriggerId)
			{
				StartMidFightPresentation();
			}

			OnHealthTriggerActivated(Trigger.TriggerId, NewNormalized);
		}
	}
}

void ADIBattleEncounter::HandlePlayerQTEInputPressed(EQTEInputType InputType)
{
	if (!IsValid(QTEComponent) || !QTEComponent->IsQTEActive())
	{
		return;
	}

	UE_LOG(
		LogDIBattleEncounter,
		Warning,
		TEXT("QTE INPUT RECEIVED | Input=%d | Expected=%d"),
		static_cast<int32>(InputType),
		static_cast<int32>(QTEComponent->GetExpectedInput()));

	QTEComponent->SubmitInput(InputType);
}

void ADIBattleEncounter::HandleQTECompleted(FName QTEId,EQTEResult Result)
{
	// 현재 완료된 QTE가 Mid-Fight용으로 등록된 Step인지 확인한다.
	const bool bIsMidFightStep = MidFightQTEConfigs.ContainsByPredicate([QTEId](const FQTEConfig& Config)
			{return Config.QTEId == QTEId;});

	if (bMidFightPresentationActive && bIsMidFightStep)
	{
		// 같은 Step의 완료 이벤트가 두 번 들어오는 것을 막는다.
		if (ResolvedMidFightQTEStepIds.Contains(QTEId))
		{
			UE_LOG(
				LogDIBattleEncounter,
				Warning,
				TEXT("MidFight QTE RESULT IGNORED | Step already resolved | StepId=%s"),
				*QTEId.ToString());

			return;
		}

		// 이 Step은 이제 완료된 것으로 기록한다.
		ResolvedMidFightQTEStepIds.Add(QTEId);

		UE_LOG(
			LogDIBattleEncounter,
			Log,
			TEXT("MidFight QTE STEP RESOLVED | StepId=%s | Result=%d"),
			*QTEId.ToString(),
			static_cast<int32>(Result));
	}

	// 실제 Success / Fail 결과는 그대로 Blueprint Presentation에 전달한다.
	// 05는 QTEId로 Step을 식별하고 Result에 따라 연출만 분기한다.
	OnEncounterQTECompleted(QTEId, Result);
}

void ADIBattleEncounter::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                                   bool bFromSweep, const FHitResult& SweepResult)
{
	if (bStarted || bCompleted)
	{
		return;
	}

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);

	if (!IsValid(Player))
	{
		return;
	}

	if (OtherActor != Player)
	{
		return;
	}

	StartEncounter();
}

void ADIBattleEncounter::StartEncounter()
{
	if (bStarted || bCompleted)
	{
		return;
	}

	bStarted = true;
	bCompleted = false;
	bCombatStarted = false;
	bCombatPausedForPresentation = false;
	bMidFightPresentationActive = false;

	AliveCount = 0;
	SpawnedEnemies.Reset();
	TriggeredHealthTriggerIndices.Reset();

	if (ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		Player->OnQTEInputPressed.AddUniqueDynamic(this,&ADIBattleEncounter::HandlePlayerQTEInputPressed
		);

		Player->SetPresentationInputLocked(true);
	}
	
	//한번 발동했으면 다시 Trigger되지 않게 함,
	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//전투구역 봉쇄
	SetBattleBlockerEnabled(true);

	UE_LOG(LogDIBattleEncounter, Log, TEXT("Encounter START: %s"), *GetName());


	//Blocker 활성화, Combat Stance, 사운드 등, 레벨별 Presentation은 Blueprint가 담당.
	OnEncounterStarted();

	SpawnEnemies();

	//Enemy 설정 누락이나 Spwan 실패로 전투가 영원히 막히는것을 방지
	if (AliveCount <= 0)
	{
		UE_LOG(LogDIBattleEncounter, Warning, TEXT("Encounter '%s' spawned zero valid enemies"), *GetName());

		CompleteEncounter();
		return;
	}

	UE_LOG(LogDIBattleEncounter, Log, TEXT("Encounter READY: %s | Enemies %d"), *GetName(), AliveCount);

	OnEncounterReady();

	if (bAutoStartCombat)
	{
		StartCombat();
	}
}

void ADIBattleEncounter::ShowEncounterHUD()
{
	ADIPlayerController* PlayerController =
		Cast<ADIPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

	if (!IsValid(PlayerController))
	{
		return;
	}
	
	UE_LOG(
	LogDIBattleEncounter,
	Warning,
	TEXT("HUD DEBUG | Encounter=%s | HUDType=%d | SpawnedEnemies=%d"),
	*GetName(),
	static_cast<int32>(HUDType),
	SpawnedEnemies.Num()
);

	ADongincheonEnemyBase* HUDTarget = nullptr;

	for (ADongincheonEnemyBase* Enemy : SpawnedEnemies)
	{
		if (IsValid(Enemy))
		{
			HUDTarget = Enemy;
			break;
		}
	}

	if (!IsValid(HUDTarget))
	{
		return;
	}
	
	UE_LOG(
	LogDIBattleEncounter,
	Warning,
	TEXT("HUD DEBUG | Target=%s"),
	*GetNameSafe(HUDTarget)
);

	switch (HUDType)
	{
	case EDIEncounterHUDType::Enemy:
		{
			PlayerController->HideBossHUD();
			PlayerController->ShowEnemyHUD(HUDTarget);
			break;
		}

	case EDIEncounterHUDType::Boss:
		{
			PlayerController->HideEnemyHUD();
			PlayerController->ShowBossHUD(HUDTarget);
			break;
		}

	case EDIEncounterHUDType::None:
	default:
		{
			PlayerController->HideEnemyHUD();
			PlayerController->HideBossHUD();
			break;
		}
	}
}

void ADIBattleEncounter::HideEncounterHUD()
{
	ADIPlayerController* PlayerController =
		Cast<ADIPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

	if (!IsValid(PlayerController))
	{
		return;
	}

	switch (HUDType)
	{
	case EDIEncounterHUDType::Enemy:
		{
			PlayerController->HideEnemyHUD();
			break;
		}

	case EDIEncounterHUDType::Boss:
		{
			PlayerController->HideBossHUD();
			break;
		}

	case EDIEncounterHUDType::None:
	default:
		{
			PlayerController->HideEnemyHUD();
			PlayerController->HideBossHUD();
			break;
		}
	}
}

void ADIBattleEncounter::StartCombat()
{
	if (!bStarted || bCompleted || bCombatStarted)
	{
		return;
	}

	bCombatStarted = true;
	bCombatPausedForPresentation = false;
	
	if (ADongincheonCharacter* Player = Cast<ADongincheonCharacter>
	   (UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		Player->SetCombatInputEnabled(true);
		Player->SetPresentationInputLocked(false);
		
		if (UInteractionComponent* Interaction = Player->FindComponentByClass<UInteractionComponent>())
		{
			Interaction->SetInteractionEnabled(false);
		}
	}

	UE_LOG(LogDIBattleEncounter, Log, TEXT("Combat START: %s"), *GetName());
	
	ShowEncounterHUD();

	for (ADongincheonEnemyBase* Enemy : SpawnedEnemies)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		if (Enemy->HealthComponent)
		{
			Enemy->HealthComponent->SetDamageEnabled(true);
		}

		ADongincheonAIController* AIController = Cast<ADongincheonAIController>(Enemy->GetController());

		if (!IsValid(AIController))
		{
			UE_LOG(LogDIBattleEncounter, Error, TEXT("Combat START failed: Enemy '%s' has invaild AIController"),
			       *GetNameSafe(Enemy));

			continue;
		}

		AIController->StartStateTreeLogic();
	}

	OnCombatStarted();
}

void ADIBattleEncounter::PauseCombatForPresentation()
{
	if (!bCombatStarted || bCompleted || bCombatPausedForPresentation)
	{
		return;
	}

	bCombatPausedForPresentation = true;

	if (ADongincheonCharacter* Player = Cast<ADongincheonCharacter>
		(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		PresentationLockedPlayer = Player;

		Player->SetPresentationInputLocked(true);
	}

	for (ADongincheonEnemyBase* Enemy : SpawnedEnemies)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		if (Enemy->HealthComponent)
		{
			Enemy->HealthComponent->SetDamageEnabled(false);
		}

		ADongincheonAIController* AIController = Cast<ADongincheonAIController>(Enemy->GetController());

		if (!AIController)
		{
			continue;
		}

		AIController->EnterPresentationState();
	}

	UE_LOG(LogDIBattleEncounter, Log, TEXT("Combat PAUSED for presentation: %s"), *GetName());
}

void ADIBattleEncounter::ReleasePresentationPlayerLock()
{
	// Presentation 시작 때 실제로 잠갔던 동일 Player만 해제한다.
	if (ADongincheonCharacter* Player = PresentationLockedPlayer.Get())
	{
		Player->SetPresentationInputLocked(false);
	}

	// 다음 Presentation에서 이전 Player reference를 재사용하지 않도록 정리한다.
	PresentationLockedPlayer.Reset();
}

void ADIBattleEncounter::ResumeCombatFromPresentation()
{
	if (!bCombatStarted || bCompleted || !bCombatPausedForPresentation)
	{
		return;
	}

	bCombatPausedForPresentation = false;

	for (ADongincheonEnemyBase* Enemy : SpawnedEnemies)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		ADongincheonAIController* AIController = Cast<ADongincheonAIController>(Enemy->GetController());

		if (!AIController)
		{
			continue;
		}

		AIController->ExitPresentationState();

		if (Enemy->HealthComponent)
		{
			Enemy->HealthComponent->SetDamageEnabled(true);
		}
	}

	ReleasePresentationPlayerLock();
	
	if (ADIPlayerController* PlayerController = Cast<ADIPlayerController>
	(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->ClearQTESource();
		PlayerController->SetHUDContext(EDIHUDContext::Gameplay);
	}

	UE_LOG(LogDIBattleEncounter, Log, TEXT("Combat RESUMED from presentation: %s"), *GetName());
}

void ADIBattleEncounter::StartMidFightPresentation()
{
	if (!bCombatStarted || bCompleted || bMidFightPresentationActive)
	{
		return;
	}

	bMidFightPresentationActive = true;
	
	// 새 Mid-Fight Presentation이 시작될 때
	// 이전 실행에서 완료된 QTE Step 기록을 모두 비운다.
	ResolvedMidFightQTEStepIds.Reset();

	PauseCombatForPresentation();

	if (!bCombatPausedForPresentation)
	{
		bMidFightPresentationActive = false;
		return;
	}

	if (ADIPlayerController* PlayerController = Cast<ADIPlayerController>
		(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->SetHUDContext(EDIHUDContext::QTE);
	}
	
	UE_LOG(
		LogDIBattleEncounter,
		Log,
		TEXT("MidFight Presentation START: %s"),
		*GetName());

	OnMidFightPresentationStarted();
}

bool ADIBattleEncounter::StartMidFightQTE(FName StepId)
{
    UE_LOG(
        LogDIBattleEncounter,
        Warning,
        TEXT("StartMidFightQTE CALLED | PresentationActive=%s | StepId=%s"),
        bMidFightPresentationActive ? TEXT("TRUE") : TEXT("FALSE"),
        *StepId.ToString());

    // Mid-Fight Presentation 중에만 Step QTE를 시작할 수 있다.
    if (!bMidFightPresentationActive)
    {
        return false;
    }

    // None은 유효한 Step 식별자가 아니다.
    if (StepId.IsNone())
    {
        UE_LOG(
            LogDIBattleEncounter,
            Error,
            TEXT("StartMidFightQTE FAILED | StepId is None"));

        return false;
    }

    // 이미 완료된 Step을 다시 실행하지 않는다.
    if (ResolvedMidFightQTEStepIds.Contains(StepId))
    {
        UE_LOG(
            LogDIBattleEncounter,
            Warning,
            TEXT("StartMidFightQTE BLOCKED | Step already resolved | StepId=%s"),
            *StepId.ToString());

        return false;
    }

    // 현재 실행 중인 QTE가 있다면 새 Step을 겹쳐서 시작하지 않는다.
    if (IsValid(QTEComponent) && QTEComponent->IsQTEActive())
    {
        UE_LOG(
            LogDIBattleEncounter,
            Warning,
            TEXT("StartMidFightQTE BLOCKED | Another QTE is already active | StepId=%s"),
            *StepId.ToString());

        return false;
    }

    // QTEId가 StepId와 일치하는 Config를 찾는다.
    const FQTEConfig* FoundConfig = MidFightQTEConfigs.FindByPredicate([StepId](const FQTEConfig& Config)
        {
            return Config.QTEId == StepId;
        });

    if (!FoundConfig)
    {
        UE_LOG(
            LogDIBattleEncounter,
            Error,
            TEXT("StartMidFightQTE FAILED | Config not found | StepId=%s"),
            *StepId.ToString());

        return false;
    }

    if (FoundConfig->Steps.IsEmpty())
    {
        UE_LOG(
            LogDIBattleEncounter,
            Error,
            TEXT("StartMidFightQTE FAILED | Config has no Steps | StepId=%s"),
            *StepId.ToString());

        return false;
    }

    const bool bStartedQTE = StartEncounterQTE(*FoundConfig);

    UE_LOG(
        LogDIBattleEncounter,
        Warning,
        TEXT("StartMidFightQTE RESULT | StepId=%s | Started=%s"),
        *StepId.ToString(),
        bStartedQTE ? TEXT("TRUE") : TEXT("FALSE"));

    return bStartedQTE;
}


void ADIBattleEncounter::FinishMidFightPresentation()
{
	if (!bMidFightPresentationActive)
	{
		return;
	}

	// 05가 전체 Mid-Fight 연출 완료를 통보했는데
	// 아직 QTE가 살아 있다면 안전하게 종료한다.
	if (IsValid(QTEComponent) && QTEComponent->IsQTEActive())
	{
		QTEComponent->CancelQTE();
	}

	bMidFightPresentationActive = false;

	UE_LOG(
		LogDIBattleEncounter,
		Log,
		TEXT("MidFight Presentation FINISH: %s | ResolvedSteps=%d"),
		*GetName(),
		ResolvedMidFightQTEStepIds.Num());
	
	// Mid-Fight 전체 연출이 끝난 뒤부터 Boss Part 2 공격 패턴을 사용한다.
	// Phase 전환을 먼저 완료한 뒤 AI Presentation 상태를 해제한다.
	for (ADongincheonEnemyBase* Enemy : SpawnedEnemies)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		Enemy->SetPhase2Active(true);
	}

	// 실제 Combat 복귀는 Mid-Fight 전체 연출이 끝난 이 시점에서만 수행한다.
	ResumeCombatFromPresentation();
}

bool ADIBattleEncounter::StartEncounterQTE(const FQTEConfig& Config)
{
	if (!IsValid(QTEComponent))
	{
		return false;
	}

	if (!bMidFightPresentationActive)
	{
		return false;
	}

	const bool bQTEStarted =
		QTEComponent->StartQTE(Config);

	if (!bQTEStarted)
	{
		return false;
	}

	if (ADIPlayerController* PlayerController = Cast<ADIPlayerController>
	(UGameplayStatics::GetPlayerController(this, 0)))
	{
		PlayerController->SetQTESource(QTEComponent);
		PlayerController->SetHUDContext(EDIHUDContext::QTE);
	}

	return true;
}

void ADIBattleEncounter::FailEncounterQTE()
{
	if (!IsValid(QTEComponent))
	{
		return;
	}

	QTEComponent->FailQTE();
}

void ADIBattleEncounter::CancelEncounterQTE()
{
	if (!IsValid(QTEComponent))
	{
		return;
	}

	QTEComponent->CancelQTE();
}

void ADIBattleEncounter::SpawnEnemies()
{
	if (!EnemyType)
	{
		UE_LOG(LogDIBattleEncounter, Error, TEXT("Encounter '%s': EnemyType is NULL"), *GetName());

		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	for (ATargetPoint* SpwanTarget : SpwanTargets)
	{
		if (!IsValid(SpwanTarget))
		{
			UE_LOG(LogDIBattleEncounter, Warning, TEXT("Encounter '%s': Invalid Spwan Target skipped."), *GetName());

			continue;
		}

		FActorSpawnParameters SpawnParams;

		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ADongincheonEnemyBase* SpawnedEnemy = World->SpawnActor<ADongincheonEnemyBase>(
			EnemyType, SpwanTarget->GetActorTransform(),
			SpawnParams);

		if (!IsValid(SpawnedEnemy))
		{
			UE_LOG(LogDIBattleEncounter, Error, TEXT("Encounter '%s': Failed to spawn enemy at '%s'.")
			       , *GetName(), *GetNameSafe(SpwanTarget));

			continue;
		}

		UHealthComponent* Health = SpawnedEnemy->FindComponentByClass<UHealthComponent>();

		UE_LOG(LogDIBattleEncounter, Warning,
		       TEXT("HEALTH CHECK Encounter | Enemy=%s | Health=%s | Ptr=%p | EnemyMemberHealth=%s | MemberPtr=%p"),
		       *GetNameSafe(SpawnedEnemy), *GetNameSafe(Health), Health, *GetNameSafe(SpawnedEnemy->HealthComponent),
		       SpawnedEnemy->HealthComponent.Get());

		if (!IsValid(Health))
		{
			UE_LOG(LogDIBattleEncounter, Error, TEXT("Enemy '%s' has no native UHealthComponent."),
			       *GetNameSafe(SpawnedEnemy));

			SpawnedEnemy->Destroy();
			continue;
		}

		Health->OnDeath.AddUniqueDynamic(this, &ADIBattleEncounter::HandleEnemyDeath);

		if (HealthTriggers.Num() > 0)
		{
			Health->OnHealthChanged.AddUniqueDynamic(this, &ADIBattleEncounter::HandleEnemyHealthChanged);
		}

		if (!bAutoStartCombat)
		{
			Health->SetDamageEnabled(false);
		}

		SpawnedEnemies.Add(SpawnedEnemy);

		++AliveCount;

		//Spwan된 Enemy가 Controller 없이 멈추는 상황 방어,
		//BP_Enemy의 Auto Possess AI도 Placed in World or Spawned로 설정하는게 정상이다.
		if (!SpawnedEnemy->GetController())
		{
			SpawnedEnemy->SpawnDefaultController();
		}

		if (!SpawnedEnemy->GetController())
		{
			UE_LOG(LogDIBattleEncounter, Error, TEXT("Enemy '%s' spawned WITHOUT AI Controller."),
			       *GetNameSafe(SpawnedEnemy));
		}
		else
		{
			UE_LOG(LogDIBattleEncounter, Log, TEXT("Spawned : %s | Controller: %s | Alive: %d"),
			       *GetNameSafe(SpawnedEnemy), *GetNameSafe(SpawnedEnemy->GetController()), AliveCount);
		}
	}
}


void ADIBattleEncounter::HandleEnemyDeath(AActor* DamageCauser)
{
	if (bCompleted)
	{
		return;
	}

	AliveCount = FMath::Max(AliveCount - 1, 0);

	UE_LOG(LogDIBattleEncounter, Log, TEXT("Enemy Dead | Remaining: %d"), AliveCount);

	if (AliveCount <= 0)
	{
		CompleteEncounter();
	}
}

void ADIBattleEncounter::CompleteEncounter()
{
	if (bCompleted)
	{
		return;
	}

	// Encounter가 Presentation 도중 종료되더라도 Player Input Lock이 남지 않도록 보장한다.
	ReleasePresentationPlayerLock();
	
	bCompleted = true;
	
	if (ADongincheonCharacter* Player = Cast<ADongincheonCharacter>
		(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		Player->SetCombatInputEnabled(false);
		Player->SetPresentationInputLocked(false);
		
		if (UInteractionComponent* Interaction = Player->FindComponentByClass<UInteractionComponent>())
		{
			Interaction->SetInteractionEnabled(true);
		}
	}
	
	HideEncounterHUD();
	
	//전투종료, Block해제
	SetBattleBlockerEnabled(false);

	UE_LOG(LogDIBattleEncounter, Log, TEXT("Encounter COMPLETE: %s"), *GetName());

	//Blocker 해제, Combat Stance 해제, 다음 Gameplay Flow는 Blueprint가 담당.
	OnEncounterCompleted();
}

void ADIBattleEncounter::SetBattleBlockerEnabled(bool bEnabled)
{
	for (AActor* Blocker : BattleBlockers)
	{
		if (!IsValid(Blocker))
		{
			continue;
		}

		Blocker->SetActorEnableCollision(bEnabled);
	}
}
