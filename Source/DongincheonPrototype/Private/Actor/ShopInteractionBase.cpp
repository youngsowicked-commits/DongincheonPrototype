#include "Actor/ShopInteractionBase.h"

#include "Character/DongincheonCharacter.h"
#include "Gameplay/Collision/DICollisionChannels.h"
#include "GameFramework/PlayerController.h"
#include "Components/SceneComponent.h"
#include "Components/HealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InteractionComponent.h"
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
    UE_LOG(LogTemp, Warning, TEXT("SHOP: Interact called"));

    ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(Interactor);

    if (!IsValid(Player))
    {
        UE_LOG(LogTemp, Error, TEXT("SHOP FAIL: Invalid Player"));
        return;
    }

    if (InteractionState != EShopInteractionState::Idle)
    {
        UE_LOG(LogTemp, Error, TEXT("SHOP FAIL: State is not Idle"));
        return;
    }

    if (Player->IsGameplayInputLocked())
    {
        UE_LOG(LogTemp, Error, TEXT("SHOP FAIL: Gameplay Input already locked"));
        return;
    }

	APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());

	if (!IsValid(PlayerController))
	{
		UE_LOG(LogTemp, Error, TEXT("SHOP FAIL: Invalid PlayerController"));

		if (UInteractionComponent* Interaction = Player->FindComponentByClass<UInteractionComponent>())
		{
			Interaction->NotifyInteractionEnded(this);
		}

		return;
	}

    InteractionState = EShopInteractionState::Entering;
    ActiveInteractor = Player;
    PreviousViewTarget = PlayerController->GetViewTarget();

    Player->SetInteractionInputLocked(true);
	
	UE_LOG(
	LogTemp,
	Warning,
	TEXT("[SHOP_POS] ENTER_BEFORE_ALIGN | Player=%s | Location=%s"),
	*GetNameSafe(Player),
	*Player->GetActorLocation().ToString()
);

	if (bAlignInteractorOnEnter)
	{
		if (!AlignInteractorToPoint(Player))
		{
			UE_LOG(LogTemp, Error, TEXT("SHOP FAIL: Alignment failed"));

			Player->SetInteractionInputLocked(false);

			ActiveInteractor = nullptr;
			PreviousViewTarget = nullptr;
			InteractionState = EShopInteractionState::Idle;

			if (UInteractionComponent* Interaction =
				Player->FindComponentByClass<UInteractionComponent>())
			{
				Interaction->NotifyInteractionEnded(this);
			}

			return;
		}
	}

	UE_LOG(
	LogTemp,
	Warning,
	TEXT("SHOP: Enter PASS | Align=%s | BlendTime=%.2f"),
	bAlignInteractorOnEnter ? TEXT("TRUE") : TEXT("FALSE"),
	InteractionCameraBlendTime
);

    PlayerController->SetViewTargetWithBlend(this,InteractionCameraBlendTime,VTBlend_Cubic,2.0f,false);

    if (InteractionCameraBlendTime <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("SHOP: Immediate Finish Enter"));
        FinishShopInteractionEnter();
        return;
    }

    GetWorldTimerManager().SetTimer(InteractionTransitionTimerHandle,this,&AShopInteractionBase::FinishShopInteractionEnter,
        InteractionCameraBlendTime,false);

    UE_LOG(LogTemp, Warning, TEXT("SHOP: Enter timer started"));
}

bool AShopInteractionBase::CancelInteraction_Implementation(AActor* Interactor)
{
	if (InteractionState != EShopInteractionState::Active)
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

	EndShopInteraction();

	return true;
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

	Interactor->SetActorLocationAndRotation(TargetTransform.GetLocation(),TargetTransform.Rotator(),false,
		nullptr,ETeleportType::TeleportPhysics
	);
	
	UE_LOG(
	LogTemp,
	Warning,
	TEXT("[SHOP_POS] ALIGN_AFTER_SET | Player=%s | Location=%s | Target=%s"),
	*GetNameSafe(Interactor),
	*Interactor->GetActorLocation().ToString(),
	*TargetTransform.GetLocation().ToString()
);

	constexpr float AlignmentTolerance = 5.0f;

	return FVector::DistSquared(Interactor->GetActorLocation(),TargetTransform.GetLocation()) <= FMath::Square(AlignmentTolerance);
}

