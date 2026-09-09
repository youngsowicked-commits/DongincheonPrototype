// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/BuildingEntryBase.h"
#include "Engine/Engine.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

// Sets default values
ABuildingEntryBase::ABuildingEntryBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	
	EntryTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryTrigger"));
	EntryTrigger->SetupAttachment(SceneRoot);
	EntryTrigger->SetCollisionProfileName(TEXT("Trigger"));
	
	EntryPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EntryPoint"));
	EntryPoint->SetupAttachment(SceneRoot);
	
	InsidePoint = CreateDefaultSubobject<USceneComponent>(TEXT("InsidePoint"));
	InsidePoint->SetupAttachment(SceneRoot);
}

void ABuildingEntryBase::BeginPlay()
{
	Super::BeginPlay();
	
	EntryTrigger->OnComponentBeginOverlap.AddDynamic(
	this,&ABuildingEntryBase::HandleEntryTriggerBeginOverlap	
	);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,3.0f,FColor::Green,TEXT("ENTRY READY"));
	}
}

void ABuildingEntryBase::StartEntry(ACharacter* Character)
{
	if (!IsValid(Character))
	{
		return;
	}
	
	bEntryInProgress = true;
	ActiveCharacter = Character;
	
	
	OnEntryStarted();
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Yellow,
			TEXT("ENTRY INPUT LOCKED"));
	}
}

void ABuildingEntryBase::LockPlayerInput()
{
	if (!IsValid(ActiveCharacter))
	{
		return;
	}
	
	APlayerController* PlayerController = Cast<APlayerController>(ActiveCharacter->GetController());
	
	if (!IsValid(PlayerController))
	{
		return;
	}
	
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);
}

void ABuildingEntryBase::AlignPlayerToEntryPoint()
{
	if (!IsValid(ActiveCharacter) || !IsValid(EntryPoint))
	{
		return;
	}
	
	ActiveCharacter->SetActorLocationAndRotation(
		EntryPoint->GetComponentLocation(),
		EntryPoint->GetComponentRotation());
}

void ABuildingEntryBase::MovePlayerToInsidePoint()
{
	if (!bEntryInProgress || !IsValid(ActiveCharacter) || !IsValid(InsidePoint))
	{
		return;
	}
	
	ActiveCharacter->SetActorLocationAndRotation(
		InsidePoint->GetComponentLocation(),
		InsidePoint->GetComponentRotation());
}

void ABuildingEntryBase::FinishEntry()
{
	if (!bEntryInProgress)
	{
		return;
	}
	
	if (IsValid(ActiveCharacter))
	{
		APlayerController* PlayerController = Cast<APlayerController>(ActiveCharacter->GetController());
		
		if (IsValid(PlayerController))
		{
			PlayerController->SetIgnoreMoveInput(false);
			PlayerController->SetIgnoreLookInput(false);
		}
	}
	
	OnEntryFinished();
	
	ActiveCharacter = nullptr;
	bEntryInProgress = false;
}

void ABuildingEntryBase::HandleEntryTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult)
{
	if (bEntryInProgress)
	{
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	
	if (!IsValid(Character) || !Character->IsPlayerControlled())
	{
		return;
	}
	
	StartEntry(Character);
	
}


	

