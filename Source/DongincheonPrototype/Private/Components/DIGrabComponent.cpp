#include "Components/DIGrabComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Actor.h"

UDIGrabComponent::UDIGrabComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UDIGrabComponent::CanStartGrab(AActor* Target) const
{
    const AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor) ||
        !IsValid(Target) ||
        Target == OwnerActor ||
        GrabState != EDIGrabState::None)
    {
        return false;
    }

    const UDIGrabComponent* TargetGrabComponent =
        Target->FindComponentByClass<UDIGrabComponent>();

    if (!IsValid(TargetGrabComponent) ||
        TargetGrabComponent == this)
    {
        return false;
    }

    return TargetGrabComponent->CanAcceptGrab(
        const_cast<AActor*>(OwnerActor));
}

bool UDIGrabComponent::BeginGrab(AActor* Target)
{
    if (!CanStartGrab(Target))
    {
        return false;
    }

    AActor* OwnerActor = GetOwner();

    UDIGrabComponent* TargetGrabComponent =
        Target->FindComponentByClass<UDIGrabComponent>();

    if (!IsValid(OwnerActor) ||
        !IsValid(TargetGrabComponent))
    {
        return false;
    }

    if (!TargetGrabComponent->AcceptGrab(OwnerActor))
    {
        return false;
    }

    GrabbedActor = Target;
    GrabbedByActor.Reset();

    SetGrabState(EDIGrabState::Starting);

    return true;
}

void UDIGrabComponent::CompleteGrabStart(float HoldDistance,float VictimYawOffset)
{
    if (GrabState != EDIGrabState::Starting) return;
    if (!GrabbedActor.IsValid()) { ForceRelease(); return; }

    AActor* OwnerActor = GetOwner();
    AActor* TargetActor = GrabbedActor.Get();

    if (!IsValid(OwnerActor) || !IsValid(TargetActor))
    {
        ForceRelease();
        return;
    }

    FVector TargetLocation = OwnerActor->GetActorLocation() + OwnerActor->GetActorForwardVector() * FMath::Max(0.0f,HoldDistance);
    TargetLocation.Z = TargetActor->GetActorLocation().Z;

    FRotator TargetRotation = OwnerActor->GetActorRotation();
    TargetRotation.Yaw += VictimYawOffset;

    TargetActor->SetActorLocationAndRotation(TargetLocation,TargetRotation,false,nullptr,ETeleportType::TeleportPhysics);
    TargetActor->AttachToActor(OwnerActor,FAttachmentTransformRules::KeepWorldTransform);

    SetGrabState(EDIGrabState::Holding);
}

bool UDIGrabComponent::BeginGrabAttack(float Damage)
{
    if (GrabState != EDIGrabState::Holding || !GrabbedActor.IsValid())
    {
        return false;
    }

    ActiveGrabAttackDamage = FMath::Max(0.0f, Damage);

    SetGrabState(EDIGrabState::Attacking);

    return true;
}

void UDIGrabComponent::ProcessGrabAttackHit()
{
    if (GrabState != EDIGrabState::Attacking || !GrabbedActor.IsValid() || ActiveGrabAttackDamage <= 0.0f)
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    AActor* TargetActor = GrabbedActor.Get();

    if (!IsValid(OwnerActor) || !IsValid(TargetActor))
    {
        return;
    }

    const float AppliedDamage = UGameplayStatics::ApplyDamage(TargetActor,ActiveGrabAttackDamage,OwnerActor->GetInstigatorController(),
            OwnerActor,UDamageType::StaticClass());

    if (AppliedDamage <= 0.0f)
    {
        return;
    }

    if (UDIGrabComponent* TargetGrabComponent = TargetActor->FindComponentByClass<UDIGrabComponent>())
    {
        TargetGrabComponent->ReceiveGrabAttackHit(AppliedDamage,OwnerActor);
    }
}

void UDIGrabComponent::ReceiveGrabAttackHit(float Damage,AActor* DamageCauser)
{
    if (GrabState != EDIGrabState::BeingGrabbed)
    {
        return;
    }

    OnGrabAttackReceived.Broadcast(Damage,DamageCauser);
}

void UDIGrabComponent::EndGrabAttack()
{
    if (GrabState != EDIGrabState::Attacking)
    {
        return;
    }

    if (!GrabbedActor.IsValid())
    {
        ForceRelease();
        return;
    }
    
    ActiveGrabAttackDamage = 0.0f;
    
    SetGrabState(EDIGrabState::Holding);
}

