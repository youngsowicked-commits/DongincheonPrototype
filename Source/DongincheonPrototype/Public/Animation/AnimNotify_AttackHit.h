// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_AttackHit.generated.h"

/**
 * 
 */
UCLASS(meta = (DisplayName = "Attack Hit"))
class DONGINCHEONPROTOTYPE_API UAnimNotify_AttackHit : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
	/*
	* 이 Notify가 어느 공격 부위를 사용하는지 지정한다.
	* 예: hand_r, hand_l, foot_r, foot_l
	* Gameplay Rule이 아니라 Animation Content Parameter.
	*/
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Hit")
	FName HitSocketName = NAME_None;
};
