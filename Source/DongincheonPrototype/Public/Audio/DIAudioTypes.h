#pragma once

#include "CoreMinimal.h"
#include "DIAudioTypes.generated.h"

class AActor;
class USoundBase;
class USoundAttenuation;
class USoundConcurrency;

UENUM(BlueprintType)
enum class EDIAudioPlaybackMode : uint8
{
	OwnerAttached   UMETA(DisplayName = "Owner Attached"),
	ContextLocation UMETA(DisplayName = "Context Location")
};

USTRUCT(BlueprintType)
struct FDIAudioEventContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	FName SocketName = NAME_None;
};

USTRUCT(BlueprintType)
struct FDIAudioEventDefinition
{
	GENERATED_BODY()

	// SoundWave / SoundCue / MetaSound 모두 USoundBase 계열이라 들어갈 수 있음.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TArray<TObjectPtr<USoundBase>> Variants;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio",
		meta = (ClampMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	FVector2D PitchRange = FVector2D(1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	EDIAudioPlaybackMode PlaybackMode = EDIAudioPlaybackMode::OwnerAttached;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundAttenuation> AttenuationOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<USoundConcurrency> ConcurrencyOverride = nullptr;
};