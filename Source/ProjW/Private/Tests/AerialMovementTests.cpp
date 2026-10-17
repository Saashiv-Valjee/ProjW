#include "Simulation/FightingSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
    using Sim = FFightingSimulation;
    constexpr uint16 L = static_cast<uint16>(EFighterInputButton::Left);
    constexpr uint16 R = static_cast<uint16>(EFighterInputButton::Right);
    constexpr uint16 U = static_cast<uint16>(EFighterInputButton::Up);
    constexpr uint16 D = static_cast<uint16>(EFighterInputButton::Down);
    constexpr int32 Unit = Sim::SubunitsPerWorldUnit;

    FMatchState Initial()
    {
        FMatchState State;
        State.Fighter1.PositionX = -200 * Unit;
        State.Fighter2.PositionX = 200 * Unit;
        State.Fighter2.Facing = EFighterFacing::Left;
        return State;
    }

    void Step(FMatchState& State, uint16 P1 = 0, uint16 P2 = 0)
    {
        Sim::SimulateFrame(State, FFighterInput{P1}, FFighterInput{P2});
    }

    void Launch(FMatchState& State, uint16 Direction = 0)
    {
        Step(State, static_cast<uint16>(U | Direction));
        for (int32 Frame = 0; Frame < Sim::JumpStartupFrames; ++Frame)
        {
            Step(State, Direction);
        }
    }

    void DoubleTap(FMatchState& State, uint16 Key)
    {
        Step(State, Key);
        Step(State);
        Step(State, Key);
    }

    bool InBounds(const FFighterState& Fighter)
    {
        return Fighter.PositionX >= Sim::MinFighterX && Fighter.PositionX <= Sim::MaxFighterX;
    }

    bool IsAerialState(EFighterState State)
    {
        return State == EFighterState::Jumping || State == EFighterState::DoubleJumping;
    }

    bool IsDashState(EFighterState State)
    {
        return State == EFighterState::ForwardDash || State == EFighterState::BackDash;
    }

    void Replay(FAutomationTestBase& Test, const TCHAR* Label, const FMatchState& Snapshot,
        const TArray<uint16>& Inputs)
    {
        FMatchState First = Snapshot;
        for (uint16 Input : Inputs) { Step(First, Input); }
        FMatchState Second = Snapshot;
        for (uint16 Input : Inputs) { Step(Second, Input); }
        Test.TestTrue(Label, First == Second);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJumpStartupTest, "ProjW.Simulation.Aerial.JumpStartup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJumpStartupTest::RunTest(const FString& Parameters)
{
    FMatchState State = Initial();
    const int32 StartX = State.Fighter1.PositionX;
    Step(State, U | R);
    TestTrue(TEXT("Up press enters startup"), State.Fighter1.CurrentState == EFighterState::JumpStartup);
    TestEqual(TEXT("Startup begins at frame zero"), State.Fighter1.StateFrame, uint32(0));
    TestTrue(TEXT("Startup remains grounded"), State.Fighter1.bGrounded);
    TestEqual(TEXT("Direction commits at initiation"), State.Fighter1.CommittedJumpVelocityX,
        Sim::ForwardJumpHorizontalSpeed);
    TestEqual(TEXT("No startup movement"), State.Fighter1.PositionX, StartX);

    for (int32 Frame = 1; Frame < Sim::JumpStartupFrames; ++Frame)
    {
        Step(State, L | U); // Later direction changes do not alter commitment.
        TestTrue(TEXT("All startup frames stay grounded"), State.Fighter1.bGrounded);
        TestTrue(TEXT("All startup frames use startup state"),
            State.Fighter1.CurrentState == EFighterState::JumpStartup);
        TestEqual(TEXT("Exact startup StateFrame"), State.Fighter1.StateFrame, uint32(Frame));
        TestEqual(TEXT("Startup X remains fixed"), State.Fighter1.PositionX, StartX);
        TestEqual(TEXT("Committed direction remains fixed"), State.Fighter1.CommittedJumpVelocityX,
            Sim::ForwardJumpHorizontalSpeed);
    }
    Step(State, L | U);
    TestFalse(TEXT("Call after final startup frame launches"), State.Fighter1.bGrounded);
    TestTrue(TEXT("Launch enters normal jump"), State.Fighter1.CurrentState == EFighterState::Jumping);
    TestEqual(TEXT("Air state begins at frame zero"), State.Fighter1.StateFrame, uint32(0));
    TestEqual(TEXT("Impulse applied exactly once"), State.Fighter1.VelocityY, Sim::JumpVerticalSpeed);
    TestEqual(TEXT("Committed launch velocity used"), State.Fighter1.VelocityX,
        Sim::ForwardJumpHorizontalSpeed);
    Step(State, L | U);
    TestEqual(TEXT("Next frame applies gravity once"), State.Fighter1.VelocityY,
        Sim::JumpVerticalSpeed + Sim::Gravity);
    TestEqual(TEXT("Held opposite cannot steer"), State.Fighter1.VelocityX,
        Sim::ForwardJumpHorizontalSpeed);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeldUpAerialTest, "ProjW.Simulation.Aerial.HeldUp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeldUpAerialTest::RunTest(const FString& Parameters)
{
    FMatchState State = Initial();
    int32 Launches = 0;
    bool bPreviousGrounded = true;
    for (int32 Frame = 0; Frame < 240; ++Frame)
    {
        Step(State, U);
        if (bPreviousGrounded && !State.Fighter1.bGrounded) { ++Launches; }
        bPreviousGrounded = State.Fighter1.bGrounded;
        TestFalse(TEXT("Held Up never consumes double jump"), State.Fighter1.bDoubleJumpUsed);
        TestTrue(TEXT("Held jump never goes below ground"), State.Fighter1.PositionY >= 0);
    }
    TestEqual(TEXT("Held Up produces exactly one launch"), Launches, 1);
    TestTrue(TEXT("Held Up remains grounded after landing"), State.Fighter1.bGrounded);
    Step(State);
    Step(State, U);
    TestTrue(TEXT("Release then press starts another jump"),
        State.Fighter1.CurrentState == EFighterState::JumpStartup);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNeutralJumpTest, "ProjW.Simulation.Aerial.NeutralJump",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNeutralJumpTest::RunTest(const FString& Parameters)
{
    FMatchState State = Initial();
    const int32 StartX = State.Fighter1.PositionX;
    Launch(State);
    TestEqual(TEXT("Neutral launch X velocity"), State.Fighter1.VelocityX,
        Sim::NeutralJumpHorizontalSpeed);
    TestEqual(TEXT("Exact vertical launch impulse"), State.Fighter1.VelocityY,
        Sim::JumpVerticalSpeed);
    int32 PriorVelocityY = State.Fighter1.VelocityY;
    bool bLanded = false;
    for (int32 Frame = 0; Frame < 240; ++Frame)
    {
        Step(State, Frame % 2 ? L : R); // No continuous air steering.
        TestEqual(TEXT("Neutral trajectory keeps X"), State.Fighter1.PositionX, StartX);
        TestTrue(TEXT("Trajectory stays above ground"), State.Fighter1.PositionY >= 0);
        if (State.Fighter1.bGrounded)
        {
            bLanded = true;
            TestEqual(TEXT("Landing is exact ground"), State.Fighter1.PositionY, 0);
            TestEqual(TEXT("Landing clears vertical velocity"), State.Fighter1.VelocityY, 0);
            break;
        }
        TestEqual(TEXT("Gravity changes velocity by exact constant"), State.Fighter1.VelocityY,
            PriorVelocityY + Sim::Gravity);
        PriorVelocityY = State.Fighter1.VelocityY;
    }
    TestTrue(TEXT("Neutral jump eventually lands"), bLanded);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDirectionalJumpTest, "ProjW.Simulation.Aerial.DirectionalJump",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDirectionalJumpTest::RunTest(const FString& Parameters)
{
    for (const int32 Facing : {-1, 1})
    {
        for (const bool bForward : {false, true})
        {
            FMatchState State = Initial();
            State.Fighter1.PositionX = 0;
            State.Fighter2.PositionX = Facing * 300 * Unit;
            State.Fighter1.Facing = Facing > 0 ? EFighterFacing::Right : EFighterFacing::Left;
            State.Fighter2.Facing = Facing > 0 ? EFighterFacing::Left : EFighterFacing::Right;
            const int32 Direction = bForward ? Facing : -Facing;
            const uint16 Key = Direction > 0 ? R : L;
            const int32 Speed = bForward ? Sim::ForwardJumpHorizontalSpeed
                : Sim::BackwardJumpHorizontalSpeed;
            Launch(State, Key);
            TestEqual(TEXT("Directional launch world velocity"), State.Fighter1.VelocityX,
                Direction * Speed);
            for (int32 Frame = 0; Frame < 8; ++Frame)
            {
                Step(State, Direction > 0 ? L : R);
                TestEqual(TEXT("Air input cannot change committed momentum"),
                    State.Fighter1.VelocityX, Direction * Speed);
            }
            State.Fighter2.bGrounded = false;
            State.Fighter2.PositionY = 100 * Unit;
            State.Fighter2.PositionX = State.Fighter1.PositionX - Direction * 100 * Unit;
            Step(State, Direction > 0 ? L : R);
            const EFighterFacing ExpectedFacing = State.Fighter1.PositionX < State.Fighter2.PositionX
                ? EFighterFacing::Right : EFighterFacing::Left;
            TestTrue(TEXT("Facing follows final simulation positions"),
                State.Fighter1.Facing == ExpectedFacing);
            TestEqual(TEXT("Facing change cannot reverse momentum"), State.Fighter1.VelocityX,
                Direction * Speed);
        }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDoubleJumpTest, "ProjW.Simulation.Aerial.DoubleJump",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDoubleJumpTest::RunTest(const FString& Parameters)
{
    FMatchState State = Initial();
    Step(State, U);
    Step(State); // Fresh Up during startup must not double jump.
    Step(State, U);
    TestTrue(TEXT("Startup cannot double jump"),
        State.Fighter1.CurrentState == EFighterState::JumpStartup && !State.Fighter1.bDoubleJumpUsed);
    while (State.Fighter1.CurrentState == EFighterState::JumpStartup) { Step(State); }
    Step(State); // Ensure Up is released in air.
    Step(State, U);
    TestTrue(TEXT("Fresh airborne Up enters double jump"),
        State.Fighter1.CurrentState == EFighterState::DoubleJumping);
    TestEqual(TEXT("Double jump resets exact vertical velocity"), State.Fighter1.VelocityY,
        Sim::DoubleJumpVerticalSpeed);
    TestTrue(TEXT("Double jump usage recorded"), State.Fighter1.bDoubleJumpUsed);
    Step(State);
    const int32 BeforeThird = State.Fighter1.VelocityY;
    Step(State, U);
    TestEqual(TEXT("Third press does not reset vertical velocity"), State.Fighter1.VelocityY,
        BeforeThird + Sim::Gravity);
    TestTrue(TEXT("Third press stays in double jump state"),
        State.Fighter1.CurrentState == EFighterState::DoubleJumping);
    while (!State.Fighter1.bGrounded) { Step(State); }
    TestFalse(TEXT("Landing resets double jump usage"), State.Fighter1.bDoubleJumpUsed);
    Step(State, U);
    TestTrue(TEXT("Grounded jump available again"),
        State.Fighter1.CurrentState == EFighterState::JumpStartup);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDirectionalDoubleJumpTest, "ProjW.Simulation.Aerial.DirectionalDoubleJump",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDirectionalDoubleJumpTest::RunTest(const FString& Parameters)
{
    for (const int32 Facing : {-1, 1})
    {
        for (const int32 Relative : {-1, 0, 1})
        {
            FMatchState State = Initial();
            State.Fighter1.PositionX = 0;
            State.Fighter2.PositionX = Facing * 300 * Unit;
            State.Fighter1.Facing = Facing > 0 ? EFighterFacing::Right : EFighterFacing::Left;
            State.Fighter2.Facing = Facing > 0 ? EFighterFacing::Left : EFighterFacing::Right;
            Launch(State);
            Step(State); // Up release.
            const int32 Direction = Relative * Facing;
            const uint16 Key = Direction > 0 ? R : Direction < 0 ? L : 0;
            Step(State, static_cast<uint16>(U | Key));
            const int32 Speed = Relative > 0 ? Sim::ForwardDoubleJumpHorizontalSpeed
                : Relative < 0 ? Sim::BackwardDoubleJumpHorizontalSpeed
                : Sim::NeutralDoubleJumpHorizontalSpeed;
            TestEqual(TEXT("Double jump horizontal replacement"), State.Fighter1.VelocityX,
                Direction * Speed);
            TestEqual(TEXT("Double jump vertical replacement"), State.Fighter1.VelocityY,
                Sim::DoubleJumpVerticalSpeed);
        }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAirCrossingTest, "ProjW.Simulation.Aerial.CrossingFacing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAirCrossingTest::RunTest(const FString& Parameters)
{
    FMatchState State = Initial();
    State.Fighter1.PositionX = -30 * Unit;
    State.Fighter2.PositionX = 30 * Unit;
    Launch(State, R);
    Step(State); // Release Up.
    // Place the opponent just behind P1 while both are airborne, then simulate;
    // authoritative transforms are test setup only and never presentation input.
    State.Fighter2.bGrounded = false;
    State.Fighter2.PositionY = State.Fighter1.PositionY;
    State.Fighter2.PositionX = State.Fighter1.PositionX - Unit;
    Step(State);
    TestTrue(TEXT("Airborne fighter crosses without push"),
        !State.Fighter1.bGrounded && State.Fighter1.PositionX > State.Fighter2.PositionX);
    TestTrue(TEXT("Crossed fighter now faces left"), State.Fighter1.Facing == EFighterFacing::Left);
    TestEqual(TEXT("Crossing preserves world-right momentum"), State.Fighter1.VelocityX,
        Sim::ForwardJumpHorizontalSpeed);
    Step(State, U | L); // Left is forward under the new facing.
    TestEqual(TEXT("Double jump uses new facing"), State.Fighter1.VelocityX,
        -Sim::ForwardDoubleJumpHorizontalSpeed);
    TestTrue(TEXT("Directional reset enters double jump"),
        State.Fighter1.CurrentState == EFighterState::DoubleJumping);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAerialStageBoundTest, "ProjW.Simulation.Aerial.StageBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAerialStageBoundTest::RunTest(const FString& Parameters)
{
    for (const int32 Direction : {-1, 1})
    {
        FMatchState State = Initial();
        State.Fighter1.PositionX = Direction > 0 ? Sim::MaxFighterX - Unit : Sim::MinFighterX + Unit;
        State.Fighter2.PositionX = -Direction * 200 * Unit;
        State.Fighter1.Facing = Direction > 0 ? EFighterFacing::Right : EFighterFacing::Left;
        const uint16 Key = Direction > 0 ? R : L;
        Launch(State, Key);
        TestTrue(TEXT("Launch remains bounded"), InBounds(State.Fighter1));
        TestEqual(TEXT("Wall blocks horizontal launch velocity"), State.Fighter1.VelocityX, 0);
        const int32 Height = State.Fighter1.PositionY;
        const int32 Vertical = State.Fighter1.VelocityY;
        Step(State, Direction > 0 ? L : R);
        TestTrue(TEXT("Air wall remains bounded"), InBounds(State.Fighter1));
        TestEqual(TEXT("Wall has no horizontal bounce"), State.Fighter1.VelocityX, 0);
        TestEqual(TEXT("Vertical gravity continues at wall"), State.Fighter1.VelocityY,
            Vertical + Sim::Gravity);
        TestEqual(TEXT("Vertical position continues at wall"), State.Fighter1.PositionY,
            Height + Vertical + Sim::Gravity);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAerialGroundInteractionTest, "ProjW.Simulation.Aerial.GroundedInteractions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAerialGroundInteractionTest::RunTest(const FString& Parameters)
{
    for (const bool bForward : {false, true})
    {
        FMatchState State = Initial();
        const uint16 Key = bForward ? R : L;
        DoubleTap(State, Key);
        const EFighterState DashState = bForward ? EFighterState::ForwardDash : EFighterState::BackDash;
        TestTrue(TEXT("Dash setup entered expected state"), State.Fighter1.CurrentState == DashState);
        Step(State, U);
        TestTrue(TEXT("Up cannot cancel active dash"), State.Fighter1.CurrentState == DashState);
        TestTrue(TEXT("Dash remains grounded"), State.Fighter1.bGrounded);
    }

    FMatchState State = Initial();
    Launch(State);
    Step(State, R);
    Step(State);
    Step(State, R);
    TestTrue(TEXT("Airborne double tap cannot dash"), IsAerialState(State.Fighter1.CurrentState));

    State = Initial();
    Step(State, D);
    Step(State, D | U);
    TestTrue(TEXT("Fresh Up from crouch priority enters startup"),
        State.Fighter1.CurrentState == EFighterState::JumpStartup);
    TestTrue(TEXT("Crouch-to-jump startup remains grounded"), State.Fighter1.bGrounded);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAerialSnapshotTest, "ProjW.Simulation.Aerial.SnapshotReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAerialSnapshotTest::RunTest(const FString& Parameters)
{
    TArray<uint16> Continuation;
    Continuation.Reserve(180);
    for (int32 Frame = 0; Frame < 180; ++Frame)
    {
        Continuation.Add(Frame == 8 ? static_cast<uint16>(U | L) : 0);
    }

    FMatchState Startup = Initial();
    Step(Startup, U | R);
    Step(Startup, R);
    TestTrue(TEXT("Startup snapshot is in startup"), Startup.Fighter1.CurrentState == EFighterState::JumpStartup);
    Replay(*this, TEXT("Startup snapshot exact replay"), Startup, Continuation);

    FMatchState Airborne = Initial();
    Launch(Airborne, R);
    Step(Airborne);
    TestTrue(TEXT("Pre-double snapshot is normal jump"), Airborne.Fighter1.CurrentState == EFighterState::Jumping);
    Replay(*this, TEXT("First-airborne snapshot exact replay"), Airborne, Continuation);

    FMatchState Doubled = Airborne;
    Step(Doubled, U | L);
    Step(Doubled);
    TestTrue(TEXT("Post-double snapshot records usage"), Doubled.Fighter1.bDoubleJumpUsed);
    Replay(*this, TEXT("Post-double snapshot exact replay"), Doubled, Continuation);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAerialRepeatabilityTest, "ProjW.Simulation.Aerial.MixedRepeatability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAerialRepeatabilityTest::RunTest(const FString& Parameters)
{
    FMatchState A = Initial();
    FMatchState B = A;
    bool bStartup = false, bJump = false, bDouble = false, bDash = false;
    bool bCrouch = false, bCrossed = false, bWall = false;
    for (uint32 Frame = 0; Frame < 30000; ++Frame)
    {
        const uint32 Phase = Frame % 300;
        uint16 P1 = 0;
        uint16 P2 = 0;
        if (Phase == 0) { P1 = U | R; P2 = U | L; }
        else if (Phase < 12) { P1 = R; P2 = L; }
        else if (Phase == 12) { P1 = U | R; P2 = U | L; }
        else if (Phase < 55) { P1 = R; P2 = L; }
        else if (Phase >= 75 && Phase < 90) { P1 = D; P2 = D; }
        else if (Phase == 100 || Phase == 102) { P1 = R; P2 = L; }
        else if (Phase > 102 && Phase < 125) { P1 = R; P2 = L; }
        else if (Phase == 140 || Phase == 142) { P1 = L; P2 = R; }
        else if (Phase > 142 && Phase < 165) { P1 = L; P2 = R; }
        else if (Phase >= 180 && Phase < 240) { P1 = R; P2 = L; }
        Step(A, P1, P2);
        Step(B, P1, P2);
        if (!(A == B))
        {
            AddError(FString::Printf(TEXT("Aerial mixed replay diverged at frame %u"), Frame));
            return false;
        }
        TestTrue(TEXT("Mixed aerial state remains in bounds"), InBounds(A.Fighter1) && InBounds(A.Fighter2));
        TestTrue(TEXT("Mixed aerial state never below ground"),
            A.Fighter1.PositionY >= 0 && A.Fighter2.PositionY >= 0);
        bStartup |= A.Fighter1.CurrentState == EFighterState::JumpStartup;
        bJump |= A.Fighter1.CurrentState == EFighterState::Jumping;
        bDouble |= A.Fighter1.CurrentState == EFighterState::DoubleJumping;
        bDash |= IsDashState(A.Fighter1.CurrentState);
        bCrouch |= A.Fighter1.CurrentState == EFighterState::Crouching;
        bCrossed |= A.Fighter1.PositionX > A.Fighter2.PositionX;
        bWall |= A.Fighter1.PositionX == Sim::MinFighterX || A.Fighter1.PositionX == Sim::MaxFighterX
            || A.Fighter2.PositionX == Sim::MinFighterX || A.Fighter2.PositionX == Sim::MaxFighterX;
    }
    TestTrue(TEXT("Mixed sequence covered startup"), bStartup);
    TestTrue(TEXT("Mixed sequence covered normal jump"), bJump);
    TestTrue(TEXT("Mixed sequence covered double jump"), bDouble);
    TestTrue(TEXT("Mixed sequence covered dash"), bDash);
    TestTrue(TEXT("Mixed sequence covered crouch"), bCrouch);
    TestTrue(TEXT("Mixed sequence covered airborne crossing"), bCrossed);
    TestTrue(TEXT("Mixed sequence covered stage contact"), bWall);
    TestEqual(TEXT("Exactly 30000 frames simulated"), A.SimulationFrame, uint64(30000));
    return !HasAnyErrors();
}
#endif
