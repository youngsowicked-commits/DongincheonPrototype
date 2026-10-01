#include "UI/Widgets/DIContextActionHintWidgetBase.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UDIContextActionHintWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(ContextActionHintRoot))
    {
        ContextActionHintRoot->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(ContextHintHideAnimation))
    {
        FWidgetAnimationDynamicEvent FinishedEvent;
        FinishedEvent.BindDynamic(this, &UDIContextActionHintWidgetBase::HandleContextHintHideFinished);
        BindToAnimationFinished(ContextHintHideAnimation, FinishedEvent);
    }
}

void UDIContextActionHintWidgetBase::NativeDestruct()
{
    if (IsValid(ContextHintShowAnimation))
    {
        StopAnimation(ContextHintShowAnimation);
    }

    if (IsValid(ContextHintHideAnimation))
    {
        StopAnimation(ContextHintHideAnimation);
        UnbindAllFromAnimationFinished(ContextHintHideAnimation);
    }

    bContextActionHintVisible = false;
    Super::NativeDestruct();
}

void UDIContextActionHintWidgetBase::ShowContextActionHint(const FText& ActionText)
{
    if (!IsValid(ContextActionHintRoot))
    {
        return;
    }

    if (IsValid(ContextActionText))
    {
        ContextActionText->SetText(ActionText);
    }

    if (IsValid(ContextHintHideAnimation))
    {
        StopAnimation(ContextHintHideAnimation);
    }

    ContextActionHintRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
    bContextActionHintVisible = true;

    if (IsValid(ContextHintShowAnimation))
    {
        StopAnimation(ContextHintShowAnimation);
        PlayAnimation(ContextHintShowAnimation);
    }
}

void UDIContextActionHintWidgetBase::SetContextActionIcon(const FSlateBrush& InBrush)
{
    if (IsValid(ContextActionIcon))
    {
        ContextActionIcon->SetBrush(InBrush);
    }
}

void UDIContextActionHintWidgetBase::HideContextActionHint()
{
    if (!bContextActionHintVisible)
    {
        return;
    }

    if (IsValid(ContextHintShowAnimation))
    {
        StopAnimation(ContextHintShowAnimation);
    }

    if (IsValid(ContextHintHideAnimation))
    {
        StopAnimation(ContextHintHideAnimation);
        PlayAnimation(ContextHintHideAnimation);
        return;
    }

    HandleContextHintHideFinished();
}

bool UDIContextActionHintWidgetBase::IsContextActionHintVisible() const
{
    return bContextActionHintVisible;
}

void UDIContextActionHintWidgetBase::HandleContextHintHideFinished()
{
    bContextActionHintVisible = false;

    if (IsValid(ContextActionHintRoot))
    {
        ContextActionHintRoot->SetVisibility(ESlateVisibility::Collapsed);
    }
}