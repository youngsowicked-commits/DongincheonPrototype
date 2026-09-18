#include "Gameplay/Battle/DIBattleEncounter.h"

#include "Character/DongincheonCharacter.h"
#include "Character/DongincheonEnemyBase.h"
#include "Components/HealthComponent.h"
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
	
	SetRootComponent(BattleTrigger);
	
	BattleTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	BattleTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	BattleTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	BattleTrigger->SetGenerateOverlapEvents(true);
}

void ADIBattleEncounter::BeginPlay()
{
	Super::BeginPlay();
	
	BattleTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this,&ADIBattleEncounter::HandleTriggerBeginOverlap);
	
	//게임 시작시 전투 Blocker 비활성화
	SetBattleBlockerEnabled(false);
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
			
			UE_LOG(LogDIBattleEncounter,Log,TEXT("HEALTH TRIGGER: %s | Id=%s | HP=%.1f/%.1f | Normalized=%.2f"),
				*GetName(),*Trigger.TriggerId.ToString(),NewHealth,MaxHealth,NewNormalized);
			
			OnHealthTriggerActivated(Trigger.TriggerId,NewNormalized);
		}
	}
}

void ADIBattleEncounter::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
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
	
	AliveCount = 0;
	SpawnedEnemies.Reset();
	TriggeredHealthTriggerIndices.Reset();

	
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
	
	UE_LOG(LogDIBattleEncounter,Log,TEXT("Encounter READY: %s | Enemies %d"),*GetName(),AliveCount);
	
	OnEncounterReady();
	
	if (bAutoStartCombat)
	{
		StartCombat();
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
	
	UE_LOG(LogDIBattleEncounter,Log,TEXT("Combat START: %s"), *GetName());
	
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
			UE_LOG(LogDIBattleEncounter,Error,TEXT("Combat START failed: Enemy '%s' has invaild AIController"),
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
	
	if (ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(UGameplayStatics::GetPlayerCharacter(this,0)))
	{
		Player->SetPrentationInputLocked(true);
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
	
	UE_LOG(LogDIBattleEncounter,Log,TEXT("Combat PAUSED for presentation: %s"), *GetName());
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
			continue;;
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
	
	if (ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(UGameplayStatics::GetPlayerCharacter(this,0)))
	{
		Player->SetPrentationInputLocked(false);
	}
	
	UE_LOG(LogDIBattleEncounter,Log,TEXT("Combat RESUMED from presentation: %s"), *GetName());
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
		
		ADongincheonEnemyBase* SpawnedEnemy = World->SpawnActor<ADongincheonEnemyBase>(EnemyType,SpwanTarget->GetActorTransform(),
			SpawnParams);
		
		if (!IsValid(SpawnedEnemy))
		{
			UE_LOG(LogDIBattleEncounter,Error,TEXT("Encounter '%s': Failed to spawn enemy at '%s'.")
				,*GetName(), *GetNameSafe(SpwanTarget));
			
			continue;
		}
		
		UHealthComponent* Health = SpawnedEnemy->FindComponentByClass<UHealthComponent>();
		
		UE_LOG(LogDIBattleEncounter,Warning,TEXT("HEALTH CHECK Encounter | Enemy=%s | Health=%s | Ptr=%p | EnemyMemberHealth=%s | MemberPtr=%p"),
			*GetNameSafe(SpawnedEnemy),*GetNameSafe(Health),Health,*GetNameSafe(SpawnedEnemy->HealthComponent), SpawnedEnemy->HealthComponent.Get());
		
		if (!IsValid(Health))
		{
			UE_LOG(LogDIBattleEncounter,Error,TEXT("Enemy '%s' has no native UHealthComponent."),
				*GetNameSafe(SpawnedEnemy));
			
			SpawnedEnemy->Destroy();
			continue;
		}
		
		Health->OnDeath.AddUniqueDynamic(this,&ADIBattleEncounter::HandleEnemyDeath);
		
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
				*GetNameSafe(SpawnedEnemy), *GetNameSafe(SpawnedEnemy->GetController()),AliveCount);
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
	
	bCompleted = true;
	
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