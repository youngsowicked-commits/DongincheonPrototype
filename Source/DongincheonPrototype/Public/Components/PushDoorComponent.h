// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PushDoorComponent.generated.h"


class USceneComponent;
class UBoxComponent;
class UPrimitiveComponent;
class ACharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DONGINCHEONPROTOTYPE_API UPushDoorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPushDoorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void InitializeDoor(USceneComponent* InDoorPivot, UBoxComponent* InPushTrigger);
	
protected:
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> DoorPivot = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UBoxComponent> PushTrigger = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> PushingCharacter = nullptr;
	
	float CurrentDoorAngle = 0.0f;
	float CurrentAngularVelocity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float MaxOpenAngle = 135.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float PushAngularAcceleration = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float AngularDamping = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float MaxAngularSpeed = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float CloseSpringStrength = 8.0f;

	UFUNCTION()
	void HandlePushTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void HandlePushTriggerEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);
};
