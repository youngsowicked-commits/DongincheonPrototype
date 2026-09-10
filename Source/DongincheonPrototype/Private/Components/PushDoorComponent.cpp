// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/PushDoorComponent.h"

#include "Components/BoxComponent.h"

#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

#include "Engine/Engine.h"

// Sets default values for this component's properties
UPushDoorComponent::UPushDoorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// ...
}


// Called when the game starts
void UPushDoorComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UPushDoorComponent::TickComponent(
	float DeltaTime, 
	ELevelTick TickType, 
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(DoorPivot))
	{
		return;
	}
	
	// 플레이어가 Trigger 안에 있으면 미는 힘 계산
	if (IsValid(PushingCharacter))
	{
		FVector ToPlayer = PushingCharacter->GetActorLocation() - DoorPivot->GetComponentLocation();
		
		FVector MoveDirection = PushingCharacter->GetVelocity();
		
		ToPlayer.Z = 0.0f;
		MoveDirection.Z = 0.0f;
		
		if (!ToPlayer.IsNearlyZero() && !MoveDirection.IsNearlyZero())
		{
			ToPlayer.Normalize();
			MoveDirection.Normalize();
			
			const float PushTorque = FVector::CrossProduct(ToPlayer, MoveDirection).Z;
			
			CurrentAngularVelocity += PushTorque * PushAngularAcceleration * DeltaTime;
		}
	}
	else
	{
		//플레이어가 떠났으면 닫힌 각도(0도) 쪽으로 복귀
		CurrentAngularVelocity += (-CurrentDoorAngle * CloseSpringStrength) * DeltaTime;
	}
	
	CurrentAngularVelocity = FMath::Clamp(CurrentAngularVelocity, -MaxAngularSpeed, MaxAngularSpeed);
	
	CurrentAngularVelocity = FMath::FInterpTo(CurrentAngularVelocity, 0.0f, DeltaTime, AngularDamping);
	
	CurrentDoorAngle += CurrentAngularVelocity * DeltaTime;
	
	if (CurrentDoorAngle >= MaxOpenAngle)
	{
		CurrentDoorAngle = MaxOpenAngle;
		
		if (CurrentAngularVelocity > 0.0f)
		{
			CurrentAngularVelocity = 0.0f;
		}
	}
	else if (CurrentDoorAngle <= -MaxOpenAngle)
	{
		CurrentDoorAngle = -MaxOpenAngle;
		
		if (CurrentAngularVelocity < 0.0f)
		{
			CurrentAngularVelocity = 0.0f;
		}
	}
	
	DoorPivot->SetRelativeRotation(FRotator(0.0f,CurrentDoorAngle,0.0f));
	
	//완전히 닫히고 멈추면 Component Tick 종료
	if (!IsValid(PushingCharacter) && FMath::Abs(CurrentDoorAngle) < 0.5f && FMath::Abs(CurrentAngularVelocity) < 1.0f)
	{
		CurrentDoorAngle = 0.0f;
		CurrentAngularVelocity = 0.0f;
		
		DoorPivot->SetRelativeRotation(FRotator::ZeroRotator);
		
		SetComponentTickEnabled(false);
	}
}

void UPushDoorComponent::InitializeDoor(USceneComponent* InDoorPivot, UBoxComponent* InPushTrigger)
{
	DoorPivot = InDoorPivot;
	PushTrigger = InPushTrigger;
	
	if (!IsValid(DoorPivot) || !IsValid(PushTrigger))
	{
		return;
	}
	
	PushTrigger->OnComponentBeginOverlap.AddDynamic(this, &UPushDoorComponent::HandlePushTriggerBeginOverlap);
	PushTrigger->OnComponentEndOverlap.AddDynamic(this, &UPushDoorComponent::HandlePushTriggerEndOverlap);
}

void UPushDoorComponent::HandlePushTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			TEXT("PUSH TRIGGER OVERLAP")
		);
	}
	
	
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	
	if (!IsValid(Character) || !Character->IsPlayerControlled())
	{
		return;
	}
	
	if (OtherComp != Character->GetCapsuleComponent())
	{
		return;
	}
	
	PushingCharacter = Character;
	
	SetComponentTickEnabled(true);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Green,
			TEXT("PUSH PLAYER VALID")
		);
	}
}

void UPushDoorComponent::HandlePushTriggerEndOverlap(
	UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	
	if (!IsValid(Character))
	{
		return;
	}
	
	if (OtherComp != Character->GetCapsuleComponent())
	{
		return;
	}
	
	if (Character != PushingCharacter.Get())
	{
		return;
	}
	
	PushingCharacter = nullptr;
}

