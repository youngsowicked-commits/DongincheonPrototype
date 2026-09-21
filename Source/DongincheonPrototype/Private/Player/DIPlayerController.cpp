#include "Player/DIPlayerController.h"

#include "Components/HealthComponent.h"
#include "UI/Widgets/DIHUDRootWidget.h"

void ADIPlayerController::BeginPlay()
{
    Super::BeginPlay();

    CreateHUD();
    BindPlayerHealthSource();
}

void ADIPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    BindPlayerHealthSource();
}

UDIHUDRootWidget* ADIPlayerController::GetHUDRootWidget() const
{
    return HUDRootWidget;
}

void ADIPlayerController::SetHUDContext(EDIHUDContext NewContext)
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->SetHUDContext(NewContext);
}

void ADIPlayerController::SetBossHUD(UHealthComponent* BossHealthComponent,bool bActive)
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    if (!IsValid(BossHealthComponent))
    {
        HUDRootWidget->ClearBossHealthSource();
        return;
    }

    HUDRootWidget->SetBossHealthSource(BossHealthComponent);
    HUDRootWidget->SetBossHUDActive(bActive);
}

void ADIPlayerController::ClearBossHUD()
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->ClearBossHealthSource();
}

void ADIPlayerController::CreateHUD()
{
    if (!IsLocalController())
    {
        return;
    }

    if (IsValid(HUDRootWidget))
    {
        return;
    }

    if (!HUDRootWidgetClass)
    {
        return;
    }

    HUDRootWidget = CreateWidget<UDIHUDRootWidget>(this,HUDRootWidgetClass);

    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->AddToPlayerScreen();
}

void ADIPlayerController::BindPlayerHealthSource()
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    APawn* ControlledPawn = GetPawn();

    if (!IsValid(ControlledPawn))
    {
        return;
    }

    UHealthComponent* HealthComponent = ControlledPawn->FindComponentByClass<UHealthComponent>();

    if (!IsValid(HealthComponent))
    {
        return;
    }

    HUDRootWidget->SetPlayerHealthSource(HealthComponent);
}