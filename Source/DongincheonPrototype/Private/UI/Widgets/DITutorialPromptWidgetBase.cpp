#include "UI/Widgets/DITutorialPromptWidgetBase.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

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

void UDITutorialPromptWidgetBase::ShowTutorialPrompt(
    const FText& InTitle,
    const FText& InDescription)
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
        TutorialDescriptionText->SetVisibility(
            ESlateVisibility::HitTestInvisible);
    }

    // 2행 표시 숨김
    if (IsValid(TutorialRow1Text))
    {
        TutorialRow1Text->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(TutorialRow2Text))
    {
        TutorialRow2Text->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(TutorialHideAnimation))
    {
        StopAnimation(TutorialHideAnimation);
    }

    TutorialPromptRoot->SetVisibility(
        ESlateVisibility::HitTestInvisible);

    bTutorialPromptVisible = true;

    if (IsValid(TutorialShowAnimation))
    {
        StopAnimation(TutorialShowAnimation);
        PlayAnimation(TutorialShowAnimation);
    }
}

void UDITutorialPromptWidgetBase::ShowTutorialPromptRows(
    const FText& Title,
    FName Row1IconId,
    const FText& Row1Text,
    FName Row2IconId,
    const FText& Row2Text)
{
    // 기존 제목 및 표시 애니메이션 재사용
    ShowTutorialPrompt(Title, FText::GetEmpty());

    // 기존 단일 설명 숨김
    if (IsValid(TutorialDescriptionText))
    {
        TutorialDescriptionText->SetVisibility(
            ESlateVisibility::Collapsed);
    }

    if (IsValid(TutorialInputIcon))
    {
        TutorialInputIcon->SetVisibility(
            ESlateVisibility::Collapsed);
    }

    const auto GetKeyDisplayText = [](FName IconId) -> FText
    {
        if (IconId == FName(TEXT("LMB")))
        {
            return FText::FromString(TEXT("좌클릭"));
        }

        if (IconId == FName(TEXT("RMB")))
        {
            return FText::FromString(TEXT("우클릭"));
        }

        return FText::FromName(IconId);
    };
    
    // 첫 번째 행: [LMB] 기본 공격
    if (IsValid(TutorialRow1Text))
    {
        TutorialRow1Text->SetText(
            FText::Format(
                FText::FromString(TEXT("[{0}]  {1}")),
                GetKeyDisplayText(Row1IconId),
                Row1Text));

        TutorialRow1Text->SetVisibility(
            ESlateVisibility::HitTestInvisible);
    }

    // 두 번째 행: [RMB] 강공격
    if (IsValid(TutorialRow2Text))
    {
        TutorialRow2Text->SetText(
            FText::Format(
                FText::FromString(TEXT("[{0}]  {1}")),
                GetKeyDisplayText(Row2IconId),
                Row2Text));

        TutorialRow2Text->SetVisibility(
            ESlateVisibility::HitTestInvisible);
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