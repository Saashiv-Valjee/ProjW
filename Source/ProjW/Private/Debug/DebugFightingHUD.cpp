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
    const TCHAR* StateName = TEXT("Unknown");
    switch (F.CurrentState)
    {
        case EFighterState::Idle: StateName = TEXT("Idle"); break;
        case EFighterState::Walking: StateName = TEXT("Walking"); break;
        case EFighterState::Crouching: StateName = TEXT("Crouching"); break;
        case EFighterState::Jumping: StateName = TEXT("Jumping"); break;
        case EFighterState::ForwardDash: StateName = TEXT("ForwardDash"); break;
        case EFighterState::BackDash: StateName = TEXT("BackDash"); break;
        case EFighterState::JumpStartup: StateName = TEXT("JumpStartup"); break;
        case EFighterState::DoubleJumping: StateName = TEXT("DoubleJumping"); break;
        case EFighterState::Attack: StateName = TEXT("Attack"); break;
        case EFighterState::Hitstun: StateName = TEXT("Hitstun"); break;
    }
    const TCHAR* MoveName = TEXT("None");
    switch (F.CurrentMoveId)
    {
        case EMoveId::Light: MoveName = TEXT("Light"); break;
        case EMoveId::Medium: MoveName = TEXT("Medium"); break;
        case EMoveId::Heavy: MoveName = TEXT("Heavy"); break;
        default: break;
    }
    const FString Text = FString::Printf(
        TEXT("MILESTONE 5C | Combat Rules: Baseline | 60 Hz | Frame: %llu | Render: %.1f FPS\n")
        TEXT("P1 BLUE: A/D walk | double tap A/D dash | W press jump | S crouch\n")
        TEXT("Numpad 7 light | Numpad 8 medium | Numpad 9 heavy | U special | I assist1 | O assist2\n")
        TEXT("P1 Pos: (%d, %d)  Vel: (%d, %d) [integer subunits]\n")
        TEXT("Grounded: %s | State: %s | Move: %s | Phase: %s | StateFrame: %u\n")
        TEXT("Facing: %s | Health: %d | Connected: %s | Hits: %u\n")
        TEXT("Hitstop: %u | PendingReaction: %s | KnockbackX: %d\n")
        TEXT("Held: 0x%03X | Pressed: 0x%03X | Released: 0x%03X | Catch-up clamps: %llu\n")
        TEXT("Click viewport to control. Shift+F1 releases mouse. P2 ORANGE is neutral."),
        Match.SimulationFrame, Driver->GetRenderFPS(), F.PositionX, F.PositionY,
        F.VelocityX, F.VelocityY, F.bGrounded ? TEXT("true") : TEXT("false"),
        StateName, MoveName,
        F.CurrentState == EFighterState::Attack ? (F.MovePhase == EMovePhase::Startup ? TEXT("Startup")
            : F.MovePhase == EMovePhase::Active ? TEXT("Active") : TEXT("Recovery")) : TEXT("-"),
        F.StateFrame, F.Facing == EFighterFacing::Right ? TEXT("Right") : TEXT("Left"),
        F.Health, F.bCurrentMoveConnected ? TEXT("yes") : TEXT("no"), Match.HitEventCount,
        F.HitstopFramesRemaining, F.bPendingHitReaction ? TEXT("yes") : TEXT("no"), F.PendingKnockbackVelocityX,
        static_cast<uint32>(F.PreviousInput.Buttons), static_cast<uint32>(F.PressedInput.Buttons),
        static_cast<uint32>(F.ReleasedInput.Buttons), Driver->GetHitchCount());
    DrawRect(FLinearColor(0, 0, 0, 0.8f), 12, 12, FMath::Min(1000.0f, Canvas->ClipX - 24), 195);
    DrawText(Text, FLinearColor::White, 22, 20, nullptr, 1.0f);
}
