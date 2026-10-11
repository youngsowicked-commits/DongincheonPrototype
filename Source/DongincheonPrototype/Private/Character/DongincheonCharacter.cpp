// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/DongincheonCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Components/TargetingComponent.h"
#include "Components/HealthComponent.h"
#include "Components/DIGrabComponent.h"
#include "Components/DIHeatActionComponent.h"
#include "Components/DICharacterAudioComponent.h"
#include "Components/DICombatVFXComponent.h"
#include "Gameplay/Data/DIHeatActionDefinition.h"
#include "NativeGameplayTags.h"
#include "Components/InteractionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "TimerManager.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DI_HeatAction_Context_GrabActive, "HeatAction.Context.Grab.Active");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DI_HeatAction_Context_GrabHolding, "HeatAction.Context.Grab.Holding");

// Sets default values
ADongincheonCharacter::ADongincheonCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	CombatVFXComponent = CreateDefaultSubobject<UDICombatVFXComponent>(TEXT("CombatVFX"));
	GrabComponent = CreateDefaultSubobject<UDIGrabComponent>(TEXT("GrabComponent"));
	HeatActionComponent = CreateDefaultSubobject<UDIHeatActionComponent>(TEXT("HeatActionComponent"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("Targeting"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	CharacterAudioComponent = CreateDefaultSubobject<UDICharacterAudioComponent>(TEXT("CharacterAudioComponent"));
}

//BeginPlay
void ADongincheonCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(HealthComponent))
	{
		HealthComponent->OnDamaged.AddUniqueDynamic(this, &ADongincheonCharacter::HandleHealthDamaged);
		HealthComponent->OnDeath.AddUniqueDynamic(this, &ADongincheonCharacter::HandleHealthDeath);
	}

	if (IsValid(CombatComponent))
	{
		CombatComponent->OnImpactConfirmed.AddUniqueDynamic(this, &ADongincheonCharacter::HandleCombatHitConfirmed);
	}

	if (IsValid(GrabComponent))
	{
		GrabComponent->OnGrabStateChanged.AddDynamic(this, &ADongincheonCharacter::HandleGrabStateChanged);
		GrabComponent->OnGrabAttackReceived.AddDynamic(this, &ADongincheonCharacter::HandleGrabAttackReceived);
		GrabComponent->OnGrabAttackHitConfirmed.AddDynamic(this, &ADongincheonCharacter::HandleGrabAttackHitConfirmed);
	}

	if (IsValid(HeatActionComponent))
	{
		HeatActionComponent->OnHeatChanged.AddUniqueDynamic(this, &ADongincheonCharacter::HandleHeatChanged);
	}

	RefreshMovementSpeed();
}

void ADongincheonCharacter::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	bTutorialInfiniteHeatEnabled = false;
	bTutorialHitReportedForActiveAttack = false;

	Super::EndPlay(EndPlayReason);
}

void ADongincheonCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateAttackAssistMove(DeltaTime);

	HeatActionAvailabilityCheckElapsed += DeltaTime;

	if (HeatActionAvailabilityCheckElapsed >= 0.1f)
	{
		HeatActionAvailabilityCheckElapsed = 0.0f;
		RefreshHeatActionAvailability();
	}
}

//Input
void ADongincheonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!IsValid(EnhancedInput))
	{
		return;
	}

	if (IsValid(MoveAction))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this,
		                          &ADongincheonCharacter::HandleMoveInputStartedOrTriggered);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this,
		                          &ADongincheonCharacter::HandleMoveInputCompleted);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this,
		                          &ADongincheonCharacter::HandleMoveInputCompleted);
	}

	if (IsValid(RunAction))
	{
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Triggered, this, &ADongincheonCharacter::HandleRunStarted);
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Completed, this, &ADongincheonCharacter::HandleRunEnded);
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Canceled, this, &ADongincheonCharacter::HandleRunEnded);
	}

	if (IsValid(DodgeAction))
	{
		EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Started, this, &ADongincheonCharacter::HandleDodgeInput);
	}

	if (IsValid(GuardAction))
	{
		EnhancedInput->BindAction(GuardAction, ETriggerEvent::Started, this,
		                          &ADongincheonCharacter::HandleGuardStarted);
		EnhancedInput->BindAction(GuardAction, ETriggerEvent::Completed, this,
		                          &ADongincheonCharacter::HandleGuardEnded);
		EnhancedInput->BindAction(GuardAction, ETriggerEvent::Canceled, this, &ADongincheonCharacter::HandleGuardEnded);
	}

	if (IsValid(AttackAction))
	{
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this,
		                          &ADongincheonCharacter::HandleAttackInput);
	}

	if (IsValid(HeavyAttackAction))
	{
		EnhancedInput->BindAction(HeavyAttackAction, ETriggerEvent::Started, this,
		                          &ADongincheonCharacter::HandleHeavyAttackInput);
	}

	if (IsValid(GrabAction))
	{
		EnhancedInput->BindAction(GrabAction, ETriggerEvent::Started, this, &ADongincheonCharacter::HandleGrabInput);
	}

	if (IsValid(LockOnAction))
	{
		EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this,
		                          &ADongincheonCharacter::HandleLockOnStarted);
		EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Completed, this,
		                          &ADongincheonCharacter::HandleLockOnEnded);
		EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Canceled, this,
		                          &ADongincheonCharacter::HandleLockOnEnded);
	}

	if (IsValid(LockOnSwitchAction))
	{
		EnhancedInput->BindAction(LockOnSwitchAction, ETriggerEvent::Triggered, this,
		                          &ADongincheonCharacter::HandleLockOnSwitch);
	}
}

void ADongincheonCharacter::HandleLockOnSwitch(const FInputActionValue& Value)
{
	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!bCombatInputEnabled)
	{
		return;
	}

	if (!IsMovementInputAllowed()) return;

	if (IsValid(GrabComponent) && GrabComponent->GetGrabState() != EDIGrabState::None) return;

	if (!IsValid(TargetingComponent))
	{
		return;
	}

	if (!TargetingComponent->IsLockedOn())
	{
		return;
	}

	const float Direction = Value.Get<float>();

	if (FMath::IsNearlyZero(Direction))
	{
		return;
	}

	AActor* PreviousTarget =
	TargetingComponent->GetLockOnTarget();

	TargetingComponent->SwitchTarget(Direction);

	AActor* NewTarget =
		TargetingComponent->GetLockOnTarget();

	if (IsValid(PreviousTarget) &&
		IsValid(NewTarget) &&
		PreviousTarget != NewTarget)
	{
		OnTutorialAction.Broadcast(
			EDIPlayerTutorialAction::LockOnSwitched);
	}
}

void ADongincheonCharacter::HandleLockOnStarted(const FInputActionValue& Value)
{
	(void)Value;

	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!bCombatInputEnabled)
	{
		return;
	}

	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}

	if (!IsMovementInputAllowed()) return;

	if (IsValid(GrabComponent) && GrabComponent->GetGrabState() != EDIGrabState::None) return;

	if (IsValid(TargetingComponent))
	{
		TargetingComponent->TryLockOn();
		
		if (TargetingComponent->IsLockedOn() && IsValid(TargetingComponent->GetLockOnTarget()))
		{
			OnTutorialAction.Broadcast(
				EDIPlayerTutorialAction::LockOnAcquired);
		}
		
		RefreshMovementSpeed();
	}
}

void ADongincheonCharacter::HandleLockOnEnded(const FInputActionValue& Value)
{
	(void)Value;

	if (IsValid(TargetingComponent))
	{
		TargetingComponent->ClearLockOn();
		RefreshMovementSpeed();
	}
}

void ADongincheonCharacter::HandleMoveInputStartedOrTriggered(const FInputActionValue& Value)
{
	if (IsGameplayInputLocked())
	{
		CachedMoveInput = FVector2D::ZeroVector;
		return;
	}

	CachedMoveInput = Value.Get<FVector2D>();
}

void ADongincheonCharacter::HandleMoveInputCompleted(const FInputActionValue& Value)
{
	(void)Value;

	CachedMoveInput = FVector2D::ZeroVector;
}

void ADongincheonCharacter::HandleRunStarted()
{
	if (bRunInputHeld || bCombatInputEnabled || IsGameplayInputLocked()) return;
	bRunInputHeld = true;
	RefreshMovementSpeed();
}

void ADongincheonCharacter::HandleRunEnded()
{
	if (!bRunInputHeld) return;
	bRunInputHeld = false;
	RefreshMovementSpeed();
}

void ADongincheonCharacter::RefreshMovementSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!IsValid(Movement) || bGrabWalkSpeedOverridden) return;

	float TargetSpeed = bRunInputHeld && !bCombatInputEnabled ? ExplorationRunSpeed : ExplorationWalkSpeed;
	if (bCombatInputEnabled) TargetSpeed = CombatMoveSpeed;
	if (bCombatInputEnabled && IsValid(TargetingComponent) && TargetingComponent->IsLockedOn()) TargetSpeed =
		LockOnMoveSpeed;

	Movement->MaxWalkSpeed = TargetSpeed;
}

bool ADongincheonCharacter::IsGameplayInputLocked() const
{
	return bInteractionInputLocked || bPresentationInputLocked ||
		(IsValid(HeatActionComponent) && HeatActionComponent->IsExecuting());
}

// Runtime Query
bool ADongincheonCharacter::IsMovementInputAllowed() const
{
	if (IsGameplayInputLocked()) return false;
	if (!IsValid(HealthComponent) || HealthComponent->IsDead()) return false;

	if (IsValid(GrabComponent))
	{
		if (GrabComponent->IsBeingGrabbed()) return false;
		if (GrabComponent->IsGrabbing() && !GrabComponent->IsHoldingGrab()) return false;
	}

	const bool bGuardLocked = IsValid(CombatComponent) && (CombatComponent->IsGuarding() || CombatComponent->
		IsGuardBroken());

	return !bPlayerAttackActive && !bPlayerDodging && !bGuardLocked && !IsValid(ActiveGrabStartMontage) && !
		bPlayerHitReacting && !bPlayerDeathStarted;
}

bool ADongincheonCharacter::HasTutorialMovementInput() const
{
	// WASD 입력이 있고 실제 이동이 허용된 경우만 인정
	return !CachedMoveInput.IsNearlyZero(0.1f)
		&& IsMovementInputAllowed();
}

void ADongincheonCharacter::SetPresentationInputLocked(bool blocked)
{
	if (blocked && !bHeatActionPresentationActive)
	{
		SetTutorialInfiniteHeatEnabled(false);
	}
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("PLAYER PRESENTATION LOCK REQUEST | Player=%s | Current=%s | Requested=%s"),
		*GetNameSafe(this),
		bPresentationInputLocked ? TEXT("TRUE") : TEXT("FALSE"),
		blocked ? TEXT("TRUE") : TEXT("FALSE"));

	if (bPresentationInputLocked == blocked)
	{
		return;
	}

	bPresentationInputLocked = blocked;
	RefreshHeatActionAvailability();

	if (!blocked)
	{
		return;
	}

	//진행중이던 일반 Gameplay Action 정리
	CancelPlayerAttack(0.05f);
	CancelPlayerDodge(0.05f);
	CancelPlayerHeatAction(0.05f);
	CancelPlayerGrab(0.05f);

	bGuardInputHeld = false;

	if (IsValid(CombatComponent) && CombatComponent->IsGuarding())
	{
		StopPlayerGuard(0.05f);
	}

	CachedMoveInput = FVector2D::ZeroVector;

	if (IsValid(TargetingComponent))
	{
		TargetingComponent->ClearLockOn();
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
}

