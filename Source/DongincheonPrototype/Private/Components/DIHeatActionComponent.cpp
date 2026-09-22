#include "Components/DIHeatActionComponent.h"

#include "Components/HealthComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

UDIHeatActionComponent::UDIHeatActionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

AActor* UDIHeatActionComponent::FindBestHeatActionTarget(const FHeatActionConfig& Config) const
{
    AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor) || !IsValid(GetWorld()) || Config.MaxDistance <= 0.0f)
    {
        return nullptr;
    }

    TArray<FOverlapResult> Overlaps;

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HeatActionTargetSearch),false,OwnerActor);

    const bool bFoundAny = GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        OwnerActor->GetActorLocation(),
        FQuat::Identity,
        ObjectQueryParams,
        FCollisionShape::MakeSphere(Config.MaxDistance),
        QueryParams);

    if (!bFoundAny)
    {
        return nullptr;
    }

    AActor* BestTarget = nullptr;
    float BestDistanceSquared = TNumericLimits<float>::Max();

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Candidate = Overlap.GetActor();

        if (!IsValidHeatActionTarget(Candidate,Config))
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared2D(OwnerActor->GetActorLocation(),Candidate->GetActorLocation());

        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestTarget = Candidate;
        }
    }

    return BestTarget;
}

bool UDIHeatActionComponent::CanStartHeatAction(AActor* Target,const FHeatActionConfig& Config) const
{
    if (HeatActionState != EDIHeatActionState::None)
    {
        return false;
    }

    if (Config.Type == EDIHeatActionType::None || !IsValid(Config.PlayerMontage))
    {
        return false;
    }

    return IsValidHeatActionTarget(Target,Config);
}

bool UDIHeatActionComponent::BeginHeatAction(AActor* Target,const FHeatActionConfig& Config)
{
    if (!CanStartHeatAction(Target,Config))
    {
        return false;
    }

    AActor* OwnerActor = GetOwner();
    UDIHeatActionComponent* TargetHeatActionComponent = Target->FindComponentByClass<UDIHeatActionComponent>();

    if (!IsValid(OwnerActor) || !IsValid(TargetHeatActionComponent))
    {
        return false;
    }

    if (!TargetHeatActionComponent->AcceptHeatAction(OwnerActor,Config))
    {
        return false;
    }

    TargetActor = Target;
    SourceActor.Reset();
    ActiveConfig = Config;
    bHitProcessed = false;

    SetHeatActionState(EDIHeatActionState::Executing);
    return true;
}

void UDIHeatActionComponent::ProcessHeatActionHit()
{
    if (HeatActionState != EDIHeatActionState::Executing || bHitProcessed || !TargetActor.IsValid())
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    AActor* Target = TargetActor.Get();

    if (!IsValid(OwnerActor) || !IsValid(Target) || ActiveConfig.Damage <= 0.0f)
    {
        return;
    }

    bHitProcessed = true;

    UGameplayStatics::ApplyDamage(
        Target,
        ActiveConfig.Damage,
        OwnerActor->GetInstigatorController(),
        OwnerActor,
        UDamageType::StaticClass());
}

void UDIHeatActionComponent::CompleteHeatAction()
{
    if (HeatActionState != EDIHeatActionState::Executing)
    {
        return;
    }

    ReleaseTarget();
    ClearHeatActionState();
}

void UDIHeatActionComponent::CancelHeatAction()
{
    if (HeatActionState == EDIHeatActionState::None)
    {
        return;
    }

    if (HeatActionState == EDIHeatActionState::Executing)
    {
        ReleaseTarget();
    }

    if (HeatActionState == EDIHeatActionState::BeingVictim)
    {
        AActor* OwnerActor = GetOwner();
        AActor* Source = SourceActor.Get();

        if (IsValid(Source))
        {
            if (UDIHeatActionComponent* SourceComponent = Source->FindComponentByClass<UDIHeatActionComponent>())
            {
                if (SourceComponent->TargetActor.Get() == OwnerActor)
                {
                    SourceComponent->ClearHeatActionState();
                }
            }
        }
    }

    ClearHeatActionState();
}

