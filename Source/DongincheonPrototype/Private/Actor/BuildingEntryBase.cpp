// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/BuildingEntryBase.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"

#include "Components/BoxComponent.h"
#include  "Components/PushDoorComponent.h"
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
	
	ExitTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitTrigger"));
	ExitTrigger->SetupAttachment(SceneRoot);
	ExitTrigger->SetCollisionProfileName(TEXT("Trigger"));
	
	EntryPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EntryPoint"));
	EntryPoint->SetupAttachment(SceneRoot);
	
	InsidePoint = CreateDefaultSubobject<USceneComponent>(TEXT("InsidePoint"));
	InsidePoint->SetupAttachment(SceneRoot);
	
	EntryCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("EntryCamera"));
	EntryCamera->SetupAttachment(SceneRoot);
	
	DoorPivot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorPivot"));
	DoorPivot->SetupAttachment(SceneRoot);
	DoorPivot->SetMobility(EComponentMobility::Movable);
	
	PushTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("PushTrigger"));
	PushTrigger->SetupAttachment(SceneRoot);
	
	PushTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PushTrigger->SetGenerateOverlapEvents(true);
	PushTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PushTrigger->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
	
	
	PushDoorComponent = CreateDefaultSubobject<UPushDoorComponent>(TEXT("PushDoorComponent"));
}

void ABuildingEntryBase::BeginPlay()
{
	Super::BeginPlay();
	
	DoorPivot->SetMobility(EComponentMobility::Movable);
	
	EntryTrigger->OnComponentBeginOverlap.AddDynamic(this,&ABuildingEntryBase::HandleEntryTriggerBeginOverlap);
	
	ExitTrigger->OnComponentBeginOverlap.AddDynamic(this,&ABuildingEntryBase::HandleExitTiggerBeginOverlap);
	
	PushDoorComponent->InitializeDoor(DoorPivot, PushTrigger);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1,3.0f,FColor::Green,TEXT("ENTRY READY"));
	}
}

void ABuildingEntryBase::StartEntry(ACharacter* Character)
{
	if (!IsValid(Character) || bEntryInProgress || bPlayerInside)
	{
		return;
	}
	
	bEntryInProgress = true;
	bPlayerInside = true;
	ActiveCharacter = Character;
	
	OnEntryStarted();
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

void ABuildingEntryBase::BlendToEntryCamera()
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
	
	PlayerController->SetViewTargetWithBlend(this, 0.35f);
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

void ABuildingEntryBase::HandleExitTiggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult)
{
	if (!bPlayerInside)
	{
		return;
	}
	
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	
	if (!IsValid(Character) || !Character->IsPlayerControlled())
	{
		return;
	}
	
	bPlayerInside = false;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Cyan,
			TEXT("PLAYER OUTSIDE")
		);
	}
}


	

