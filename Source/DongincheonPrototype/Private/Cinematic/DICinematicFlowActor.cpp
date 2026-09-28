#include "Cinematic/DICinematicFlowActor.h"

#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Player/DIPlayerController.h"

ADICinematicFlowActor::ADICinematicFlowActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADICinematicFlowActor::PlayCinematic()
{
	PlaySequenceAsset(Sequence);
}

void ADICinematicFlowActor::PlaySequenceAsset(ULevelSequence* InSequence)
{
	if (bIsPlayingCinematic)
	{
		return;
	}

	if (!IsValid(InSequence))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[DICinematicFlowActor] Sequence is not assigned: %s"),
			*GetName()
		);

		return;
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;

	ALevelSequenceActor* CreatedSequenceActor = nullptr;

	SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(this,InSequence,PlaybackSettings,
		CreatedSequenceActor);

	SequenceActor = CreatedSequenceActor;

	if (!IsValid(SequencePlayer))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[DICinematicFlowActor] Failed to create SequencePlayer: %s"),
			*GetName()
		);

		return;
	}

	SequencePlayer->OnFinished.AddUniqueDynamic(this,&ADICinematicFlowActor::HandleSequenceFinished);

	bIsPlayingCinematic = true;

	if (bManagePresentationState)
	{
		EnterCinematicPresentation();
	}

	SequencePlayer->Play();
}

bool ADICinematicFlowActor::IsPlayingCinematic() const
{
	return bIsPlayingCinematic;
}

void ADICinematicFlowActor::HandleSequenceFinished()
{
	if (!bIsPlayingCinematic)
	{
		return;
	}

	bIsPlayingCinematic = false;

	if (IsValid(SequencePlayer))
	{
		SequencePlayer->OnFinished.RemoveDynamic(this,&ADICinematicFlowActor::HandleSequenceFinished);
	}

	SequencePlayer = nullptr;
	SequenceActor = nullptr;

	HandleSequenceSegmentFinished();
}

void ADICinematicFlowActor::HandleSequenceSegmentFinished()
{
	if (bManagePresentationState && bRestoreGameplayOnFinish)
	{
		ExitCinematicPresentation();
	}

	OnCinematicFinished.Broadcast();
}

void ADICinematicFlowActor::EnterCinematicPresentation()
{
	ADIPlayerController* PlayerController =
		Cast<ADIPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

	if (!IsValid(PlayerController))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[DICinematicFlowActor] DIPlayerController not found: %s"),
			*GetName()
		);

		return;
	}

	PlayerController->SetCinematicControlLocked(true);
	PlayerController->SetHUDContext(EDIHUDContext::Cinematic);
}

void ADICinematicFlowActor::ExitCinematicPresentation()
{
	ADIPlayerController* PlayerController =
		Cast<ADIPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

	if (!IsValid(PlayerController))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[DICinematicFlowActor] DIPlayerController not found: %s"),
			*GetName()
		);

		return;
	}

	PlayerController->SetCinematicControlLocked(false);
	PlayerController->SetHUDContext(EDIHUDContext::Gameplay);
}