void UDIHeatActionComponent::ReleaseTarget()
{
    AActor* OwnerActor = GetOwner();
    AActor* Target = TargetActor.Get();

    if (!IsValid(OwnerActor) || !IsValid(Target))
    {
        return;
    }

    if (UDIHeatActionComponent* TargetComponent = Target->FindComponentByClass<UDIHeatActionComponent>())
    {
        if (TargetComponent->SourceActor.Get() == OwnerActor)
        {
            TargetComponent->ClearHeatActionState();
        }
    }
}

bool UDIHeatActionComponent::IsExecuting() const
{
    return HeatActionState == EDIHeatActionState::Executing;
}

bool UDIHeatActionComponent::IsBeingVictim() const
{
    return HeatActionState == EDIHeatActionState::BeingVictim;
}

AActor* UDIHeatActionComponent::GetTargetActor() const
{
    return TargetActor.Get();
}

AActor* UDIHeatActionComponent::GetSourceActor() const
{
    return SourceActor.Get();
}

EDIHeatActionState UDIHeatActionComponent::GetHeatActionState() const
{
    return HeatActionState;
}

EDIHeatActionType UDIHeatActionComponent::GetActiveType() const
{
    return ActiveConfig.Type;
}

const FHeatActionConfig& UDIHeatActionComponent::GetActiveConfig() const
{
    return ActiveConfig;
}

bool UDIHeatActionComponent::IsValidHeatActionTarget(AActor* Target,const FHeatActionConfig& Config) const
{
    AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor) || !IsValid(Target) || Target == OwnerActor)
    {
        return false;
    }

    UHealthComponent* TargetHealth = Target->FindComponentByClass<UHealthComponent>();

    if (!IsValid(TargetHealth) || TargetHealth->IsDead())
    {
        return false;
    }
    
    UDIHeatActionComponent* TargetHeatActionComponent = Target->FindComponentByClass<UDIHeatActionComponent>();

    if (!IsValid(TargetHeatActionComponent) || !TargetHeatActionComponent->CanAcceptHeatAction(OwnerActor,Config))
    {
        return false;
    }

    const FVector ToTarget = Target->GetActorLocation() - OwnerActor->GetActorLocation();

    if (ToTarget.SizeSquared2D() > FMath::Square(Config.MaxDistance))
    {
        return false;
    }

    const FVector Direction = ToTarget.GetSafeNormal2D();

    if (Direction.IsNearlyZero())
    {
        return false;
    }

    const FVector Forward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
    return FVector::DotProduct(Forward,Direction) >= Config.MinForwardDot;
}

bool UDIHeatActionComponent::CanAcceptHeatAction(AActor* Source,const FHeatActionConfig& Config) const
{
    AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor) || !IsValid(Source) || Source == OwnerActor)
    {
        return false;
    }

    if (Config.Type == EDIHeatActionType::None)
    {
        return false;
    }

    return HeatActionState == EDIHeatActionState::None;
}

bool UDIHeatActionComponent::AcceptHeatAction(AActor* Source,const FHeatActionConfig& Config)
{
    if (!CanAcceptHeatAction(Source,Config))
    {
        return false;
    }

    TargetActor.Reset();
    SourceActor = Source;
    ActiveConfig = Config;
    bHitProcessed = false;

    SetHeatActionState(EDIHeatActionState::BeingVictim);
    return true;
}

void UDIHeatActionComponent::SetHeatActionState(EDIHeatActionState NewState)
{
    if (HeatActionState == NewState)
    {
        return;
    }

    const EDIHeatActionState PreviousState = HeatActionState;
    HeatActionState = NewState;

    OnHeatActionStateChanged.Broadcast(PreviousState,HeatActionState);
}

void UDIHeatActionComponent::ClearHeatActionState()
{
    TargetActor.Reset();
    SourceActor.Reset();
    ActiveConfig = FHeatActionConfig();
    bHitProcessed = false;

    SetHeatActionState(EDIHeatActionState::None);
}