// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/PushDoorBase.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

// Sets default values
APushDoorBase::APushDoorBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	
	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(SceneRoot);
	
	PushTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PushTrigger"));
	PushTrigger->SetupAttachment(SceneRoot);
	PushTrigger->SetCollisionProfileName(TEXT("Trigger"));

}

// Called when the game starts or when spawned
void APushDoorBase::BeginPlay()
{
	Super::BeginPlay();
	
	PushTrigger->OnComponentBeginOverlap.AddDynamic(this,&APushDoorBase::HandlePushTriggerBeginOverlap);
	PushTrigger->OnComponentEndOverlap.AddDynamic(this,&APushDoorBase::HandlePushTriggerEndOverlap);
	
}

void APushDoorBase::HandlePushTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
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
	SetActorTickEnabled(true);
}

void APushDoorBase::HandlePushTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
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
	SetActorTickEnabled(false);
}

// Called every frame
void APushDoorBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!IsValid(PushingCharacter))
	{
		return;
	}
	
	FVector ToPlayer = PushingCharacter->GetActorLocation() - DoorPivot->GetComponentLocation();
	
	FVector MoveDirection = PushingCharacter->GetVelocity();
	
	ToPlayer.Z = 0.0f;
	MoveDirection.Z = 0.0f;
	
	if (ToPlayer.IsNearlyZero() || MoveDirection.IsNearlyZero())
	{
		return;
	}
	
	ToPlayer.Normalize();
	MoveDirection.Normalize();
	
	const float PushTorque = FVector::CrossProduct(ToPlayer,MoveDirection).Z;
	
	if (FMath::Abs(PushTorque) < 0.05f)
	{
		return;
	}
	
	CurrentDoorAngle += PushTorque * PushOpenSpeed * DeltaTime;
	
	CurrentDoorAngle = FMath::Clamp(CurrentDoorAngle,-MaxOpenAngle,MaxOpenAngle);
	
	DoorPivot->SetRelativeRotation(FRotator(0.0f,CurrentDoorAngle,0.0f));
}

