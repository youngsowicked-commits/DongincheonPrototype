#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/DIHUDTypes.h"
#include "Styling/SlateBrush.h"
#include "DIPlayerController.generated.h"

class UDIHUDRootWidget;
class UHealthComponent;
class UQTEComponent;
class UTargetingComponent;

UCLASS()
class DONGINCHEONPROTOTYPE_API ADIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "HUD")
	UDIHUDRootWidget* GetHUDRootWidget() const;
	
	UFUNCTION(BlueprintCallable, Category = "Player|Cinematic")
	void SetCinematicControlLocked(bool bLocked);

	UFUNCTION(BlueprintPure, Category = "Player|Cinematic")
	bool IsCinematicControlLocked() const;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetHUDContext(EDIHUDContext NewContext);
	
	UFUNCTION(BlueprintCallable, Category = "HUD|QTE")
	void SetQTESource(UQTEComponent* InQTEComponent);

	UFUNCTION(BlueprintCallable, Category = "HUD|QTE")
	void ClearQTESource();
	
	// Tutorial ------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
	void ShowTutorialPrompt(const FText& Title, const FText& Description);

	UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
	void SetTutorialProgress(const FText& ProgressText);

	UFUNCTION(BlueprintCallable, Category = "HUD|Tutorial")
	void HideTutorialPrompt();
	
	// Context Action Hint ------------------------------------
	UFUNCTION(BlueprintCallable, Category = "HUD|ContextHint")
	void ShowContextActionHint(const FText& ActionText);

	UFUNCTION(BlueprintCallable, Category = "HUD|ContextHint")
	void SetContextActionIcon(const FSlateBrush& InBrush);

	UFUNCTION(BlueprintCallable, Category = "HUD|ContextHint")
	void HideContextActionHint();
	
	// Boss Intro ----------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "HUD|BossIntro")
	void ShowBossIntro(const FText& BossRole, const FText& BossName);

	UFUNCTION(BlueprintCallable, Category = "HUD|BossIntro")
	void HideBossIntro();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetBossHUD(UHealthComponent* BossHealthComponent,bool bActive);
	
	UFUNCTION(BlueprintCallable, Category = "HUD|Enemy")
	void ShowEnemyHUD(AActor* EnemyActor);

	UFUNCTION(BlueprintCallable, Category = "HUD|Enemy")
	void HideEnemyHUD();
	
	UFUNCTION(BlueprintCallable, Category = "HUD|Boss")
	void ShowBossHUD(AActor* BossActor);

	UFUNCTION(BlueprintCallable, Category = "HUD|Boss")
	void HideBossHUD();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClearBossHUD();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UDIHUDRootWidget> HUDRootWidgetClass;
	
	void BindPlayerHealthToHUD();
	
	void BindTargetingToHUD();

	UFUNCTION()
	void HandleLockOnTargetChanged(AActor* NewTarget);
	
	UFUNCTION()
	void HandleEnemyHUDTargetDeath(AActor* DamageCauser);

	UPROPERTY(Transient)
	TObjectPtr<UDIHUDRootWidget> HUDRootWidget;
	
	UPROPERTY(Transient)
	TObjectPtr<UTargetingComponent> TargetingSource;
	
	UPROPERTY(Transient)
	TObjectPtr<UHealthComponent> EnemyHUDHealthSource;
	
	UPROPERTY(Transient)
	EDIHUDContext CurrentHUDContext = EDIHUDContext::Gameplay;

	UPROPERTY(Transient)
	bool bCinematicControlLocked = false;
	
	void CreateHUD();
};