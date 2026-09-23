#include "Player/DIPlayerController.h"

#include "Components/HealthComponent.h"
#include "Components/TargetingComponent.h"
#include "UI/Widgets/DIHUDRootWidget.h"

void ADIPlayerController::BeginPlay()
{
    Super::BeginPlay();

    CreateHUD();
}

void ADIPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    BindPlayerHealthToHUD();
    BindTargetingToHUD();
}

void ADIPlayerController::BindPlayerHealthToHUD()
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

void ADIPlayerController::BindTargetingToHUD()
{
    if (IsValid(TargetingSource))
    {
        TargetingSource->OnLockOnTargetChanged.RemoveDynamic(this,&ADIPlayerController::HandleLockOnTargetChanged);
    }

    TargetingSource = nullptr;

    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    APawn* ControlledPawn = GetPawn();

    if (!IsValid(ControlledPawn))
    {
        return;
    }

    TargetingSource = ControlledPawn->FindComponentByClass<UTargetingComponent>();

    if (!IsValid(TargetingSource))
    {
        return;
    }

    TargetingSource->OnLockOnTargetChanged.AddUniqueDynamic(this,&ADIPlayerController::HandleLockOnTargetChanged);

    HUDRootWidget->SetLockOnTarget(TargetingSource->GetLockOnTarget());
}

void ADIPlayerController::HandleLockOnTargetChanged(AActor* NewTarget)
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->SetLockOnTarget(NewTarget);
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

void ADIPlayerController::ShowEnemyHUD(AActor* EnemyActor)
{
    if (!IsValid(HUDRootWidget) || !IsValid(EnemyActor))
    {
        return;
    }

    if (HUDRootWidget->IsBossHUDActive())
    {
        return;
    }

    UHealthComponent* EnemyHealthComponent =
        EnemyActor->FindComponentByClass<UHealthComponent>();

    if (!IsValid(EnemyHealthComponent))
    {
        return;
    }

    // 죽은 Enemy를 HitConfirmed가 다시 HUD에 올리는 것 방지
    if (EnemyHealthComponent->IsDead())
    {
        HideEnemyHUD();
        return;
    }

    // HUD Target이 바뀌었으면 이전 Enemy의 Death Delegate 해제
    if (EnemyHUDHealthSource != EnemyHealthComponent)
    {
        if (IsValid(EnemyHUDHealthSource))
        {
            EnemyHUDHealthSource->OnDeath.RemoveDynamic(this,&ADIPlayerController::HandleEnemyHUDTargetDeath);
        }

        EnemyHUDHealthSource = EnemyHealthComponent;

        EnemyHUDHealthSource->OnDeath.AddUniqueDynamic(this,&ADIPlayerController::HandleEnemyHUDTargetDeath);
    }

    HUDRootWidget->SetEnemyHealthSource(EnemyHealthComponent);
    HUDRootWidget->SetEnemyHUDActive(true);
}

void ADIPlayerController::HideEnemyHUD()
{
    if (IsValid(EnemyHUDHealthSource))
    {
        EnemyHUDHealthSource->OnDeath.RemoveDynamic(this,&ADIPlayerController::HandleEnemyHUDTargetDeath);
    }

    EnemyHUDHealthSource = nullptr;

    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->ClearEnemyHealthSource();
}

void ADIPlayerController::HandleEnemyHUDTargetDeath(AActor* DamageCauser)
{
    HideEnemyHUD();
}

void ADIPlayerController::ShowBossHUD(AActor* BossActor)
{
    if (!IsValid(HUDRootWidget) || !IsValid(BossActor))
    {
        return;
    }

    UHealthComponent* BossHealthComponent =
        BossActor->FindComponentByClass<UHealthComponent>();

    if (!IsValid(BossHealthComponent))
    {
        return;
    }

    HUDRootWidget->SetBossHealthSource(BossHealthComponent);
    HUDRootWidget->SetBossHUDActive(true);
}

void ADIPlayerController::HideBossHUD()
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->ClearBossHealthSource();
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
    
    BindPlayerHealthToHUD();
    
    BindTargetingToHUD();
}