#include "Debug/FixedSimulationClock.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFixedClockRateTest, "ProjW.DebugHarness.RenderRateIndependence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFixedClockRateTest::RunTest(const FString& Parameters)
{
    for (const int32 RenderHz : {30, 60, 144, 240})
    {
        FFixedSimulationClock Clock;
        FMatchState State;
        FMatchState Reference;
        // Input is indexed by simulation frame, never render frame.
        const auto Step = [](FMatchState& Match)
        {
            FFighterInput Input;
            Input.Buttons = static_cast<uint16>((Match.SimulationFrame / 30) % 2 == 0
                ? EFighterInputButton::Right : EFighterInputButton::Left);
            if (Match.SimulationFrame % 67 == 0)
            {
                Input.Buttons |= static_cast<uint16>(EFighterInputButton::Up);
            }
            FFightingSimulation::SimulateFrame(Match, Input, {});
        };
        for (int32 RenderFrame = 0; RenderFrame < RenderHz * 10; ++RenderFrame)
        {
            const int32 Steps = Clock.Consume(1.0 / RenderHz);
            for (int32 Index = 0; Index < Steps; ++Index) { Step(State); }
        }
        for (int32 Frame = 0; Frame < 600; ++Frame) { Step(Reference); }
        TestEqual(FString::Printf(TEXT("600 steps at %d render FPS"), RenderHz), State.SimulationFrame, uint64(600));
        const auto CheckFighter = [this](const FFighterState& A, const FFighterState& B)
        {
            TestTrue(TEXT("Every fighter field matches direct fixed-step reference"),
                A.PositionX == B.PositionX && A.PositionY == B.PositionY
                && A.VelocityX == B.VelocityX && A.VelocityY == B.VelocityY
                && A.Facing == B.Facing && A.bGrounded == B.bGrounded && A.Health == B.Health
                && A.CurrentState == B.CurrentState && A.StateFrame == B.StateFrame);
        };
        CheckFighter(State.Fighter1, Reference.Fighter1);
        CheckFighter(State.Fighter2, Reference.Fighter2);
        TestEqual(TEXT("No normal-rate catch-up clamps"), Clock.HitchCount, uint64(0));
    }
    FFixedSimulationClock Irregular;
    int32 TotalSteps = 0;
    for (int32 Index = 0; Index < 100; ++Index)
    {
        TotalSteps += Irregular.Consume(0.003);
        TotalSteps += Irregular.Consume(0.007);
    }
    TestEqual(TEXT("Irregular subframe timing totals one second"), TotalSteps, 60);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFixedClockHitchTest, "ProjW.DebugHarness.HitchProtection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFixedClockHitchTest::RunTest(const FString& Parameters)
{
    FFixedSimulationClock Clock;
    TestEqual(TEXT("Zero elapsed time"), Clock.Consume(0), 0);
    TestEqual(TEXT("Negative elapsed time"), Clock.Consume(-1), 0);
    TestEqual(TEXT("NaN ignored"), Clock.Consume(std::numeric_limits<double>::quiet_NaN()), 0);
    TestEqual(TEXT("Infinity ignored"), Clock.Consume(std::numeric_limits<double>::infinity()), 0);
    TestEqual(TEXT("Half frame waits"), Clock.Consume(FFixedSimulationClock::StepSeconds / 2), 0);
    TestEqual(TEXT("Fractional time retained"), Clock.Consume(FFixedSimulationClock::StepSeconds / 2), 1);
    TestEqual(TEXT("Debugger pause bounded"), Clock.Consume(120), FFixedSimulationClock::MaxStepsPerTick);
    TestEqual(TEXT("Clamp visible in diagnostics"), Clock.HitchCount, uint64(1));
    TestEqual(TEXT("No backlog remains"), Clock.Consume(FFixedSimulationClock::StepSeconds), 1);
    return !HasAnyErrors();
}
#endif
