#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DIBattleEncounter.generated.h"

class ATargetPoint;
class ADongincheonEnemyBase;

UCLASS()
class DONGINCHEONPROTOTYPE_API ADIBattleEncounter : public AActor
{
	GENERATED_BODY()
	
public:
	ADIBattleEncounter();
	
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void StartEncounter();
	
	UFUNCTION(BlueprintPure, Category = "Battle")
	int32 GetAliveCount() const {return AliveCount;}
	
	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsStarted() const {return bStarted;}
	
	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsCompleted() const {return bCompleted;}
	
protected:
	virtual void BeginPlay() override;
	
	//Spawn 할 Production Enemy, BP_Enemy 등을 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle|Spawn")
	TSubclassOf<ADongincheonEnemyBase> EnemyType;
	
	//레벨에 배치된 TargetPoint등을 Encounter Actor Instance에서 지정.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Battle|Spawn")
	TArray<TObjectPtr<ATargetPoint>> SpwanTargets;
	
	//문 닫기, Combat State, 사운드, 연출 등은 Blueprint가 담당
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|Presentation")
	void OnEncounterStarted();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|Presentation")
	void OnEncounterCompleted();
	
private:
	UPROPERTY(VisibleInstanceOnly, Category = "Battle|Runtime")
	int32 AliveCount = 0;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Battle|Runtime")
	bool bStarted = false;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Battle|Runtime")
	bool bCompleted = false;
	
	UFUNCTION()
	void HandleEnemyDeath(AActor* DamageCauser);
	
	void SpawnEnemies();
	void CompleteEncounter();
};