#include "UI/Widgets/DIHeatActionPromptWidgetBase.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Widget.h"

void UDIHeatActionPromptWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	bHeatActionPromptVisible = false;

	if (IsValid(HeatActionPromptRoot))
	{
		HeatActionPromptRoot->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(HeatActionPromptHideAnimation))
	{
		FWidgetAnimationDynamicEvent HideFinishedEvent;
		HideFinishedEvent.BindDynamic(this, &UDIHeatActionPromptWidgetBase::HandleHeatActionPromptHideFinished);
		BindToAnimationFinished(HeatActionPromptHideAnimation, HideFinishedEvent);
	}
	
	UE_LOG(LogTemp, Warning,
	TEXT("[HEAT_PROMPT] Root=%s | ShowAnim=%s | HideAnim=%s"),
	*GetNameSafe(HeatActionPromptRoot.Get()),
	*GetNameSafe(HeatActionPromptShowAnimation.Get()),
	*GetNameSafe(HeatActionPromptHideAnimation.Get()));
}

void UDIHeatActionPromptWidgetBase::NativeDestruct()
{
	StopAllAnimations();

	if (IsValid(HeatActionPromptHideAnimation))
	{
		UnbindAllFromAnimationFinished(HeatActionPromptHideAnimation);
	}

	Super::NativeDestruct();
}

void UDIHeatActionPromptWidgetBase::ShowHeatActionPrompt()
{
	UE_LOG(LogTemp, Warning, TEXT("[HEAT_PROMPT] SHOW CALLED | Visible=%d | AnimValid=%d"),
	bHeatActionPromptVisible,
	IsValid(HeatActionPromptShowAnimation));
	
	if (bHeatActionPromptVisible || !IsValid(HeatActionPromptRoot))
	{
		return;
	}

	if (IsValid(HeatActionPromptHideAnimation))
	{
		StopAnimation(HeatActionPromptHideAnimation);
	}

	HeatActionPromptRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	bHeatActionPromptVisible = true;

	if (IsValid(HeatActionPromptShowAnimation))
	{
		PlayAnimation(HeatActionPromptShowAnimation);
	}
}

void UDIHeatActionPromptWidgetBase::HideHeatActionPrompt()
{
	UE_LOG(LogTemp, Warning, TEXT("[HEAT_PROMPT] HIDE CALLED | Visible=%d | AnimValid=%d"),
	bHeatActionPromptVisible,
	IsValid(HeatActionPromptHideAnimation));
	
	if (!bHeatActionPromptVisible || !IsValid(HeatActionPromptRoot))
	{
		return;
	}

	if (IsValid(HeatActionPromptShowAnimation))
	{
		StopAnimation(HeatActionPromptShowAnimation);
	}

	if (IsValid(HeatActionPromptHideAnimation))
	{
		PlayAnimation(HeatActionPromptHideAnimation);
		return;
	}

	HandleHeatActionPromptHideFinished();
}

bool UDIHeatActionPromptWidgetBase::IsHeatActionPromptVisible() const
{
	return bHeatActionPromptVisible;
}

void UDIHeatActionPromptWidgetBase::HandleHeatActionPromptHideFinished()
{
	if (IsValid(HeatActionPromptRoot))
	{
		HeatActionPromptRoot->SetVisibility(ESlateVisibility::Collapsed);
	}

	bHeatActionPromptVisible = false;
}