void ADongincheonCharacter::SetInteractionInputLocked(bool bLocked)
{
	bInteractionInputLocked = bLocked;
	RefreshHeatActionAvailability();
}

void ADongincheonCharacter::SetCombatInputEnabled(bool bEnabled)
{
	if (!bEnabled)
	{
		SetTutorialInfiniteHeatEnabled(false);
	}
	
	if (bCombatInputEnabled == bEnabled)
	{
		return;
	}

	bCombatInputEnabled = bEnabled;
	RefreshHeatActionAvailability();

	if (bEnabled)
	{
		bRunInputHeld = false;
		RefreshMovementSpeed();
		return;
	}

	// Combat 종료 후 예약 입력이 Exploration에서 이어지지 않게 정리.
	QueuedAttackType = EQueuedAttackType::None;

	// Guard는 Hold 입력이므로 Combat 종료 시 반드시 해제한다.
	bGuardInputHeld = false;

	if (IsValid(CombatComponent) && CombatComponent->IsGuarding())
	{
		StopPlayerGuard(0.05f);
	}

	// Grab 상태에서는 Combat 종료 후 Release 입력도 막히므로 즉시 정리한다.
	CancelPlayerGrab(0.05f);

	// Exploration에서는 Lock-On을 유지하지 않는다.
	if (IsValid(TargetingComponent))
	{
		TargetingComponent->ClearLockOn();
	}

	RefreshMovementSpeed();
}

bool ADongincheonCharacter::IsCombatInputEnabled() const
{
	return bCombatInputEnabled;
}

void ADongincheonCharacter::SetTutorialInfiniteHeatEnabled(bool bEnabled)
{
	if (bEnabled)
	{
		if (!IsValid(HeatActionComponent) ||
			HeatActionComponent->GetMaxHeat() <= 0.0f)
		{
			return;
		}
	}

	bTutorialInfiniteHeatEnabled = bEnabled;

	if (bEnabled)
	{
		const float MissingHeat =
			HeatActionComponent->GetMaxHeat() -
			HeatActionComponent->GetCurrentHeat();

		if (MissingHeat > 0.0f)
		{
			HeatActionComponent->AddHeat(MissingHeat);
		}
	}

	RefreshHeatActionAvailability();
}

//Attack Input
void ADongincheonCharacter::HandleAttackInput()
{
	if (bPresentationInputLocked)
	{
		OnQTEInputPressed.Broadcast(EQTEInputType::Light);
		return;
	}

	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!bCombatInputEnabled)
	{
		return;
	}

	if (!IsValid(HealthComponent) || !IsValid(CombatComponent) || !IsValid(GrabComponent))
	{
		return;
	}

	if (HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging ||
		CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken())
	{
		return;
	}

	if (GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	if (GrabComponent->IsHoldingGrab())
	{
		StartPlayerGrabAttack();
		return;
	}

	if (GrabComponent->IsGrabbing())
	{
		return;
	}

	// 새 Light Combo 시작
	if (!bPlayerAttackActive)
	{
		if (ComboAttacks.IsEmpty())
		{
			return;
		}

		bPlayerAttackActive = true;

		ActiveAttackMode = EPlayerAttackMode::LightCombo;
		QueuedAttackType = EQueuedAttackType::None;

		ActiveComboIndex = 0;
		ActiveHeavyBranchIndex = INDEX_NONE;
		ActiveHeavyComboIndex = INDEX_NONE;

		StartPlayerComboAttack();
		return;
	}

	// Heavy 체인 중에는 LMB로 다시 Light로 돌아가지 않음
	if (ActiveAttackMode != EPlayerAttackMode::LightCombo)
	{
		return;
	}

	// 이미 Heavy가 예약돼 있으면 Light가 덮어쓰지 않음
	if (QueuedAttackType == EQueuedAttackType::Heavy)
	{
		return;
	}

	if (ActiveComboIndex + 1 < ComboAttacks.Num())
	{
		QueuedAttackType = EQueuedAttackType::Light;
	}
}

bool ADongincheonCharacter::CanUseContextualHeatAction() const
{
	if (!IsValid(HealthComponent) || !IsValid(CombatComponent) || !IsValid(HeatActionComponent))
	{
		return false;
	}

	if (!bCombatInputEnabled || IsGameplayInputLocked())
	{
		return false;
	}

	if (HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging ||
		bPlayerAttackActive || CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken())
	{
		return false;
	}

	if (IsValid(GrabComponent) && GrabComponent->IsBeingGrabbed())
	{
		return false;
	}

	const UDIHeatActionDefinition* Definition = nullptr;
	AActor* Target = ResolveContextualHeatActionTarget(Definition);

	return IsValid(Target) && IsValid(Definition);
}

void ADongincheonCharacter::RefreshHeatActionAvailability()
{
	const bool bNewAvailable = CanUseContextualHeatAction();

	if (bHeatActionAvailable == bNewAvailable)
	{
		return;
	}

	bHeatActionAvailable = bNewAvailable;

	UE_LOG(LogTemp, Warning, TEXT("[HEAT_AVAILABILITY] %s"),
	       bHeatActionAvailable ? TEXT("TRUE") : TEXT("FALSE"));

	OnHeatActionAvailabilityChanged.Broadcast(bHeatActionAvailable);
}

void ADongincheonCharacter::HandleHeatChanged(float CurrentHeat, float MaxHeat)
{
	UE_LOG(LogTemp, Warning,
	       TEXT("[HEAT_VALUE] %.1f / %.1f | Definitions=%d | Combat=%s"),
	       CurrentHeat,
	       MaxHeat,
	       HeatActionDefinitions.Num(),
	       bCombatInputEnabled ? TEXT("TRUE") : TEXT("FALSE"));

	RefreshHeatActionAvailability();
}

bool ADongincheonCharacter::IsHeatActionExecuting() const
{
	return IsValid(HeatActionComponent) && HeatActionComponent->IsExecuting();
}

AActor* ADongincheonCharacter::GetActiveHeatActionTarget() const
{
	if (!IsValid(HeatActionComponent))
	{
		return nullptr;
	}

	return HeatActionComponent->GetTargetActor();
}

FGameplayTag ADongincheonCharacter::GetActiveHeatActionTag() const
{
	if (!IsValid(HeatActionComponent))
	{
		return FGameplayTag();
	}

	return HeatActionComponent->GetActiveActionTag();
}

void ADongincheonCharacter::BeginHeatActionPresentationLifecycle(AActor* Target,const FGameplayTag& ActionTag)
{
	if (bHeatActionPresentationActive || !IsValid(Target) || !ActionTag.IsValid())
	{
		return;
	}

	bHeatActionPresentationActive = true;
	ActiveHeatActionPresentationTarget = Target;
	ActiveHeatActionPresentationTag = ActionTag;

	UE_LOG(LogTemp,Warning,
		TEXT("[HEAT_PRESENTATION] START | Target=%s | Tag=%s"),
		*GetNameSafe(Target),
		*ActionTag.ToString());

	OnHeatActionPresentationStarted.Broadcast(
		Target,
		ActionTag);
}

void ADongincheonCharacter::EndHeatActionPresentationLifecycle(
	EDIHeatActionPresentationEndReason EndReason)
{
	if (!bHeatActionPresentationActive)
	{
		return;
	}

	// 정상 완료가 아닌 경우 Tutorial Infinite Heat 해제
	if (EndReason != EDIHeatActionPresentationEndReason::Completed)
	{
		SetTutorialInfiniteHeatEnabled(false);
	}

	AActor* Target = ActiveHeatActionPresentationTarget.Get();
	const FGameplayTag ActionTag = ActiveHeatActionPresentationTag;

	bHeatActionPresentationActive = false;
	ActiveHeatActionPresentationTarget.Reset();
	ActiveHeatActionPresentationTag = FGameplayTag();

	UE_LOG(LogTemp, Warning,
		TEXT("[HEAT_PRESENTATION] END | Reason=%d | Target=%s | Tag=%s"),
		static_cast<int32>(EndReason),
		*GetNameSafe(Target),
		*ActionTag.ToString());

	OnHeatActionPresentationEnded.Broadcast(
		EndReason,
		Target,
		ActionTag);
}

AActor* ADongincheonCharacter::ResolveContextualHeatActionTarget(const UDIHeatActionDefinition*& OutDefinition) const
{
	OutDefinition = nullptr;

	if (!IsValid(HeatActionComponent))
	{
		return nullptr;
	}

	FGameplayTagContainer ContextTags;

	if (IsValid(GrabComponent) && GrabComponent->IsGrabbing())
	{
		ContextTags.AddTag(TAG_DI_HeatAction_Context_GrabActive);

		if (GrabComponent->IsHoldingGrab())
		{
			ContextTags.AddTag(TAG_DI_HeatAction_Context_GrabHolding);
		}
	}

	AActor* BestTarget = nullptr;
	int32 BestPriority = MIN_int32;

	for (const TObjectPtr<UDIHeatActionDefinition>& DefinitionPtr : HeatActionDefinitions)
	{
		UDIHeatActionDefinition* Definition = DefinitionPtr.Get();

		if (!IsValid(Definition))
		{
			UE_LOG(LogTemp, Warning, TEXT("[HEAT_RESOLVE] Invalid Definition"));
			continue;
		}

		const bool bActivationMatches =
			Definition->ActivationQuery.IsEmpty() ||
			Definition->ActivationQuery.Matches(ContextTags);

		if (HeatActionComponent->GetCurrentHeat() >= 100.0f)
		{
			UE_LOG(LogTemp, Warning,
			       TEXT(
				       "[HEAT_RESOLVE] Def=%s | Tag=%s | Source=%d | Activation=%s | Cost=%.1f | MaxDist=%.1f | MinDot=%.2f | PMontage=%s | VMontage=%s"
			       ),
			       *GetNameSafe(Definition),
			       *Definition->ActionTag.ToString(),
			       static_cast<int32>(Definition->TargetSource),
			       bActivationMatches ? TEXT("PASS") : TEXT("FAIL"),
			       Definition->Config.HeatCost,
			       Definition->Config.MaxDistance,
			       Definition->Config.MinForwardDot,
			       *GetNameSafe(Definition->Config.PlayerMontage),
			       *GetNameSafe(Definition->Config.VictimMontage));
		}

		if (!bActivationMatches)
		{
			continue;
		}

		AActor* CandidateTarget = nullptr;

		switch (Definition->TargetSource)
		{
		case EDIHeatActionTargetSource::CurrentGrabTarget:
			if (!IsValid(GrabComponent) || !GrabComponent->IsHoldingGrab())
			{
				if (HeatActionComponent->GetCurrentHeat() >= 100.0f)
				{
					UE_LOG(LogTemp, Warning,
					       TEXT("[HEAT_RESOLVE] %s -> FAIL CurrentGrabTarget: not holding grab"),
					       *GetNameSafe(Definition));
				}

				continue;
			}

			CandidateTarget = GrabComponent->GetGrabbedActor();
			break;

		case EDIHeatActionTargetSource::NearbyTarget:
		default:
			CandidateTarget = HeatActionComponent->FindBestHeatActionTarget(
				Definition->ActionTag,
				Definition->Config);
			break;
		}

		if (HeatActionComponent->GetCurrentHeat() >= 100.0f)
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("[HEAT_RESOLVE] %s -> Candidate=%s"),
			       *GetNameSafe(Definition),
			       *GetNameSafe(CandidateTarget));
		}

		const bool bCanStart = HeatActionComponent->CanStartHeatAction(
			CandidateTarget,
			Definition->ActionTag,
			Definition->Config);

		if (HeatActionComponent->GetCurrentHeat() >= 100.0f)
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("[HEAT_RESOLVE] %s -> CanStart=%s"),
			       *GetNameSafe(Definition),
			       bCanStart ? TEXT("TRUE") : TEXT("FALSE"));
		}

		if (!bCanStart)
		{
			continue;
		}

		if (Definition->Priority <= BestPriority)
		{
			continue;
		}

		BestPriority = Definition->Priority;
		BestTarget = CandidateTarget;
		OutDefinition = Definition;
	}

	return BestTarget;
}

