#include "UI/Widgets/DIGameMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Brushes/SlateColorBrush.h"
#include "InputCoreTypes.h"
#include "Player/DIPlayerController.h"
#include "Styling/CoreStyle.h"

void UDIGameMenuWidget::Configure(EDIGameMenuMode InMode)
{
    Mode = InMode;
    SetIsFocusable(true);
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
    Backdrop->SetBrushColor(FLinearColor(0.016f, 0.019f, 0.024f, 0.97f));
    Backdrop->SetHorizontalAlignment(HAlign_Center);
    Backdrop->SetVerticalAlignment(VAlign_Center);
    Backdrop->SetPadding(FMargin(32));
    WidgetTree->RootWidget = Backdrop;

    USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
    Width->SetWidthOverride(460);
    Backdrop->SetContent(Width);
    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
    Width->SetContent(Stack);
    auto AddText = [&](const FText& Text, int32 Size, const FLinearColor& Color, float Bottom)
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(Text);
        Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size));
        Label->SetColorAndOpacity(FSlateColor(Color));
        Label->SetJustification(ETextJustify::Center);
        Label->SetAutoWrapText(true);
        Stack->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 0, 0, Bottom));
    };
    const bool bTitle = Mode == EDIGameMenuMode::Title;
    const bool bGameOver = Mode == EDIGameMenuMode::GameOver;
    AddText(FText::FromString(bTitle ? TEXT("DONGINCHEON") : bGameOver ? TEXT("GAME OVER") : TEXT("PAUSED")),
        42, FLinearColor(0.93f, 0.77f, 0.43f), 12);
    AddText(FText::FromString(bTitle ? TEXT("동인천  ·  PHASE 2") : bGameOver ? TEXT("다시 도전할 수 있습니다.") : TEXT("일시정지")),
        20, FLinearColor(0.77f, 0.79f, 0.82f), 36);

    auto AddButton = [&](const TCHAR* Name, const TCHAR* Caption)
    {
        USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>();
        Height->SetHeightOverride(56);
        Stack->AddChildToVerticalBox(Height)->SetPadding(FMargin(0, 0, 0, 12));
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(Name));
        FButtonStyle Style = Button->GetStyle();
        Style.SetNormal(FSlateColorBrush(FLinearColor(0.09f, 0.11f, 0.14f)));
        Style.SetHovered(FSlateColorBrush(FLinearColor(0.22f, 0.18f, 0.11f)));
        Style.SetPressed(FSlateColorBrush(FLinearColor(0.34f, 0.24f, 0.11f)));
        Button->SetStyle(Style);
        Height->SetContent(Button);
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Caption));
        Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 21));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Button->SetContent(Label);
        return Button;
    };
    PrimaryButton = AddButton(TEXT("PrimaryAction"), bTitle ? TEXT("게임 시작") : bGameOver ? TEXT("다시 시작") : TEXT("계속하기"));
    PrimaryButton->OnClicked.AddUniqueDynamic(this, &UDIGameMenuWidget::PrimaryClicked);
    if (Mode == EDIGameMenuMode::Pause)
    {
        AddButton(TEXT("RestartAction"), TEXT("처음부터 다시 시작"))->OnClicked.AddUniqueDynamic(this, &UDIGameMenuWidget::RestartClicked);
    }
    AddButton(TEXT("QuitAction"), TEXT("게임 종료"))->OnClicked.AddUniqueDynamic(this, &UDIGameMenuWidget::QuitClicked);
    AddText(FText::FromString(Mode == EDIGameMenuMode::Pause ? TEXT("ESC · 계속하기") : TEXT("동인천 Vertical Slice")),
        14, FLinearColor(0.50f, 0.54f, 0.60f), 0);
}

void UDIGameMenuWidget::FocusPrimaryAction()
{
    if (PrimaryButton) PrimaryButton->SetUserFocus(GetOwningPlayer());
}

FReply UDIGameMenuWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape && Mode == EDIGameMenuMode::Pause)
    {
        if (ADIPlayerController* PC = Cast<ADIPlayerController>(GetOwningPlayer())) PC->ResumeFromMenu();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UDIGameMenuWidget::PrimaryClicked()
{
    if (ADIPlayerController* PC = Cast<ADIPlayerController>(GetOwningPlayer()))
    {
        if (Mode == EDIGameMenuMode::GameOver) PC->RestartFromMenu();
        else PC->ResumeFromMenu();
    }
}

void UDIGameMenuWidget::RestartClicked()
{
    if (ADIPlayerController* PC = Cast<ADIPlayerController>(GetOwningPlayer())) PC->RestartFromMenu();
}

void UDIGameMenuWidget::QuitClicked()
{
    if (ADIPlayerController* PC = Cast<ADIPlayerController>(GetOwningPlayer())) PC->QuitFromMenu();
}
