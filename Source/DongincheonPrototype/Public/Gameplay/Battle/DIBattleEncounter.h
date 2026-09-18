#pragma once

#include "CoreMinimal.h"
#include "Character/DongincheonEnemyBase.h"
#include "GameFramework/Actor.h"
#include "DIBattleEncounter.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class ATargetPoint;
class ADongincheonEnemyBase;

USTRUCT(BlueprintType)
struct FDIBattleHealthTrigger
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle")
	FName TriggerId = NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthThreshold = 0.5;
};

UCLASS()
class DONGINCHEONPROTOTYPE_API ADIBattleEncounter : public AActor
{
	GENERATED_BODY()
	
public:
	ADIBattleEncounter();
	
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void StartEncounter();
	
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void StartCombat();
	
	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsCombatStarted() const {return bCombatStarted;}
	
	UFUNCTION(BlueprintCallable, Category = "Battle|Flow")
	void PauseCombatForPresentation();
	
	UFUNCTION(BlueprintCallable, Category = "Battle|Flow")
	void ResumeCombatFromPresentation();
	
	UFUNCTION(BlueprintPure, Category = "Battle")
	int32 GetAliveCount() const {return AliveCount;}
	
	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsStarted() const {return bStarted;}
	
	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsCompleted() const {return bCompleted;}
	
protected:
	virtual void BeginPlay() override;
	
	//Player가 밟는 Encouter Trigger, 실제 Box 크기나 위치는 BP에서 조정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Battle|Trigger")
	TObjectPtr<UBoxComponent> BattleTrigger;
	
	//Spawn 할 Production Enemy, BP_Enemy 등을 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Battle|Spawn")
	TSubclassOf<ADongincheonEnemyBase> EnemyType;
	
	//레벨에 배치된 TargetPoint등을 Encounter Actor Instance에서 지정.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Battle|Spawn")
	TArray<TObjectPtr<ATargetPoint>> SpwanTargets;
	
	//전투시작시 Collision on, 종료시 off, 기존 BP BattleBlocker Actor Array 대응
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Battle|Blocker")
	TArray<TObjectPtr<AActor>> BattleBlockers;
	
	//true: spawn완료 후 바로 StartCombat(), false: OnEncounterReady()이후 대기
	//Cinematic,Placeholder 종료시 BP에서 StartCombat()g 호출
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Battle|Flow")
	bool bAutoStartCombat = true;

	//체력이 임계점 아래로 최초 진입했을 떄 호출
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Battle|Flow")
	TArray<FDIBattleHealthTrigger> HealthTriggers;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|Presentation")
	void OnHealthTriggerActivated(FName TriggerId, float HealthNormalized);
	
	//문 닫기, Combat State, 사운드, 연출 등은 Blueprint가 담당
	
	//Trigger가 발동 된 순간
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|Presentation")
	void OnEncounterStarted();
	
	//Enemy Spawn까지 완료된 순간
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|Presentation")
	void OnEncounterReady();
	
	//실제 AI 전투가 시작되는 순간
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|Presentation")
	void OnCombatStarted();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|Presentation")
	void OnEncounterCompleted();
	
private:
	UPROPERTY(VisibleInstanceOnly, Category = "Battle|Runtime")
	int32 AliveCount = 0;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Battle|Runtime")
	bool bStarted = false;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Battle|Runtime")
	bool bCombatStarted = false;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Battle|Runtime")
	bool bCompleted = false;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Battle|Runtime")
	bool bCombatPausedForPresentation = false;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<ADongincheonEnemyBase>> SpawnedEnemies;
	
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	TSet<int32> TriggeredHealthTriggerIndices;
	
	UFUNCTION()
	void HandleEnemyHealthChanged(float OldHealth, float NewHealth, float MaxHealth);
	
	UFUNCTION()
	void HandleEnemyDeath(AActor* DamageCauser);
	
	void SpawnEnemies();
	void CompleteEncounter();
	void SetBattleBlockerEnabled(bool bEnabled);
};