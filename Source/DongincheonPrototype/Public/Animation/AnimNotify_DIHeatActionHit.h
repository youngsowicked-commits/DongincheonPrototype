#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_DIHeatActionHit.generated.h"

UCLASS(meta=(DisplayName="DI Heat Action Hit"))
class DONGINCHEONPROTOTYPE_API UAnimNotify_DIHeatActionHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};