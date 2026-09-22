#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "Gameplay/Data/DIShopFoodDefinition.h"
#include "ShopInteractionBase.generated.h"

class USceneComponent;
class UCameraComponent;
class UBoxComponent;
class ADongincheonCharacter;

UENUM(BlueprintType)
enum class EShopInteractionState : uint8
{
	Idle,
	Entering,
	Active,
	Returning
};

UCLASS()
class DONGINCHEONPROTOTYPE_API AShopInteractionBase
	: public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AShopInteractionBase();

	// IInteractable
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionText_Implementation() const override;

	// Alignment용 기준 Transform
	UFUNCTION(BlueprintPure, Category = "Interaction|Alignment")
	FTransform GetInteractionPointTransform() const;
	
	UFUNCTION(BlueprintCallable, Category = "Interaction|Alignment")
	bool AlignInteractorToPoint(AActor* Interactor);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction|Shop")
	bool TryPurchaseFood(FName FoodId);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void EndShopInteraction();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Presentation")
	void OnShopInteractionActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Presentation")
	void OnShopInteractionDeactivated();
	
	void ApplyShopUIInputMode();
	void RestoreGameplayInputMode();
	
	void FinishShopInteractionEnter();
	void FinishShopInteractionReturn();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|Detection")
	TObjectPtr<UBoxComponent> InteractionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|Alignment")
	TObjectPtr<USceneComponent> InteractionPoint;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|Camera")
	TObjectPtr<UCameraComponent> InteractionCamera;
	
	UPROPERTY(EditDefaultsOnly, Category = "Interaction|Shop")
	TArray<FDIShopFoodDefinition> FoodDefinitions;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction|Runtime")
	EShopInteractionState InteractionState = EShopInteractionState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<ADongincheonCharacter> ActiveInteractor;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviousViewTarget;

	FTimerHandle InteractionTransitionTimerHandle;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Camera", meta = (ClampMin = "0.0"))
	float InteractionCameraBlendTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	FText InteractionText;
	
private:
	const FDIShopFoodDefinition* FindFoodDefinition(FName FoodId) const;
};