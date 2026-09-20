#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "ShopInteractionBase.generated.h"

class USceneComponent;

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

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|Alignment")
	TObjectPtr<USceneComponent> InteractionPoint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	FText InteractionText;
};