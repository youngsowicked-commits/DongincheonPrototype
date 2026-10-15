#include "Player/DIPlayerController.h"

#include "Character/DongincheonCharacter.h"
#include "Components/HealthComponent.h"
#include "Components/DIHeatActionComponent.h"
#include "Components/TargetingComponent.h"
#include "Character/DongincheonEnemyBase.h"
#include "AI/Data/DIEnemyDefinition.h"
#include "UI/Widgets/DIHUDRootWidget.h"
#include "UI/Widgets/DIGameMenuWidget.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"

namespace
{
    FText ResolveCombatantDisplayName(const AActor* InActor, const FText& Fallback)
    {
        const ADongincheonEnemyBase* Enemy =
            Cast<ADongincheonEnemyBase>(InActor);

        if (!IsValid(Enemy))
        {
            return Fallback;
        }

        // 1순위: 레벨에 배치된 Enemy 개체별 이름
        if (!Enemy->DisplayNameOverride.IsEmpty())
        {
            return Enemy->DisplayNameOverride;
        }

        // 2순위: 공용 EnemyDefinition 이름
        if (IsValid(Enemy->EnemyDefinition) &&
            !Enemy->EnemyDefinition->DisplayName.IsEmpty())
        {
            return Enemy->EnemyDefinition->DisplayName;
        }

        // 3순위: 기본 이름
        return Fallback;
    }
}

void ADIPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (InputComponent)
    {
        FInputKeyBinding& PauseBinding = InputComponent->BindKey(
            EKeys::Escape, IE_Pressed, this, &ADIPlayerController::TogglePauseMenu);
        PauseBinding.bExecuteWhenPaused = true;
    }
}

void ADIPlayerController::TogglePauseMenu()
{
    if (GameMenuWidget)
    {
        if (GameMenuWidget->GetMode() == EDIGameMenuMode::Pause) ResumeFromMenu();
        return;
    }
    if (bMenuTravelPending || UGameplayStatics::IsGamePaused(this)) return;
    if (PlayerMenuHealth.IsValid() && PlayerMenuHealth->IsDead()) return;
    ShowGameMenu(EDIGameMenuMode::Pause);
}

void ADIPlayerController::ShowGameMenu(EDIGameMenuMode Mode)
{
    if (!IsLocalController() || bMenuTravelPending) return;
    if (GameMenuWidget)
    {
        // Death supersedes an already open menu without forgetting the original pause state.
        if (Mode != EDIGameMenuMode::GameOver) return;
        GameMenuWidget->RemoveFromParent();
        GameMenuWidget = nullptr;
    }
    else
    {
        bMenuWasPaused = UGameplayStatics::IsGamePaused(this);
        bMenuHadCursor = bShowMouseCursor;
    }
    GameMenuWidget = CreateWidget<UDIGameMenuWidget>(this, UDIGameMenuWidget::StaticClass());
    if (!GameMenuWidget) return;
    GameMenuWidget->Configure(Mode);
    GameMenuWidget->AddToViewport(10000);
    SetPause(true);
    bShowMouseCursor = true;
    FInputModeUIOnly UI;
    UI.SetWidgetToFocus(GameMenuWidget->TakeWidget());
    UI.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(UI);
    GameMenuWidget->FocusPrimaryAction();
    UE_LOG(LogTemp, Log, TEXT("FINAL_MENU | Open | Mode=%d"), static_cast<int32>(Mode));
}

void ADIPlayerController::ResumeFromMenu()
{
    if (!GameMenuWidget || bMenuTravelPending ||
        GameMenuWidget->GetMode() == EDIGameMenuMode::GameOver) return;
    GameMenuWidget->RemoveFromParent();
    GameMenuWidget = nullptr;
    SetPause(bMenuWasPaused);
    bShowMouseCursor = bMenuHadCursor;
    SetInputMode(FInputModeGameOnly());
    // Existing Intro/QTE/presentation input locks are deliberately left intact.
    UE_LOG(LogTemp, Log, TEXT("FINAL_MENU | Resume | CinematicLocked=%d"), bCinematicControlLocked);
}

void ADIPlayerController::RestartFromMenu()
{
    if (!GameMenuWidget || bMenuTravelPending) return;
    const FString Map = UGameplayStatics::GetCurrentLevelName(this, true);
    if (Map.IsEmpty()) return;
    bMenuTravelPending = true;
    GetWorldTimerManager().ClearTimer(GameOverMenuTimer);
    GameMenuWidget->RemoveFromParent();
    GameMenuWidget = nullptr;
    SetPause(false);
    SetInputMode(FInputModeGameOnly());
    bShowMouseCursor = false;
    UE_LOG(LogTemp, Log, TEXT("FINAL_MENU | Restart | Map=%s"), *Map);
    UGameplayStatics::OpenLevel(this, FName(*Map));
}

