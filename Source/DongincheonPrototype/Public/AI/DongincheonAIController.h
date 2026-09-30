#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Character/DongincheonEnemyBase.h"
#include "DongincheonAIController.generated.h"

class UStateTreeAIComponent;
class APawn;

/**
 * 
 */
UCLASS()
class DONGINCHEONPROTOTYPE_API ADongincheonAIController : public AAIController
{
	GENERATED_BODY()
	
	
public:
	ADongincheonAIController();
	
	void SendHitReactEvent();
	void SendDeadEvent();
	void SendGuardBrokenEvent();
	
	void SendGrabbedEvent();
	void SendGrabReleasedEvent();
	
	// Enemy가 Grab을 강제로 풀고 BreakFree 연출 상태로 진입한다.
	void SendGrabBreakEvent();

	// BreakFree 연출이 끝나 정상 Combat으로 복귀한다.
	void SendGrabBreakFinishedEvent();
	
	void SendHeatActionVictimEvent();
	void SendHeatActionVictimReleasedEvent();
	
	AActor* GetCombatTarget() const
	{
		return CombatTarget.Get();
	}
	void MarkGuardUsed();
	bool CanUseGuard(float CooldownDuration) const;
	
	void MarkHitReactUsed();
	bool CanUseHitReact(float CooldownDuration) const;
	
	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void StartStateTreeLogic();
	
	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void EnterPresentationState();
	
	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void ExitPresentationState();
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
	
private:
	TWeakObjectPtr<AActor> CombatTarget;
	double LastGuardTime = -1.0;
	double LastHitReactTime = -1.0;
};
