#include "Debug/DebugFightingHUD.h"
#include "Debug/DebugMatchDriver.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Simulation/FightingSimulation.h"

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
        case EFighterState::Parrying: StateName = TEXT("Parrying"); break;
        case EFighterState::ParrySuccess: StateName = TEXT("ParrySuccess"); break;
        case EFighterState::ParryStun: StateName = TEXT("ParryStun"); break;
    }
    const TCHAR* MoveName = TEXT("None");
    switch (F.CurrentMoveId)
    {
        case EMoveId::Light: MoveName = TEXT("Light"); break;
        case EMoveId::Medium: MoveName = TEXT("Medium"); break;
        case EMoveId::Heavy: MoveName = TEXT("Heavy"); break;
        case EMoveId::GroundRupture: MoveName = TEXT("GroundRupture"); break;
        default: break;
    }
    const EFighterInputButton Away = F.Facing == EFighterFacing::Right
        ? EFighterInputButton::Left : EFighterInputButton::Right;
    const TCHAR* GuardName = !F.bGrounded || !F.PreviousInput.IsHeld(Away)
        ? TEXT("None") : F.PreviousInput.IsHeld(EFighterInputButton::Down)
            ? TEXT("Crouching") : TEXT("Standing");
    const TCHAR* ReactionName = F.PendingReaction == EPendingReaction::Hit ? TEXT("Hit")
        : F.PendingReaction == EPendingReaction::Block ? TEXT("Block")
        : F.PendingReaction == EPendingReaction::Parry ? TEXT("ParryStun") : TEXT("None");
    const TCHAR* ParryName = F.CurrentState == EFighterState::Parrying
        ? F.ParryHeight == EParryHeight::High ? (F.StateFrame < FFightingSimulation::ParryActiveFrames ? TEXT("High active") : TEXT("High recovery"))
            : F.ParryHeight == EParryHeight::Low ? (F.StateFrame < FFightingSimulation::ParryActiveFrames ? TEXT("Low active") : TEXT("Low recovery")) : TEXT("-")
        : F.CurrentState == EFighterState::ParrySuccess ? TEXT("Success")
        : F.CurrentState == EFighterState::ParryStun ? TEXT("ParryStun") : TEXT("-");
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
    if (!Driver->IsDetailedHudVisible())
    {
        DrawText(TEXT("F2 diagnostics   F3 combat geometry   F4 camera/movement diagnostics"),
            FLinearColor(0.8f, 0.86f, 0.9f), 20, 38, nullptr, 0.8f);
        if (!Driver->IsPresentationDiagnosticsVisible()) return;
    }
    if (Driver->IsPresentationDiagnosticsVisible())
    {
        const FString Diagnostic = Driver->GetPresentationDiagnosticText();
        DrawRect(FLinearColor(0, 0, 0, 0.78f), 12, 64, FMath::Min(1440.0f, Canvas->ClipX - 24), 105);
        DrawText(Diagnostic, FLinearColor(0.88f, 0.96f, 1.0f), 18, 70, nullptr, 0.72f);
    }
    if (!Driver->IsDetailedHudVisible()) return;
    const FString Text = FString::Printf(
        TEXT("MILESTONE 5G | Combat Lab: Block / Pivot / High-Low Parry | 60 Hz | Frame: %llu | Render: %.1f FPS\n")
        TEXT("P1 LEFT: A/D move | W jump | S crouch | Q/E Pivot | R/F/C Light/Medium/Heavy | V rupture | T/G High/Low Parry\n")
        TEXT("P2 RIGHT: Arrows move | Num 4/6 Pivot | Num 7/8/9 Light/Medium/Heavy | Num 5 rupture | Num 1/2 High/Low Parry\n")
        TEXT("P1 Pos X/Y/Z: (%d, %d, %d)  Vel X/Y/Z: (%d, %d, %d) [integer subunits]\n")
        TEXT("Grounded: %s | State: %s | Pivot: %s | Parry: %s | Move: %s | Phase: %s | StateFrame: %u\n")
        TEXT("Facing: %s | Guard input: %s | Health: %d | Connected: %s | Hits: %u\n")
        TEXT("Hitstop: %u | PendingReaction: %s (%u frames) | Pushback X/Z: %d/%d\n")
        TEXT("Held: 0x%03X | Pressed: 0x%03X | Released: 0x%03X | Catch-up clamps: %llu\n")
        TEXT("Click viewport to control both local fighters. Shift+F1 releases mouse."),
        Match.SimulationFrame, Driver->GetRenderFPS(), F.PositionX, F.PositionY, F.PositionZ,
        F.VelocityX, F.VelocityY, F.VelocityZ, F.bGrounded ? TEXT("true") : TEXT("false"),
        StateName, F.CurrentState == EFighterState::Pivoting
            ? (F.PivotDirection > 0 ? TEXT("CCW") : TEXT("CW")) : TEXT("-"), ParryName, MoveName,
        F.CurrentState == EFighterState::Attack ? (F.MovePhase == EMovePhase::Startup ? TEXT("Startup")
            : F.MovePhase == EMovePhase::Active ? TEXT("Active") : TEXT("Recovery")) : TEXT("-"),
        F.StateFrame, F.Facing == EFighterFacing::Right ? TEXT("Right") : TEXT("Left"), GuardName,
        F.Health, F.bCurrentMoveConnected ? TEXT("yes") : TEXT("no"), Match.HitEventCount,
        F.HitstopFramesRemaining, ReactionName, F.PendingReactionFrames,
        F.PendingKnockbackVelocityX, F.PendingKnockbackVelocityZ,
        static_cast<uint32>(F.PreviousInput.Buttons), static_cast<uint32>(F.PressedInput.Buttons),
        static_cast<uint32>(F.ReleasedInput.Buttons), Driver->GetHitchCount());
    DrawRect(FLinearColor(0, 0, 0, 0.68f), 12, 38, FMath::Min(1080.0f, Canvas->ClipX - 24), 210);
    DrawText(Text, FLinearColor::White, 20, Driver->IsPresentationDiagnosticsVisible() ? 178 : 44, nullptr, 0.82f);
    DrawRect(FLinearColor(0, 0, 0, 0.68f), 12, 254, FMath::Min(1480.0f, Canvas->ClipX - 24), 58);
    DrawText(Driver->GetInputPathDiagnostic(), FLinearColor(0.78f, 0.92f, 1.0f), 20, 258, nullptr, 0.62f);
    DrawText(Driver->GetContactDiagnostic(), FLinearColor(1.0f, 0.82f, 0.48f), 20, 280, nullptr, 0.72f);
}
