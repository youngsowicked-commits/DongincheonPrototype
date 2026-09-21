#pragma once

#include "CoreMinimal.h"
#include "DIHUDTypes.generated.h"

UENUM(BlueprintType)
enum class EDIHUDContext : uint8
{
	Gameplay    UMETA(DisplayName = "Gameplay"),
	Cinematic   UMETA(DisplayName = "Cinematic"),
	Interaction UMETA(DisplayName = "Interaction")
};