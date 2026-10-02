#include "Components/DICombatVFXComponent.h"
#include "Gameplay/Data/DICombatVFXData.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"


UDICombatVFXComponent::UDICombatVFXComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDICombatVFXComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	CombatComponent = Owner->FindComponentByClass<UCombatComponent>();
	if (!IsValid(CombatComponent))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[CombatVFX] %s has no CombatComponent."),
			*GetNameSafe(Owner));
		return;
	}

	CombatComponent->OnImpactConfirmed.AddUniqueDynamic(this,&UDICombatVFXComponent::HandleImpactConfirmed);
}

void UDICombatVFXComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(CombatComponent))
	{
		CombatComponent->OnImpactConfirmed.RemoveDynamic(this,&UDICombatVFXComponent::HandleImpactConfirmed);
	}

	Super::EndPlay(EndPlayReason);
}

void UDICombatVFXComponent::HandleImpactConfirmed(AActor* HitActor,FVector HitLocation,FName HitSocketName,
	float AppliedDamage,EDICombatImpactResult ImpactResult)
{
	UNiagaraSystem* ImpactSystem = ResolveImpactSystem(ImpactResult);
	if (!IsValid(ImpactSystem))
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,ImpactSystem,HitLocation);
}

UNiagaraSystem* UDICombatVFXComponent::ResolveImpactSystem(
	EDICombatImpactResult ImpactResult) const
{
	if (!IsValid(CombatVFXData))
	{
		return nullptr;
	}

	switch (ImpactResult)
	{
	case EDICombatImpactResult::Hit:
		return CombatVFXData->NormalHit;

	case EDICombatImpactResult::GuardHit:
		return CombatVFXData->GuardHit;

	case EDICombatImpactResult::GuardBreak:
		return CombatVFXData->GuardBreak;

	case EDICombatImpactResult::None:
	default:
		return nullptr;
	}
}