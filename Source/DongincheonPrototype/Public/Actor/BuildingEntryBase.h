// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "BuildingEntryBase.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class ACharacter;

UCLASS()
class DONGINCHEONPROTOTYPE_API ABuildingEntryBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABuildingEntryBase();

protected:
	virtual void BeginPlay() override;
	
	void StartEntry(ACharacter* Character);
	
	void LockPlayerInput();
	
	void AlignPlayerToEntryPoint();
	
	UFUNCTION(BlueprintCallable, Category = "Entry")
	void MovePlayerToInsidePoint();
	
	UFUNCTION(BlueprintCallable, Category = "Entry")
	void FinishEntry();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Entry")
	void OnEntryStarted();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Entry")
	void OnEntryFinished();
	
	UFUNCTION()
	void HandleEntryTriggerBeginOverlap
	(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
	);
	
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> ActiveCharacter = nullptr;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entry")
	TObjectPtr<USceneComponent> SceneRoot;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entry")
	TObjectPtr<UBoxComponent> EntryTrigger;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entry")
	TObjectPtr<USceneComponent> EntryPoint;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entry")
	TObjectPtr<USceneComponent> InsidePoint;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Entry")
	bool bEntryInProgress = false;
};
