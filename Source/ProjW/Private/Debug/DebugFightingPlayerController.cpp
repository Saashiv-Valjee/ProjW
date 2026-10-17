#include "Debug/DebugFightingPlayerController.h"
#include "InputCoreTypes.h"

ADebugFightingPlayerController::ADebugFightingPlayerController()
{
    bAutoManageActiveCameraTarget = false;
}

void ADebugFightingPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (IsLocalController())
    {
        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = false;
    }
}

FFighterInput ADebugFightingPlayerController::BuildFrameInput() const
{
    FFighterInput Input;
    const auto Add = [this, &Input](const FKey& Key, EFighterInputButton Button)
    {
        if (IsInputKeyDown(Key))
        {
            Input.Buttons |= static_cast<uint16>(Button);
        }
    };
    Add(EKeys::A, EFighterInputButton::Left);
    Add(EKeys::D, EFighterInputButton::Right);
    Add(EKeys::W, EFighterInputButton::Up);
    Add(EKeys::S, EFighterInputButton::Down);
    Add(EKeys::NumPadSeven, EFighterInputButton::Light);
    Add(EKeys::NumPadEight, EFighterInputButton::Medium);
    Add(EKeys::NumPadNine, EFighterInputButton::Heavy);
    Add(EKeys::U, EFighterInputButton::Special);
    Add(EKeys::I, EFighterInputButton::Assist1);
    Add(EKeys::O, EFighterInputButton::Assist2);
    return Input;
}
