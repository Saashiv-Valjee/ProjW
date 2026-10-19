#include "Debug/DebugFightingPlayerController.h"

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

FDebugPhysicalInput ADebugFightingPlayerController::SamplePlayer1Keys() const
{
    FDebugPhysicalInput K;
    using namespace ProjW::DebugInput::Keys;
    K.bScreenLeft = IsInputKeyDown(P1Left());
    K.bScreenRight = IsInputKeyDown(P1Right());
    K.bJump = IsInputKeyDown(P1Jump());
    K.bCrouch = IsInputKeyDown(P1Crouch());
    K.bPivotCounterClockwise = IsInputKeyDown(P1PivotCCW());
    K.bPivotClockwise = IsInputKeyDown(P1PivotCW());
    K.bLight = IsInputKeyDown(P1Light());
    K.bMedium = IsInputKeyDown(P1Medium());
    K.bHeavy = IsInputKeyDown(P1Heavy());
    K.bGroundRupture = IsInputKeyDown(P1GroundRupture());
    K.bHighParry = IsInputKeyDown(P1HighParry());
    K.bLowParry = IsInputKeyDown(P1LowParry());
    return K;
}

FDebugPhysicalInput ADebugFightingPlayerController::SamplePlayer2Keys() const
{
    FDebugPhysicalInput K;
    using namespace ProjW::DebugInput::Keys;
    K.bScreenLeft = IsInputKeyDown(P2Left());
    K.bScreenRight = IsInputKeyDown(P2Right());
    K.bJump = IsInputKeyDown(P2Jump());
    K.bCrouch = IsInputKeyDown(P2Crouch());
    K.bPivotCounterClockwise = IsInputKeyDown(P2PivotCCW());
    K.bPivotClockwise = IsInputKeyDown(P2PivotCW());
    K.bLight = IsInputKeyDown(P2Light());
    K.bMedium = IsInputKeyDown(P2Medium());
    K.bHeavy = IsInputKeyDown(P2Heavy());
    K.bGroundRupture = IsInputKeyDown(P2GroundRupture());
    K.bHighParry = IsInputKeyDown(P2HighParry());
    K.bLowParry = IsInputKeyDown(P2LowParry());
    return K;
}

FFighterInput ADebugFightingPlayerController::BuildFrameInput(double ScreenRightAlignment)
{
    LastPlayer1Input = ProjW::DebugInput::BuildPlayer1Input(
        SamplePlayer1Keys(), ScreenRightAlignment, Player1MappingSign);
    return LastPlayer1Input;
}

FFighterInput ADebugFightingPlayerController::BuildOpponentFrameInput(double ScreenRightAlignment)
{
    LastPlayer2Input = ProjW::DebugInput::BuildPlayer2Input(
        SamplePlayer2Keys(), ScreenRightAlignment, Player2MappingSign);
    return LastPlayer2Input;
}

FString ADebugFightingPlayerController::GetInputDiagnosticText() const
{
    const FDebugPhysicalInput P1 = SamplePlayer1Keys();
    const FDebugPhysicalInput P2 = SamplePlayer2Keys();
    const auto Bits = [](const FFighterInput& I) { return static_cast<uint32>(I.Buttons); };
    return FString::Printf(
        TEXT("Raw P1 A/D/W/S Q/E R/F/C V T/G=%d%d%d%d %d%d %d%d%d %d %d%d -> %04X; ")
        TEXT("P2 Arrows KP4/6 7/8/9 5 1/2=%d%d%d%d %d%d %d%d%d %d %d%d -> %04X"),
        P1.bScreenLeft, P1.bScreenRight, P1.bJump, P1.bCrouch,
        P1.bPivotCounterClockwise, P1.bPivotClockwise,
        P1.bLight, P1.bMedium, P1.bHeavy, P1.bGroundRupture, P1.bHighParry, P1.bLowParry,
        Bits(LastPlayer1Input), P2.bScreenLeft, P2.bScreenRight, P2.bJump, P2.bCrouch,
        P2.bPivotCounterClockwise, P2.bPivotClockwise,
        P2.bLight, P2.bMedium, P2.bHeavy, P2.bGroundRupture, P2.bHighParry, P2.bLowParry,
        Bits(LastPlayer2Input));
}
