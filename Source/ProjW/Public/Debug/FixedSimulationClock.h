#pragma once

#include "CoreMinimal.h"
#include "Simulation/FightingSimulation.h"

// Presentation scheduling only: never part of a simulation snapshot.
struct FFixedSimulationClock
{
    static constexpr double StepSeconds = 1.0 / FFightingSimulation::FramesPerSecond;
    static constexpr int32 MaxStepsPerTick = 8;
    double Accumulator = 0.0;
    uint64 HitchCount = 0;

    int32 Consume(double ElapsedSeconds)
    {
        if (!FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds <= 0.0)
        {
            return 0;
        }
        Accumulator += ElapsedSeconds;
        // Discard excessive elapsed time, not simulation frame numbers. After a
        // long pause the local debug match slows relative to wall time.
        if (Accumulator > StepSeconds * MaxStepsPerTick)
        {
            Accumulator = StepSeconds * MaxStepsPerTick;
            ++HitchCount;
        }
        const int32 Steps = FMath::Min(MaxStepsPerTick,
            static_cast<int32>((Accumulator + 1.e-10) / StepSeconds));
        Accumulator = FMath::Max(0.0, Accumulator - Steps * StepSeconds);
        return Steps;
    }
};
