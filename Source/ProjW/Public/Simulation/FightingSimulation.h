#pragma once

#include "Simulation/FightingSimulationTypes.h"

// Stateless authoritative simulation. No Actors, world, physics, animation or clock.
class PROJW_API FFightingSimulation
{
public:
    static constexpr int32 FramesPerSecond = 60;
    static constexpr int32 SubunitsPerWorldUnit = 1024;
    static constexpr int32 WalkSpeed = 4 * SubunitsPerWorldUnit;
    static constexpr int32 JumpSpeed = 12 * SubunitsPerWorldUnit;
    static constexpr int32 Gravity = -SubunitsPerWorldUnit / 2;

    // Exactly one 1/60-second gameplay step. Caller owns scheduling and inputs.
    // Left+right cancel. Up jumps whenever grounded (held Up repeats on landing).
    // Horizontal control also applies in air. Other buttons are currently inert.
    // Facing uses both post-step positions; equal X preserves previous facing.
    static void SimulateFrame(FMatchState& State,
        const FFighterInput& Player1Input, const FFighterInput& Player2Input);
};
