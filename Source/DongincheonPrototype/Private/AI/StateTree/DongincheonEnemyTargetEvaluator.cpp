#include "AI/StateTree/DongincheonEnemyTargetEvaluator.h"

#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"

void FDongincheonEnemyTargetEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	InstanceData.TargetActor = nullptr;
	
	if (!IsValid(InstanceData.Pawn))
	{
		return;
	}
	
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(InstanceData.Pawn, 0);
	
	if (IsValid(PlayerPawn) && PlayerPawn != InstanceData.Pawn)
	{
		InstanceData.TargetActor = PlayerPawn;
	}
}

void FDongincheonEnemyTargetEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	//이미 정상적인 Target이 있으면 다시 찾지 않는다.
	
	if (IsValid(InstanceData.TargetActor))
	{
		return;
	}
	
	if (!IsValid(InstanceData.Pawn))
	{
		return;
	}
	
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(InstanceData.Pawn,0);
	
	if (IsValid(PlayerPawn) && PlayerPawn != InstanceData.Pawn)
	{
		InstanceData.TargetActor = PlayerPawn;
	}
	
}
