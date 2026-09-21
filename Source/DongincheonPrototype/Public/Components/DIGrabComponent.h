#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DIGrabComponent.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EDIGrabState : uint8
{
    None,

    // 내가 상대를 잡는 쪽
    Starting,
    Holding,
    Attacking,
    Releasing,

    // 내가 상대에게 잡힌 쪽
    BeingGrabbed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FDIGrabStateChangedSignature,
    EDIGrabState, PreviousState,
    EDIGrabState, NewState);

UCLASS(ClassGroup=(DI), meta=(BlueprintSpawnableComponent))
class DONGINCHEONPROTOTYPE_API UDIGrabComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UDIGrabComponent();

    // Grab Lifecycle
    bool CanStartGrab(AActor* Target) const;

    bool BeginGrab(AActor* Target);

    void CompleteGrabStart();

    bool BeginGrabAttack();

    void EndGrabAttack();

    bool BeginReleaseGrab();

    void CompleteReleaseGrab();

    void ForceRelease();

    // Availability
    UFUNCTION(BlueprintCallable, Category = "Grab")
    void SetCanBeGrabbed(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "Grab")
    bool CanBeGrabbed() const;

    // Queries
    UFUNCTION(BlueprintPure, Category = "Grab")
    bool IsGrabbing() const;

    UFUNCTION(BlueprintPure, Category = "Grab")
    bool IsHoldingGrab() const;

    UFUNCTION(BlueprintPure, Category = "Grab")
    bool IsGrabAttacking() const;

    UFUNCTION(BlueprintPure, Category = "Grab")
    bool IsBeingGrabbed() const;

    UFUNCTION(BlueprintPure, Category = "Grab")
    AActor* GetGrabbedActor() const;

    UFUNCTION(BlueprintPure, Category = "Grab")
    AActor* GetGrabbedByActor() const;

    UFUNCTION(BlueprintPure, Category = "Grab")
    EDIGrabState GetGrabState() const;

    UPROPERTY(BlueprintAssignable, Category = "Grab")
    FDIGrabStateChangedSignature OnGrabStateChanged;

private:
    bool CanAcceptGrab(AActor* Grabber) const;

    bool AcceptGrab(AActor* Grabber);

    void SetGrabState(EDIGrabState NewState);

    void ClearGrabState();

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grab",
        meta = (AllowPrivateAccess = "true"))
    bool bCanBeGrabbed = true;

    // Runtime relationship은 소유 관계가 아니므로 Weak reference.
    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> GrabbedActor;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> GrabbedByActor;

    UPROPERTY(Transient)
    EDIGrabState GrabState = EDIGrabState::None;
};