#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_DIComboChainPoint.generated.h"

UCLASS(meta=(DisplayName="DI Combo Chain Point"))
class DONGINCHEONPROTOTYPE_API UAnimNotify_DIComboChainPoint : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp,UAnimSequenceBase* Animation,const FAnimNotifyEventReference& EventReference) override;
};