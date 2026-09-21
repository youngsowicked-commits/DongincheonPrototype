#include "UI/Widgets/DIHUDRootWidget.h"

#include "UI/Widgets/DIHealthBarWidgetBase.h"
#include "Components/Widget.h"
#include "Components/HealthComponent.h"

void UDIHUDRootWidget::SetPlayerHealthSource(UHealthComponent* InHealthComponent)
{
	if (!IsValid(PlayerHealthBar))
	{
		return;
	}

	PlayerHealthBar->SetHealthSource(InHealthComponent);
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

EDIHUDContext UDIHUDRootWidget::GetHUDContext() const
{
	return CurrentHUDContext;
}

void UDIHUDRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshHUDVisibility();
}

void UDIHUDRootWidget::RefreshHUDVisibility()
{
	const bool bShowCombatHUD = CurrentHUDContext == EDIHUDContext::Gameplay;

	if (IsValid(CombatHUDContainer))
	{
		CombatHUDContainer->SetVisibility(bShowCombatHUD ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (IsValid(BossHUDContainer))
	{
		BossHUDContainer->SetVisibility(bBossHUDActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}