bool ADongincheonCharacter::TryStartContextualHeatAction()
{
	if (!CanUseContextualHeatAction())
	{
		return false;
	}

	const UDIHeatActionDefinition* Definition = nullptr;
	AActor* Target = ResolveContextualHeatActionTarget(Definition);

	if (!IsValid(Target) || !IsValid(Definition))
	{
		return false;
	}

	StartPlayerHeatAction(Target, *Definition);

	RefreshHeatActionAvailability();

	return IsValid(HeatActionComponent) && HeatActionComponent->IsExecuting();
}

void ADongincheonCharacter::StartPlayerHeatAction(AActor* Target, const UDIHeatActionDefinition& Definition)
{
	const FHeatActionConfig& Config = Definition.Config;

	if (!Definition.ActionTag.IsValid() || !IsValid(Target) || !IsValid(HeatActionComponent) ||
		!IsValid(GetMesh()) || !IsValid(Config.PlayerMontage) || !IsValid(Config.VictimMontage))
	{
		return;
	}

	ACharacter* VictimCharacter = Cast<ACharacter>(Target);

	if (!IsValid(VictimCharacter) || !IsValid(VictimCharacter->GetMesh()))
	{
		return;
	}

	UAnimInstance* PlayerAnimInstance = GetMesh()->GetAnimInstance();
	UAnimInstance* VictimAnimInstance = VictimCharacter->GetMesh()->GetAnimInstance();

	if (!IsValid(PlayerAnimInstance) || !IsValid(VictimAnimInstance))
	{
		return;
	}

	const bool bWasGrabHeat = IsValid(GrabComponent) && GrabComponent->IsHoldingGrab() &&
		GrabComponent->GetGrabbedActor() == Target;

	if (!HeatActionComponent->BeginHeatAction(Target, Definition.ActionTag, Config))
	{
		return;
	}

	if (bWasGrabHeat)
	{
		CancelPlayerGrab(0.05f);
	}

	FVector TargetLocation = GetActorLocation() + GetActorForwardVector() * Config.TargetDistance;

	TargetLocation.Z = Target->GetActorLocation().Z;

	FRotator TargetRotation = GetActorRotation();
	TargetRotation.Yaw += Config.VictimYawOffset;

	Target->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr,
	                                    ETeleportType::TeleportPhysics);

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	const float SafePlayRate = Config.PlayRate > 0.0f ? Config.PlayRate : 1.0f;

	const float VictimMontageResult = VictimAnimInstance->Montage_Play(Config.VictimMontage, SafePlayRate);

	if (VictimMontageResult <= 0.0f)
	{
		HeatActionComponent->CancelHeatAction();

		// Victim Montage 재생 실패 시 Infinite Heat 정리
		SetTutorialInfiniteHeatEnabled(false);

		return;
	}

	ActiveHeatActionVictimCharacter = VictimCharacter;
	ActiveHeatActionVictimMontage = Config.VictimMontage;

	const float PlayerMontageResult = PlayerAnimInstance->Montage_Play(Config.PlayerMontage, SafePlayRate);

	if (PlayerMontageResult <= 0.0f)
	{
		StopHeatActionVictimMontage(0.05f);
		HeatActionComponent->CancelHeatAction();

		// Player Montage 재생 실패 시 Infinite Heat 정리
		SetTutorialInfiniteHeatEnabled(false);

		return;
	}

	ActiveHeatActionMontage = Config.PlayerMontage;

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleHeatActionMontageEnded);

	PlayerAnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveHeatActionMontage);
	
	BeginHeatActionPresentationLifecycle(Target,Definition.ActionTag);
}

void ADongincheonCharacter::HandleHeatActionMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveHeatActionMontage)
	{
		return;
	}

	ActiveHeatActionMontage = nullptr;

	StopHeatActionVictimMontage(0.05f);

	if (!IsValid(HeatActionComponent))
	{
		EndHeatActionPresentationLifecycle(EDIHeatActionPresentationEndReason::Interrupted);

		return;
	}

	if (bInterrupted)
	{
		HeatActionComponent->CancelHeatAction();

		EndHeatActionPresentationLifecycle(EDIHeatActionPresentationEndReason::Interrupted);

		return;
	}

	HeatActionComponent->CompleteHeatAction();

	EndHeatActionPresentationLifecycle(EDIHeatActionPresentationEndReason::Completed);
}

void ADongincheonCharacter::CancelPlayerHeatAction(float BlendOutTime)
{
	UAnimMontage* MontageToStop = ActiveHeatActionMontage;
	ActiveHeatActionMontage = nullptr;

	StopHeatActionVictimMontage(BlendOutTime);

	if (IsValid(HeatActionComponent))
	{
		HeatActionComponent->CancelHeatAction();
	}

	EndHeatActionPresentationLifecycle(EDIHeatActionPresentationEndReason::Cancelled);

	if (!IsValid(MontageToStop) || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	FOnMontageEnded EmptyEndDelegate;
	AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate,MontageToStop);

	AnimInstance->Montage_Stop(FMath::Max(0.0f,BlendOutTime),MontageToStop);
}

void ADongincheonCharacter::StopHeatActionVictimMontage(float BlendOutTime)
{
	ACharacter* VictimCharacter = ActiveHeatActionVictimCharacter.Get();
	UAnimMontage* MontageToStop = ActiveHeatActionVictimMontage;

	ActiveHeatActionVictimCharacter.Reset();
	ActiveHeatActionVictimMontage = nullptr;

	if (!IsValid(VictimCharacter) || !IsValid(MontageToStop) || !IsValid(VictimCharacter->GetMesh()))
	{
		return;
	}

	if (UAnimInstance* VictimAnimInstance = VictimCharacter->GetMesh()->GetAnimInstance())
	{
		VictimAnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), MontageToStop);
	}
}

void ADongincheonCharacter::HandleHeavyAttackInput()
{
	if (bPresentationInputLocked)
	{
		OnQTEInputPressed.Broadcast(EQTEInputType::Heavy);
		return;
	}

	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!bCombatInputEnabled)
	{
		return;
	}

	if (!IsValid(HealthComponent) || !IsValid(CombatComponent))
	{
		return;
	}

	if (HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging ||
		CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken())
	{
		return;
	}

	if (IsValid(GrabComponent) && GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	if (TryStartContextualHeatAction())
	{
		return;
	}

	if (IsValid(GrabComponent) && GrabComponent->IsGrabbing())
	{
		return;
	}

	// 아무 Light도 안 친 상태에서 RMB
	if (!bPlayerAttackActive)
	{
		if (NeutralHeavyCombo.IsEmpty())
		{
			return;
		}

		bPlayerAttackActive = true;

		ActiveAttackMode = EPlayerAttackMode::Heavy;
		QueuedAttackType = EQueuedAttackType::None;

		ActiveComboIndex = 0;
		ActiveHeavyBranchIndex = INDEX_NONE;
		ActiveHeavyComboIndex = 0;

		StartPlayerHeavyComboAttack();
		return;
	}

	// Light 도중 RMB -> 현재 Light 단계의 Heavy Branch 예약
	if (ActiveAttackMode == EPlayerAttackMode::LightCombo)
	{
		if (!HeavyBranches.IsValidIndex(ActiveComboIndex))
		{
			return;
		}

		if (HeavyBranches[ActiveComboIndex].Attacks.IsEmpty())
		{
			return;
		}

		QueuedAttackType = EQueuedAttackType::Heavy;
		return;
	}

	// 이미 Heavy Branch 안에 들어온 뒤 RMB -> 다음 Heavy
	if (ActiveAttackMode == EPlayerAttackMode::Heavy)
	{
		const TArray<FAttackConfig>* HeavyCombo = GetActiveHeavyCombo();

		if (!HeavyCombo)
		{
			return;
		}

		if (ActiveHeavyComboIndex + 1 < HeavyCombo->Num())
		{
			QueuedAttackType = EQueuedAttackType::Heavy;
		}
	}
}

//Attack LifeCycle
void ADongincheonCharacter::StartPlayerComboAttack()
{
	if (!ComboAttacks.IsValidIndex(ActiveComboIndex))
	{
		ResetPlayerAttackState();
		return;
	}

	StartPlayerAttack(ComboAttacks[ActiveComboIndex]);
}

void ADongincheonCharacter::StartPlayerHeavyComboAttack()
{
	const TArray<FAttackConfig>* HeavyCombo = GetActiveHeavyCombo();

	if (!HeavyCombo || !HeavyCombo->IsValidIndex(ActiveHeavyComboIndex))
	{
		ResetPlayerAttackState();
		return;
	}

	StartPlayerAttack((*HeavyCombo)[ActiveHeavyComboIndex]);
}

void ADongincheonCharacter::StartPlayerAttack(const FAttackConfig& Attack)
{
	if (!IsValid(CombatComponent) || !IsValid(GetMesh()) || !IsValid(Attack.Montage))
	{
		ResetPlayerAttackState();
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		ResetPlayerAttackState();
		return;
	}

	const float SafePlayRate = Attack.PlayRate > 0.0f ? Attack.PlayRate : 1.0f;

	AActor* AssistTarget = nullptr;

	if (Attack.bUseAttackAssist && IsValid(TargetingComponent))
	{
		AssistTarget = TargetingComponent->FindBestAttackAssistTarget(Attack.MaxAssistDistance,
		                                                              Attack.MinAssistForwardDot);
	}

	CombatComponent->BeginAttack(Attack.Damage, Attack.KnockbackStrength, Attack.bBreaksGuard, Attack.HitTraceRadius,
	                             Attack.ImpactFreezeDuration, Attack.Montage);

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	StartAttackAssistMove(AssistTarget, Attack);

	const float MontageLength = AnimInstance->Montage_Play(Attack.Montage, SafePlayRate,
	                                                       EMontagePlayReturnType::MontageLength, 0.0f,
	                                                       Attack.bStopAllMontages);

	if (MontageLength <= 0.0f)
	{
		ResetPlayerAttackState();
		return;
	}

	ActiveAttackMontage = Attack.Montage;
	
	// Tutorial: 실제 다음 공격 Montage 시작 성공을 기준으로 집계
	if (ActiveAttackMode == EPlayerAttackMode::LightCombo &&
		ActiveComboIndex == 1)
	{
		OnTutorialAction.Broadcast(
			EDIPlayerTutorialAction::LightComboChained);
	}
	else if (ActiveAttackMode == EPlayerAttackMode::Heavy &&
			 ActiveHeavyBranchIndex != INDEX_NONE &&
			 ActiveHeavyComboIndex == 0)
	{
		OnTutorialAction.Broadcast(
			EDIPlayerTutorialAction::LightToHeavyBranched);
	}
	
	// 실제 공격 Montage 재생에 성공했으므로
	// 이번 공격의 Tutorial Hit 집계 허용
	bTutorialHitReportedForActiveAttack = false;

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleAttackMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveAttackMontage);
}

