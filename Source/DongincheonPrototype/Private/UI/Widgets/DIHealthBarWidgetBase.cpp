#include "UI/Widgets/DIHealthBarWidgetBase.h"

#include "Components/HealthComponent.h"
#include "Components/ProgressBar.h"

void UDIHealthBarWidgetBase::SetHealthSource(UHealthComponent* InHealthComponent)
{
    if (HealthSource == InHealthComponent)
    {
        RefreshHealthBar();
        return;
    }

    UnbindHealthSource();

    HealthSource = InHealthComponent;

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

    BindHealthSource();
    RefreshHealthBar();
}

void UDIHealthBarWidgetBase::NativeDestruct()
{
    UnbindHealthSource();

    Super::NativeDestruct();
}

void UDIHealthBarWidgetBase::HandleHealthChanged(float OldHealth,float NewHealth,float MaxHealth)
{
    RefreshHealthBar();
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
    if (!IsValid(HealthProgressBar))
    {
        return;
    }

    if (!IsValid(HealthSource))
    {
        HealthProgressBar->SetPercent(0.0f);
        return;
    }

    const float HealthPercent = FMath::Clamp(HealthSource->GetHealthNormalized(),0.0f,1.0f);

    HealthProgressBar->SetPercent(HealthPercent);
}