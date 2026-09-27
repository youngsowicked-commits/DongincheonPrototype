#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/QTEComponent.h"
#include "Styling/SlateBrush.h"
#include "DIQTEPromptWidgetBase.generated.h"

class UWidget;
class UImage;
class UMaterialInstanceDynamic;

UCLASS(Abstract, Blueprintable)
class DONGINCHEONPROTOTYPE_API UDIQTEPromptWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "HUD|QTE")
    void SetQTESource(UQTEComponent* InQTEComponent);

    UFUNCTION(BlueprintCallable, Category = "HUD|QTE")
    void ClearQTESource();

    UFUNCTION(BlueprintPure, Category = "HUD|QTE")
    UQTEComponent* GetQTESource() const;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    virtual void NativeTick(const FGeometry& MyGeometry,float InDeltaTime) override;

private:
    // WBP_QTEPrompt의 전체 표시 Root.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UWidget> QTEPromptRoot;

    // 실제 버튼 아이콘.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> QTEInputIcon;

    // 남은 시간을 표시하는 Material Image.
    // Brush에는 Progress Scalar Parameter를 가진 UI Material을 지정한다.
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> QTETimeFillImage;

    // Icon Content
    // C++은 어떤 Brush를 쓸지만 결정하고,
    // 실제 Texture / Material은 WBP Class Defaults에서 지정한다.

    UPROPERTY(EditDefaultsOnly, Category = "HUD|QTE|Icons")
    FSlateBrush LightIconBrush;

    UPROPERTY(EditDefaultsOnly, Category = "HUD|QTE|Icons")
    FSlateBrush HeavyIconBrush;

    UPROPERTY(EditDefaultsOnly, Category = "HUD|QTE|Icons")
    FSlateBrush DodgeIconBrush;

    UPROPERTY(EditDefaultsOnly, Category = "HUD|QTE|Icons")
    FSlateBrush GuardIconBrush;

    // QTETimeFillImage Material에서 사용할 Scalar Parameter 이름.
    UPROPERTY(EditDefaultsOnly, Category = "HUD|QTE|Timer")
    FName TimeProgressParameterName = TEXT("Progress");

    UPROPERTY(Transient)
    TObjectPtr<UQTEComponent> QTESource;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> QTETimeFillMID;

    UFUNCTION()
    void HandleQTEStarted(FName QTEId,int32 RequiredInputCount);

    UFUNCTION()
    void HandleQTEProgress(FName QTEId,int32 CurrentInputCount,int32 RequiredInputCount);

    UFUNCTION()
    void HandleQTECompleted(FName QTEId,EQTEResult Result);

    void BindQTESource();
    void UnbindQTESource();

    void EnsureTimerMaterial();

    void RefreshPrompt();
    void RefreshInputIcon();
    void RefreshTimer();

    const FSlateBrush* GetBrushForInput(EQTEInputType InputType) const;

    void SetPromptVisible(bool bVisible);
};