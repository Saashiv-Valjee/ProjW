#include "Debug/DebugFightingHUD.h"
#include "Debug/DebugMatchDriver.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"

void ADebugFightingHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Driver.IsValid())
    {
        for (TActorIterator<ADebugMatchDriver> It(GetWorld()); It; ++It)
        {
            Driver = *It;
            break;
        }
    }
    if (!Canvas || !Driver.IsValid()) { return; }
    const FMatchState& Match = Driver->GetMatchState();
    const FFighterState& F = Match.Fighter1;
    const TCHAR* StateName = F.CurrentState == EFighterState::Idle ? TEXT("Idle")
        : F.CurrentState == EFighterState::Walking ? TEXT("Walking") : TEXT("Jumping");
    const FString Text = FString::Printf(
        TEXT("MILESTONE 2 | 60 Hz | Frame: %llu | Render: %.1f FPS\n")
        TEXT("P1 BLUE: A/D move | W jump (held repeats) | S down\n")
        TEXT("J light | K medium | L heavy | U special | I assist1 | O assist2 (inert)\n")
        TEXT("P1 Pos: (%d, %d)  Vel: (%d, %d) [integer subunits]\n")
        TEXT("Grounded: %s | State: %s | StateFrame: %u | Facing: %s\n")
        TEXT("Last simulated input: 0x%03X | Catch-up clamps: %llu\n")
        TEXT("Click viewport to control. Shift+F1 releases mouse. P2 ORANGE is neutral."),
        Match.SimulationFrame, Driver->GetRenderFPS(), F.PositionX, F.PositionY,
        F.VelocityX, F.VelocityY, F.bGrounded ? TEXT("true") : TEXT("false"),
        StateName, F.StateFrame, F.Facing == EFighterFacing::Right ? TEXT("Right") : TEXT("Left"),
        static_cast<uint32>(Driver->GetLastInput().Buttons), Driver->GetHitchCount());
    DrawRect(FLinearColor(0, 0, 0, 0.8f), 12, 12, FMath::Min(1000.0f, Canvas->ClipX - 24), 150);
    DrawText(Text, FLinearColor::White, 22, 20, nullptr, 1.0f);
}