void ADongincheonCharacter::StartAttackAssistMove(AActor* Target, const FAttackConfig& Attack)
{
	StopAttackAssistMove();

	AttackFacingStartYaw = GetActorRotation().Yaw;

	AttackFacingMaxAngle = FMath::Max(Attack.MaxFacingAssistAngle, 0.0f);

	FVector MoveDirection = GetActorForwardVector();

	MoveDirection.Z = 0.0f;

	if (!MoveDirection.Normalize())
	{
		return;
	}

	// 1. Target이 없어도 공격 자체가 가지는 기본 전진
	AttackMoveFallbackDirection = MoveDirection;

	AttackMoveBaseRemainingDistance = FMath::Max(Attack.BaseAdvanceDistance, 0.0f);

	AttackAssistPreferredDistance = FMath::Max(Attack.PreferredTargetDistance, 0.0f);

	AttackAssistMaxDistance = FMath::Max(Attack.MaxAssistDistance, 0.0f);

	float InitialAssistDistance = 0.0f;

	// 2. Target이 Assist Range 안에 있으면
	//    Target Magnetism 활성화
	if (Attack.bUseAttackAssist && IsValid(Target) && AttackAssistPreferredDistance > 0.0f && AttackAssistMaxDistance >
		AttackAssistPreferredDistance)
	{
		FVector ToTarget = Target->GetActorLocation() - GetActorLocation();

		ToTarget.Z = 0.0f;

		const float TargetDistance = ToTarget.Size();

		if (TargetDistance <= AttackAssistMaxDistance && ToTarget.Normalize())
		{
			AttackAssistMoveTarget = Target;

			AttackMoveFallbackDirection = ToTarget;

			InitialAssistDistance = FMath::Max(TargetDistance - AttackAssistPreferredDistance, 0.0f);
		}
	}

	// 3. 실제 필요한 이동거리
	// Base Advance와
	// Target까지 필요한 거리 중 큰 값 사용
	const float InitialMoveDistance = FMath::Max(AttackMoveBaseRemainingDistance, InitialAssistDistance);

	if (InitialMoveDistance <= KINDA_SMALL_NUMBER)
	{
		StopAttackAssistMove();
		return;
	}

	AttackMoveInitialDistance = InitialMoveDistance;

	// 4. Duration은 이동 종료시간이 아님
	//    접근 속도를 계산하는 기준값일 뿐
	const float SafeDuration = FMath::Max(Attack.AttackAssistDuration, 0.01f);

	AttackMoveBaseSpeed = FMath::Min(InitialMoveDistance / SafeDuration, AttackAssistMaxSpeed);

	bAttackMoveWindowRequired = Attack.bUseAttackMoveWindow;

	bAttackMoveWindowOpen = !bAttackMoveWindowRequired;

	if (AttackMoveBaseSpeed <= KINDA_SMALL_NUMBER)
	{
		StopAttackAssistMove();
	}
}

void ADongincheonCharacter::UpdateAttackAssistMove(float DeltaTime)
{
	if (AttackMoveBaseSpeed <= 0.0f)
	{
		StopAttackAssistMove();
		return;
	}

	// Move Window 공격이면 Notify가 Window를 열기 전에는
	// Runtime Data만 유지하고 실제 이동은 하지 않는다.
	if (bAttackMoveWindowRequired && !bAttackMoveWindowOpen)
	{
		return;
	}

	if (bAttackMoveWindowRequired && AttackMoveWindowTimeRemaining <= KINDA_SMALL_NUMBER)
	{
		bAttackMoveWindowOpen = false;
		return;
	}

	// Target을 잃으면 마지막 유효 방향,
	// Target 자체가 없었던 공격이면 공격 시작 Forward.
	FVector MoveDirection = AttackMoveFallbackDirection;

	float AssistRemainingDistance = 0.0f;

	AActor* Target = AttackAssistMoveTarget.Get();

	// Target Magnetism
	if (IsValid(Target))
	{
		FVector ToTarget = Target->GetActorLocation() - GetActorLocation();

		ToTarget.Z = 0.0f;

		const float TargetDistance = ToTarget.Size();

		if (TargetDistance <= AttackAssistMaxDistance && ToTarget.Normalize())
		{
			MoveDirection = ToTarget;

			AttackMoveFallbackDirection = ToTarget;

			AssistRemainingDistance = FMath::Max(TargetDistance - AttackAssistPreferredDistance, 0.0f);
		}
		else
		{
			// Target 추적만 종료.
			// 공격 자체 Base Advance는 계속 가능.
			AttackAssistMoveTarget.Reset();
			Target = nullptr;
		}
	}

	// --------------------------------------------------
	// 실제 남은 이동거리
	//
	// Target이 있으면 Preferred까지 필요한 거리,
	// Target이 없어도 BaseAdvance는 보장.
	// --------------------------------------------------

	const float RemainingDistance = FMath::Max(AttackMoveBaseRemainingDistance, AssistRemainingDistance);

	if (RemainingDistance <= KINDA_SMALL_NUMBER)
	{
		StopAttackAssistMove();
		return;
	}

	if (!MoveDirection.Normalize())
	{
		StopAttackAssistMove();
		return;
	}

	// --------------------------------------------------
	// Facing Follow
	//
	// Target이 살아있을 때만 타겟 방향으로 회전.
	// 공격 시작 방향 기준 MaxFacingAssistAngle을 넘지 않는다.
	// --------------------------------------------------

	if (IsValid(Target) && AttackFacingMaxAngle > 0.0f && AttackFacingFollowSpeed > 0.0f)
	{
		const float DesiredYaw = MoveDirection.Rotation().Yaw;

		const float DeltaFromAttackStart = FMath::FindDeltaAngleDegrees(AttackFacingStartYaw, DesiredYaw);

		const float ClampedDelta = FMath::Clamp(DeltaFromAttackStart, -AttackFacingMaxAngle, AttackFacingMaxAngle);

		const float AllowedTargetYaw = FRotator::NormalizeAxis(AttackFacingStartYaw + ClampedDelta);

		FRotator CurrentRotation = GetActorRotation();

		CurrentRotation.Pitch = 0.0f;
		CurrentRotation.Roll = 0.0f;

		CurrentRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, AllowedTargetYaw,
		                                       AttackFacingFollowSpeed * DeltaTime);

		SetActorRotation(CurrentRotation);
	}

	// --------------------------------------------------
	// Move Speed
	//
	// Legacy / Window 미사용 공격:
	// 기존 AttackMoveBaseSpeed 사용.
	//
	// Move Window 공격:
	// 남은 거리 / 남은 Window 시간으로 매 Tick 재계산.
	// --------------------------------------------------

	float CurrentMoveSpeed = AttackMoveBaseSpeed;

	if (bAttackMoveWindowRequired)
	{
		const float SafeRemainingTime = FMath::Max(AttackMoveWindowTimeRemaining, DeltaTime);

		const float RequiredMoveSpeed = RemainingDistance / SafeRemainingTime;

		CurrentMoveSpeed = FMath::Min(RequiredMoveSpeed, AttackAssistMaxSpeed);
	}

	const float MoveDistance = FMath::Min(CurrentMoveSpeed * DeltaTime, RemainingDistance);

	if (MoveDistance <= KINDA_SMALL_NUMBER)
	{
		StopAttackAssistMove();
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!IsValid(Movement))
	{
		StopAttackAssistMove();
		return;
	}

	const FVector BeforeMoveLocation = GetActorLocation();

	FHitResult Hit;

	Movement->SafeMoveUpdatedComponent(MoveDirection * MoveDistance, GetActorQuat(), true, Hit);

	const float ActualMovedDistance = FVector::Dist2D(BeforeMoveLocation, GetActorLocation());

	// Base Advance 예산 감소
	AttackMoveBaseRemainingDistance = FMath::Max(AttackMoveBaseRemainingDistance - ActualMovedDistance, 0.0f);

	// Move Window 남은 시간 감소
	if (bAttackMoveWindowRequired)
	{
		AttackMoveWindowTimeRemaining = FMath::Max(AttackMoveWindowTimeRemaining - DeltaTime, 0.0f);

		if (AttackMoveWindowTimeRemaining <= KINDA_SMALL_NUMBER)
		{
			bAttackMoveWindowOpen = false;
		}
	}

	// 벽 / Capsule 등에 실제로 막힘
	if (Hit.bBlockingHit)
	{
		StopAttackAssistMove();
		return;
	}

	// 비정상 stuck 방지
	if (ActualMovedDistance <= KINDA_SMALL_NUMBER)
	{
		StopAttackAssistMove();
	}
}

void ADongincheonCharacter::StopAttackAssistMove()
{
	AttackAssistMoveTarget.Reset();

	AttackMoveFallbackDirection = FVector::ZeroVector;

	AttackAssistPreferredDistance = 0.0f;
	AttackAssistMaxDistance = 0.0f;

	AttackMoveBaseRemainingDistance = 0.0f;
	AttackMoveInitialDistance = 0.0f;
	AttackMoveBaseSpeed = 0.0f;

	AttackMoveWindowTimeRemaining = 0.0f;

	AttackFacingStartYaw = 0.0f;
	AttackFacingMaxAngle = 0.0f;

	bAttackMoveWindowRequired = false;
	bAttackMoveWindowOpen = false;
}

void ADongincheonCharacter::BeginAttackMoveWindow(float MoveWindowDuration)
{
	if (!bPlayerAttackActive || !bAttackMoveWindowRequired || AttackMoveBaseSpeed <= 0.0f || MoveWindowDuration <= 0.0f)
	{
		return;
	}

	AttackMoveWindowTimeRemaining = MoveWindowDuration;

	bAttackMoveWindowOpen = true;
}

void ADongincheonCharacter::EndAttackMoveWindow()
{
	if (!bAttackMoveWindowRequired)
	{
		return;
	}

	bAttackMoveWindowOpen = false;

	AttackMoveWindowTimeRemaining = 0.0f;
}

bool ADongincheonCharacter::TryAdvanceQueuedCombo()
{
	if (!bPlayerAttackActive || QueuedAttackType == EQueuedAttackType::None || bPlayerHitReacting || bPlayerDeathStarted
		|| !IsValid(ActiveAttackMontage) || !IsValid(GetMesh()) || (IsValid(HealthComponent) && HealthComponent->
			IsDead())) return false;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!IsValid(AnimInstance) || !AnimInstance->Montage_IsActive(ActiveAttackMontage)) return false;

	bool bAdvanceLight = false;
	bool bEnterHeavy = false;
	bool bAdvanceHeavy = false;

	if (ActiveAttackMode == EPlayerAttackMode::LightCombo)
	{
		if (QueuedAttackType == EQueuedAttackType::Heavy && HeavyBranches.IsValidIndex(ActiveComboIndex) && !
			HeavyBranches[ActiveComboIndex].Attacks.IsEmpty()) bEnterHeavy = true;
		else if (QueuedAttackType == EQueuedAttackType::Light && ActiveComboIndex + 1 < ComboAttacks.Num())
			bAdvanceLight = true;
	}
	else if (ActiveAttackMode == EPlayerAttackMode::Heavy && QueuedAttackType == EQueuedAttackType::Heavy)
	{
		const TArray<FAttackConfig>* HeavyCombo = GetActiveHeavyCombo();
		if (HeavyCombo && ActiveHeavyComboIndex + 1 < HeavyCombo->Num()) bAdvanceHeavy = true;
	}

	if (!bAdvanceLight && !bEnterHeavy && !bAdvanceHeavy) return false;

	UAnimMontage* PreviousMontage = ActiveAttackMontage;

	FOnMontageEnded EmptyEndDelegate;
	AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, PreviousMontage);

	ActiveAttackMontage = nullptr;

	StopAttackAssistMove();
	ClearHitStop();
	if (IsValid(CombatComponent)) CombatComponent->EndAttack();

	AnimInstance->Montage_Stop(0.03f, PreviousMontage);

	if (bEnterHeavy)
	{
		ActiveAttackMode = EPlayerAttackMode::Heavy;
		ActiveHeavyBranchIndex = ActiveComboIndex;
		ActiveHeavyComboIndex = 0;
		QueuedAttackType = EQueuedAttackType::None;

		UE_LOG(LogTemp, Warning, TEXT("[COMBO_CHAIN] Light %d -> Heavy 0"), ActiveComboIndex);

		StartPlayerHeavyComboAttack();
		return true;
	}

	if (bAdvanceLight)
	{
		++ActiveComboIndex;
		QueuedAttackType = EQueuedAttackType::None;

		UE_LOG(LogTemp, Warning, TEXT("[COMBO_CHAIN] -> Light %d"), ActiveComboIndex);

		StartPlayerComboAttack();
		return true;
	}

	++ActiveHeavyComboIndex;
	QueuedAttackType = EQueuedAttackType::None;

	UE_LOG(LogTemp, Warning, TEXT("[COMBO_CHAIN] -> Heavy %d"), ActiveHeavyComboIndex);

	StartPlayerHeavyComboAttack();
	return true;
}

