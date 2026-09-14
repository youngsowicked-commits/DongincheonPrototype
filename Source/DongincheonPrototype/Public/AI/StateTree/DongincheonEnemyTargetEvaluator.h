#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "DongincheonEnemyTargetEvaluator.generated.h"

class AActor;
class APawn;
struct FStateTreeExecutionContext;

//Enemy Target Evaluator가 실행 중 사용할 데이터
USTRUCT()
struct DONGINCHEONPROTOTYPE_API FDongincheonEnemyTargetEvaluatorInstanceData
{
	GENERATED_BODY()
	
	//ST_EnemyCombat의 Context에 있는 Pawn을 연결한다.
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
	
	//Chase , Condition 등 다른 StateTree Node가 사용할 Target
	UPROPERTY(VisibleAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;
};

//Enemy StateTre에 Player Target을 공급하는 Evaluator

USTRUCT(meta = (DisplayName = "Dongincheon Enemy Target"))
struct DONGINCHEONPROTOTYPE_API FDongincheonEnemyTargetEvaluator : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FDongincheonEnemyTargetEvaluatorInstanceData;
	
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FInstanceDataType::StaticStruct();
	}
	
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};