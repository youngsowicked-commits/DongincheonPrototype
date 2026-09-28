#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "NPCInteractionBase.generated.h"

class USceneComponent;
class UBoxComponent;
class ADongincheonCharacter;

UENUM(BlueprintType)
enum class ENPCInteractionState : uint8
{
    Idle,
    Active
};

UCLASS()
class DONGINCHEONPROTOTYPE_API ANPCInteractionBase
    : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    ANPCInteractionBase();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
    virtual bool CancelInteraction_Implementation(AActor* Interactor) override;

    UFUNCTION(BlueprintCallable, Category = "Interaction|NPC")
    void EndInteraction();

    UFUNCTION(BlueprintPure, Category = "Interaction|NPC")
    bool IsInteractionActive() const
    {
        return InteractionState == ENPCInteractionState::Active;
    }

    UFUNCTION(BlueprintPure, Category = "Interaction|NPC")
    FText GetNPCMessage() const
    {
        return NPCMessage;
    }

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|NPC")
    void OnNPCInteractionStarted(AActor* Interactor, const FText& Message);

    UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|NPC")
    void OnNPCInteractionEnded();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> InteractionBounds;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText InteractionText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|NPC",
        meta = (MultiLine = "true"))
    FText NPCMessage;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|NPC")
    FText NPCDisplayName = FText::FromString(TEXT("주민"));

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Interaction|Runtime")
    ENPCInteractionState InteractionState = ENPCInteractionState::Idle;

    UPROPERTY(Transient)
    TObjectPtr<ADongincheonCharacter> ActiveInteractor;
};