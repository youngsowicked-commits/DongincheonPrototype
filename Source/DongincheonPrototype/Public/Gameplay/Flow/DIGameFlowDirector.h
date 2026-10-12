#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Character/DongincheonCharacter.h"
#include "TimerManager.h"
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
	
	// Tutorial 01 Encounter Ready 이후 이동 학습 시작
	UFUNCTION(BlueprintCallable, Category = "Game Flow|Tutorial")
	void NotifyTutorialEncounterReady(ADIBattleEncounter* Encounter);

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void NotifyEndingFinished();

protected:
	virtual void Tick(float DeltaSeconds) override;
	
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Content references
	// 길 따라 1~2명씩 잡는 Tutorial Encounter들
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Game Flow|Encounter")
	TArray<TObjectPtr<ADIBattleEncounter>> TutorialEncounters;

	// 일반 전투 Encounter들 - 배열 순서대로 진행
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Game Flow|Encounter")
	TArray<TObjectPtr<ADIBattleEncounter>> MainEncounters;

	// Boss
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Game Flow|Encounter")
	TObjectPtr<ADIBattleEncounter> BossEncounter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Flow")
	bool bAutoStartFlow = true;

	// Presentation hooks
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnIntroRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnChaseStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Presentation")
	void OnTutorialStarted();
	
	// 이동 + 카메라 완료 후 Tutorial 01 전투 시작 요청
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Flow|Tutorial")
	void OnTutorialMovementFinished(ADIBattleEncounter* Encounter);

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
	void HandleEncounterCompleted(ADIBattleEncounter* Encounter);
	
	UFUNCTION()
	void HandleTutorialCombatStarted(ADIBattleEncounter* Encounter);
	
	UFUNCTION()
	void HandleTutorialAction(EDIPlayerTutorialAction Action);
	
	UFUNCTION()
	void HandleTutorialHeatActionEnded(EDIHeatActionPresentationEndReason EndReason,AActor* Target,FGameplayTag ActionTag);

	void RefreshTutorialProgress();
	
	void TryFinishSpecialTutorial();
	
	void TryFinishComboGrabTutorial();
	
	void StartTutorialMovement();
	void UpdateTutorialMovement(float DeltaSeconds);
	void ShowTutorialPage();
	void SetTutorialStage(int32 NewStage);
	void CleanupTutorial();
	
	void ShowTutorialFightInstruction();
	void HideTutorialFightInstruction();

	FTimerHandle TutorialFightPromptTimer;

	int32 TutorialStage = INDEX_NONE;

	// Tutorial 01: 0 이동, 1 공격, 2 콤보/잡기
	// Tutorial 02: 0 방어, 1 록온/히트 액션

	int32 TutorialFirstCount = 0;
	int32 TutorialSecondCount = 0;
	
	int32 TutorialComboCount = 0;
	int32 TutorialHeavyBranchCount = 0;
	int32 TutorialGrabCount = 0;
	int32 TutorialGrabAttackCount = 0;

	bool bTutorialComboGrabCompleted = false;
	
	int32 TutorialLockOnCount = 0;
	int32 TutorialSwitchCount = 0;
	int32 TutorialHeatCount = 0;

	bool bTutorialSpecialCompleted = false;

	float TutorialMovedCm = 0.0f;
	float TutorialCameraDegrees = 0.0f;
	float TutorialUIRefreshElapsed = 0.0f;

	FVector TutorialLastLocation = FVector::ZeroVector;
	float TutorialLastControlYaw = 0.0f;
	bool bTutorialMovementSampling = false;

	UPROPERTY(EditAnywhere, Category = "Game Flow|Tutorial")
	float TutorialRequiredMoveCm = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Game Flow|Tutorial")
	float TutorialRequiredCameraDegrees = 90.0f;
	
	void UnbindTutorialAction();

	TWeakObjectPtr<ADongincheonCharacter> TutorialActionPlayer;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Game Flow|Runtime")
	EDIGameFlowPhase CurrentPhase = EDIGameFlowPhase::None;

	UPROPERTY(VisibleInstanceOnly, Category = "Game Flow|Runtime")
	int32 CurrentTutorialIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, Category = "Game Flow|Runtime")
	int32 CurrentMainEncounterIndex = INDEX_NONE;
};