const TArray<FAttackConfig>* ADongincheonCharacter::GetActiveHeavyCombo() const
{
	// Light 없이 시작한 RMB 체인
	if (ActiveHeavyBranchIndex == INDEX_NONE)
	{
		return &NeutralHeavyCombo;
	}

	// Light N타 후 들어온 Heavy Branch
	if (!HeavyBranches.IsValidIndex(ActiveHeavyBranchIndex))
	{
		return nullptr;
	}

	return &HeavyBranches[ActiveHeavyBranchIndex].Attacks;
}

void ADongincheonCharacter::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveAttackMontage)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[PLAYER_ATTACK_END] Montage=%s | Interrupted=%s"),
		*GetNameSafe(Montage),
		bInterrupted ? TEXT("TRUE") : TEXT("FALSE"));

	ActiveAttackMontage = nullptr;

	StopAttackAssistMove();

	ClearHitStop();

	if (IsValid(CombatComponent))
	{
		CombatComponent->EndAttack();
	}

	if (bInterrupted || bPlayerHitReacting || bPlayerDeathStarted || (IsValid(HealthComponent) && HealthComponent->
		IsDead()))
	{
		bPlayerAttackActive = false;
		ActiveAttackMode = EPlayerAttackMode::None;
		QueuedAttackType = EQueuedAttackType::None;

		ActiveComboIndex = 0;
		ActiveHeavyBranchIndex = INDEX_NONE;
		ActiveHeavyComboIndex = INDEX_NONE;
		return;
	}

	// Light Combo가 끝난 경우
	if (ActiveAttackMode == EPlayerAttackMode::LightCombo)
	{
		// RMB가 예약되어 있으면 현재 Light 단계의 Heavy Branch로 진입
		if (QueuedAttackType == EQueuedAttackType::Heavy)
		{
			if (HeavyBranches.IsValidIndex(ActiveComboIndex) && !HeavyBranches[ActiveComboIndex].Attacks.IsEmpty())
			{
				ActiveAttackMode = EPlayerAttackMode::Heavy;

				ActiveHeavyBranchIndex = ActiveComboIndex;
				ActiveHeavyComboIndex = 0;

				QueuedAttackType = EQueuedAttackType::None;

				StartPlayerHeavyComboAttack();
				return;
			}
		}

		// LMB가 예약되어 있으면 다음 Light
		if (QueuedAttackType == EQueuedAttackType::Light && ActiveComboIndex + 1 < ComboAttacks.Num())
		{
			QueuedAttackType = EQueuedAttackType::None;

			++ActiveComboIndex;

			StartPlayerComboAttack();
			return;
		}
	}

	// Heavy Combo가 끝난 경우
	if (ActiveAttackMode == EPlayerAttackMode::Heavy && QueuedAttackType == EQueuedAttackType::Heavy)
	{
		const TArray<FAttackConfig>* HeavyCombo =
			GetActiveHeavyCombo();

		if (HeavyCombo &&
			ActiveHeavyComboIndex + 1 < HeavyCombo->Num())
		{
			QueuedAttackType = EQueuedAttackType::None;

			++ActiveHeavyComboIndex;

			StartPlayerHeavyComboAttack();
			return;
		}
	}

	// 더 이상 이어질 공격 없음
	bPlayerAttackActive = false;
	ActiveAttackMode = EPlayerAttackMode::None;
	QueuedAttackType = EQueuedAttackType::None;

	ActiveComboIndex = 0;
	ActiveHeavyBranchIndex = INDEX_NONE;
	ActiveHeavyComboIndex = INDEX_NONE;
}

void ADongincheonCharacter::CancelPlayerAttack(float BlendOutTime)
{
	UAnimMontage* MontageToStop = ActiveAttackMontage;

	ActiveAttackMontage = nullptr;

	StopAttackAssistMove();

	ClearHitStop();

	if (IsValid(CombatComponent))
	{
		CombatComponent->EndAttack();
	}

	bPlayerAttackActive = false;

	ActiveAttackMode = EPlayerAttackMode::None;
	QueuedAttackType = EQueuedAttackType::None;

	ActiveComboIndex = 0;
	ActiveHeavyBranchIndex = INDEX_NONE;
	ActiveHeavyComboIndex = INDEX_NONE;

	if (!IsValid(MontageToStop) || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), MontageToStop);
}

void ADongincheonCharacter::ResetPlayerAttackState()
{
	ActiveAttackMontage = nullptr;

	StopAttackAssistMove();

	ClearHitStop();

	if (IsValid(CombatComponent))
	{
		CombatComponent->EndAttack();
	}

	bPlayerAttackActive = false;

	ActiveAttackMode = EPlayerAttackMode::None;
	QueuedAttackType = EQueuedAttackType::None;

	ActiveComboIndex = 0;
	ActiveHeavyBranchIndex = INDEX_NONE;
	ActiveHeavyComboIndex = INDEX_NONE;
}

//Health
void ADongincheonCharacter::HandleHealthDamaged(float DamageAmount, AActor* DamageCauser)
{
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}

	if (IsValid(GrabComponent) && GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	StartPlayerHitReact();
}

void ADongincheonCharacter::HandleHealthDeath(AActor* DamageCauser)
{
	StartPlayerDeath();
}

//Dodge
void ADongincheonCharacter::HandleDodgeInput()
{
	if (bPresentationInputLocked)
	{
		OnQTEInputPressed.Broadcast(EQTEInputType::Dodge);
		return;
	}

	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!bCombatInputEnabled)
	{
		return;
	}

	if (!IsValid(HealthComponent) || HealthComponent->IsDead())
	{
		return;
	}

	if (bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging || bPlayerAttackActive || (IsValid(CombatComponent)
		&&
		(CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken())))
	{
		return;
	}

	StartPlayerDodge();
}

void ADongincheonCharacter::HandleGrabInput()
{
	// Interaction 중 Q는 Grab보다 Cancel 우선.
	if (IsValid(InteractionComponent))
	{
		if (InteractionComponent->CancelCurrentInteraction())
		{
			return;
		}
	}

	// Cancel할 Interaction이 없을 때만 일반 Gameplay Lock 판정.
	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!bCombatInputEnabled)
	{
		return;
	}

	if (!IsValid(HealthComponent) || !IsValid(CombatComponent) || !IsValid(GrabComponent))
	{
		return;
	}

	if (HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging ||
		bPlayerAttackActive || CombatComponent->IsGuarding() || CombatComponent->IsGuardBroken())
	{
		return;
	}

	if (GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	if (GrabComponent->IsHoldingGrab())
	{
		StartPlayerGrabRelease();
		return;
	}

	if (GrabComponent->IsGrabbing())
	{
		return;
	}

	if (IsValid(ActiveGrabStartMontage))
	{
		return;
	}

	StartPlayerGrabAttempt();
}

AActor* ADongincheonCharacter::FindBestGrabTarget() const
{
	if (!IsValid(GrabComponent) || !IsValid(GetWorld()) || GrabSearchRadius <= 0.0f)
	{
		return nullptr;
	}

	TArray<FOverlapResult> Overlaps;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PlayerGrabSearch), false, this);

	const bool bFoundAny = GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
	                                                            ObjectQueryParams,
	                                                            FCollisionShape::MakeSphere(GrabSearchRadius),
	                                                            QueryParams);

	if (!bFoundAny)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	const FVector PlayerLocation = GetActorLocation();
	const FVector PlayerForward = GetActorForwardVector().GetSafeNormal2D();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();

		if (!IsValid(Candidate) || Candidate == this)
		{
			continue;
		}

		if (!GrabComponent->CanStartGrab(Candidate))
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - PlayerLocation;

		const FVector Direction = ToCandidate.GetSafeNormal2D();

		if (Direction.IsNearlyZero())
		{
			continue;
		}

		const float ForwardDot = FVector::DotProduct(PlayerForward, Direction);

		if (ForwardDot < GrabMinForwardDot)
		{
			continue;
		}

		const float DistanceSquared = ToCandidate.SizeSquared2D();

		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

void ADongincheonCharacter::StartPlayerGrabAttempt()
{
	if (!IsValid(GrabComponent) || !IsValid(GetMesh()) || !IsValid(GrabConfig.GrabStartMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;
	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.GrabStartMontage, SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		return;
	}

	ActiveGrabStartMontage = GrabConfig.GrabStartMontage;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement()) Movement->StopMovementImmediately();

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleGrabStartMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveGrabStartMontage);
}

void ADongincheonCharacter::TryCommitPlayerGrab()
{
	if (!IsValid(GrabComponent) || !IsValid(ActiveGrabStartMontage))
	{
		return;
	}

	if (GrabComponent->IsGrabbing() || GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	AActor* Target = FindBestGrabTarget();

	if (!IsValid(Target))
	{
		return;
	}

	GrabComponent->BeginGrab(Target);
}

void ADongincheonCharacter::HandleGrabStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveGrabStartMontage)
	{
		return;
	}

	ActiveGrabStartMontage = nullptr;

	if (!IsValid(GrabComponent))
	{
		return;
	}

	if (bInterrupted)
	{
		if (GrabComponent->IsGrabbing())
		{
			GrabComponent->ForceRelease();
		}

		return;
	}

	if (GrabComponent->GetGrabState() != EDIGrabState::Starting)
	{
		return;
	}

	GrabComponent->CompleteGrabStart(GrabConfig.GrabHoldDistance, GrabConfig.GrabVictimYawOffset);

	if (GrabComponent->IsHoldingGrab())
	{
		PlayPlayerGrabHold();
	}
}


void ADongincheonCharacter::PlayPlayerGrabHold()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsHoldingGrab() || !IsValid(GetMesh()) || !IsValid(
		GrabConfig.GrabHoldMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;
	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.GrabHoldMontage, SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		return;
	}

	ActiveGrabHoldMontage = GrabConfig.GrabHoldMontage;
}

void ADongincheonCharacter::StopPlayerGrabHold(float BlendOutTime)
{
	UAnimMontage* MontageToStop = ActiveGrabHoldMontage;
	ActiveGrabHoldMontage = nullptr;

	if (!IsValid(MontageToStop) || !IsValid(GetMesh()))
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), MontageToStop);
	}
}

