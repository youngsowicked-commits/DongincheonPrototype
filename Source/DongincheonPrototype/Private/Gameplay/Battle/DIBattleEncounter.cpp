#include "Gameplay/Battle/DIBattleEncounter.h"

#include "Character/DongincheonEnemyBase.h"
#include "Components/HealthComponent.h"

#include "Engine/TargetPoint.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogDIBattleEncounter, Log, All);

ADIBattleEncounter::ADIBattleEncounter()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADIBattleEncounter::BeginPlay()
{
	Super::BeginPlay();
}

void ADIBattleEncounter::StartEncounter()
{
	if (bStarted || bCompleted)
	{
		return;
	}
	
	bStarted = true;
	AliveCount = 0;
	
	UE_LOG(LogDIBattleEncounter,Log,TEXT("Encounter START %s"), *GetName());
	
	//Blocker 활성화, Combat Stance, 사운드 등, 레벨별 Presentation은 Blueprint가 담당.
	OnEncounterStarted();
	
	SpawnEnemies();
	
	//Enemy 설정 누락이나 Spwan 실패로 전투가 영원히 막히는것을 방지
	if (AliveCount <= 0)
	{
		UE_LOG(LogDIBattleEncounter, Warning, TEXT("Encounter '%s' spawned zero valid enemies"), *GetName());
		
		CompleteEncounter();
	}
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
		
		if (!IsValid(Health))
		{
			UE_LOG(LogDIBattleEncounter,Error,TEXT("Enemy '%s' has no native UHealthComponent."),
				*GetNameSafe(SpawnedEnemy));
			
			SpawnedEnemy->Destroy();
			continue;
		}
		
		Health->OnDeath.AddUniqueDynamic(this,&ADIBattleEncounter::HandleEnemyDeath);
		
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
	
	UE_LOG(LogDIBattleEncounter, Log, TEXT("Encounter COMPLETE: %s"), *GetName());
	
	//Blocker 해제, Combat Stance 해제, 다음 Gameplay Flow는 Blueprint가 담당.
	OnEncounterCompleted();
}











