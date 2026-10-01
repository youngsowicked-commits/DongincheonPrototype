#include "UI/Widgets/DITutorialPromptWidgetBase.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UDITutorialPromptWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(TutorialPromptRoot))
    {
        TutorialPromptRoot->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(TutorialHideAnimation))
    {
        FWidgetAnimationDynamicEvent FinishedEvent;
        FinishedEvent.BindDynamic(this, &UDITutorialPromptWidgetBase::HandleTutorialHideFinished);
        BindToAnimationFinished(TutorialHideAnimation, FinishedEvent);
    }
}

void UDITutorialPromptWidgetBase::NativeDestruct()
{
    if (IsValid(TutorialShowAnimation))
    {
        StopAnimation(TutorialShowAnimation);
    }

    if (IsValid(TutorialHideAnimation))
    {
        StopAnimation(TutorialHideAnimation);
        UnbindAllFromAnimationFinished(TutorialHideAnimation);
    }

    bTutorialPromptVisible = false;

    Super::NativeDestruct();
}

void UDITutorialPromptWidgetBase::ShowTutorialPrompt(const FText& InTitle, const FText& InDescription)
{
    if (!IsValid(TutorialPromptRoot))
    {
        return;
    }

    if (IsValid(TutorialTitleText))
    {
        TutorialTitleText->SetText(InTitle);
    }

    if (IsValid(TutorialDescriptionText))
    {
        TutorialDescriptionText->SetText(InDescription);
    }

    if (IsValid(TutorialHideAnimation))
    {
        StopAnimation(TutorialHideAnimation);
    }

    TutorialPromptRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
    bTutorialPromptVisible = true;

    if (IsValid(TutorialShowAnimation))
    {
        StopAnimation(TutorialShowAnimation);
        PlayAnimation(TutorialShowAnimation);
    }
}

void UDITutorialPromptWidgetBase::SetTutorialProgress(const FText& InProgressText)
{
    if (!IsValid(TutorialProgressText))
    {
        return;
    }

    TutorialProgressText->SetText(InProgressText);
    TutorialProgressText->SetVisibility(InProgressText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UDITutorialPromptWidgetBase::SetTutorialInputIcon(const FSlateBrush& InBrush)
{
    if (!IsValid(TutorialInputIcon))
    {
        return;
    }

    TutorialInputIcon->SetBrush(InBrush);
}

void UDITutorialPromptWidgetBase::HideTutorialPrompt()
{
    if (!bTutorialPromptVisible)
    {
        return;
    }

    if (IsValid(TutorialShowAnimation))
    {
        StopAnimation(TutorialShowAnimation);
    }

    if (IsValid(TutorialHideAnimation))
    {
        StopAnimation(TutorialHideAnimation);
        PlayAnimation(TutorialHideAnimation);
        return;
    }

    HandleTutorialHideFinished();
}

bool UDITutorialPromptWidgetBase::IsTutorialPromptVisible() const
{
    return bTutorialPromptVisible;
}

void UDITutorialPromptWidgetBase::HandleTutorialHideFinished()
{
    bTutorialPromptVisible = false;

    if (IsValid(TutorialPromptRoot))
    {
        TutorialPromptRoot->SetVisibility(ESlateVisibility::Collapsed);
    }
}