void ADIPlayerController::QuitFromMenu()
{
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ADIPlayerController::BindPlayerDeathMenu()
{
    UnbindPlayerDeathMenu();
    if (APawn* Controlled = GetPawn())
    {
        PlayerMenuHealth = Controlled->FindComponentByClass<UHealthComponent>();
        if (PlayerMenuHealth.IsValid())
            PlayerMenuHealth->OnDeath.AddUniqueDynamic(this, &ADIPlayerController::HandlePlayerMenuDeath);
    }
}

void ADIPlayerController::UnbindPlayerDeathMenu()
{
    GetWorldTimerManager().ClearTimer(GameOverMenuTimer);
    if (PlayerMenuHealth.IsValid())
        PlayerMenuHealth->OnDeath.RemoveDynamic(this, &ADIPlayerController::HandlePlayerMenuDeath);
    PlayerMenuHealth.Reset();
}

void ADIPlayerController::HandlePlayerMenuDeath(AActor* DamageCauser)
{
    if (bMenuTravelPending || GetWorldTimerManager().IsTimerActive(GameOverMenuTimer)) return;
    // Existing death montage runs first; no new combat or death implementation.
    GetWorldTimerManager().SetTimer(GameOverMenuTimer, this, &ADIPlayerController::ShowGameOverMenu, 2.0f, false);
}

void ADIPlayerController::ShowGameOverMenu()
{
    if (PlayerMenuHealth.IsValid() && PlayerMenuHealth->IsDead()) ShowGameMenu(EDIGameMenuMode::GameOver);
}

void ADIPlayerController::SetCinematicControlLocked(bool bLocked)
{
    if (bCinematicControlLocked == bLocked)
    {
        return;
    }

    bCinematicControlLocked = bLocked;

    SetIgnoreMoveInput(bLocked);
    SetIgnoreLookInput(bLocked);

    if (ADongincheonCharacter* PlayerCharacter = Cast<ADongincheonCharacter>(GetPawn()))
    {
        PlayerCharacter->SetPresentationInputLocked(bLocked);
    }
}

bool ADIPlayerController::IsCinematicControlLocked() const
{
    return bCinematicControlLocked;
}

void ADIPlayerController::BeginPlay()
{
    Super::BeginPlay();

    CreateHUD();
    BindPlayerDeathMenu();

    // Keep the existing Night map and its Intro; title pauses it before gameplay ticks.
    // Remember the GameInstance so retry does not show the initial title again.
    static TSet<TWeakObjectPtr<UGameInstance>> TitleShownInstances;
    UGameInstance* Instance = GetGameInstance();
    if (IsLocalController() && GetWorld()->WorldType != EWorldType::PIE &&
        GetWorld()->GetMapName().EndsWith(TEXT("L_Station_P2_Integration_Night")) &&
        !TitleShownInstances.Contains(Instance))
    {
        TitleShownInstances.Add(Instance);
        ShowGameMenu(EDIGameMenuMode::Title);
    }
}

void ADIPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    BindPlayerHealthToHUD();
    BindPlayerHeatToHUD();
    BindTargetingToHUD();

    BindHeatActionAvailability(Cast<ADongincheonCharacter>(InPawn));
    BindPlayerDeathMenu();

    if (bCinematicControlLocked)
    {
        if (ADongincheonCharacter* PlayerCharacter = Cast<ADongincheonCharacter>(InPawn))
        {
            PlayerCharacter->SetPresentationInputLocked(true);
        }
    }
}

void ADIPlayerController::OnUnPossess()
{
    UnbindPlayerDeathMenu();
    UnbindHeatActionAvailability();

    Super::OnUnPossess();
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

void ADIPlayerController::BindPlayerHeatToHUD()
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

    UDIHeatActionComponent* HeatComponent = ControlledPawn->FindComponentByClass<UDIHeatActionComponent>();

    if (!IsValid(HeatComponent))
    {
        return;
    }

    HUDRootWidget->SetPlayerHeatSource(HeatComponent);
}

void ADIPlayerController::BindHeatActionAvailability(ADongincheonCharacter* InCharacter)
{
    UnbindHeatActionAvailability();

    if (!IsValid(InCharacter))
    {
        return;
    }

    HeatActionSourceCharacter = InCharacter;

    InCharacter->OnHeatActionAvailabilityChanged.AddUniqueDynamic(
        this,
        &ADIPlayerController::HandleHeatActionAvailabilityChanged);

    bHeatActionAvailable = InCharacter->CanUseContextualHeatAction();

    SyncHeatActionPrompt();
}

