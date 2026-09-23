#include "UI/Widgets/DIHealthBarWidgetBase.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void UDIHealthBarWidgetBase::SetHealthSource(UHealthComponent* InHealthComponent)
{
    if (HealthSource == InHealthComponent)
    {
        EnsureDynamicMaterial();
        RefreshHealthBar();
        return;
    }

    UnbindHealthSource();

    HealthSource = InHealthComponent;

    EnsureDynamicMaterial();
    BindHealthSource();
    RefreshHealthBar();
}

void UDIHealthBarWidgetBase::ClearHealthSource()
{
    UnbindHealthSource();

    HealthSource = nullptr;

    RefreshHealthBar();
}

UHealthComponent* UDIHealthBarWidgetBase::GetHealthSource() const
{
    return HealthSource;
}

void UDIHealthBarWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    EnsureDynamicMaterial();
    BindHealthSource();
    RefreshHealthBar();
}

void UDIHealthBarWidgetBase::NativeDestruct()
{
    UnbindHealthSource();

    Super::NativeDestruct();
}

void UDIHealthBarWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bDamageLagAnimating || !IsValid(HPDamageLagMID))
    {
        return;
    }

    if (DamageLagDelayRemaining > 0.0f)
    {
        DamageLagDelayRemaining -= InDeltaTime;
        return;
    }

    DamageLagAnimationElapsed += InDeltaTime;

    const float SafeDuration = FMath::Max(DamageLagDuration, KINDA_SMALL_NUMBER);
    const float Alpha = FMath::Clamp(DamageLagAnimationElapsed / SafeDuration, 0.0f, 1.0f);

    DamageLagCurrentPercent = FMath::Lerp(DamageLagStartPercent,DamageLagTargetPercent,Alpha);

    HPDamageLagMID->SetScalarParameterValue(TEXT("Progress"), DamageLagCurrentPercent);

    if (Alpha >= 1.0f)
    {
        DamageLagCurrentPercent = DamageLagTargetPercent;
        bDamageLagAnimating = false;
    }
}

void UDIHealthBarWidgetBase::HandleHealthChanged(float OldHealth, float NewHealth, float MaxHealth)
{
    if (MaxHealth <= 0.0f)
    {
        return;
    }

    EnsureDynamicMaterial();

    const float NewPercent = FMath::Clamp(NewHealth / MaxHealth, 0.0f, 1.0f);

    if (IsValid(HPMainMID))
    {
        HPMainMID->SetScalarParameterValue(TEXT("Progress"), NewPercent);
    }

    // Damage
    if (NewHealth < OldHealth)
    {
        DamageLagStartPercent = DamageLagCurrentPercent;
        DamageLagTargetPercent = NewPercent;
        DamageLagDelayRemaining = DamageLagDelay;
        DamageLagAnimationElapsed = 0.0f;
        bDamageLagAnimating = true;

        return;
    }

    // Heal 또는 기타 증가
    DamageLagCurrentPercent = NewPercent;
    DamageLagStartPercent = NewPercent;
    DamageLagTargetPercent = NewPercent;
    DamageLagDelayRemaining = 0.0f;
    DamageLagAnimationElapsed = 0.0f;
    bDamageLagAnimating = false;

    if (IsValid(HPDamageLagMID))
    {
        HPDamageLagMID->SetScalarParameterValue(TEXT("Progress"), NewPercent);
    }
}

void UDIHealthBarWidgetBase::HandleMaxHealthChanged(float OldMaxHealth,float NewMaxHealth,float CurrentHealth)
{
    RefreshHealthBar();
}

void UDIHealthBarWidgetBase::BindHealthSource()
{
    if (!IsValid(HealthSource))
    {
        return;
    }

    HealthSource->OnHealthChanged.AddUniqueDynamic(this,&UDIHealthBarWidgetBase::HandleHealthChanged);

    HealthSource->OnMaxHealthChanged.AddUniqueDynamic(this,&UDIHealthBarWidgetBase::HandleMaxHealthChanged);
}

void UDIHealthBarWidgetBase::UnbindHealthSource()
{
    if (!IsValid(HealthSource))
    {
        return;
    }

    HealthSource->OnHealthChanged.RemoveDynamic(this,&UDIHealthBarWidgetBase::HandleHealthChanged);

    HealthSource->OnMaxHealthChanged.RemoveDynamic(this,&UDIHealthBarWidgetBase::HandleMaxHealthChanged);
}

void UDIHealthBarWidgetBase::RefreshHealthBar()
{
    if (!IsValid(HealthSource))
    {
        return;
    }

    EnsureDynamicMaterial();

    const float HealthPercent = HealthSource->GetHealthNormalized();

    if (IsValid(HPMainMID))
    {
        HPMainMID->SetScalarParameterValue(TEXT("Progress"), HealthPercent);
    }

    DamageLagCurrentPercent = HealthPercent;
    DamageLagStartPercent = HealthPercent;
    DamageLagTargetPercent = HealthPercent;
    DamageLagDelayRemaining = 0.0f;
    DamageLagAnimationElapsed = 0.0f;
    bDamageLagAnimating = false;

    if (IsValid(HPDamageLagMID))
    {
        HPDamageLagMID->SetScalarParameterValue(TEXT("Progress"), HealthPercent);
    }
}

void UDIHealthBarWidgetBase::EnsureDynamicMaterial()
{
    if (!IsValid(HPMainMID) && IsValid(HPMainImage))
    {
        HPMainMID = HPMainImage->GetDynamicMaterial();
    }

    if (!IsValid(HPDamageLagMID) && IsValid(HPDamageLagImage))
    {
        HPDamageLagMID = HPDamageLagImage->GetDynamicMaterial();
    }
}
