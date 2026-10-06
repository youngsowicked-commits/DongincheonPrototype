#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "Components/DIHeatActionComponent.h"
#include "UI/DIHUDTypes.h"
#include "DIHeatActionPresentationActor.generated.h"

class ADongincheonCharacter;
class ADIPlayerController;
class UCameraShakeBase;

UCLASS()
class DONGINCHEONPROTOTYPE_API ADIHeatActionPresentationActor : public ACameraActor
{
    GENERATED_BODY()

public:
    ADIHeatActionPresentationActor();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION()
    void HandleHeatActionStateChanged(
        EDIHeatActionState PreviousState,
        EDIHeatActionState NewState);
    
    UFUNCTION()
    void HandleHeatActionHitProcessed(AActor* Target);
    
    void StartHitSlowMotion(AActor* Target);
    void StopHitSlowMotion();

    void BeginPresentation();
    void EndPresentation();

    bool CalculateDesiredCamera(FVector& OutLocation,FRotator& OutRotation) const;
    
    bool TraceCameraCandidate(const FVector& FocusLocation,const FVector& CandidateLocation,AActor* Target,FVector& OutSafeLocation) const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera")
    float BlendInTime = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera")
    float BlendOutTime = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera")
    float BackOffset = 240.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera")
    float SideOffset = 110.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera")
    float HeightOffset = 110.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera")
    float FocusHeight = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera")
    float LocationInterpSpeed = 14.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera")
    float RotationInterpSpeed = 18.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera|Occlusion",meta=(ClampMin="1.0"))
    float CameraCollisionRadius = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera|Occlusion",meta=(ClampMin="0.0"))
    float CameraWallPadding = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera|Occlusion",meta=(ClampMin="0.0"))
    float FallbackBackOffset = 170.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Camera|Occlusion",meta=(ClampMin="0.0"))
    float FallbackHeightOffset = 180.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Hit Feedback")
    TSubclassOf<UCameraShakeBase> HitCameraShakeClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Hit Feedback",meta=(ClampMin="0.0"))
    float HitCameraShakeScale = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Hit Feedback",meta=(ClampMin="0.05", ClampMax="1.0"))
    float HitSlowMotionScale = 0.30f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Heat Action|Hit Feedback",meta=(ClampMin="0.0"))
    float HitSlowMotionDuration = 0.10f;

private:
    UPROPERTY(Transient)
    TObjectPtr<ADongincheonCharacter> Player;

    UPROPERTY(Transient)
    TObjectPtr<UDIHeatActionComponent> HeatActionComponent;

    UPROPERTY(Transient)
    TObjectPtr<AActor> PreviousViewTarget;
    
    bool bPresentationActive = false;
    
    FTimerHandle HitSlowMotionTimerHandle;

    float PreviousGlobalTimeDilation = 1.0f;

    bool bHitSlowMotionActive = false;
};