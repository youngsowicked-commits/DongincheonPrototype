#include "UI/Widgets/DIQTEPromptWidgetBase.h"

#include "Components/Image.h"
#include "Components/Widget.h"
#include "Materials/MaterialInstanceDynamic.h"

void UDIQTEPromptWidgetBase::SetQTESource(UQTEComponent* InQTEComponent)
{
    if (QTESource == InQTEComponent)
    {
        BindQTESource();
        RefreshPrompt();
        return;
    }

    UnbindQTESource();

    QTESource = InQTEComponent;

    BindQTESource();
    RefreshPrompt();
}

void UDIQTEPromptWidgetBase::ClearQTESource()
{
    UnbindQTESource();

    QTESource = nullptr;

    if (IsValid(QTEInputIcon))
    {
        QTEInputIcon->SetBrush(FSlateBrush());
    }

    if (IsValid(QTETimeFillMID))
    {
        QTETimeFillMID->SetScalarParameterValue(TimeProgressParameterName,0.0f);
    }

    SetPromptVisible(false);
}

UQTEComponent* UDIQTEPromptWidgetBase::GetQTESource() const
{
    return QTESource;
}

void UDIQTEPromptWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();

    EnsureTimerMaterial();
    BindQTESource();
    RefreshPrompt();
}

void UDIQTEPromptWidgetBase::NativeDestruct()
{
    UnbindQTESource();

    QTETimeFillMID = nullptr;

    Super::NativeDestruct();
}

void UDIQTEPromptWidgetBase::NativeTick(const FGeometry& MyGeometry,float InDeltaTime)
{
    Super::NativeTick(MyGeometry,InDeltaTime);

    if (!IsValid(QTESource))
    {
        return;
    }

    if (!QTESource->IsQTEActive())
    {
        return;
    }

    RefreshTimer();
}

void UDIQTEPromptWidgetBase::BindQTESource()
{
    if (!IsValid(QTESource))
    {
        return;
    }

    QTESource->OnQTEStarted.AddUniqueDynamic(this,&UDIQTEPromptWidgetBase::HandleQTEStarted);

    QTESource->OnQTEProgress.AddUniqueDynamic(this,&UDIQTEPromptWidgetBase::HandleQTEProgress);

    QTESource->OnQTECompleted.AddUniqueDynamic(this,&UDIQTEPromptWidgetBase::HandleQTECompleted);
}

void UDIQTEPromptWidgetBase::UnbindQTESource()
{
    if (!IsValid(QTESource))
    {
        return;
    }

    QTESource->OnQTEStarted.RemoveDynamic(this,&UDIQTEPromptWidgetBase::HandleQTEStarted);

    QTESource->OnQTEProgress.RemoveDynamic(this,&UDIQTEPromptWidgetBase::HandleQTEProgress);

    QTESource->OnQTECompleted.RemoveDynamic(this,&UDIQTEPromptWidgetBase::HandleQTECompleted);
}

void UDIQTEPromptWidgetBase::HandleQTEStarted(FName QTEId,int32 RequiredInputCount)
{
    RefreshPrompt();
}

void UDIQTEPromptWidgetBase::HandleQTEProgress(FName QTEId,int32 CurrentInputCount,int32 RequiredInputCount)
{
    if (CurrentInputCount >= RequiredInputCount)
    {
        return;
    }

    RefreshInputIcon();
    RefreshTimer();
}

void UDIQTEPromptWidgetBase::HandleQTECompleted(FName QTEId,EQTEResult Result)
{
    if (IsValid(QTETimeFillMID))
    {
        QTETimeFillMID->SetScalarParameterValue(TimeProgressParameterName,0.0f);
    }

    SetPromptVisible(false);
}

void UDIQTEPromptWidgetBase::EnsureTimerMaterial()
{
    if (IsValid(QTETimeFillMID))
    {
        return;
    }

    if (!IsValid(QTETimeFillImage))
    {
        return;
    }

    QTETimeFillMID = QTETimeFillImage->GetDynamicMaterial();
}

void UDIQTEPromptWidgetBase::RefreshPrompt()
{
    if (!IsValid(QTESource) || !QTESource->IsQTEActive())
    {
        SetPromptVisible(false);
        return;
    }

    EnsureTimerMaterial();

    RefreshInputIcon();
    RefreshTimer();

    SetPromptVisible(true);
}

void UDIQTEPromptWidgetBase::RefreshInputIcon()
{
    if (!IsValid(QTEInputIcon))
    {
        return;
    }

    if (!IsValid(QTESource) || !QTESource->IsQTEActive())
    {
        QTEInputIcon->SetBrush(FSlateBrush());
        return;
    }

    const FSlateBrush* InputBrush = GetBrushForInput(QTESource->GetExpectedInput());

    if (!InputBrush)
    {
        QTEInputIcon->SetBrush(FSlateBrush());
        return;
    }

    QTEInputIcon->SetBrush(*InputBrush);
}

void UDIQTEPromptWidgetBase::RefreshTimer()
{
    EnsureTimerMaterial();

    if (!IsValid(QTETimeFillMID))
    {
        return;
    }

    if (!IsValid(QTESource) || !QTESource->IsQTEActive())
    {
        QTETimeFillMID->SetScalarParameterValue(TimeProgressParameterName,0.0f);

        return;
    }

    QTETimeFillMID->SetScalarParameterValue(TimeProgressParameterName,QTESource->GetRemainingTimeNormalized());
}

const FSlateBrush* UDIQTEPromptWidgetBase::GetBrushForInput(EQTEInputType InputType) const
{
    switch (InputType)
    {
    case EQTEInputType::Light:
        return &LightIconBrush;

    case EQTEInputType::Heavy:
        return &HeavyIconBrush;

    case EQTEInputType::Dodge:
        return &DodgeIconBrush;

    case EQTEInputType::Guard:
        return &GuardIconBrush;

    case EQTEInputType::None:
    default:
        return nullptr;
    }
}

void UDIQTEPromptWidgetBase::SetPromptVisible(
    bool bVisible)
{
    if (!IsValid(QTEPromptRoot))
    {
        return;
    }

    QTEPromptRoot->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}