void ADongincheonCharacter::StartGrabAutoReleaseTimer()
{
	if (GrabAutoReleaseDuration <= 0.0f || bPendingGrabAutoRelease)
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerActive(GrabAutoReleaseTimerHandle))
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		GrabAutoReleaseTimerHandle,
		this,
		&ADongincheonCharacter::HandleGrabAutoReleaseExpired,
		GrabAutoReleaseDuration,
		false);
}

void ADongincheonCharacter::ClearGrabAutoReleaseTimer()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(GrabAutoReleaseTimerHandle);
	}

	bPendingGrabAutoRelease = false;
}

void ADongincheonCharacter::HandleGrabAutoReleaseExpired()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsGrabbing())
	{
		ClearGrabAutoReleaseTimer();
		return;
	}

	if (GrabComponent->GetGrabState() == EDIGrabState::Attacking)
	{
		bPendingGrabAutoRelease = true;
		return;
	}

	if (GrabComponent->IsHoldingGrab())
	{
		StartPlayerGrabRelease();
		return;
	}

	ClearGrabAutoReleaseTimer();
}

void ADongincheonCharacter::StartPlayerGrabAttack()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsHoldingGrab() || !IsValid(GetMesh()) || !IsValid(
		GrabConfig.GrabAttackMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	if (!GrabComponent->BeginGrabAttack(GrabConfig.GrabAttackDamage))
	{
		return;
	}

	StopPlayerGrabHold(0.05f);

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.GrabAttackMontage, SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		GrabComponent->EndGrabAttack();
		PlayPlayerGrabHold();
		return;
	}

	ActiveGrabAttackMontage = GrabConfig.GrabAttackMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleGrabAttackMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveGrabAttackMontage);
}

void ADongincheonCharacter::HandleGrabAttackMontageEnded(UAnimMontage* Montage,bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveGrabAttackMontage)
	{
		return;
	}

	ActiveGrabAttackMontage = nullptr;

	if (!IsValid(GrabComponent))
	{
		return;
	}

	// Grab Attack 정상 종료 시 Attacking 상태를 반드시 끝낸다.
	if (GrabComponent->GetGrabState() == EDIGrabState::Attacking)
	{
		GrabComponent->EndGrabAttack();
	}
	
	if (bPendingGrabAutoRelease)
	{
		if (GrabComponent->IsHoldingGrab())
		{
			StartPlayerGrabRelease();
			return;
		}

		ClearGrabAutoReleaseTimer();

		if (GrabComponent->IsGrabbing())
		{
			GrabComponent->ForceRelease();
		}

		return;
	}

	// 상대가 아직 Grab 상태면 다시 Hold로 복귀.
	if (GrabComponent->IsHoldingGrab() &&
		IsValid(GrabComponent->GetGrabbedActor()))
	{
		PlayPlayerGrabHold();
		return;
	}

	// 공격 도중 상대 Grab이 깨졌다면 공격자 쪽 상태도 반드시 정리.
	if (GrabComponent->IsGrabbing())
	{
		GrabComponent->ForceRelease();
	}
}

void ADongincheonCharacter::StartPlayerGrabRelease()
{
	ClearGrabAutoReleaseTimer();
	
	if (!IsValid(GrabComponent) || !GrabComponent->IsHoldingGrab())
	{
		return;
	}

	if (!GrabComponent->BeginReleaseGrab())
	{
		return;
	}

	StopPlayerGrabHold(0.05f);

	if (!IsValid(GrabConfig.GrabReleaseMontage) || !IsValid(GetMesh()))
	{
		GrabComponent->CompleteReleaseGrab();
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		GrabComponent->CompleteReleaseGrab();
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.GrabReleaseMontage, SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		GrabComponent->CompleteReleaseGrab();
		return;
	}

	ActiveGrabReleaseMontage = GrabConfig.GrabReleaseMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleGrabReleaseMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveGrabReleaseMontage);
}

void ADongincheonCharacter::HandleGrabReleaseMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveGrabReleaseMontage)
	{
		return;
	}

	ActiveGrabReleaseMontage = nullptr;

	if (!IsValid(GrabComponent))
	{
		return;
	}

	if (bInterrupted)
	{
		GrabComponent->ForceRelease();
		return;
	}

	GrabComponent->CompleteReleaseGrab();
}

void ADongincheonCharacter::HandleGrabStateChanged(EDIGrabState PreviousState, EDIGrabState NewState)
{
	if (NewState == EDIGrabState::Holding)
	{
		StartGrabAutoReleaseTimer();

		if (IsValid(GrabComponent) &&
			GrabComponent->IsHoldingGrab() &&
			IsValid(GrabComponent->GetGrabbedActor()))
		{
			OnTutorialAction.Broadcast(
				EDIPlayerTutorialAction::GrabSucceeded);
		}
	}
	else if (NewState == EDIGrabState::None || NewState == EDIGrabState::Releasing)
	{
		ClearGrabAutoReleaseTimer();
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (PreviousState == EDIGrabState::None && NewState == EDIGrabState::Starting)
		{
			PreGrabWalkSpeed = Movement->MaxWalkSpeed;
			bGrabWalkSpeedOverridden = true;
			Movement->StopMovementImmediately();
		}

		if (NewState == EDIGrabState::Holding && bGrabWalkSpeedOverridden)
		{
			Movement->MaxWalkSpeed = FMath::Max(0.0f, GrabConfig.GrabMoveSpeed);
		}

		if (NewState == EDIGrabState::Attacking || NewState == EDIGrabState::Releasing)
		{
			Movement->StopMovementImmediately();
		}

		if (NewState == EDIGrabState::BeingGrabbed)
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}

		if (PreviousState == EDIGrabState::BeingGrabbed && NewState == EDIGrabState::None)
		{
			if (!bPlayerDeathStarted && IsValid(HealthComponent) && !HealthComponent->IsDead())
			{
				Movement->SetMovementMode(MOVE_Walking);
			}
		}

		if (NewState == EDIGrabState::None && bGrabWalkSpeedOverridden)
		{
			PreGrabWalkSpeed = 0.0f;
			bGrabWalkSpeedOverridden = false;
			RefreshMovementSpeed();
		}
	}

	if (NewState == EDIGrabState::BeingGrabbed)
	{
		StartPlayerBeingGrabbed();
		return;
	}

	if (PreviousState == EDIGrabState::BeingGrabbed && NewState == EDIGrabState::None)
	{
		if (IsValid(GetMesh()))
		{
			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				if (IsValid(ActiveBeingGrabbedStartMontage))
				{
					AnimInstance->Montage_Stop(0.15f, ActiveBeingGrabbedStartMontage);
				}

				if (IsValid(ActiveBeingGrabbedHoldMontage))
				{
					AnimInstance->Montage_Stop(0.15f, ActiveBeingGrabbedHoldMontage);
				}

				if (IsValid(ActiveBeingGrabbedHitReactMontage))
				{
					AnimInstance->Montage_Stop(0.15f, ActiveBeingGrabbedHitReactMontage);
				}
			}
		}

		ActiveBeingGrabbedStartMontage = nullptr;
		ActiveBeingGrabbedHoldMontage = nullptr;
		ActiveBeingGrabbedHitReactMontage = nullptr;
	}
}

void ADongincheonCharacter::StartPlayerBeingGrabbed()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed() || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	if (!IsValid(GrabConfig.BeingGrabbedStartMontage))
	{
		PlayPlayerBeingGrabbedHold();
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.BeingGrabbedStartMontage, SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		PlayPlayerBeingGrabbedHold();
		return;
	}

	ActiveBeingGrabbedStartMontage = GrabConfig.BeingGrabbedStartMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleBeingGrabbedStartMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveBeingGrabbedStartMontage);
}

void ADongincheonCharacter::HandleBeingGrabbedStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveBeingGrabbedStartMontage)
	{
		return;
	}

	ActiveBeingGrabbedStartMontage = nullptr;

	if (bInterrupted)
	{
		return;
	}

	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	PlayPlayerBeingGrabbedHold();
}

void ADongincheonCharacter::PlayPlayerBeingGrabbedHold()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed() || !IsValid(GetMesh()) ||
		!IsValid(GrabConfig.BeingGrabbedHoldMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.BeingGrabbedHoldMontage, SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		return;
	}

	ActiveBeingGrabbedHoldMontage = GrabConfig.BeingGrabbedHoldMontage;
}

void ADongincheonCharacter::HandleGrabAttackReceived(float Damage, AActor* DamageCauser)
{
	if (Damage <= 0.0f || !IsValid(DamageCauser) || !IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}

	StartPlayerBeingGrabbedHitReact();
}

void ADongincheonCharacter::HandleGrabAttackHitConfirmed(AActor* HitActor, float AppliedDamage)
{
	if (!IsValid(HitActor) || AppliedDamage <= 0.0f)
	{
		return;
	}
	
	OnTutorialAction.Broadcast(
	EDIPlayerTutorialAction::GrabAttackHit);

	if (IsValid(HeatActionComponent))
	{
		HeatActionComponent->AddHeat(HeatGainPerHit);
	}
}

void ADongincheonCharacter::StartPlayerBeingGrabbedHitReact()
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed() || !IsValid(GetMesh())
		|| !IsValid(GrabConfig.BeingGrabbedHitReactMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	if (IsValid(ActiveBeingGrabbedHitReactMontage) && AnimInstance->
		Montage_IsPlaying(ActiveBeingGrabbedHitReactMontage))
	{
		return;
	}

	if (IsValid(ActiveBeingGrabbedHoldMontage))
	{
		AnimInstance->Montage_Stop(0.05f, ActiveBeingGrabbedHoldMontage);

		ActiveBeingGrabbedHoldMontage = nullptr;
	}

	const float SafePlayRate = GrabConfig.GrabPlayRate > 0.0f ? GrabConfig.GrabPlayRate : 1.0f;

	const float MontageResult = AnimInstance->Montage_Play(GrabConfig.BeingGrabbedHitReactMontage, SafePlayRate);

	if (MontageResult <= 0.0f)
	{
		PlayPlayerBeingGrabbedHold();
		return;
	}

	ActiveBeingGrabbedHitReactMontage = GrabConfig.BeingGrabbedHitReactMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleBeingGrabbedHitReactMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveBeingGrabbedHitReactMontage);
}

void ADongincheonCharacter::HandleBeingGrabbedHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveBeingGrabbedHitReactMontage)
	{
		return;
	}

	ActiveBeingGrabbedHitReactMontage = nullptr;

	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}

	if (!IsValid(GrabComponent) || !GrabComponent->IsBeingGrabbed())
	{
		return;
	}

	PlayPlayerBeingGrabbedHold();
}

void ADongincheonCharacter::CancelPlayerGrab(float BlendOutTime)
{
	ClearGrabAutoReleaseTimer();
	
	StopPlayerGrabHold(BlendOutTime);

	if (IsValid(GrabComponent))
	{
		GrabComponent->ForceRelease();
	}

	if (!IsValid(GetMesh()))
	{
		ActiveGrabStartMontage = nullptr;
		ActiveGrabAttackMontage = nullptr;
		ActiveGrabReleaseMontage = nullptr;
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		ActiveGrabStartMontage = nullptr;
		ActiveGrabAttackMontage = nullptr;
		ActiveGrabReleaseMontage = nullptr;
		return;
	}

	const float SafeBlendOutTime = FMath::Max(0.0f, BlendOutTime);

	if (IsValid(ActiveGrabStartMontage))
	{
		UAnimMontage* MontageToStop = ActiveGrabStartMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, MontageToStop);

		ActiveGrabStartMontage = nullptr;

		AnimInstance->Montage_Stop(SafeBlendOutTime, MontageToStop);
	}

	if (IsValid(ActiveGrabAttackMontage))
	{
		UAnimMontage* MontageToStop = ActiveGrabAttackMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, MontageToStop);

		ActiveGrabAttackMontage = nullptr;

		AnimInstance->Montage_Stop(SafeBlendOutTime, MontageToStop);
	}

	if (IsValid(ActiveGrabReleaseMontage))
	{
		UAnimMontage* MontageToStop = ActiveGrabReleaseMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, MontageToStop);

		ActiveGrabReleaseMontage = nullptr;

		AnimInstance->Montage_Stop(SafeBlendOutTime, MontageToStop);
	}
}

