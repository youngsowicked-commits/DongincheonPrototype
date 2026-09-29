#include "Animation/AnimNotifyState_DIAttackHitWindow.h"

#include "Components/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_DIAttackHitWindow::NotifyBegin(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    float TotalDuration,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(
        MeshComp,
        Animation,
        TotalDuration,
        EventReference);

    if (!IsValid(MeshComp) ||
        HitSocketName.IsNone() ||
        !MeshComp->DoesSocketExist(HitSocketName))
    {
        return;
    }

    PreviousSocketLocations.Add(
        MeshComp,
        MeshComp->GetSocketLocation(HitSocketName));
}

void UAnimNotifyState_DIAttackHitWindow::NotifyTick(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    float FrameDeltaTime,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(
        MeshComp,
        Animation,
        FrameDeltaTime,
        EventReference);

    if (!IsValid(MeshComp) ||
        HitSocketName.IsNone() ||
        !MeshComp->DoesSocketExist(HitSocketName))
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();

    if (!IsValid(Owner))
    {
        return;
    }

    UCombatComponent* CombatComponent =
        Owner->FindComponentByClass<UCombatComponent>();

    if (!IsValid(CombatComponent) ||
        !CombatComponent->IsAttackActive())
    {
        return;
    }

    const FVector CurrentSocketLocation =
        MeshComp->GetSocketLocation(HitSocketName);

    FVector* PreviousSocketLocation =
        PreviousSocketLocations.Find(MeshComp);

    if (!PreviousSocketLocation)
    {
        PreviousSocketLocations.Add(
            MeshComp,
            CurrentSocketLocation);

        return;
    }

    CombatComponent->ProcessSocketTrajectoryHit(
    HitSocketName,
    *PreviousSocketLocation,
    CurrentSocketLocation);

    *PreviousSocketLocation =
        CurrentSocketLocation;
}

void UAnimNotifyState_DIAttackHitWindow::NotifyEnd(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(
        MeshComp,
        Animation,
        EventReference);

    if (IsValid(MeshComp))
    {
        PreviousSocketLocations.Remove(MeshComp);
    }
}

FString UAnimNotifyState_DIAttackHitWindow::GetNotifyName_Implementation() const
{
    return TEXT("DI Attack Hit Window");
}