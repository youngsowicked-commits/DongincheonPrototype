#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "DIAttackTask.generated.h"

class AActor;
class APawn;

struct FStateTreeExecutionContext;
struct FStateTreeTransitionResult;

USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDIAttackTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AActor> Target = nullptr;
	
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0"))
	int32 AttackIndex = 0;
	
	/*
	* Approach MoveTo의 Acceptance Radius와는 역할이 다름.
	* Approach Radius:
	* "이 정도면 접근 이동을 끝내도 된다."
	* 
	* MaxAttackDistance:
	* "실제로 지금 공격을 시작해도 된다."
	*
	* 플레이어가 Approach 종료 직후 뒤로 빠지는 상황을 잡기 위한
	* Attack 진입 직전 최종 검증값.
	*/
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.0"))
	float MaxAttackDistance = 200.0f;

	/*
	 * 콤보 타격과 타격 사이에서 플레이어를 향해
	 * 부드럽게 재조준하는 회전 속도.
	 *
	 * 첫 공격 진입 전 FaceTarget은 현재 300 deg/s.
	 * 콤보 중에는 더 약하게 따라붙도록 기본 240 deg/s.
	 */
	UPROPERTY(EditAnywhere, Category = "Attack|ReAim", meta = (ClampMin = "1.0"))
	float ComboReAimSpeed = 240.0f;
	
	// 현재 방향과 Target 방향 차이가 이 값 이하가 되면 재조준 완료로 보고 다음 공격을 시작한다. 
	UPROPERTY(EditAnywhere, Category = "Attack|ReAim", meta = (ClampMin = "0.1"))
	float ComboReAimAcceptanceAngle = 8.0f;
	
	//플레이어가 계속 움직여도 다음 공격이 무한정 늦어지지 않도록 재조준에 사용할 수 있는 최대 시간.
	UPROPERTY(EditAnywhere, Category = "Attack|ReAim", meta = (ClampMin = "0.0"))
	float ComboReAimMaxTime = 0.15f;
	
	// Runtime Only
	UPROPERTY(Transient)
	TArray<int32> ActiveAttackSequece;

	UPROPERTY(Transient)
	int32 CurrentSequenceIndex = INDEX_NONE;
	
	//한 타가 끝났고, 다음 타를 시작하기 전에 현재 재조준 중인지.
	UPROPERTY(Transient)
	bool bIsReAiming = false;

	
	//현재 재조준에 사용한 시간.
	UPROPERTY(Transient)
	float ReAimElapsedTime = 0.0f;
};

USTRUCT(meta = (DisplayName = "DI Attack"))
struct DONGINCHEONPROTOTYPE_API FDIAttackTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FDIAttackTaskInstanceData;
	
	FDIAttackTask()
	{
		bShouldCallTick = true;
	}
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};