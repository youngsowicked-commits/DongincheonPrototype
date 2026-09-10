// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/PushDoorBase.h"
#include "Engine/Engine.h"

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
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,2.0f,FColor::Red,TEXT("Door End Overlap"));
	}
	
	PushingCharacter = nullptr;
}

// Called every frame
void APushDoorBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//플레이어가 밀고 있을때만 회전 가속도 증가
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
		const float SpringDelta =
	   (-CurrentDoorAngle * CloseSpringStrength) * DeltaTime;

		CurrentAngularVelocity += SpringDelta;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				2002,
				0.0f,
				FColor::Green,
				FString::Printf(
					TEXT("Angle %.1f | Spring %.2f | Vel %.2f"),
					CurrentDoorAngle,
					SpringDelta,
					CurrentAngularVelocity
				)
			);
		}
	}
	
	//최대 회전속도 제한
	CurrentAngularVelocity = FMath::Clamp(CurrentAngularVelocity, -MaxAngularSpeed, MaxAngularSpeed);
	
	//마찰 ,감쇠
	CurrentAngularVelocity = FMath::FInterpTo(CurrentAngularVelocity, 0.0f, DeltaTime, AngularDamping);
	
	//속도로 실제 각도 갱신
	CurrentDoorAngle += CurrentAngularVelocity * DeltaTime;
	
	//최대 열림 각도 처리
	if (CurrentDoorAngle >= MaxOpenAngle)
	{
		CurrentDoorAngle = MaxOpenAngle;
		
		if ( CurrentAngularVelocity > 0.0f)
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
	
	DoorPivot->SetRelativeRotation(FRotator(0.0f, CurrentDoorAngle, 0));
	
	//플레이어가 떠났고 문도 사실상 멈췄으면 Tick Off
	if (!IsValid(PushingCharacter) && FMath::Abs(CurrentAngularVelocity) < 1.0f && FMath::Abs(CurrentDoorAngle) < 0.5f)
	{
		CurrentDoorAngle = 0.0f;
		CurrentAngularVelocity = 0.0f;
		
		DoorPivot->SetRelativeRotation(FRotator::ZeroRotator);
		
		SetActorTickEnabled(false);
	}
}

