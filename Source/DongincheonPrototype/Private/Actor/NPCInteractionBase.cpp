#include "Actor/NPCInteractionBase.h"

#include "Character/DongincheonCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/InteractionComponent.h"
#include "Gameplay/Collision/DICollisionChannels.h"

ANPCInteractionBase::ANPCInteractionBase()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));

    InteractionBounds->SetupAttachment(SceneRoot);

    InteractionBounds->InitBoxExtent(FVector(75.0f, 75.0f, 100.0f));

    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    InteractionBounds->SetCollisionObjectType(DICollision::Interactable);

    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Overlap);

    InteractionBounds->SetGenerateOverlapEvents(false);

    InteractionText = NSLOCTEXT(
        "DongincheonInteraction",
        "Talk",
        "대화하기"
    );

    NPCMessage = NSLOCTEXT(
        "DongincheonInteraction",
        "DefaultNPCMessage",
        "..."
    );
}

void ANPCInteractionBase::Interact_Implementation(AActor* Interactor)
{
    if (InteractionState != ENPCInteractionState::Idle)
    {
        return;
    }

    ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(Interactor);
    
    if (!IsValid(Player))
    {
        return;
    }

    if (Player->IsGameplayInputLocked())
    {
        return;
    }

    ActiveInteractor = Player;
    InteractionState = ENPCInteractionState::Active;

    Player->SetInteractionInputLocked(true);

    OnNPCInteractionStarted(Player,NPCMessage);
}

FText ANPCInteractionBase::GetInteractionText_Implementation() const
{
    return InteractionText;
}

bool ANPCInteractionBase::CancelInteraction_Implementation(AActor* Interactor)
{
    if (InteractionState != ENPCInteractionState::Active)
    {
        return false;
    }

    if (!IsValid(ActiveInteractor))
    {
        return false;
    }

    if (Interactor != ActiveInteractor)
    {
        return false;
    }

    EndInteraction();

    return true;
}

void ANPCInteractionBase::EndInteraction()
{
    if (InteractionState != ENPCInteractionState::Active)
    {
        return;
    }

    // ActiveInteractor를 지우기 전에 Player를 보관한다.
    ADongincheonCharacter* Player = ActiveInteractor.Get();

    // 1. 먼저 Native Runtime State 종료
    InteractionState = ENPCInteractionState::Idle;
    ActiveInteractor = nullptr;

    // 2. Player Gameplay Input 복구
    if (IsValid(Player))
    {
        Player->SetInteractionInputLocked(false);
    }

    // 3. BP는 Presentation만 종료
    //    현재 BP_NPCInteractionBase에서는 여기서 NPC Widget을 제거한다.
    OnNPCInteractionEnded();

    // 4. InteractionComponent에게
    //    "Active Interaction이 실제로 종료됐다"고 통보
    if (IsValid(Player))
    {
        if (UInteractionComponent* Interaction =
            Player->FindComponentByClass<UInteractionComponent>())
        {
            Interaction->NotifyInteractionEnded(this);
        }
    }
}