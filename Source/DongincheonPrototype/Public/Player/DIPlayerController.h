#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/DIHUDTypes.h"
#include "DIPlayerController.generated.h"

class UDIHUDRootWidget;
class UHealthComponent;

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

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ClearBossHUD();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UDIHUDRootWidget> HUDRootWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDIHUDRootWidget> HUDRootWidget;

	void CreateHUD();
	void BindPlayerHealthSource();
};