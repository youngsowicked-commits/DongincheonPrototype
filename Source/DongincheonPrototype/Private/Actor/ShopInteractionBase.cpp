#include "Actor/ShopInteractionBase.h"

#include "Character/DongincheonCharacter.h"
#include "Gameplay/Collision/DICollisionChannels.h"
#include "GameFramework/PlayerController.h"
#include "Components/SceneComponent.h"
#include "Components/HealthComponent.h"
#include "Components/BoxComponent.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"

AShopInteractionBase::AShopInteractionBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractionPoint = CreateDefaultSubobject<USceneComponent>(TEXT("InteractionPoint"));
	InteractionPoint->SetupAttachment(SceneRoot);
	
	InteractionCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InteractionCamera"));
	InteractionCamera->SetupAttachment(SceneRoot);
	
	InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(SceneRoot);
	InteractionBounds->InitBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionObjectType(DICollision::Interactable);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionBounds->SetGenerateOverlapEvents(false);

	InteractionText = NSLOCTEXT(
		"DongincheonInteraction",
		"ShopOrder",
		"주문하기"
	);
}

void AShopInteractionBase::Interact_Implementation(AActor* Interactor)
{
	ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(Interactor);

	if (!IsValid(Player))
	{
		return;
	}

	if (InteractionState != EShopInteractionState::Idle)
	{
		return;
	}

	if (Player->IsGameplayInputLocked())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());

	if (!IsValid(PlayerController))
	{
		return;
	}

	InteractionState = EShopInteractionState::Entering;
	ActiveInteractor = Player;
	PreviousViewTarget = PlayerController->GetViewTarget();

	Player->SetInteractionInputLocked(true);

	if (!AlignInteractorToPoint(Player))
	{
		Player->SetInteractionInputLocked(false);
		ActiveInteractor = nullptr;
		PreviousViewTarget = nullptr;
		InteractionState = EShopInteractionState::Idle;
		return;
	}

	PlayerController->SetViewTargetWithBlend(this, InteractionCameraBlendTime,
		VTBlend_Cubic, 2.0f, false);

	if (InteractionCameraBlendTime <= 0.0f)
	{
		FinishShopInteractionEnter();
		return;
	}

	GetWorldTimerManager().SetTimer(InteractionTransitionTimerHandle, this,
		&AShopInteractionBase::FinishShopInteractionEnter, InteractionCameraBlendTime, false);
}

FText AShopInteractionBase::GetInteractionText_Implementation() const
{
	return InteractionText;
}

FTransform AShopInteractionBase::GetInteractionPointTransform() const
{
	if (!IsValid(InteractionPoint))
	{
		return GetActorTransform();
	}

	return InteractionPoint->GetComponentTransform();
}

bool AShopInteractionBase::AlignInteractorToPoint(AActor* Interactor)
{
	if (!IsValid(Interactor) || !IsValid(InteractionPoint))
	{
		return false;
	}

	const FTransform TargetTransform = InteractionPoint->GetComponentTransform();
	FHitResult SweepHit;

	return Interactor->SetActorLocationAndRotation(TargetTransform.GetLocation(),
		TargetTransform.Rotator(), true, &SweepHit, ETeleportType::None);
}

bool AShopInteractionBase::TryPurchaseFood(FName FoodId)
{
	if (InteractionState != EShopInteractionState::Active || !IsValid(ActiveInteractor))
	{
		return false;
	}

	const FDIShopFoodDefinition* FoodDefinition = FindFoodDefinition(FoodId);

	if (!FoodDefinition || FoodDefinition->HealAmount <= 0.0f)
	{
		return false;
	}

	UHealthComponent* HealthComponent = ActiveInteractor->FindComponentByClass<UHealthComponent>();

	if (!IsValid(HealthComponent))
	{
		return false;
	}

	HealthComponent->Heal(FoodDefinition->HealAmount);
	return true;
}

const FDIShopFoodDefinition* AShopInteractionBase::FindFoodDefinition(FName FoodId) const
{
	if (FoodId.IsNone())
	{
		return nullptr;
	}

	return FoodDefinitions.FindByPredicate([FoodId](const FDIShopFoodDefinition& Definition)
	{
		return Definition.FoodId == FoodId;
	});
}

void AShopInteractionBase::EndShopInteraction()
{
	if (InteractionState != EShopInteractionState::Active)
	{
		return;
	}

	if (!IsValid(ActiveInteractor))
	{
		InteractionState = EShopInteractionState::Idle;
		PreviousViewTarget = nullptr;
		return;
	}

	InteractionState = EShopInteractionState::Returning;
	OnShopInteractionDeactivated();

	APlayerController* PlayerController = Cast<APlayerController>(ActiveInteractor->GetController());

	if (!IsValid(PlayerController))
	{
		FinishShopInteractionReturn();
		return;
	}

	AActor* ReturnViewTarget = IsValid(PreviousViewTarget) ? PreviousViewTarget.Get() : ActiveInteractor.Get();

	PlayerController->SetViewTargetWithBlend(ReturnViewTarget, InteractionCameraBlendTime,
		VTBlend_Cubic, 2.0f, false);

	if (InteractionCameraBlendTime <= 0.0f)
	{
		FinishShopInteractionReturn();
		return;
	}

	GetWorldTimerManager().SetTimer(InteractionTransitionTimerHandle, this,
		&AShopInteractionBase::FinishShopInteractionReturn, InteractionCameraBlendTime, false);
}

void AShopInteractionBase::ApplyShopUIInputMode()
{
	if (!IsValid(ActiveInteractor))
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(ActiveInteractor->GetController());

	if (!IsValid(PlayerController))
	{
		return;
	}

	FInputModeUIOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;
}

void AShopInteractionBase::RestoreGameplayInputMode()
{
	if (!IsValid(ActiveInteractor))
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(ActiveInteractor->GetController());

	if (!IsValid(PlayerController))
	{
		return;
	}

	FInputModeGameOnly InputMode;
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = false;
}

void AShopInteractionBase::FinishShopInteractionEnter()
{
	if (InteractionState != EShopInteractionState::Entering)
	{
		return;
	}

	if (!IsValid(ActiveInteractor))
	{
		InteractionState = EShopInteractionState::Idle;
		PreviousViewTarget = nullptr;
		return;
	}

	InteractionState = EShopInteractionState::Active;
	OnShopInteractionActivated();
	ApplyShopUIInputMode();
}

void AShopInteractionBase::FinishShopInteractionReturn()
{
	if (InteractionState != EShopInteractionState::Returning)
	{
		return;
	}

	RestoreGameplayInputMode();

	if (IsValid(ActiveInteractor))
	{
		ActiveInteractor->SetInteractionInputLocked(false);
	}

	ActiveInteractor = nullptr;
	PreviousViewTarget = nullptr;
	InteractionState = EShopInteractionState::Idle;
}