void ADongincheonCharacter::StartPlayerDodge()
{
	if (!IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const EPlayerDodgeDirection DodgeDirection = ResolvePlayerDodgeDirection();

	UAnimMontage* MontageToPlay = GetDodgeMontage(DodgeDirection);

	if (!IsValid(MontageToPlay))
	{
		return;
	}

	const FVector DodgeWorldDirection = GetDodgeWorldDirection(DodgeDirection);

	if (DodgeWorldDirection.IsNearlyZero())
	{
		return;
	}

	bPlayerDodging = true;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();

		if (DodgeStrength > 0.0f)
		{
			Movement->AddImpulse(DodgeWorldDirection * DodgeStrength, true);
		}
	}

	const float SafePlayRate = DodgePlayRate > 0.0f ? DodgePlayRate : 1.0f;
	const float MontageLength = AnimInstance->Montage_Play(MontageToPlay, SafePlayRate,
	                                                       EMontagePlayReturnType::MontageLength,
	                                                       0.0f, bDodgeStopAllMontages);

	if (MontageLength <= 0.0f)
	{
		ResetPlayerDodgeState();
		return;
	}

	ActiveDodgeMontage = MontageToPlay;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleDodgeMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveDodgeMontage);
	
	OnTutorialAction.Broadcast(EDIPlayerTutorialAction::DodgePerformed);
}

EPlayerDodgeDirection ADongincheonCharacter::ResolvePlayerDodgeDirection() const
{
	if (CachedMoveInput.IsNearlyZero()) return EPlayerDodgeDirection::Backward;

	if (!IsValid(TargetingComponent) || !TargetingComponent->IsLockedOn())
	{
		if (CachedMoveInput.X < -0.1f) return EPlayerDodgeDirection::Left;
		if (CachedMoveInput.X > 0.1f) return EPlayerDodgeDirection::Right;
		return EPlayerDodgeDirection::Backward;
	}

	const FRotator CameraYaw(0.0f, GetControlRotation().Yaw, 0.0f);
	const FVector CameraForward = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::X);
	const FVector CameraRight = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y);
	const FVector InputWorldDirection = (CameraForward * CachedMoveInput.Y + CameraRight * CachedMoveInput.X).
		GetSafeNormal2D();

	const FVector CharacterForward = GetActorForwardVector().GetSafeNormal2D();
	const FVector CharacterRight = GetActorRightVector().GetSafeNormal2D();

	const float ForwardDot = FVector::DotProduct(InputWorldDirection, CharacterForward);
	const float RightDot = FVector::DotProduct(InputWorldDirection, CharacterRight);

	if (ForwardDot < -0.35f) return EPlayerDodgeDirection::Backward;
	if (FMath::Abs(RightDot) < 0.35f) return EPlayerDodgeDirection::Backward;

	return RightDot < 0.0f ? EPlayerDodgeDirection::Left : EPlayerDodgeDirection::Right;
}

UAnimMontage* ADongincheonCharacter::GetDodgeMontage(EPlayerDodgeDirection Direction) const
{
	switch (Direction)
	{
	case EPlayerDodgeDirection::Backward:
		return DodgeBackwardMontage;

	case EPlayerDodgeDirection::Left:
		return DodgeLeftMontage;

	case EPlayerDodgeDirection::Right:
		return DodgeRightMontage;

	default:
		return nullptr;
	}
}

FVector ADongincheonCharacter::GetDodgeWorldDirection(EPlayerDodgeDirection Direction) const
{
	switch (Direction)
	{
	case EPlayerDodgeDirection::Backward:
		return -GetActorForwardVector().GetSafeNormal2D();

	case EPlayerDodgeDirection::Left:
		return -GetActorRightVector().GetSafeNormal2D();

	case EPlayerDodgeDirection::Right:
		return GetActorRightVector().GetSafeNormal2D();

	default:
		return FVector::ZeroVector;
	}
}

void ADongincheonCharacter::HandleDodgeMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)bInterrupted;

	if (!IsValid(Montage) || Montage != ActiveDodgeMontage)
	{
		return;
	}

	ActiveDodgeMontage = nullptr;

	ResetPlayerDodgeState();
}

void ADongincheonCharacter::CancelPlayerDodge(float BlendOutTIme)
{
	UAnimMontage* MontageToStop = ActiveDodgeMontage;

	ActiveDodgeMontage = nullptr;
	bPlayerDodging = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	if (!IsValid(MontageToStop) || !IsValid(GetMesh()))
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTIme), MontageToStop);
	}
}

void ADongincheonCharacter::ResetPlayerDodgeState()
{
	ActiveDodgeMontage = nullptr;
	bPlayerDodging = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
}

//Guard
void ADongincheonCharacter::HandleGuardStarted()
{
	if (bPresentationInputLocked)
	{
		OnQTEInputPressed.Broadcast(EQTEInputType::Guard);
		return;
	}

	if (IsGameplayInputLocked())
	{
		return;
	}

	if (!bCombatInputEnabled)
	{
		return;
	}

	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || !IsValid(CombatComponent))
	{
		return;
	}

	bGuardInputHeld = true;

	if (bPlayerDeathStarted || bPlayerHitReacting || bPlayerDodging || bPlayerAttackActive ||
		CombatComponent->IsGuardBroken())
	{
		return;
	}

	if (IsValid(ActiveGuardHitReactMontage))
	{
		CombatComponent->BeginGuard();
		return;
	}

	if (CombatComponent->IsGuarding())
	{
		return;
	}

	StartPlayerGuard();
}

void ADongincheonCharacter::HandleGuardEnded()
{
	//shift를 뗐음
	bGuardInputHeld = false;


	if (!IsValid(CombatComponent))
	{
		return;
	}

	//가드 히트 리액션 재생중이면 애니메이션 끝까지 재생후 추가공격 가드하지 않음
	if (IsValid(ActiveGuardHitReactMontage))
	{
		CombatComponent->EndGuard();
		return;
	}

	if (!CombatComponent->IsGuarding())
	{
		return;
	}

	StopPlayerGuard(0.1f);
}

void ADongincheonCharacter::StartPlayerGuard()
{
	if (!IsValid(CombatComponent) || CombatComponent->IsGuardBroken() || !IsValid(GetMesh())
		|| !IsValid(GuardConfig.GuardMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const float SafePlayRate = GuardConfig.GuardPlayRate > 0.0f ? GuardConfig.GuardPlayRate : 1.0f;

	const float MontageLength =
		AnimInstance->Montage_Play(GuardConfig.GuardMontage, SafePlayRate);

	if (MontageLength <= 0.0f)
	{
		return;
	}

	CombatComponent->BeginGuard();

	ActiveGuardMontage = GuardConfig.GuardMontage;

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleGuardMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveGuardMontage);
}

void ADongincheonCharacter::StopPlayerGuard(float BlendOutTime)
{
	UAnimMontage* MontageToStop = ActiveGuardMontage;
	ActiveGuardMontage = nullptr;

	if (IsValid(CombatComponent))
	{
		CombatComponent->EndGuard();
	}

	if (!IsValid(MontageToStop) || !IsValid(GetMesh()))
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(FMath::Max(0.0f, BlendOutTime), MontageToStop);
	}
}

void ADongincheonCharacter::HandleGuardMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)bInterrupted;

	if (!IsValid(Montage) || Montage != ActiveGuardMontage)
	{
		return;
	}

	ActiveGuardMontage = nullptr;

	if (IsValid(CombatComponent))
	{
		CombatComponent->EndGuard();
	}
}

void ADongincheonCharacter::StartPlayerGuardHitReact()
{
	if (!IsValid(GetMesh()) || !IsValid(GuardConfig.GuardHitReactMontage))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	//기존 가드 루프를 끊고 액티브 가드 몽타주를 먼저 비워서 스탑으로 발생하는 몽타주엔디드가 가드 상태를 해제하지 못하게 한다.
	if (IsValid(ActiveGuardMontage))
	{
		UAnimMontage* GuardLoopToStop = ActiveGuardMontage;
		ActiveGuardMontage = nullptr;

		AnimInstance->Montage_Stop(0.05f, GuardLoopToStop);
	}

	//이미 Guard Hit Reaction이 재생중이면 같은 Reaction 중복 안시킴
	if (IsValid(ActiveGuardHitReactMontage))
	{
		return;
	}

	const float MontageLength = AnimInstance->Montage_Play(GuardConfig.GuardHitReactMontage, 1.0f);

	if (MontageLength <= 0.0f)
	{
		//재생을 실패했는데 Guard키를 계속 누르고 있다면 Guard Loop를 복구
		if (bGuardInputHeld)
		{
			StartPlayerGuard();
		}

		return;
	}

	ActiveGuardHitReactMontage = GuardConfig.GuardHitReactMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleGuardHitReactMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveGuardHitReactMontage);
}

void ADongincheonCharacter::HandleGuardHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)bInterrupted;

	if (!IsValid(Montage) || Montage != ActiveGuardHitReactMontage)
	{
		return;
	}

	ActiveGuardHitReactMontage = nullptr;

	// Death -> 일반 HitReact로 넘어가는 중이면 Guard로 다시 복귀하면 안됨
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted || bPlayerHitReacting)
	{
		return;
	}

	//Guard키를 계속 누르고 있으면 Gurad Loop로 복귀
	if (bGuardInputHeld)
	{
		StartPlayerGuard();
		return;
	}

	//Guard키를 이미 똈다면 일반 상태로 복귀
	if (IsValid(CombatComponent))
	{
		CombatComponent->EndGuard();
	}
}

void ADongincheonCharacter::StartPlayerGuardBreak()
{
	if (!IsValid(CombatComponent) || !CombatComponent->IsGuardBroken() || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		CombatComponent->RecoverFromGuardBreak();
		return;
	}

	if (IsValid(ActiveGuardMontage))
	{
		UAnimMontage* MontageToStop = ActiveGuardMontage;
		ActiveGuardMontage = nullptr;
		AnimInstance->Montage_Stop(0.05f, MontageToStop);
	}

	if (IsValid(ActiveGuardHitReactMontage))
	{
		UAnimMontage* MontageToStop = ActiveGuardHitReactMontage;
		ActiveGuardHitReactMontage = nullptr;
		AnimInstance->Montage_Stop(0.05f, MontageToStop);
	}

	if (!IsValid(GuardConfig.GuardBreakMontage))
	{
		FinishPlayerGuardBreak();
		return;
	}

	const float SafePlayRate = GuardConfig.GuardBreakPlayRate > 0.0f ? GuardConfig.GuardBreakPlayRate : 1.0f;

	const float MontageLength = AnimInstance->Montage_Play(GuardConfig.GuardBreakMontage, SafePlayRate);

	if (MontageLength <= 0.0f)
	{
		FinishPlayerGuardBreak();
		return;
	}

	ActiveGuardBreakMontage = GuardConfig.GuardBreakMontage;

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleGuardBreakMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveGuardBreakMontage);
}

