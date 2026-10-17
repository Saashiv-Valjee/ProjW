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
        case EFighterState::Blockstun: StateName = TEXT("Blockstun"); break;
        case EFighterState::Pivoting: StateName = TEXT("Pivoting"); break;
    }
    const TCHAR* MoveName = TEXT("None");
    switch (F.CurrentMoveId)
    {
        case EMoveId::Light: MoveName = TEXT("Light"); break;
        case EMoveId::Medium: MoveName = TEXT("Medium"); break;
        case EMoveId::Heavy: MoveName = TEXT("Heavy"); break;
        default: break;
    }
    const EFighterInputButton Away = F.Facing == EFighterFacing::Right
        ? EFighterInputButton::Left : EFighterInputButton::Right;
    const TCHAR* GuardName = !F.bGrounded || !F.PreviousInput.IsHeld(Away)
        ? TEXT("None") : F.PreviousInput.IsHeld(EFighterInputButton::Down)
            ? TEXT("Crouching") : TEXT("Standing");
    const TCHAR* ReactionName = F.PendingReaction == EPendingReaction::Hit ? TEXT("Hit")
        : F.PendingReaction == EPendingReaction::Block ? TEXT("Block") : TEXT("None");
    const float BarWidth = FMath::Min(390.0f, (Canvas->ClipX - 60.0f) * 0.5f);
    const float RightBarX = Canvas->ClipX - 20.0f - BarWidth;
    const float P1Fraction = FMath::Clamp(static_cast<float>(Match.Fighter1.Health)
        / FFighterState::MaxHealth, 0.0f, 1.0f);
    const float P2Fraction = FMath::Clamp(static_cast<float>(Match.Fighter2.Health)
        / FFighterState::MaxHealth, 0.0f, 1.0f);
    DrawRect(FLinearColor(0.12f, 0.12f, 0.12f, 0.9f), 20, 12, BarWidth, 18);
    DrawRect(FLinearColor(0.12f, 0.12f, 0.12f, 0.9f), RightBarX, 12, BarWidth, 18);
    DrawRect(FLinearColor(0.05f, 0.3f, 1.0f), 20, 12, BarWidth * P1Fraction, 18);
    DrawRect(FLinearColor(1.0f, 0.15f, 0.05f), RightBarX + BarWidth * (1.0f - P2Fraction),
        12, BarWidth * P2Fraction, 18);
    const FString Text = FString::Printf(
        TEXT("MILESTONE 5F | Combat Rules: 3D Pivot Prototype | 60 Hz | Frame: %llu | Render: %.1f FPS\n")
        TEXT("P1 BLUE: A/D local walk | double tap A/D dash | W jump | S crouch | Q CCW Pivot | E CW Pivot\n")
        TEXT("Numpad 7 light | Numpad 8 medium | Numpad 9 heavy | U special | I assist1 | O assist2\n")
        TEXT("P1 Pos X/Y/Z: (%d, %d, %d)  Vel X/Y/Z: (%d, %d, %d) [integer subunits]\n")
        TEXT("Grounded: %s | State: %s | Pivot: %s | Move: %s | Phase: %s | StateFrame: %u\n")
        TEXT("Facing: %s | Guard input: %s | Health: %d | Connected: %s | Hits: %u\n")
        TEXT("Hitstop: %u | PendingReaction: %s (%u frames) | Pushback X/Z: %d/%d\n")
        TEXT("Held: 0x%03X | Pressed: 0x%03X | Released: 0x%03X | Catch-up clamps: %llu\n")
        TEXT("Click viewport to control. Shift+F1 releases mouse. P2 ORANGE is neutral."),
        Match.SimulationFrame, Driver->GetRenderFPS(), F.PositionX, F.PositionY, F.PositionZ,
        F.VelocityX, F.VelocityY, F.VelocityZ, F.bGrounded ? TEXT("true") : TEXT("false"),
        StateName, F.CurrentState == EFighterState::Pivoting
            ? (F.PivotDirection > 0 ? TEXT("CCW") : TEXT("CW")) : TEXT("-"), MoveName,
        F.CurrentState == EFighterState::Attack ? (F.MovePhase == EMovePhase::Startup ? TEXT("Startup")
            : F.MovePhase == EMovePhase::Active ? TEXT("Active") : TEXT("Recovery")) : TEXT("-"),
        F.StateFrame, F.Facing == EFighterFacing::Right ? TEXT("Right") : TEXT("Left"), GuardName,
        F.Health, F.bCurrentMoveConnected ? TEXT("yes") : TEXT("no"), Match.HitEventCount,
        F.HitstopFramesRemaining, ReactionName, F.PendingReactionFrames,
        F.PendingKnockbackVelocityX, F.PendingKnockbackVelocityZ,
        static_cast<uint32>(F.PreviousInput.Buttons), static_cast<uint32>(F.PressedInput.Buttons),
        static_cast<uint32>(F.ReleasedInput.Buttons), Driver->GetHitchCount());
    DrawRect(FLinearColor(0, 0, 0, 0.8f), 12, 38, FMath::Min(1000.0f, Canvas->ClipX - 24), 195);
    DrawText(Text, FLinearColor::White, 22, 46, nullptr, 1.0f);
}