bool AShopInteractionBase::TryPurchaseFood(FName FoodId)
{
	UE_LOG(LogTemp, Warning,
		TEXT("SHOP PURCHASE: Called | FoodId=%s"),
		*FoodId.ToString());

	if (InteractionState != EShopInteractionState::Active)
	{
		UE_LOG(LogTemp, Error,
			TEXT("SHOP PURCHASE FAIL: State is not Active"));
		return false;
	}

	if (!IsValid(ActiveInteractor))
	{
		UE_LOG(LogTemp, Error,
			TEXT("SHOP PURCHASE FAIL: Invalid ActiveInteractor"));
		return false;
	}

	const FDIShopFoodDefinition* FoodDefinition = FindFoodDefinition(FoodId);

	if (!FoodDefinition)
	{
		UE_LOG(LogTemp, Error,
			TEXT("SHOP PURCHASE FAIL: FoodId not found | FoodId=%s"),
			*FoodId.ToString());
		return false;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("SHOP PURCHASE: Food found | FoodId=%s | Heal=%.1f"),
		*FoodDefinition->FoodId.ToString(),
		FoodDefinition->HealAmount);

	if (FoodDefinition->HealAmount <= 0.0f)
	{
		UE_LOG(LogTemp, Error,
			TEXT("SHOP PURCHASE FAIL: HealAmount <= 0"));
		return false;
	}

	UHealthComponent* HealthComponent = ActiveInteractor->FindComponentByClass<UHealthComponent>();

	if (!IsValid(HealthComponent))
	{
		UE_LOG(LogTemp, Error,
			TEXT("SHOP PURCHASE FAIL: HealthComponent not found"));
		return false;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("SHOP PURCHASE: Calling Heal %.1f"),
		FoodDefinition->HealAmount);

	HealthComponent->Heal(FoodDefinition->HealAmount);

	UE_LOG(LogTemp, Warning,
		TEXT("SHOP PURCHASE: Heal call finished"));

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
	
	UE_LOG(
	LogTemp,
	Warning,
	TEXT("[SHOP_POS] END_BEFORE_DEACTIVATE | Player=%s | Location=%s"),
	*GetNameSafe(ActiveInteractor.Get()),
	*ActiveInteractor->GetActorLocation().ToString()
);

	InteractionState = EShopInteractionState::Returning;
	OnShopInteractionDeactivated();
	
	UE_LOG(
	LogTemp,
	Warning,
	TEXT("[SHOP_POS] END_AFTER_DEACTIVATE | Player=%s | Location=%s"),
	*GetNameSafe(ActiveInteractor.Get()),
	*ActiveInteractor->GetActorLocation().ToString()
);

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

	FInputModeGameAndUI InputMode;
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
	UE_LOG(LogTemp, Warning, TEXT("SHOP: FinishShopInteractionEnter called"));
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

	// ActiveInteractor를 지우기 전에 Player 보관
	ADongincheonCharacter* Player = ActiveInteractor.Get();
	
	if (IsValid(Player))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[SHOP_POS] RETURN_FINISH_START | Player=%s | Location=%s"),
			*GetNameSafe(Player),
			*Player->GetActorLocation().ToString()
		);
	}
	

	// Input Mode는 ActiveInteractor가 살아 있을 때 복구해야 함
	RestoreGameplayInputMode();

	if (IsValid(Player))
	{
		Player->SetInteractionInputLocked(false);
	}

	// Shop Native Runtime State 종료
	ActiveInteractor = nullptr;
	PreviousViewTarget = nullptr;
	InteractionState = EShopInteractionState::Idle;

	// InteractionComponent에게
	// "Shop Interaction이 완전히 종료됐다"고 알려준다.
	if (IsValid(Player))
	{
		if (UInteractionComponent* Interaction = Player->FindComponentByClass<UInteractionComponent>())
		{
			Interaction->NotifyInteractionEnded(this);
		}
	}
}
