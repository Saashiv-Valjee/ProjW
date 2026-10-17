#include "Simulation/FightingSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
    FFighterInput Input(uint16 Buttons)
    {
        return FFighterInput{Buttons};
    }

    uint16 Bit(EFighterInputButton Button)
    {
        return static_cast<uint16>(Button);
    }

    // Enumerates all ten button bits without random generators or wall-clock data.
    FFighterInput Sequence(uint32 Frame, uint32 Player)
    {
        return Input(static_cast<uint16>(((Frame * 37u) ^ (Frame / 19u) ^ (Player * 173u)) & 0x3ffu));
    }

    FMatchState InitialState()
    {
        FMatchState State;
        State.Fighter1.PositionX = -100 * FFightingSimulation::SubunitsPerWorldUnit;
        State.Fighter2.PositionX = 100 * FFightingSimulation::SubunitsPerWorldUnit;
        State.Fighter2.Facing = EFighterFacing::Left;
        return State;
    }

    bool EqualFighter(FAutomationTestBase& Test, const FFighterState& A, const FFighterState& B)
    {
        bool Equal = true;
        Equal &= Test.TestEqual(TEXT("PositionX"), A.PositionX, B.PositionX);
        Equal &= Test.TestEqual(TEXT("PositionY"), A.PositionY, B.PositionY);
        Equal &= Test.TestEqual(TEXT("VelocityX"), A.VelocityX, B.VelocityX);
        Equal &= Test.TestEqual(TEXT("VelocityY"), A.VelocityY, B.VelocityY);
        Equal &= Test.TestEqual(TEXT("Facing"), static_cast<int8>(A.Facing), static_cast<int8>(B.Facing));
        Equal &= Test.TestEqual(TEXT("Grounded"), A.bGrounded, B.bGrounded);
        Equal &= Test.TestEqual(TEXT("Health"), A.Health, B.Health);
        Equal &= Test.TestEqual(TEXT("CurrentState"), static_cast<uint8>(A.CurrentState), static_cast<uint8>(B.CurrentState));
        Equal &= Test.TestEqual(TEXT("StateFrame"), A.StateFrame, B.StateFrame);
        return Equal;
    }

    bool EqualMatch(FAutomationTestBase& Test, const FMatchState& A, const FMatchState& B)
    {
        const bool FrameEqual = Test.TestEqual(TEXT("SimulationFrame"), A.SimulationFrame, B.SimulationFrame);
        const bool FirstEqual = EqualFighter(Test, A.Fighter1, B.Fighter1);
        const bool SecondEqual = EqualFighter(Test, A.Fighter2, B.Fighter2);
        return FrameEqual && FirstEqual && SecondEqual;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightingRepeatabilityTest, "ProjW.Simulation.Repeatability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFightingRepeatabilityTest::RunTest(const FString& Parameters)
{
    FMatchState A = InitialState();
    FMatchState B = A;
    for (uint32 Frame = 0; Frame < 10000; ++Frame)
    {
        FFightingSimulation::SimulateFrame(A, Sequence(Frame, 0), Sequence(Frame, 1));
        FFightingSimulation::SimulateFrame(B, Sequence(Frame, 0), Sequence(Frame, 1));
        if (!EqualMatch(*this, A, B))
        {
            AddError(FString::Printf(TEXT("Diverged at input frame %u"), Frame));
            return false;
        }
    }
    TestEqual(TEXT("Completed 10000 steps"), A.SimulationFrame, uint64(10000));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightingSnapshotTest, "ProjW.Simulation.SnapshotReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFightingSnapshotTest::RunTest(const FString& Parameters)
{
    FMatchState State = InitialState();
    constexpr uint32 SnapshotFrame = 137;
    constexpr uint32 EndFrame = 10000;
    for (uint32 Frame = 0; Frame < SnapshotFrame; ++Frame)
    {
        FFightingSimulation::SimulateFrame(State, Sequence(Frame, 0), Sequence(Frame, 1));
    }
    const FMatchState Snapshot = State;
    for (uint32 Frame = SnapshotFrame; Frame < EndFrame; ++Frame)
    {
        FFightingSimulation::SimulateFrame(State, Sequence(Frame, 0), Sequence(Frame, 1));
    }
    const FMatchState Expected = State;
    State = Snapshot;
    for (uint32 Frame = SnapshotFrame; Frame < EndFrame; ++Frame)
    {
        FFightingSimulation::SimulateFrame(State, Sequence(Frame, 0), Sequence(Frame, 1));
    }
    return EqualMatch(*this, State, Expected);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightingMovementTest, "ProjW.Simulation.MovementInvariants",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFightingMovementTest::RunTest(const FString& Parameters)
{
    FMatchState State = InitialState();
    const int32 StartX = State.Fighter1.PositionX;
    FFightingSimulation::SimulateFrame(State, Input(Bit(EFighterInputButton::Right)), Input(Bit(EFighterInputButton::Left)));
    TestEqual(TEXT("Walk right"), State.Fighter1.PositionX, StartX + FFightingSimulation::WalkSpeed);
    TestEqual(TEXT("Walk left"), State.Fighter2.VelocityX, -FFightingSimulation::WalkSpeed);
    TestTrue(TEXT("Walking state"), State.Fighter1.CurrentState == EFighterState::Walking);
    TestEqual(TEXT("State entry frame"), State.Fighter1.StateFrame, uint32(0));
    FFightingSimulation::SimulateFrame(State, Input(Bit(EFighterInputButton::Left) | Bit(EFighterInputButton::Right)), {});
    TestEqual(TEXT("Opposed directions cancel"), State.Fighter1.VelocityX, 0);
    TestTrue(TEXT("Idle state"), State.Fighter1.CurrentState == EFighterState::Idle);
    FFightingSimulation::SimulateFrame(State, Input(Bit(EFighterInputButton::Up)), {});
    TestFalse(TEXT("Jump leaves ground"), State.Fighter1.bGrounded);
    TestTrue(TEXT("Jump rises"), State.Fighter1.PositionY > 0);
    TestTrue(TEXT("Jump state"), State.Fighter1.CurrentState == EFighterState::Jumping);
    const int32 AirStartX = State.Fighter1.PositionX;
    FFightingSimulation::SimulateFrame(State, Input(Bit(EFighterInputButton::Left)), {});
    TestEqual(TEXT("Horizontal control in air"), State.Fighter1.PositionX,
        AirStartX - FFightingSimulation::WalkSpeed);
    TestEqual(TEXT("Continuing state increments timer"), State.Fighter1.StateFrame, uint32(1));
    bool Landed = false;
    for (uint32 Frame = 0; Frame < 240; ++Frame)
    {
        const uint64 Before = State.SimulationFrame;
        FFightingSimulation::SimulateFrame(State, {}, {});
        TestEqual(TEXT("Exactly one frame per call"), State.SimulationFrame, Before + uint64(1));
        TestTrue(TEXT("Fighter1 above ground"), State.Fighter1.PositionY >= 0);
        TestTrue(TEXT("Fighter2 above ground"), State.Fighter2.PositionY >= 0);
        Landed |= State.Fighter1.bGrounded;
    }
    TestTrue(TEXT("Jump eventually lands"), Landed);
    TestEqual(TEXT("Ground height"), State.Fighter1.PositionY, 0);
    TestEqual(TEXT("Landing clears vertical speed"), State.Fighter1.VelocityY, 0);
    TestEqual(TEXT("Initial calls also count"), State.SimulationFrame, uint64(244));
    TestTrue(TEXT("Facing right toward opponent"), State.Fighter1.Facing == EFighterFacing::Right);
    TestTrue(TEXT("Facing left toward opponent"), State.Fighter2.Facing == EFighterFacing::Left);

    State.Fighter1.PositionX = 10;
    State.Fighter2.PositionX = -10;
    FFightingSimulation::SimulateFrame(State, {}, {});
    TestTrue(TEXT("Facing switches after crossing"), State.Fighter1.Facing == EFighterFacing::Left
        && State.Fighter2.Facing == EFighterFacing::Right);
    State.Fighter2.PositionX = State.Fighter1.PositionX;
    FFightingSimulation::SimulateFrame(State, {}, {});
    TestTrue(TEXT("Equal X retains facing"), State.Fighter1.Facing == EFighterFacing::Left
        && State.Fighter2.Facing == EFighterFacing::Right);

    const FMatchState BeforeUnused = State;
    FMatchState Neutral = State;
    FFightingSimulation::SimulateFrame(State, Input(0x3f8u), Input(0x3f8u));
    FFightingSimulation::SimulateFrame(Neutral, {}, {});
    EqualMatch(*this, State, Neutral);
    TestEqual(TEXT("Health unchanged"), State.Fighter1.Health, BeforeUnused.Fighter1.Health);

    State.Fighter1.PositionX = std::numeric_limits<int32>::max();
    State.Fighter2.PositionX = std::numeric_limits<int32>::min();
    FFightingSimulation::SimulateFrame(State, Input(Bit(EFighterInputButton::Right)), Input(Bit(EFighterInputButton::Left)));
    TestEqual(TEXT("Positive bound does not overflow"), State.Fighter1.PositionX, std::numeric_limits<int32>::max());
    TestEqual(TEXT("Negative bound does not overflow"), State.Fighter2.PositionX, std::numeric_limits<int32>::min());

    State = InitialState();
    bool WasGrounded = true;
    uint32 Takeoffs = 0;
    for (uint32 Frame = 0; Frame < 120; ++Frame)
    {
        FFightingSimulation::SimulateFrame(State, Input(Bit(EFighterInputButton::Up)), {});
        if (WasGrounded && !State.Fighter1.bGrounded)
        {
            ++Takeoffs;
        }
        WasGrounded = State.Fighter1.bGrounded;
    }
    TestTrue(TEXT("Held Up repeats jumps after landing"), Takeoffs >= 2);
    return !HasAnyErrors();
}
#endif
