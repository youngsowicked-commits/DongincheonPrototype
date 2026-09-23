#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/DIHUDTypes.h"
#include "DIPlayerController.generated.h"

class UDIHUDRootWidget;
class UHealthComponent;
class UTargetingComponent;

UCLASS()
class DONGINCHEONPROTOTYPE_API ADIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "HUD")
	UDIHUDRootWidget* GetHUDRootWidget() const;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetHUDContext(EDIHUDContext NewContext);

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

	void CreateHUD();
};