bool UDIGrabComponent::BeginReleaseGrab()
{
    if (!IsGrabbing() ||
        GrabState == EDIGrabState::Releasing)
    {
        return false;
    }

    if (!GrabbedActor.IsValid())
    {
        ForceRelease();
        return false;
    }

    SetGrabState(EDIGrabState::Releasing);

    return true;
}

void UDIGrabComponent::CompleteReleaseGrab()
{
    if (GrabState != EDIGrabState::Releasing)
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    AActor* TargetActor = GrabbedActor.Get();

    if (IsValid(TargetActor))
    {
        if (UDIGrabComponent* TargetGrabComponent =
            TargetActor->FindComponentByClass<UDIGrabComponent>())
        {
            if (TargetGrabComponent->GrabbedByActor.Get() == OwnerActor)
            {
                TargetGrabComponent->ClearGrabState();
            }
        }
    }

    ClearGrabState();
}

void UDIGrabComponent::ForceRelease()
{
    AActor* OwnerActor = GetOwner();

    // 내가 잡고 있는 상대 정리
    if (AActor* TargetActor = GrabbedActor.Get())
    {
        if (UDIGrabComponent* TargetGrabComponent =
            TargetActor->FindComponentByClass<UDIGrabComponent>())
        {
            if (TargetGrabComponent->GrabbedByActor.Get() == OwnerActor)
            {
                TargetGrabComponent->ClearGrabState();
            }
        }
    }

    // 나를 잡고 있는 상대 정리
    if (AActor* GrabberActor = GrabbedByActor.Get())
    {
        if (UDIGrabComponent* GrabberGrabComponent = GrabberActor->FindComponentByClass<UDIGrabComponent>())
        {
            if (GrabberGrabComponent->GrabbedActor.Get() == OwnerActor)
            {
                GrabberGrabComponent->ClearGrabState();
            }
        }
    }

    ClearGrabState();
}

void UDIGrabComponent::SetCanBeGrabbed(bool bEnabled)
{
    bCanBeGrabbed = bEnabled;

    if (!bCanBeGrabbed && IsBeingGrabbed())
    {
        ForceRelease();
    }
}

bool UDIGrabComponent::CanBeGrabbed() const
{
    return bCanBeGrabbed;
}

bool UDIGrabComponent::IsGrabbing() const
{
    return GrabState == EDIGrabState::Starting || GrabState == EDIGrabState::Holding || GrabState 
    == EDIGrabState::Attacking || GrabState == EDIGrabState::Releasing;
}

bool UDIGrabComponent::IsHoldingGrab() const
{
    return GrabState == EDIGrabState::Holding;
}

bool UDIGrabComponent::IsGrabAttacking() const
{
    return GrabState == EDIGrabState::Attacking;
}

bool UDIGrabComponent::IsBeingGrabbed() const
{
    return GrabState == EDIGrabState::BeingGrabbed;
}

AActor* UDIGrabComponent::GetGrabbedActor() const
{
    return GrabbedActor.Get();
}

AActor* UDIGrabComponent::GetGrabbedByActor() const
{
    return GrabbedByActor.Get();
}

EDIGrabState UDIGrabComponent::GetGrabState() const
{
    return GrabState;
}

bool UDIGrabComponent::CanAcceptGrab(AActor* Grabber) const
{
    const AActor* OwnerActor = GetOwner();

    return bCanBeGrabbed && IsValid(OwnerActor) && IsValid(Grabber) && Grabber != OwnerActor &&
           GrabState == EDIGrabState::None;
}

bool UDIGrabComponent::AcceptGrab(AActor* Grabber)
{
    if (!CanAcceptGrab(Grabber))
    {
        return false;
    }

    GrabbedActor.Reset();
    GrabbedByActor = Grabber;

    SetGrabState(EDIGrabState::BeingGrabbed);

    return true;
}

void UDIGrabComponent::SetGrabState(EDIGrabState NewState)
{
    if (GrabState == NewState)
    {
        return;
    }

    const EDIGrabState PreviousState = GrabState;

    GrabState = NewState;

    OnGrabStateChanged.Broadcast(PreviousState,GrabState);
}

void UDIGrabComponent::ClearGrabState()
{
    DetachFromGrabber();

    GrabbedActor.Reset();
    GrabbedByActor.Reset();
    ActiveGrabAttackDamage = 0.0f;

    SetGrabState(EDIGrabState::None);
}

void UDIGrabComponent::DetachFromGrabber()
{
    if (GrabState != EDIGrabState::BeingGrabbed) return;

    AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor)) return;

    if (OwnerActor->GetAttachParentActor() == GrabbedByActor.Get())
    {
        OwnerActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    }
}