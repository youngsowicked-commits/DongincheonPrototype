#include "Cinematic/DIHeatActionPresentationActor.h"

#include "Character/DongincheonCharacter.h"
#include "Player/DIPlayerController.h"

#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"

#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ADIHeatActionPresentationActor::ADIHeatActionPresentationActor()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(false);
}

void ADIHeatActionPresentationActor::BeginPlay()
{
    Super::BeginPlay();

    Player = Cast<ADongincheonCharacter>(
        UGameplayStatics::GetPlayerCharacter(this, 0));

    if (!IsValid(Player))
    {
        return;
    }

    HeatActionComponent =
        Player->FindComponentByClass<UDIHeatActionComponent>();

    if (!IsValid(HeatActionComponent))
    {
        return;
    }

    HeatActionComponent->OnHeatActionStateChanged.AddUniqueDynamic(
        this,
        &ADIHeatActionPresentationActor::HandleHeatActionStateChanged);
    
    HeatActionComponent->OnHeatActionHitProcessed.AddUniqueDynamic(
    this,
    &ADIHeatActionPresentationActor::HandleHeatActionHitProcessed);
}

void ADIHeatActionPresentationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopHitSlowMotion();
    
    if (IsValid(HeatActionComponent))
    {
        HeatActionComponent->OnHeatActionStateChanged.RemoveDynamic(
            this,
            &ADIHeatActionPresentationActor::HandleHeatActionStateChanged);

        HeatActionComponent->OnHeatActionHitProcessed.RemoveDynamic(
            this,
            &ADIHeatActionPresentationActor::HandleHeatActionHitProcessed);
    }

    Super::EndPlay(EndPlayReason);
}

void ADIHeatActionPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bPresentationActive)
    {
        return;
    }

    FVector DesiredLocation;
    FRotator DesiredRotation;

    if (!CalculateDesiredCamera(DesiredLocation,DesiredRotation))
    {
        return;
    }

    SetActorLocation(FMath::VInterpTo(GetActorLocation(),DesiredLocation,DeltaSeconds,LocationInterpSpeed));

    SetActorRotation(FMath::RInterpTo(GetActorRotation(),DesiredRotation,DeltaSeconds,RotationInterpSpeed));
}

void ADIHeatActionPresentationActor::HandleHeatActionStateChanged(EDIHeatActionState PreviousState,EDIHeatActionState NewState)
{
    if (NewState == EDIHeatActionState::Executing)
    {
        BeginPresentation();
        return;
    }

    if (PreviousState == EDIHeatActionState::Executing && NewState == EDIHeatActionState::None)
    {
        EndPresentation();
    }
}

void ADIHeatActionPresentationActor::HandleHeatActionHitProcessed(
    AActor* Target)
{
    if (!bPresentationActive || !IsValid(Target))
    {
        return;
    }

    APlayerController* PlayerController =
        UGameplayStatics::GetPlayerController(this, 0);

    if (IsValid(PlayerController) &&
        IsValid(PlayerController->PlayerCameraManager) &&
        HitCameraShakeClass)
    {
        PlayerController->PlayerCameraManager->StartCameraShake(
            HitCameraShakeClass,
            HitCameraShakeScale);
    }

    StartHitSlowMotion(Target);
}

void ADIHeatActionPresentationActor::StartHitSlowMotion(
    AActor* Target)
{
    if (HitSlowMotionDuration <= 0.0f)
    {
        return;
    }

    StopHitSlowMotion();

    bHitSlowMotionActive = true;

    PreviousGlobalTimeDilation =
        UGameplayStatics::GetGlobalTimeDilation(this);

    const float SafeTimeDilation =
        FMath::Clamp(
            HitSlowMotionScale,
            0.05f,
            1.0f);

    UGameplayStatics::SetGlobalTimeDilation(
        this,
        SafeTimeDilation);

    // Timer도 Global Time Dilation 영향을 받으므로
    // 실제 체감시간을 맞추기 위해 보정한다.
    const float TimerDuration =
        HitSlowMotionDuration * SafeTimeDilation;

    GetWorldTimerManager().SetTimer(
        HitSlowMotionTimerHandle,
        this,
        &ADIHeatActionPresentationActor::StopHitSlowMotion,
        TimerDuration,
        false);
}

void ADIHeatActionPresentationActor::StopHitSlowMotion()
{
    if (!bHitSlowMotionActive)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(
        HitSlowMotionTimerHandle);

    UGameplayStatics::SetGlobalTimeDilation(
        this,
        PreviousGlobalTimeDilation);

    PreviousGlobalTimeDilation = 1.0f;
    bHitSlowMotionActive = false;
}

void ADIHeatActionPresentationActor::BeginPresentation()
{
    if (bPresentationActive || !IsValid(Player) || !IsValid(HeatActionComponent))
    {
        return;
    }

    ADIPlayerController* PlayerController = Cast<ADIPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

    if (!IsValid(PlayerController))
    {
        return;
    }

    FVector DesiredLocation;
    FRotator DesiredRotation;

    if (!CalculateDesiredCamera(DesiredLocation,DesiredRotation))
    {
        return;
    }

    bPresentationActive = true;

    PreviousViewTarget = PlayerController->GetViewTarget();

    SetActorLocationAndRotation(DesiredLocation,DesiredRotation);

    PlayerController->SetHUDContext(EDIHUDContext::Cinematic);

    PlayerController->SetViewTargetWithBlend(this,BlendInTime,EViewTargetBlendFunction::VTBlend_Cubic);

    SetActorTickEnabled(true);
}

