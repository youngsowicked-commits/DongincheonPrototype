#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "InspectableInteractionBase.generated.h"

class USceneComponent;
class UBoxComponent;
class UCameraComponent;
class ADongincheonCharacter;

UENUM(BlueprintType)
enum class EInspectableInteractionState : uint8
{
	Idle,
	Active,
	Returning
};

UCLASS()
class DONGINCHEONPROTOTYPE_API AInspectableInteractionBase
	: public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInspectableInteractionBase();

	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual bool CancelInteraction_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionText_Implementation() const override;

	UFUNCTION(BlueprintCallable, Category = "Interaction|Inspectable")
	void EndInspection();
	
	void FinishInspectionReturn();

	UFUNCTION(BlueprintPure, Category = "Interaction|Inspectable")
	bool IsInspectionActive() const
	{
		return InteractionState == EInspectableInteractionState::Active;
	}

	UFUNCTION(BlueprintPure, Category = "Interaction|Inspectable")
	FText GetInspectMessage() const
	{
		return InspectMessage;
	}

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Inspectable")
	void OnInspectionStarted(AActor* Interactor, const FText& Message);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|Inspectable")
	void OnInspectionEnded();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> InteractionBounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	FText InteractionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Inspectable",
		meta = (MultiLine = "true"))
	FText InspectMessage;
	
	// Interaction Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|Camera")
	TObjectPtr<UCameraComponent> InteractionCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Camera")
	bool bUseInteractionCamera = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Camera",
		meta = (ClampMin = "0.0"))
	float CameraBlendTime = 0.3f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviousViewTarget;

	//Runtime
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction|Runtime")
	EInspectableInteractionState InteractionState = EInspectableInteractionState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<ADongincheonCharacter> ActiveInteractor;
};