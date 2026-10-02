#include "UI/Widgets/DIHealthBarWidgetBase.h"

#include "Components/Image.h"
#include "Components/DIHeatActionComponent.h"
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

void UDIHealthBarWidgetBase::SetHeatSource(UDIHeatActionComponent* InHeatComponent)
{
    if (HeatSource == InHeatComponent)
    {
        EnsureDynamicMaterial();
        RefreshHeatBar();
        return;
    }

    UnbindHeatSource();

    HeatSource = InHeatComponent;

    EnsureDynamicMaterial();
    BindHeatSource();
    RefreshHeatBar();
}

void UDIHealthBarWidgetBase::ClearHeatSource()
{
    UnbindHeatSource();

    HeatSource = nullptr;

    RefreshHeatBar();
}

UDIHeatActionComponent* UDIHealthBarWidgetBase::GetHeatSource() const
{
    return HeatSource;
}

void UDIHealthBarWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    EnsureDynamicMaterial();

    BindHealthSource();
    BindHeatSource();

    RefreshHealthBar();
    RefreshHeatBar();
}

void UDIHealthBarWidgetBase::NativeDestruct()
{
    UnbindHealthSource();
    UnbindHeatSource();

    Super::NativeDestruct();
}

void UDIHealthBarWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Health Damage Lag
    if (bDamageLagAnimating && IsValid(HPDamageLagMID))
    {
        if (DamageLagDelayRemaining > 0.0f)
        {
            DamageLagDelayRemaining -= InDeltaTime;
        }
        else
        {
            DamageLagAnimationElapsed += InDeltaTime;

            const float SafeDuration = FMath::Max(
                DamageLagDuration,
                KINDA_SMALL_NUMBER
            );

            const float Alpha = FMath::Clamp(
                DamageLagAnimationElapsed / SafeDuration,
                0.0f,
                1.0f
            );

            DamageLagCurrentPercent = FMath::Lerp(
                DamageLagStartPercent,
                DamageLagTargetPercent,
                Alpha
            );

            HPDamageLagMID->SetScalarParameterValue(
                TEXT("Progress"),
                DamageLagCurrentPercent
            );

            if (Alpha >= 1.0f)
            {
                DamageLagCurrentPercent = DamageLagTargetPercent;
                bDamageLagAnimating = false;
            }
        }
    }

    // Heat Gauge Smooth Fill
    if (bHeatAnimating && IsValid(HeatMainMID))
    {
        HeatAnimationElapsed += InDeltaTime;

        const float SafeDuration = FMath::Max(
            HeatFillDuration,
            KINDA_SMALL_NUMBER
        );

        const float Alpha = FMath::Clamp(
            HeatAnimationElapsed / SafeDuration,
            0.0f,
            1.0f
        );

        // SmoothStep: 시작/끝이 딱딱하지 않게 감속/가속
        const float SmoothAlpha =
            Alpha * Alpha * (3.0f - 2.0f * Alpha);

        HeatCurrentPercent = FMath::Lerp(
            HeatStartPercent,
            HeatTargetPercent,
            SmoothAlpha
        );

        HeatMainMID->SetScalarParameterValue(
            TEXT("Progress"),
            HeatCurrentPercent
        );

        if (Alpha >= 1.0f)
        {
            HeatCurrentPercent = HeatTargetPercent;

            HeatMainMID->SetScalarParameterValue(
                TEXT("Progress"),
                HeatCurrentPercent
            );

            bHeatAnimating = false;
        }
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

void UDIHealthBarWidgetBase::HandleHeatChanged(float CurrentHeat, float MaxHeat)
{
    EnsureDynamicMaterial();

    const float NewPercent = MaxHeat > 0.0f
        ? FMath::Clamp(CurrentHeat / MaxHeat, 0.0f, 1.0f)
        : 0.0f;

    if (!IsValid(HeatMainMID))
    {
        return;
    }

    HeatStartPercent = HeatCurrentPercent;
    HeatTargetPercent = NewPercent;
    HeatAnimationElapsed = 0.0f;

    bHeatAnimating = !FMath::IsNearlyEqual(HeatStartPercent,HeatTargetPercent);

    if (!bHeatAnimating)
    {
        HeatCurrentPercent = HeatTargetPercent;
        HeatMainMID->SetScalarParameterValue(TEXT("Progress"),HeatCurrentPercent);
    }
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

void UDIHealthBarWidgetBase::BindHeatSource()
{
    if (!IsValid(HeatSource))
    {
        return;
    }

    HeatSource->OnHeatChanged.AddUniqueDynamic(this,&UDIHealthBarWidgetBase::HandleHeatChanged);
}

void UDIHealthBarWidgetBase::UnbindHeatSource()
{
    if (!IsValid(HeatSource))
    {
        return;
    }

    HeatSource->OnHeatChanged.RemoveDynamic(this,&UDIHealthBarWidgetBase::HandleHeatChanged);
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

void UDIHealthBarWidgetBase::RefreshHeatBar()
{
    EnsureDynamicMaterial();

    const float HeatPercent = IsValid(HeatSource)
        ? HeatSource->GetHeatNormalized()
        : 0.0f;

    HeatCurrentPercent = HeatPercent;
    HeatStartPercent = HeatPercent;
    HeatTargetPercent = HeatPercent;
    HeatAnimationElapsed = 0.0f;
    bHeatAnimating = false;

    if (IsValid(HeatMainMID))
    {
        HeatMainMID->SetScalarParameterValue(
            TEXT("Progress"),
            HeatCurrentPercent
        );
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

    if (!IsValid(HeatMainMID) && IsValid(HeatMainImage))
    {
        HeatMainMID = HeatMainImage->GetDynamicMaterial();
    }
}