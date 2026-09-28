#pragma once

#include "CoreMinimal.h"
#include "DICinematicTypes.generated.h"

class ULevelSequence;

UENUM(BlueprintType)
enum class EDIQTECinematicFlowState : uint8
{
	Inactive			UMETA(DisplayName = "Inactive"),
	PlayingSetup		UMETA(DisplayName = "Playing Setup"),
	PlayingQTESequence	UMETA(DisplayName = "Playing QTE Sequence"),
	WaitingForQTEResult	UMETA(DisplayName = "Waiting For QTE Result"),
	PlayingResult		UMETA(DisplayName = "Playing Result"),
	Completed			UMETA(DisplayName = "Completed")
};

USTRUCT(BlueprintType)
struct DONGINCHEONPROTOTYPE_API FDIQTECinematicStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QTE")
	FName StepId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QTE|Cinematic")
	TObjectPtr<ULevelSequence> QTESequence = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QTE|Cinematic")
	TObjectPtr<ULevelSequence> SuccessSequence = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "QTE|Cinematic")
	TObjectPtr<ULevelSequence> FailSequence = nullptr;
};