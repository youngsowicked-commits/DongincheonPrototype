// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PushDoorBase.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

UCLASS()
class DONGINCHEONPROTOTYPE_API APushDoorBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APushDoorBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<USceneComponent> SceneRoot;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<USceneComponent> DoorPivot;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UBoxComponent> PushTrigger;
	
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> PushingCharacter = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float MaxOpenAngle = 135.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	
	float PushOpenSpeed = 100.0f;
	
	float CurrentDoorAngle = 0.0f;
	
	UFUNCTION()
	void HandlePushTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
	UFUNCTION()
	void HandlePushTriggerEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);
	
public:
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