void ADongincheonCharacter::HandleGuardBreakMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)bInterrupted;

	if (!IsValid(Montage) || Montage != ActiveGuardBreakMontage)
	{
		return;
	}

	ActiveGuardBreakMontage = nullptr;

	FinishPlayerGuardBreak();
}

void ADongincheonCharacter::FinishPlayerGuardBreak()
{
	if (!IsValid(CombatComponent))
	{
		return;
	}

	CombatComponent->RecoverFromGuardBreak();

	if (bPlayerDeathStarted || !IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerHitReacting)
	{
		return;
	}

	if (bGuardInputHeld)
	{
		StartPlayerGuard();
	}
}


//Hit React
void ADongincheonCharacter::StartPlayerHitReact()
{
	if (!IsValid(HealthComponent) || HealthComponent->IsDead() || bPlayerDeathStarted)
	{
		return;
	}

	CancelPlayerAttack(0.05f);
	CancelPlayerDodge(0.05f);
	CancelPlayerHeatAction(0.05f);
	CancelPlayerGrab(0.05f);
	StopPlayerGuard(0.05f);

	if (IsValid(ActiveGuardBreakMontage) && IsValid(GetMesh()))
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			UAnimMontage* GuardBreakToStop = ActiveGuardBreakMontage;

			FOnMontageEnded EmptyEndDelegate;
			AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, GuardBreakToStop);

			ActiveGuardBreakMontage = nullptr;

			AnimInstance->Montage_Stop(0.05f, GuardBreakToStop);
		}

		if (IsValid(CombatComponent))
		{
			CombatComponent->RecoverFromGuardBreak();
		}
	}

	if (!IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	//기존 HitReact가 재생중이면서 새 HitReact가 소유권을 가져간다.
	if (IsValid(ActiveHitReactMontage))
	{
		UAnimMontage* PreviousHitReact = ActiveHitReactMontage;

		//이전 HitReact 재생의 EndDelegate가 새 HitReact 상태를 종료시키지 못하도록 먼저 해제
		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, PreviousHitReact);


		ActiveHitReactMontage = nullptr;

		AnimInstance->Montage_Stop(0.05f, PreviousHitReact);
	}

	bPlayerHitReacting = true;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement()) Movement->StopMovementImmediately();

	if (!IsValid(HitReactMontage))
	{
		FinishPlayerHitReact();
		return;
	}

	const float SafePlayRate = HitReactPlayRate > 0.0f ? HitReactPlayRate : 1.0f;
	const float MontageLength = AnimInstance->Montage_Play(HitReactMontage, SafePlayRate,
	                                                       EMontagePlayReturnType::MontageLength,
	                                                       0.0f, bHitReactStopAllMontages);

	if (MontageLength <= 0.0f)
	{
		FinishPlayerHitReact();
		return;
	}

	ActiveHitReactMontage = HitReactMontage;

	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleHitReactMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveHitReactMontage);
}

void ADongincheonCharacter::HandleHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveHitReactMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		// 같은 HitReact Montage의 새 재생이 이미 진행 중이라면
		// 이전 재생의 종료 Callback이므로 현재 상태를 종료하지 않는다.
		if (IsValid(GetMesh()))
		{
			if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
			{
				if (AnimInstance->Montage_IsPlaying(ActiveHitReactMontage))
				{
					return;
				}
			}
		}
	}

	ActiveHitReactMontage = nullptr;
	FinishPlayerHitReact();
}

void ADongincheonCharacter::FinishPlayerHitReact()
{
	bPlayerHitReacting = false;

	if (bPlayerDeathStarted || !IsValid(HealthComponent) || HealthComponent->IsDead())
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
}

//Death
void ADongincheonCharacter::StartPlayerDeath()
{
	if (bPlayerDeathStarted)
	{
		return;
	}
	
	// Player 사망 시 Tutorial Infinite Heat 종료
	SetTutorialInfiniteHeatEnabled(false);

	bPlayerDeathStarted = true;
	bPlayerHitReacting = false;

	CancelPlayerAttack(0.0f);
	CancelPlayerDodge(0.0f);
	CancelPlayerHeatAction(0.0f);
	CancelPlayerGrab(0.0f);
	StopPlayerGuard(0.0f);

	bGuardInputHeld = false;

	if (IsValid(CombatComponent))
	{
		CombatComponent->RecoverFromGuardBreak();
	}

	if (IsValid(TargetingComponent))
	{
		TargetingComponent->ClearLockOn();
	}

	if (!IsValid(GetMesh()))
	{
		FinalizePlayerDeath();
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		FinalizePlayerDeath();
		return;
	}

	if (IsValid(ActiveGuardHitReactMontage))
	{
		UAnimMontage* GuardHitToStop = ActiveGuardHitReactMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, GuardHitToStop);

		ActiveGuardHitReactMontage = nullptr;

		AnimInstance->Montage_Stop(0.0f, GuardHitToStop);
	}

	if (IsValid(ActiveGuardBreakMontage))
	{
		UAnimMontage* GuardBreakToStop = ActiveGuardBreakMontage;

		FOnMontageEnded EmptyEndDelegate;
		AnimInstance->Montage_SetEndDelegate(EmptyEndDelegate, GuardBreakToStop);

		ActiveGuardBreakMontage = nullptr;

		AnimInstance->Montage_Stop(0.0f, GuardBreakToStop);
	}

	if (IsValid(ActiveHitReactMontage))
	{
		UAnimMontage* PreviousHitReact = ActiveHitReactMontage;

		ActiveHitReactMontage = nullptr;

		AnimInstance->Montage_Stop(0.0f, PreviousHitReact);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	if (!IsValid(DeathMontage))
	{
		FinalizePlayerDeath();
		return;
	}

	const float SafePlayRate = DeathPlayRate > 0.0f ? DeathPlayRate : 1.0f;

	const float MontageLength = AnimInstance->Montage_Play(DeathMontage, SafePlayRate,
	                                                       EMontagePlayReturnType::MontageLength,
	                                                       0.0f, bDeathStopAllMontages);

	if (MontageLength <= 0.0f)
	{
		FinalizePlayerDeath();
		return;
	}

	ActiveDeathMontage = DeathMontage;

	//Auto Blend Out이 켜진 Montage에 대한 fallback, DeathEnd Native Notify는 다음 단계에서 별도로 추가
	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(this, &ADongincheonCharacter::HandleDeathMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveDeathMontage);
}

void ADongincheonCharacter::HandleDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(Montage) || Montage != ActiveDeathMontage)
	{
		return;
	}

	ActiveDeathMontage = nullptr;

	FinalizePlayerDeath();
}

void ADongincheonCharacter::FinalizePlayerDeath()
{
	if (bPlayerDeathFinalized)
	{
		return;
	}

	bPlayerDeathFinalized = true;

	ClearHitStop();

	if (IsValid(CombatComponent))
	{
		CombatComponent->EndAttack();
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

float ADongincheonCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
                                        class AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	if (IsValid(CombatComponent))
	{
		const EGuardResult GuardResult = CombatComponent->TryBlockDamage(DamageAmount, DamageCauser);

		switch (GuardResult)
		{
		case EGuardResult::Blocked:
			{
				if (GuardConfig.GuardHitRecoilStrength > 0.0f)
				{
					FVector RecoilDirection = -GetActorForwardVector();

					if (IsValid(DamageCauser))
					{
						RecoilDirection =
							(GetActorLocation() - DamageCauser->GetActorLocation())
							.GetSafeNormal2D();
					}

					if (UCharacterMovementComponent* Movement =
						GetCharacterMovement())
					{
						Movement->AddImpulse(
							RecoilDirection * GuardConfig.GuardHitRecoilStrength,
							true);
					}
				}

				StartPlayerGuardHitReact();
				
				OnTutorialAction.Broadcast(EDIPlayerTutorialAction::GuardBlocked);
				
				return 0.0f;
			}

		case EGuardResult::GuardBroken:
			StartPlayerGuardBreak();
			return 0.0f;

		case EGuardResult::NotBlocked:
		default:
			break;
		}
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}


//Hit Confirm Feedback
void ADongincheonCharacter::HandleCombatHitConfirmed(
	AActor* HitActor,
	FVector HitLocation,
	FName HitSocketName,
	float AppliedDamage,
	EDICombatImpactResult ImpactResult)
{
	if (!IsValid(HitActor))
	{
		return;
	}

	if (ImpactResult == EDICombatImpactResult::Hit && AppliedDamage <= 0.0f)
	{
		return;
	}

	StopAttackAssistMove();

	// Hit / GuardHit / GuardBreak 모두 Enemy HUD 갱신
	if (ImpactResult == EDICombatImpactResult::Hit ||
		ImpactResult == EDICombatImpactResult::GuardHit ||
		ImpactResult == EDICombatImpactResult::GuardBreak)
	{
		if (ADIPlayerController* DIPlayerController =
			Cast<ADIPlayerController>(GetController()))
		{
			DIPlayerController->ShowEnemyHUD(HitActor);
		}
	}

	// Heat 획득은 실제 데미지 Hit에만 적용
	if (ImpactResult == EDICombatImpactResult::Hit)
	{
		if (IsValid(HeatActionComponent))
		{
			HeatActionComponent->AddHeat(HeatGainPerHit);
		}
	}
	
	// Tutorial: 한 공격 Montage당 실제 적중 1회만 집계
	if (ImpactResult == EDICombatImpactResult::Hit &&
		AppliedDamage > 0.0f &&
		bPlayerAttackActive &&
		IsValid(ActiveAttackMontage) &&
		!bTutorialHitReportedForActiveAttack)
	{
		if (ActiveAttackMode == EPlayerAttackMode::LightCombo ||
			ActiveAttackMode == EPlayerAttackMode::Heavy)
		{
			// Broadcast 전에 기록해 중복 호출 방지
			bTutorialHitReportedForActiveAttack = true;

			OnTutorialAction.Broadcast(
				ActiveAttackMode == EPlayerAttackMode::LightCombo
					? EDIPlayerTutorialAction::LightHit
					: EDIPlayerTutorialAction::HeavyHit);
		}
	}

	// 기존 Camera Shake 유지
	if (HitCameraShakeClass)
	{
		if (APlayerController* PlayerController =
			Cast<APlayerController>(GetController()))
		{
			if (APlayerCameraManager* CameraManager =
				PlayerController->PlayerCameraManager)
			{
				CameraManager->StartCameraShake(
					HitCameraShakeClass,
					HitCameraShakeScale,
					ECameraShakePlaySpace::CameraLocal,
					FRotator::ZeroRotator);
			}
		}
	}
}

void ADongincheonCharacter::StartHitStop()
{
	if (!IsValid(ActiveAttackMontage) || !IsValid(GetMesh()))
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const bool bIsFinisher = ComboAttacks.Num() > 0 && ActiveComboIndex == ComboAttacks.Num() - 1;
	const float Duration = bIsFinisher ? FinisherHitStopDuration : HitStopDuration;

	if (Duration <= 0.0f)
	{
		return;
	}

	bHitStopActive = true;

	AnimInstance->Montage_Pause(ActiveAttackMontage);

	GetWorldTimerManager().SetTimer(HitStopTimerHandle, this, &ADongincheonCharacter::ResumeHitStop, Duration, false);
}

void ADongincheonCharacter::ResumeHitStop()
{
	if (IsValid(ActiveAttackMontage) && IsValid(GetMesh()))
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Resume(ActiveAttackMontage);
		}
	}

	bHitStopActive = false;
}

void ADongincheonCharacter::ClearHitStop()
{
	GetWorldTimerManager().ClearTimer(HitStopTimerHandle);

	bHitStopActive = false;
}