void ADIPlayerController::UnbindHeatActionAvailability()
{
    if (ADongincheonCharacter* BoundCharacter = HeatActionSourceCharacter.Get())
    {
        BoundCharacter->OnHeatActionAvailabilityChanged.RemoveDynamic(
            this,
            &ADIPlayerController::HandleHeatActionAvailabilityChanged);
    }

    HeatActionSourceCharacter.Reset();
    bHeatActionAvailable = false;

    SyncHeatActionPrompt();
}

void ADIPlayerController::HandleHeatActionAvailabilityChanged(bool bAvailable)
{
    bHeatActionAvailable = bAvailable;

    SyncHeatActionPrompt();
}

void ADIPlayerController::SyncHeatActionPrompt()
{
    if (CurrentHUDContext == EDIHUDContext::Gameplay && bHeatActionAvailable)
    {
        ShowHeatActionPrompt();
        return;
    }

    HideHeatActionPrompt();
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
    CurrentHUDContext = NewContext;

    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    if (CurrentHUDContext != EDIHUDContext::Gameplay)
    {
        HUDRootWidget->HideContextActionHint();
    }

    HUDRootWidget->SetHUDContext(CurrentHUDContext);

    SyncHeatActionPrompt();
}

void ADIPlayerController::SetQTESource(UQTEComponent* InQTEComponent)
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->SetQTESource(InQTEComponent);
}

void ADIPlayerController::ShowTutorialPrompt(const FText& Title, const FText& Description)
{
    if (IsValid(HUDRootWidget))
    {
        HUDRootWidget->ShowTutorialPrompt(Title, Description);
    }
}

void ADIPlayerController::SetTutorialProgress(const FText& ProgressText)
{
    if (IsValid(HUDRootWidget))
    {
        HUDRootWidget->SetTutorialProgress(ProgressText);
    }
}

void ADIPlayerController::HideTutorialPrompt()
{
    if (IsValid(HUDRootWidget))
    {
        HUDRootWidget->HideTutorialPrompt();
    }
}

void ADIPlayerController::ShowContextActionHint(const FText& ActionText)
{
    if (CurrentHUDContext != EDIHUDContext::Gameplay)
    {
        return;
    }

    if (IsValid(HUDRootWidget))
    {
        HUDRootWidget->ShowContextActionHint(ActionText);
    }
}

void ADIPlayerController::SetContextActionIcon(const FSlateBrush& InBrush)
{
    if (IsValid(HUDRootWidget))
    {
        HUDRootWidget->SetContextActionIcon(InBrush);
    }
}

void ADIPlayerController::HideContextActionHint()
{
    if (IsValid(HUDRootWidget))
    {
        HUDRootWidget->HideContextActionHint();
    }
}

void ADIPlayerController::ShowHeatActionPrompt()
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->ShowHeatActionPrompt();
}

void ADIPlayerController::HideHeatActionPrompt()
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->HideHeatActionPrompt();
}

void ADIPlayerController::ClearQTESource()
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->ClearQTESource();
}

void ADIPlayerController::ShowBossIntro(const FText& BossRole,const FText& BossName)
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->ShowBossIntro(BossRole,BossName);
}

void ADIPlayerController::HideBossIntro()
{
    if (!IsValid(HUDRootWidget))
    {
        return;
    }

    HUDRootWidget->HideBossIntro();
}

void ADIPlayerController::SetBossHUD(UHealthComponent* BossHealthComponent, bool bActive)
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

    if (bActive)
    {
        HideEnemyHUD();
        
        const FText DisplayName = ResolveCombatantDisplayName(
            BossHealthComponent->GetOwner(),
            NSLOCTEXT("DongincheonHUD", "UnknownBoss", "보스"));

        HUDRootWidget->SetBossDisplayName(DisplayName);
    }
    else
    {
        HUDRootWidget->SetBossDisplayName(FText::GetEmpty());
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

    // Ordinary enemy HUD uses stable actor suffixes; boss identity stays unchanged.
    const int32 EnemyIndex = FMath::Max(0, EnemyActor->GetFName().GetNumber() - 1);
    const FText DisplayName = FText::FromString(
        FString::Printf(TEXT("적 %c"), static_cast<TCHAR>(TEXT('A') + EnemyIndex % 26)));

    HUDRootWidget->SetEnemyDisplayName(DisplayName);
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
    
    HideEnemyHUD();

    const FText DisplayName = ResolveCombatantDisplayName(
        BossActor,
        NSLOCTEXT("DongincheonHUD", "UnknownBoss", "보스"));

    HUDRootWidget->SetBossDisplayName(DisplayName);
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

    HUDRootWidget->SetHUDContext(CurrentHUDContext);

    HUDRootWidget->AddToPlayerScreen();

    BindPlayerHealthToHUD();
    
    BindPlayerHeatToHUD();

    BindTargetingToHUD();
    
    BindHeatActionAvailability(Cast<ADongincheonCharacter>(GetPawn()));
}
