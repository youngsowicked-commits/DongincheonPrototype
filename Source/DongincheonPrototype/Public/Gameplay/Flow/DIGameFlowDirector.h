#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DIGameFlowDirector.generated.h"

class ADIBattleEncounter;

UENUM(BlueprintType)
enum class EDIGameFlowPhase : uint8
{
	None,
	Intro,
	Chase,
	Tutorial,
	MainEncounter,
	BossEncounter,
	Ending,
	FreeRoam
};

UCLASS()
class DONGINCHEONPROTOTYPE_API ADIGameFlowDirector : public AActor
{
	GENERATED_BODY()

public:
	ADIGameFlowDirector();

	UFUNCTION(BlueprintPure, Category = "Game Flow")
	EDIGameFlowPhase GetCurrentPhase() const
	{
		return CurrentPhase;
	}

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void NotifyIntroFinished();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void NotifyChaseFinished();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void NotifyEndingFinished();

protected:
	virtual void BeginPlay() override;

	// -------------------------
	// Content references
	// -------------------------

	// 길 따라 1~2명씩 잡는 Tutorial Encounter들
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Game Flow|Encounter")
	TArray<TObjectPtr<ADIBattleEncounter>> TutorialEncounters;

	// 일반 큰 전투
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Game Flow|Encounter")
	TObjectPtr<ADIBattleEncounter> MainEncounter;

	// Boss
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Game Flow|Encounter")
	TObjectPtr<ADIBattleEncounter> BossEncounter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Flow")
	bool bAutoStartFlow = true;

	// -------------------------
	// Presentation hooks
	// -------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnIntroRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnChaseStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnTutorialStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnMainEncounterStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnBossEncounterStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnEndingRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnFreeRoamStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnFlowPhaseChanged(
		EDIGameFlowPhase PreviousPhase,
		EDIGameFlowPhase NewPhase);

private:
	void SetPhase(EDIGameFlowPhase NewPhase);

	void StartCurrentTutorialEncounter();

	void BindEncounter(ADIBattleEncounter* Encounter);

	void DisableEncounterTrigger(ADIBattleEncounter* Encounter);

	UFUNCTION()
	void HandleEncounterCompleted(
		ADIBattleEncounter* Encounter);

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Game Flow|Runtime")
	EDIGameFlowPhase CurrentPhase = EDIGameFlowPhase::None;

	UPROPERTY(VisibleInstanceOnly, Category = "Game Flow|Runtime")
	int32 CurrentTutorialIndex = INDEX_NONE;
};