#include "Actor/InspectableInteractionBase.h"

#include "TimerManager.h"
#include "Character/DongincheonCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/Collision/DICollisionChannels.h"

AInspectableInteractionBase::AInspectableInteractionBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	
	InteractionCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InteractionCamera"));
	InteractionCamera->SetupAttachment(SceneRoot);
	InteractionCamera->bAutoActivate = false;

	InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(SceneRoot);

	InteractionBounds->InitBoxExtent(FVector(75.0f, 75.0f, 75.0f));

	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionObjectType(DICollision::Interactable);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionBounds->SetGenerateOverlapEvents(false);

	InteractionText = NSLOCTEXT(
		"DongincheonInteraction",
		"Inspect",
		"조사하기"
	);
}

void AInspectableInteractionBase::Interact_Implementation(AActor* Interactor)
{
	ADongincheonCharacter* Player = Cast<ADongincheonCharacter>(Interactor);

	if (!IsValid(Player))
	{
		return;
	}

	if (InteractionState != EInspectableInteractionState::Idle)
	{
		return;
	}

	if (Player->IsGameplayInputLocked())
	{
		return;
	}

	ActiveInteractor = Player;
	InteractionState = EInspectableInteractionState::Active;

	Player->SetInteractionInputLocked(true);

	// Hero Inspectable인 경우에만 전용 Camera 사용
	if (bUseInteractionCamera && IsValid(InteractionCamera))
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Player->GetController()))
		{
			// 나중에 원래 Gameplay Camera로 돌아가기 위해 저장
			PreviousViewTarget = PlayerController->GetViewTarget();

			// 이 Actor의 InteractionCamera를 ViewTarget Camera로 사용
			InteractionCamera->Activate(true);

			PlayerController->SetViewTargetWithBlend(this,CameraBlendTime,VTBlend_Cubic);
		}
	}

	// BP는 기존처럼 조사 UI만 담당
	OnInspectionStarted(Player, InspectMessage);
}

bool AInspectableInteractionBase::CancelInteraction_Implementation(AActor* Interactor)
{
	if (InteractionState != EInspectableInteractionState::Active)
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

	EndInspection();
	return true;
}

FText AInspectableInteractionBase::GetInteractionText_Implementation() const
{
	return InteractionText;
}

void AInspectableInteractionBase::EndInspection()
{
	if (InteractionState != EInspectableInteractionState::Active)
	{
		return;
	}

	ADongincheonCharacter* Player = ActiveInteractor.Get();

	if (!IsValid(Player))
	{
		FinishInspectionReturn();
		return;
	}

	// UI는 Q를 누르는 즉시 닫는다.
	OnInspectionEnded();

	// Interaction Camera를 사용한 조사물이라면 Gameplay Camera로 Smooth Return 한다.
	if (bUseInteractionCamera && IsValid(InteractionCamera) && IsValid(PreviousViewTarget))
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Player->GetController()))
		{
			InteractionState = EInspectableInteractionState::Returning;

			PlayerController->SetViewTargetWithBlend(
				PreviousViewTarget.Get(),
				CameraBlendTime,
				VTBlend_Cubic
			);

			if (CameraBlendTime > 0.0f)
			{
				FTimerHandle ReturnTimerHandle;

				GetWorldTimerManager().SetTimer(
					ReturnTimerHandle,
					this,
					&AInspectableInteractionBase::FinishInspectionReturn,
					CameraBlendTime,
					false
				);

				return;
			}
		}
	}

	// 카메라를 사용하지 않았거나 Blend가 필요 없으면 즉시 종료
	FinishInspectionReturn();
}

void AInspectableInteractionBase::FinishInspectionReturn()
{
	ADongincheonCharacter* Player = ActiveInteractor.Get();

	if (IsValid(InteractionCamera))
	{
		InteractionCamera->Deactivate();
	}

	PreviousViewTarget = nullptr;

	InteractionState = EInspectableInteractionState::Idle;
	ActiveInteractor = nullptr;

	if (IsValid(Player))
	{
		Player->SetInteractionInputLocked(false);

		if (UInteractionComponent* Interaction =
			Player->FindComponentByClass<UInteractionComponent>())
		{
			Interaction->NotifyInteractionEnded(this);
		}
	}
}