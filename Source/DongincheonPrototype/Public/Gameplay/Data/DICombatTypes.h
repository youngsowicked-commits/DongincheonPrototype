#pragma once

#include "CoreMinimal.h"
#include "DICombatTypes.generated.h"

UENUM(BlueprintType)
enum class EDICombatImpactResult : uint8
{
	None,
	Hit,
	GuardHit,
	GuardBreak
};