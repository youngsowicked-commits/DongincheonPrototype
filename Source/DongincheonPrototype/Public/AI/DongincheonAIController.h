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
	
	UFUNCTION(BlueprintCallable, Category = "AI|StateTree")
	void StartStateTreeLogic();
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
	
	
};
