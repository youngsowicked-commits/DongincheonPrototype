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

void UDIHeatActionComponent::AddHeat(float Amount)
{
    if (Amount <= 0.0f || MaxHeat <= 0.0f)
    {
        return;
    }

    const float PreviousHeat = CurrentHeat;
    CurrentHeat = FMath::Clamp(CurrentHeat + Amount,0.0f,MaxHeat);

    if (!FMath::IsNearlyEqual(PreviousHeat,CurrentHeat))
    {
        OnHeatChanged.Broadcast(CurrentHeat,MaxHeat);
    }
}

bool UDIHeatActionComponent::HasEnoughHeat(float Cost) const
{
    return Cost <= 0.0f || CurrentHeat >= Cost;
}

AActor* UDIHeatActionComponent::FindBestHeatActionTarget(const FGameplayTag& ActionTag,
    const FHeatActionConfig& Config) const
{
    AActor* OwnerActor = GetOwner();

    if (!ActionTag.IsValid() || !IsValid(OwnerActor) || !IsValid(GetWorld()) || Config.MaxDistance <= 0.0f)
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

        if (!IsValidHeatActionTarget(Candidate,ActionTag,Config))
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

bool UDIHeatActionComponent::CanStartHeatAction(AActor* Target,const FGameplayTag& ActionTag,
    const FHeatActionConfig& Config) const
{
    if (HeatActionState != EDIHeatActionState::None)
    {
        return false;
    }

    if (!ActionTag.IsValid() || !IsValid(Config.PlayerMontage) || !IsValid(Config.VictimMontage))
    {
        return false;
    }

    if (!HasEnoughHeat(Config.HeatCost))
    {
        return false;
    }

    return IsValidHeatActionTarget(Target,ActionTag,Config);
}

bool UDIHeatActionComponent::BeginHeatAction(AActor* Target,const FGameplayTag& ActionTag,const FHeatActionConfig& Config)
{
    if (!CanStartHeatAction(Target,ActionTag,Config))
    {
        return false;
    }

    AActor* OwnerActor = GetOwner();
    UDIHeatActionComponent* TargetHeatActionComponent = Target->FindComponentByClass<UDIHeatActionComponent>();

    if (!IsValid(OwnerActor) || !IsValid(TargetHeatActionComponent))
    {
        return false;
    }

    if (!TargetHeatActionComponent->AcceptHeatAction(OwnerActor,ActionTag,Config))
    {
        return false;
    }

    if (!ConsumeHeat(Config.HeatCost))
    {
        TargetHeatActionComponent->CancelHeatAction();
        return false;
    }

    TargetActor = Target;
    SourceActor.Reset();
    ActiveActionTag = ActionTag;
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

FGameplayTag UDIHeatActionComponent::GetActiveActionTag() const
{
    return ActiveActionTag;
}

float UDIHeatActionComponent::GetCurrentHeat() const
{
    return CurrentHeat;
}

float UDIHeatActionComponent::GetMaxHeat() const
{
    return MaxHeat;
}

float UDIHeatActionComponent::GetHeatNormalized() const
{
    if (MaxHeat <= 0.0f)
    {
        return 0.0f;
    }

    return FMath::Clamp(CurrentHeat / MaxHeat,0.0f,1.0f);
}

const FHeatActionConfig& UDIHeatActionComponent::GetActiveConfig() const
{
    return ActiveConfig;
}

bool UDIHeatActionComponent::IsValidHeatActionTarget(AActor* Target,const FGameplayTag& ActionTag,
    const FHeatActionConfig& Config) const
{
    AActor* OwnerActor = GetOwner();

    if (!ActionTag.IsValid() || !IsValid(OwnerActor) || !IsValid(Target) || Target == OwnerActor)
    {
        return false;
    }

    UHealthComponent* TargetHealth = Target->FindComponentByClass<UHealthComponent>();

    if (!IsValid(TargetHealth) || TargetHealth->IsDead())
    {
        return false;
    }

    UDIHeatActionComponent* TargetHeatActionComponent = Target->FindComponentByClass<UDIHeatActionComponent>();

    if (!IsValid(TargetHeatActionComponent) || !TargetHeatActionComponent->CanAcceptHeatAction(OwnerActor,ActionTag,Config))
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

bool UDIHeatActionComponent::CanAcceptHeatAction(AActor* Source,const FGameplayTag& ActionTag,
    const FHeatActionConfig& Config) const
{
    (void)Config;

    AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor) || !IsValid(Source) || Source == OwnerActor)
    {
        return false;
    }

    if (!ActionTag.IsValid())
    {
        return false;
    }

    return HeatActionState == EDIHeatActionState::None;
}

bool UDIHeatActionComponent::AcceptHeatAction(AActor* Source,const FGameplayTag& ActionTag,
    const FHeatActionConfig& Config)
{
    if (!CanAcceptHeatAction(Source,ActionTag,Config))
    {
        return false;
    }

    TargetActor.Reset();
    SourceActor = Source;
    ActiveActionTag = ActionTag;
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
    ActiveActionTag = FGameplayTag();
    ActiveConfig = FHeatActionConfig();
    bHitProcessed = false;

    SetHeatActionState(EDIHeatActionState::None);
}

bool UDIHeatActionComponent::ConsumeHeat(float Amount)
{
    if (Amount <= 0.0f)
    {
        return true;
    }

    if (!HasEnoughHeat(Amount))
    {
        return false;
    }

    CurrentHeat = FMath::Clamp(CurrentHeat - Amount,0.0f,MaxHeat);
    OnHeatChanged.Broadcast(CurrentHeat,MaxHeat);

    return true;
}