#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_DIAudioEvent.generated.h"

UCLASS(meta = (DisplayName = "DI Audio Event"))
class DONGINCHEONPROTOTYPE_API UAnimNotify_DIAudioEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// "무슨 소리 파일인가"가 아니라
	// "무슨 의미의 Audio Event인가"
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DI Audio")
	FGameplayTag EventTag;

	// 손, 발 등 특정 Socket 기준 재생이 필요할 때만 사용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DI Audio")
	FName SocketName = NAME_None;
};