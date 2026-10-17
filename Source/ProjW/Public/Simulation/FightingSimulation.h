#pragma once

#include "Simulation/FightingSimulationTypes.h"

// Stateless authoritative simulation. No Actors, world, physics, animation or clock.
class PROJW_API FFightingSimulation
{
public:
    static constexpr int32 FramesPerSecond = 60;
    static constexpr int32 SubunitsPerWorldUnit = 1024;
    static constexpr int32 WalkSpeed = 4 * SubunitsPerWorldUnit;
    static constexpr int32 ForwardWalkSpeed = WalkSpeed;
    static constexpr int32 BackwardWalkSpeed = 3 * SubunitsPerWorldUnit;
    static constexpr int32 DashInputWindowFrames = 12;
    static constexpr int32 ForwardDashDurationFrames = 12;
    static constexpr int32 BackDashDurationFrames = 10;
    static constexpr int32 ForwardDashSpeed = 10 * SubunitsPerWorldUnit;
    static constexpr int32 BackDashSpeed = 8 * SubunitsPerWorldUnit;
    static constexpr int32 StageLeftBoundary = -500 * SubunitsPerWorldUnit;
    static constexpr int32 StageRightBoundary = 500 * SubunitsPerWorldUnit;
    static constexpr int32 PushHalfWidth = 20 * SubunitsPerWorldUnit;
    static constexpr int32 MinFighterX = StageLeftBoundary + PushHalfWidth;
    static constexpr int32 MaxFighterX = StageRightBoundary - PushHalfWidth;
    static constexpr int32 JumpStartupFrames = 4;
    static constexpr int32 JumpVerticalSpeed = 12 * SubunitsPerWorldUnit;
    static constexpr int32 NeutralJumpHorizontalSpeed = 0;
    static constexpr int32 ForwardJumpHorizontalSpeed = 5 * SubunitsPerWorldUnit;
    static constexpr int32 BackwardJumpHorizontalSpeed = 4 * SubunitsPerWorldUnit;
    static constexpr int32 DoubleJumpVerticalSpeed = 11 * SubunitsPerWorldUnit;
    static constexpr int32 NeutralDoubleJumpHorizontalSpeed = 0;
    static constexpr int32 ForwardDoubleJumpHorizontalSpeed = ForwardJumpHorizontalSpeed;
    static constexpr int32 BackwardDoubleJumpHorizontalSpeed = BackwardJumpHorizontalSpeed;
    // Compatibility name for old tests/content; new code uses JumpVerticalSpeed.
    static constexpr int32 JumpSpeed = JumpVerticalSpeed;
    static constexpr int32 Gravity = -SubunitsPerWorldUnit / 2;
    static constexpr int32 LightStartupFrames = 5;
    static constexpr int32 LightActiveFrames = 3;
    static constexpr int32 LightRecoveryFrames = 9;
    static constexpr int32 HurtboxHalfWidth = 24 * SubunitsPerWorldUnit;
    static constexpr int32 HurtboxHeight = 100 * SubunitsPerWorldUnit;
    static constexpr int32 LightHitboxHalfWidth = 28 * SubunitsPerWorldUnit;
    static constexpr int32 LightHitboxHeight = 50 * SubunitsPerWorldUnit;
    static constexpr int32 LightHitboxOffsetX = 44 * SubunitsPerWorldUnit;
    static constexpr int32 LightHitboxOffsetY = 55 * SubunitsPerWorldUnit;

    static FSimBox GetHurtbox(const FFighterState& Fighter);
    static bool GetLightHitbox(const FFighterState& Fighter, FSimBox& OutBox);

    // Exactly one 1/60-second gameplay step. Caller owns scheduling and inputs.
    // Left+right cancel. Grounded Up PRESSED starts a fixed jump startup.
    // Airborne momentum is committed; one fresh Up press can replace it via double jump.
    // Interpret using entering facing; classify raw double taps on second press.
    // Intended steps -> stage clamp -> grounded pair push -> facing update.
    static void SimulateFrame(FMatchState& State,
        const FFighterInput& Player1Input, const FFighterInput& Player2Input);
};
