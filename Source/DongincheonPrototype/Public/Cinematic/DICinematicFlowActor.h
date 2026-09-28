#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DICinematicFlowActor.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDICinematicFinishedSignature);

UCLASS()
class DONGINCHEONPROTOTYPE_API ADICinematicFlowActor : public AActor
{
	GENERATED_BODY()

public:
	ADICinematicFlowActor();

	UFUNCTION(BlueprintCallable, Category = "Cinematic")
	void PlayCinematic();
	
	UFUNCTION(BlueprintCallable, Category = "Cinematic")
	void PlaySequenceAsset(ULevelSequence* InSequence);

	UFUNCTION(BlueprintPure, Category = "Cinematic")
	bool IsPlayingCinematic() const;

	UPROPERTY(BlueprintAssignable, Category = "Cinematic")
	FDICinematicFinishedSignature OnCinematicFinished;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic")
	TObjectPtr<ULevelSequence> Sequence;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic")
	bool bManagePresentationState = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematic")
	bool bRestoreGameplayOnFinish = true;

	virtual void HandleSequenceSegmentFinished();
	
private:
	UFUNCTION()
	void HandleSequenceFinished();

	void EnterCinematicPresentation();
	void ExitCinematicPresentation();

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> SequencePlayer;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	UPROPERTY(Transient)
	bool bIsPlayingCinematic = false;
};