void ADIHeatActionPresentationActor::EndPresentation()
{
    if (!bPresentationActive)
    {
        return;
    }
    
    StopHitSlowMotion();

    bPresentationActive = false;
    SetActorTickEnabled(false);

    ADIPlayerController* PlayerController =
        Cast<ADIPlayerController>(
            UGameplayStatics::GetPlayerController(this, 0));

    if (!IsValid(PlayerController))
    {
        PreviousViewTarget = nullptr;
        return;
    }

    if (IsValid(PreviousViewTarget))
    {
        PlayerController->SetViewTargetWithBlend(PreviousViewTarget,BlendOutTime,EViewTargetBlendFunction::VTBlend_Cubic);
    }

    PlayerController->SetHUDContext(EDIHUDContext::Gameplay);

    PreviousViewTarget = nullptr;
}

bool ADIHeatActionPresentationActor::CalculateDesiredCamera(
    FVector& OutLocation,
    FRotator& OutRotation) const
{
    if (!IsValid(Player) ||
        !IsValid(HeatActionComponent))
    {
        return false;
    }

    AActor* Target = HeatActionComponent->GetTargetActor();

    if (!IsValid(Target))
    {
        return false;
    }

    const FVector PlayerLocation =
        Player->GetActorLocation();

    const FVector TargetLocation =
        Target->GetActorLocation();

    const FVector PlayerForward =
        Player->GetActorForwardVector().GetSafeNormal2D();

    const FVector PlayerRight =
        Player->GetActorRightVector().GetSafeNormal2D();

    if (PlayerForward.IsNearlyZero() ||
        PlayerRight.IsNearlyZero())
    {
        return false;
    }

    const FVector MidPoint =
        (PlayerLocation + TargetLocation) * 0.5f;

    const FVector FocusLocation =
        MidPoint +
        FVector::UpVector * FocusHeight;

    // ---------------------------------------------------------
    // Candidate 1
    // 기존 Heat Camera 구도
    // ---------------------------------------------------------
    const FVector PreferredCandidate =
        PlayerLocation
        - PlayerForward * BackOffset
        + PlayerRight * SideOffset
        + FVector::UpVector * HeightOffset;

    // ---------------------------------------------------------
    // Candidate 2
    // 반대편 Shoulder
    // preferred 쪽이 건물/벽으로 막혔을 때 사용
    // ---------------------------------------------------------
    const FVector OppositeCandidate =
        PlayerLocation
        - PlayerForward * BackOffset
        - PlayerRight * SideOffset
        + FVector::UpVector * HeightOffset;

    // ---------------------------------------------------------
    // Candidate 3
    // 골목 양쪽이 막혔을 때 위쪽에서 내려다보는 fallback
    // ---------------------------------------------------------
    const FVector HighCandidate =
        PlayerLocation
        - PlayerForward * FallbackBackOffset
        + FVector::UpVector * FallbackHeightOffset;

    const FVector Candidates[] =
    {
        PreferredCandidate,
        OppositeCandidate,
        HighCandidate
    };

    FVector BestFallbackLocation = PreferredCandidate;
    float BestFallbackDistanceSquared = -1.0f;

    for (const FVector& Candidate : Candidates)
    {
        FVector SafeLocation;

        const bool bClear = TraceCameraCandidate(FocusLocation,Candidate,Target,SafeLocation);

        // 완전히 안 막힌 후보 발견
        // 우선순위:
        // 기존 구도 → 반대편 → 높은 구도
        if (bClear)
        {
            OutLocation = SafeLocation;
            OutRotation = (FocusLocation - OutLocation).Rotation();

            return true;
        }

        // 전부 막힌 상황에 대비.벽 때문에 당겨진 SafeLocation 중 Focus에서 가장 멀리 확보되는 위치를 기억.
        const float SafeDistanceSquared = FVector::DistSquared(FocusLocation,SafeLocation);

        if (SafeDistanceSquared > BestFallbackDistanceSquared)
        {
            BestFallbackDistanceSquared = SafeDistanceSquared;

            BestFallbackLocation = SafeLocation;
        }
    }

    // 3방향 모두 벽에 막혀도
    // 건물 뒤에 카메라를 두지 않고
    // 가장 공간이 많이 확보된 안전 위치 사용.
    OutLocation = BestFallbackLocation;

    OutRotation = (FocusLocation - OutLocation).Rotation();

    return true;
}

bool ADIHeatActionPresentationActor::TraceCameraCandidate(
    const FVector& FocusLocation,
    const FVector& CandidateLocation,
    AActor* Target,
    FVector& OutSafeLocation) const
{
    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        OutSafeLocation = CandidateLocation;
        return true;
    }

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(HeatActionCameraOcclusion),
        false);

    QueryParams.AddIgnoredActor(this);

    if (IsValid(Player))
    {
        QueryParams.AddIgnoredActor(Player);
    }

    if (IsValid(Target))
    {
        QueryParams.AddIgnoredActor(Target);
    }

    FHitResult Hit;

    const bool bBlocked =
        World->SweepSingleByChannel(
            Hit,
            FocusLocation,
            CandidateLocation,
            FQuat::Identity,
            ECC_Visibility,
            FCollisionShape::MakeSphere(
                CameraCollisionRadius),
            QueryParams);

    if (!bBlocked)
    {
        OutSafeLocation = CandidateLocation;
        return true;
    }

    const FVector TraceDirection =
        (CandidateLocation - FocusLocation)
        .GetSafeNormal();

    // Sphere Sweep이 벽에 닿은 위치에서
    // Focus 쪽으로 약간 더 당겨
    // 벽 surface에 카메라가 딱 붙는 것 방지.
    OutSafeLocation =
        Hit.Location
        - TraceDirection * CameraWallPadding;

    return false;
}