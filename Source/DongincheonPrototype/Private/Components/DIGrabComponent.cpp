#include "Components/DIGrabComponent.h"

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

void UDIGrabComponent::CompleteGrabStart()
{
    if (GrabState != EDIGrabState::Starting)
    {
        return;
    }

    if (!GrabbedActor.IsValid())
    {
        ForceRelease();
        return;
    }

    SetGrabState(EDIGrabState::Holding);
}

bool UDIGrabComponent::BeginGrabAttack()
{
    if (GrabState != EDIGrabState::Holding ||
        !GrabbedActor.IsValid())
    {
        return false;
    }

    SetGrabState(EDIGrabState::Attacking);

    return true;
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
    GrabbedActor.Reset();
    GrabbedByActor.Reset();

    SetGrabState(EDIGrabState::None);
}