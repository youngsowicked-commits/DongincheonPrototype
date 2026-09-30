#include "UI/Widgets/DIBossIntroWidgetBase.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UDIBossIntroWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(BossIntroRoot))
    {
        BossIntroRoot->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(BossIntroAnimation))
    {
        FWidgetAnimationDynamicEvent FinishedEvent;

        FinishedEvent.BindDynamic(
            this,
            &UDIBossIntroWidgetBase::HandleBossIntroAnimationFinished);

        BindToAnimationFinished(
            BossIntroAnimation,
            FinishedEvent);
    }
}

void UDIBossIntroWidgetBase::NativeDestruct()
{
    if (IsValid(BossIntroAnimation))
    {
        StopAnimation(BossIntroAnimation);
        UnbindAllFromAnimationFinished(BossIntroAnimation);
    }

    bBossIntroPlaying = false;

    Super::NativeDestruct();
}

void UDIBossIntroWidgetBase::ShowBossIntro(
    const FText& InBossRole,
    const FText& InBossName)
{
    if (!IsValid(BossIntroRoot))
    {
        return;
    }

    if (IsValid(BossRoleText))
    {
        BossRoleText->SetText(InBossRole);
    }

    if (IsValid(BossNameText))
    {
        BossNameText->SetText(InBossName);
    }

    BossIntroRoot->SetVisibility(
        ESlateVisibility::HitTestInvisible);

    bBossIntroPlaying = true;

    if (IsValid(BossIntroAnimation))
    {
        StopAnimation(BossIntroAnimation);
        PlayAnimation(BossIntroAnimation);
    }
}

void UDIBossIntroWidgetBase::HideBossIntro()
{
    if (IsValid(BossIntroAnimation))
    {
        StopAnimation(BossIntroAnimation);
    }

    bBossIntroPlaying = false;

    if (IsValid(BossIntroRoot))
    {
        BossIntroRoot->SetVisibility(
            ESlateVisibility::Collapsed);
    }
}

bool UDIBossIntroWidgetBase::IsBossIntroPlaying() const
{
    return bBossIntroPlaying;
}

void UDIBossIntroWidgetBase::HandleBossIntroAnimationFinished()
{
    bBossIntroPlaying = false;

    if (IsValid(BossIntroRoot))
    {
        BossIntroRoot->SetVisibility(
            ESlateVisibility::Collapsed);
    }
}