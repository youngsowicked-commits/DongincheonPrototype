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
	
	if (!IsValid(SequenceActor) ||
	!PrepareSequenceActor(SequenceActor, InSequence))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[CINEMATIC] Sequence preparation failed: %s"),
			*GetNameSafe(InSequence)
		);

		if (IsValid(SequenceActor))
		{
			SequenceActor->Destroy();
		}

		SequenceActor = nullptr;
		SequencePlayer = nullptr;
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

bool ADICinematicFlowActor::PrepareSequenceActor(
	ALevelSequenceActor* InSequenceActor,
	ULevelSequence* InSequence)
{
	return true;
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

void ADICinematicFlowActor::SetActiveSequencePlayRate(float NewRate)
{
	if (IsValid(SequencePlayer))
	{
		SequencePlayer->SetPlayRate(FMath::Max(0.01f, NewRate));
	}
}

float ADICinematicFlowActor::GetActiveSequenceTimeSeconds() const
{
	return IsValid(SequencePlayer)
		? static_cast<float>(SequencePlayer->GetCurrentTime().AsSeconds())
		: 0.0f;
}

bool ADICinematicFlowActor::ReplaceActiveSequenceAtTime(
	ULevelSequence* NewSequence,
	float StartSeconds)
{
	if (!IsValid(NewSequence))
	{
		return false;
	}

	if (IsValid(SequencePlayer))
	{
		SequencePlayer->OnFinished.RemoveDynamic(
			this, &ADICinematicFlowActor::HandleSequenceFinished);

		SequencePlayer->StopAtCurrentTime();
	}

	if (IsValid(SequenceActor))
	{
		SequenceActor->Destroy();
	}

	SequencePlayer = nullptr;
	SequenceActor = nullptr;
	bIsPlayingCinematic = false;

	PlaySequenceAsset(NewSequence);

	if (!IsValid(SequencePlayer) || !bIsPlayingCinematic)
	{
		return false;
	}

	SequencePlayer->SetPlaybackPosition(
		FMovieSceneSequencePlaybackParams(
			FMath::Max(0.0f, StartSeconds),
			EUpdatePositionMethod::Jump));

	return true;
}