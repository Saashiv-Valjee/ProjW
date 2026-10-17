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
    static constexpr int32 JumpSpeed = 12 * SubunitsPerWorldUnit;
    static constexpr int32 Gravity = -SubunitsPerWorldUnit / 2;

    // Exactly one 1/60-second gameplay step. Caller owns scheduling and inputs.
    // Left+right cancel. Grounded Up PRESSED jumps; air control/arc are unchanged.
    // Interpret using entering facing; classify raw double taps on second press.
    // Intended steps -> stage clamp -> grounded pair push -> facing update.
    static void SimulateFrame(FMatchState& State,
        const FFighterInput& Player1Input, const FFighterInput& Player2Input);
};
