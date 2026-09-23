#include "UI/Widgets/DIHUDRootWidget.h"

#include "UI/Widgets/DIHealthBarWidgetBase.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Widget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HealthComponent.h"
#include "GameFramework/PlayerController.h"

void UDIHUDRootWidget::SetPlayerHealthSource(UHealthComponent* InHealthComponent)
{
	if (!IsValid(PlayerHealthBar))
	{
		return;
	}

	PlayerHealthBar->SetHealthSource(InHealthComponent);
}

void UDIHUDRootWidget::SetEnemyHealthSource(UHealthComponent* InHealthComponent)
{
	UE_LOG(
	LogTemp,
	Warning,
	TEXT("[ENEMY HUD ROOT] SetSource | Bar=%s | Health=%s"),
	*GetNameSafe(EnemyHealthBar),
	*GetNameSafe(InHealthComponent)
);
	
	if (!IsValid(EnemyHealthBar))
	{
		return;
	}

	EnemyHealthBar->SetHealthSource(InHealthComponent);
}

void UDIHUDRootWidget::ClearEnemyHealthSource()
{
	if (IsValid(EnemyHealthBar))
	{
		EnemyHealthBar->ClearHealthSource();
	}

	bEnemyHUDActive = false;

	RefreshHUDVisibility();
}

void UDIHUDRootWidget::SetEnemyHUDActive(bool bActive)
{
	UE_LOG(
	LogTemp,
	Warning,
	TEXT("[ENEMY HUD ROOT] SetActive=%s | Container=%s"),
	bActive ? TEXT("TRUE") : TEXT("FALSE"),
	*GetNameSafe(EnemyHUDContainer)
);
	
	if (bEnemyHUDActive == bActive)
	{
		return;
	}

	bEnemyHUDActive = bActive;

	RefreshHUDVisibility();
}

bool UDIHUDRootWidget::IsEnemyHUDActive() const
{
	return bEnemyHUDActive;
}

void UDIHUDRootWidget::SetBossHealthSource(UHealthComponent* InHealthComponent)
{
	if (!IsValid(BossHealthBar))
	{
		return;
	}

	BossHealthBar->SetHealthSource(InHealthComponent);
}

void UDIHUDRootWidget::ClearBossHealthSource()
{
	if (IsValid(BossHealthBar))
	{
		BossHealthBar->ClearHealthSource();
	}

	bBossHUDActive = false;

	RefreshHUDVisibility();
}

void UDIHUDRootWidget::SetBossHUDActive(bool bActive)
{
	if (bBossHUDActive == bActive)
	{
		return;
	}

	bBossHUDActive = bActive;

	RefreshHUDVisibility();
}

bool UDIHUDRootWidget::IsBossHUDActive() const
{
	return bBossHUDActive;
}

void UDIHUDRootWidget::SetHUDContext(EDIHUDContext NewContext)
{
	if (CurrentHUDContext == NewContext)
	{
		return;
	}

	CurrentHUDContext = NewContext;

	RefreshHUDVisibility();
}

void UDIHUDRootWidget::SetLockOnTarget(AActor* InTarget)
{
	LockOnTarget = InTarget;
	RefreshHUDVisibility();
}

EDIHUDContext UDIHUDRootWidget::GetHUDContext() const
{
	return CurrentHUDContext;
}

void UDIHUDRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshHUDVisibility();
}

void UDIHUDRootWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(LockOnMarker))
	{
		return;
	}

	if (CurrentHUDContext != EDIHUDContext::Gameplay || !IsValid(LockOnTarget))
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();

	if (!IsValid(PlayerController))
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FVector MarkerWorldLocation = LockOnTarget->GetActorLocation() + FVector(0.0f, 0.0f, LockOnMarkerHeightOffset);

	FVector2D MarkerScreenPosition;

	const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController,MarkerWorldLocation,
			MarkerScreenPosition,true);

	if (!bProjected)
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(LockOnMarker->Slot);

	if (!IsValid(CanvasSlot))
	{
		LockOnMarker->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	CanvasSlot->SetPosition(MarkerScreenPosition);
	LockOnMarker->SetVisibility(ESlateVisibility::Visible);
}

void UDIHUDRootWidget::RefreshHUDVisibility()
{
	const bool bShowCombatHUD = CurrentHUDContext == EDIHUDContext::Gameplay;
	const bool bShowEnemyHUD = bShowCombatHUD && bEnemyHUDActive;
	const bool bShowBossHUD = bShowCombatHUD && bBossHUDActive;
	const bool bShowLockOn = bShowCombatHUD && IsValid(LockOnTarget);

	if (IsValid(CombatHUDContainer))
	{
		CombatHUDContainer->SetVisibility(bShowCombatHUD ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (IsValid(EnemyHUDContainer))
	{
		EnemyHUDContainer->SetVisibility(bShowEnemyHUD ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (IsValid(BossHUDContainer))
	{
		BossHUDContainer->SetVisibility(bShowBossHUD ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	
	if (IsValid(LockOnMarker))
	{
		LockOnMarker->SetVisibility(bShowLockOn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
