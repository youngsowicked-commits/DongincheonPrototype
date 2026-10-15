#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DIGameMenuWidget.generated.h"

class UButton;

UENUM()
enum class EDIGameMenuMode : uint8
{
    Title,
    Pause,
    GameOver
};

/** Small presentation menu; gameplay and cinematic state remain in their existing owners. */
UCLASS()
class DONGINCHEONPROTOTYPE_API UDIGameMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(EDIGameMenuMode InMode);
    EDIGameMenuMode GetMode() const { return Mode; }
    void FocusPrimaryAction();

protected:
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
    EDIGameMenuMode Mode = EDIGameMenuMode::Title;
    UPROPERTY(Transient) TObjectPtr<UButton> PrimaryButton;
    UFUNCTION() void PrimaryClicked();
    UFUNCTION() void RestartClicked();
    UFUNCTION() void QuitClicked();
};
