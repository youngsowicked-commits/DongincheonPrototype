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

    // 기존 단일 메시지 표시
    if (IsValid(TutorialDescriptionText))
    {
        TutorialDescriptionText->SetText(InDescription);
        TutorialDescriptionText->SetVisibility(
            ESlateVisibility::HitTestInvisible);
    }

    // 이전 2행 UI 정리
    if (IsValid(TutorialRow1Icon))
        TutorialRow1Icon->SetVisibility(ESlateVisibility::Collapsed);

    if (IsValid(TutorialRow1Text))
        TutorialRow1Text->SetVisibility(ESlateVisibility::Collapsed);

    if (IsValid(TutorialRow2Icon))
        TutorialRow2Icon->SetVisibility(ESlateVisibility::Collapsed);

    if (IsValid(TutorialRow2Text))
        TutorialRow2Text->SetVisibility(ESlateVisibility::Collapsed);

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
    // 기존 제목, 등장 애니메이션, 표시 상태 재사용
    ShowTutorialPrompt(Title, FText::GetEmpty());

    if (!IsValid(TutorialPromptRoot))
    {
        return;
    }

    // 단일 메시지 UI 숨김
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

    // 각 행의 IconId -> Texture 매핑 및 텍스트 표시
    const auto ApplyRow =
        [this](
            UImage* IconWidget,
            UTextBlock* TextWidget,
            FName IconId,
            const FText& Description)
    {
        if (IsValid(TextWidget))
        {
            TextWidget->SetText(Description);
            TextWidget->SetVisibility(
                ESlateVisibility::HitTestInvisible);
        }

        if (!IsValid(IconWidget))
        {
            return;
        }

        const TObjectPtr<UTexture2D>* FoundTexture =
            TutorialIconTextures.Find(IconId);

        UTexture2D* Texture =
            FoundTexture ? FoundTexture->Get() : nullptr;

        if (IsValid(Texture))
        {
            IconWidget->SetBrushFromTexture(Texture, false);
            IconWidget->SetVisibility(
                ESlateVisibility::HitTestInvisible);
        }
        else
        {
            IconWidget->SetVisibility(
                ESlateVisibility::Collapsed);

            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Tutorial icon not assigned: %s"),
                *IconId.ToString());
        }
    };

    ApplyRow(
        TutorialRow1Icon.Get(),
        TutorialRow1Text.Get(),
        Row1IconId,
        Row1Text);

    ApplyRow(
        TutorialRow2Icon.Get(),
        TutorialRow2Text.Get(),
        Row2IconId,
        Row2Text);
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