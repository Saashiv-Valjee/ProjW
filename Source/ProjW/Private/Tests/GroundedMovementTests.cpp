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
        FMatchState S;
        S.Fighter1.PositionX = -200 * Unit;
        S.Fighter2.PositionX = 200 * Unit;
        S.Fighter2.Facing = EFighterFacing::Left;
        return S;
    }

    void Step(FMatchState& S, uint16 P1 = 0, uint16 P2 = 0)
    {
        Sim::SimulateFrame(S, FFighterInput{P1}, FFighterInput{P2});
    }

    void DoubleTap(FMatchState& S, uint16 Key)
    {
        Step(S, Key);
        Step(S);
        Step(S, Key);
    }

    bool InBounds(const FFighterState& F)
    {
        return Sim::IsInsideArena(F.PositionX, F.PositionZ);
    }

    bool CheckDash(FAutomationTestBase& Test, bool Forward)
    {
        const int32 Duration = Forward ? Sim::ForwardDashDurationFrames : Sim::BackDashDurationFrames;
        const int32 Speed = Forward ? Sim::ForwardDashSpeed : Sim::BackDashSpeed;
        const EFighterState Expected = Forward ? EFighterState::ForwardDash : EFighterState::BackDash;
        for (const int32 Facing : {-1, 1})
        {
            FMatchState S = Initial();
            S.Fighter1.PositionX = 0;
            S.Fighter2.PositionX = Facing * 400 * Unit;
            S.Fighter1.Facing = Facing > 0 ? EFighterFacing::Right : EFighterFacing::Left;
            const int32 Direction = Forward ? Facing : -Facing;
            const uint16 Key = Direction > 0 ? R : L;
            Step(S, Key);
            Step(S);
            const int32 Start = S.Fighter1.PositionX;
            Step(S, Key);
            Test.TestTrue(TEXT("Second tap enters expected dash"), S.Fighter1.CurrentState == Expected);
            Test.TestEqual(TEXT("Entry is state frame zero"), S.Fighter1.StateFrame, uint32(0));
            Test.TestEqual(TEXT("Locked world direction"), S.Fighter1.DashDirection, static_cast<int8>(Direction));
            for (int32 Index = 1; Index < Duration; ++Index)
            {
                // Jump/crouch/direction reversal cannot cancel or turn an active dash.
                Step(S, static_cast<uint16>((Direction > 0 ? L : R) | U | D));
                Test.TestTrue(TEXT("Fixed dash state for full duration"), S.Fighter1.CurrentState == Expected);
                Test.TestEqual(TEXT("Exact dash state timer"), S.Fighter1.StateFrame, static_cast<uint32>(Index));
                Test.TestEqual(TEXT("Exact dash speed"), S.Fighter1.VelocityX, Direction * Speed);
                Test.TestTrue(TEXT("Dash remains grounded"), S.Fighter1.bGrounded);
            }
            Test.TestEqual(TEXT("Full deterministic dash displacement"), S.Fighter1.PositionX - Start,
                Direction * Speed * Duration);
            Step(S);
            Test.TestTrue(TEXT("Dash ends at exact duration"), S.Fighter1.CurrentState == EFighterState::Idle);
            Test.TestEqual(TEXT("Exit state frame zero"), S.Fighter1.StateFrame, uint32(0));
            Test.TestEqual(TEXT("Dash direction cleared on exit"), S.Fighter1.DashDirection, int8(0));

            S = Initial();
            Step(S, Key);
            for (int32 Frame = 0; Frame < Sim::DashInputWindowFrames; ++Frame) { Step(S); }
            Step(S, Key);
            Test.TestTrue(TEXT("Tap beyond inclusive window cannot dash"),
                S.Fighter1.CurrentState == EFighterState::Walking);
        }
        // Exact inclusive window edge, with more than one neutral frame.
        FMatchState Boundary = Initial();
        const uint16 Key = Forward ? R : L;
        Step(Boundary, Key);
        for (int32 Frame = 1; Frame < Sim::DashInputWindowFrames; ++Frame) { Step(Boundary); }
        Step(Boundary, Key);
        Test.TestTrue(TEXT("Second press exactly at window accepted"), Boundary.Fighter1.CurrentState == Expected);
        // Holding a direction never supplies a second press or retriggers a dash.
        FMatchState Held = Initial();
        for (int32 Frame = 0; Frame < 60; ++Frame)
        {
            Step(Held, Key);
            Test.TestTrue(TEXT("One held press stays walking"), Held.Fighter1.CurrentState == EFighterState::Walking);
        }
        Held = Initial();
        DoubleTap(Held, Key);
        for (int32 Frame = 1; Frame <= Duration + 20; ++Frame)
        {
            Step(Held, Key);
            if (Frame >= Duration)
            {
                Test.TestTrue(TEXT("Consumed double tap cannot retrigger while held"),
                    Held.Fighter1.CurrentState == EFighterState::Walking);
            }
        }
        return !Test.HasAnyErrors();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputEdgesTest, "ProjW.Simulation.InputEdges",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInputEdgesTest::RunTest(const FString& Parameters)
{
    FMatchState S = Initial();
    // Check every declared button, including all currently inert action inputs.
    for (uint16 Bit = 1; Bit <= (1u << 9); Bit <<= 1)
    {
        Step(S);
        Step(S, Bit);
        TestEqual(TEXT("Fresh press held"), S.Fighter1.PreviousInput.Buttons, Bit);
        TestEqual(TEXT("Fresh press edge"), S.Fighter1.PressedInput.Buttons, Bit);
        TestEqual(TEXT("No release on press"), S.Fighter1.ReleasedInput.Buttons, uint16(0));
        Step(S, Bit);
        TestEqual(TEXT("Held remains held"), S.Fighter1.PreviousInput.Buttons, Bit);
        TestEqual(TEXT("Held is not newly pressed"), S.Fighter1.PressedInput.Buttons, uint16(0));
        Step(S);
        TestEqual(TEXT("Release edge"), S.Fighter1.ReleasedInput.Buttons, Bit);
        TestEqual(TEXT("Released no longer held"), S.Fighter1.PreviousInput.Buttons, uint16(0));
        Step(S);
        TestEqual(TEXT("Release lasts one frame"), S.Fighter1.ReleasedInput.Buttons, uint16(0));
    }
    const FFighterInputEdges E = FFighterInputEdges::From(FFighterInput{R}, FFighterInput{L});
    TestEqual(TEXT("Simultaneous release and press"), E.Released.Buttons, L);
    TestEqual(TEXT("Opposite press"), E.Pressed.Buttons, R);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeldJumpTest, "ProjW.Simulation.HeldJump",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeldJumpTest::RunTest(const FString& Parameters)
{
    FMatchState S = Initial();
    Step(S, U);
    TestTrue(TEXT("Up press starts grounded startup"), S.Fighter1.bGrounded);
    for (int32 Frame = 1; Frame <= Sim::JumpStartupFrames; ++Frame) { Step(S, U); }
    TestFalse(TEXT("Startup eventually launches"), S.Fighter1.bGrounded);
    TestEqual(TEXT("Configured first jump velocity"), S.Fighter1.VelocityY, Sim::JumpVerticalSpeed);
    bool Landed = false;
    for (int32 Frame = 0; Frame < 180; ++Frame)
    {
        Step(S, U);
        if (Landed) { TestTrue(TEXT("Held Up stays grounded after landing"), S.Fighter1.bGrounded); }
        Landed |= S.Fighter1.bGrounded;
        TestTrue(TEXT("Never below ground"), S.Fighter1.PositionY >= 0);
    }
    TestTrue(TEXT("Original jump arc returns to ground"), Landed);
    Step(S);
    Step(S, U);
    TestTrue(TEXT("Release and new press starts another startup"), S.Fighter1.bGrounded);
    TestTrue(TEXT("Second startup state"), S.Fighter1.CurrentState == EFighterState::JumpStartup);
    TestEqual(TEXT("New jump state entry"), S.Fighter1.StateFrame, uint32(0));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRelativeWalkTest, "ProjW.Simulation.FacingRelativeWalk",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRelativeWalkTest::RunTest(const FString& Parameters)
{
    for (const int32 Facing : {-1, 1})
    {
        for (const bool Forward : {false, true})
        {
            FMatchState S = Initial();
            S.Fighter1.PositionX = 0;
            S.Fighter2.PositionX = Facing * 300 * Unit;
            S.Fighter1.Facing = Facing > 0 ? EFighterFacing::Right : EFighterFacing::Left;
            const int32 Direction = Forward ? Facing : -Facing;
            const int32 Speed = Forward ? Sim::ForwardWalkSpeed : Sim::BackwardWalkSpeed;
            Step(S, Direction > 0 ? R : L);
            TestEqual(TEXT("Facing-relative speed"), S.Fighter1.VelocityX, Direction * Speed);
            TestEqual(TEXT("Facing-relative displacement"), S.Fighter1.PositionX, Direction * Speed);
            Step(S);
            TestEqual(TEXT("Neutral stops immediately"), S.Fighter1.VelocityX, 0);
            Step(S, L | R);
            TestEqual(TEXT("Opposed directions neutral"), S.Fighter1.VelocityX, 0);
            Step(S, U | (Direction > 0 ? R : L));
            for (int32 Frame = 1; Frame <= Sim::JumpStartupFrames; ++Frame) { Step(S); }
            TestEqual(TEXT("Committed jump speed respects facing"), S.Fighter1.VelocityX,
                Direction * (Forward ? Sim::ForwardJumpHorizontalSpeed : Sim::BackwardJumpHorizontalSpeed));
        }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrouchTest, "ProjW.Simulation.Crouching",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrouchTest::RunTest(const FString& Parameters)
{
    FMatchState S = Initial();
    const int32 X = S.Fighter1.PositionX;
    Step(S, D);
    TestTrue(TEXT("Down enters crouch"), S.Fighter1.CurrentState == EFighterState::Crouching);
    TestEqual(TEXT("Crouch entry timer"), S.Fighter1.StateFrame, uint32(0));
    for (uint32 Frame = 1; Frame <= 20; ++Frame)
    {
        Step(S, D | R);
        TestTrue(TEXT("Held down stays crouched"), S.Fighter1.CurrentState == EFighterState::Crouching);
        TestEqual(TEXT("Crouch timer increments"), S.Fighter1.StateFrame, Frame);
        TestEqual(TEXT("No crouch walking"), S.Fighter1.PositionX, X);
    }
    Step(S);
    TestTrue(TEXT("Release down returns idle"), S.Fighter1.CurrentState == EFighterState::Idle);
    TestEqual(TEXT("Idle entry timer"), S.Fighter1.StateFrame, uint32(0));
    Step(S, D);
    Step(S, R);
    TestTrue(TEXT("Release into direction walks"), S.Fighter1.CurrentState == EFighterState::Walking);
    Step(S, D | U);
    TestTrue(TEXT("New jump press has priority over crouch"), S.Fighter1.CurrentState == EFighterState::JumpStartup);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForwardDashTest, "ProjW.Simulation.ForwardDash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForwardDashTest::RunTest(const FString& Parameters) { return CheckDash(*this, true); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackDashTest, "ProjW.Simulation.BackDash",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBackDashTest::RunTest(const FString& Parameters) { return CheckDash(*this, false); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStageBoundsTest, "ProjW.Simulation.StageBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStageBoundsTest::RunTest(const FString& Parameters)
{
    for (const int32 Direction : {-1, 1})
    {
        FMatchState S = Initial();
        const uint16 Key = Direction > 0 ? R : L;
        S.Fighter1.PositionX = Direction * 470 * Unit;
        S.Fighter2.PositionX = -Direction * 200 * Unit;
        S.Fighter1.Facing = Direction > 0 ? EFighterFacing::Left : EFighterFacing::Right;
        for (int32 Frame = 0; Frame < 30; ++Frame) { Step(S, Key); }
        TestEqual(TEXT("Walk clamps at body-safe stage edge"), S.Fighter1.PositionX,
            Direction > 0 ? Sim::MaxFighterX : Sim::MinFighterX);
        TestEqual(TEXT("Wall reports zero displacement"), S.Fighter1.VelocityX, 0);
        Step(S);
        DoubleTap(S, Key);
        for (int32 Frame = 0; Frame < 30; ++Frame)
        {
            Step(S, Key);
            TestTrue(TEXT("Dash remains within stage"), InBounds(S.Fighter1) && InBounds(S.Fighter2));
        }
        // Forward dash into a boundary with the opponent airborne (no body push).
        S = Initial();
        S.Fighter1.PositionX = Direction * 470 * Unit;
        S.Fighter2.PositionX = Direction * Sim::MaxFighterX;
        S.Fighter2.bGrounded = false;
        S.Fighter2.PositionY = 1000000;
        S.Fighter1.Facing = Direction > 0 ? EFighterFacing::Right : EFighterFacing::Left;
        DoubleTap(S, Key);
        TestTrue(TEXT("Forward dash enters at wall"), S.Fighter1.CurrentState == EFighterState::ForwardDash);
        for (int32 Frame = 0; Frame < 20; ++Frame)
        {
            Step(S, Key);
            TestTrue(TEXT("Forward dash bounded"), InBounds(S.Fighter1) && InBounds(S.Fighter2));
        }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPushTest, "ProjW.Simulation.PushInteraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPushTest::RunTest(const FString& Parameters)
{
    for (const bool AtWall : {false, true})
    {
        FMatchState S = Initial();
        S.Fighter1.PositionX = AtWall ? Sim::MaxFighterX - 2 * Sim::PushHalfWidth : -Sim::PushHalfWidth;
        S.Fighter2.PositionX = S.Fighter1.PositionX + 2 * Sim::PushHalfWidth;
        // Odd subunit tests deterministic remainder allocation by spatial role.
        if (!AtWall) { ++S.Fighter1.PositionX; }
        FMatchState Swapped = S;
        Swapped.Fighter1 = S.Fighter2;
        Swapped.Fighter2 = S.Fighter1;
        for (int32 Frame = 0; Frame < 120; ++Frame)
        {
            const uint16 P1 = Frame == 1 ? 0 : R;
            const uint16 P2 = AtWall ? 0 : L;
            Step(S, P1, P2);
            Step(Swapped, P2, P1);
            TestTrue(TEXT("Player-ID swap yields identical spatial result"),
                S.Fighter1 == Swapped.Fighter2 && S.Fighter2 == Swapped.Fighter1);
            TestTrue(TEXT("Bodies cannot overlap or cross"),
                S.Fighter2.PositionX - S.Fighter1.PositionX >= 2 * Sim::PushHalfWidth);
            TestTrue(TEXT("Push respects stage"), InBounds(S.Fighter1) && InBounds(S.Fighter2));
            TestTrue(TEXT("Facing stable during push"), S.Fighter1.Facing == EFighterFacing::Right
                && S.Fighter2.Facing == EFighterFacing::Left);
            if (Frame == 2) { TestTrue(TEXT("Dash into body is exercised"), S.Fighter1.CurrentState == EFighterState::ForwardDash); }
        }
        if (AtWall)
        {
            TestEqual(TEXT("Wall transfers separation without overflow"), S.Fighter2.PositionX, Sim::MaxFighterX);
        }
    }
    // A single large intended crossing must preserve pre-step spatial order.
    FMatchState S = Initial();
    S.Fighter1.PositionX = -Sim::PushHalfWidth;
    S.Fighter2.PositionX = Sim::PushHalfWidth;
    S.Fighter1.CurrentState = EFighterState::ForwardDash;
    S.Fighter1.DashDirection = 1;
    S.Fighter2.CurrentState = EFighterState::ForwardDash;
    S.Fighter2.DashDirection = -1;
    Step(S);
    TestEqual(TEXT("Head-on dashes split correction evenly"), S.Fighter1.PositionX, -Sim::PushHalfWidth);
    TestEqual(TEXT("Head-on dashes preserve spacing"), S.Fighter2.PositionX, Sim::PushHalfWidth);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInputSnapshotTest, "ProjW.Simulation.InputSnapshotReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInputSnapshotTest::RunTest(const FString& Parameters)
{
    FMatchState S = Initial();
    Step(S, R);
    Step(S); // Snapshot after first tap/release, before second tap.
    const FMatchState Snapshot = S;
    Step(S, R);
    TestTrue(TEXT("Snapshot candidate leads to dash"), S.Fighter1.CurrentState == EFighterState::ForwardDash);
    const FMatchState ActiveDash = S;
    for (int32 Frame = 0; Frame < 90; ++Frame) { Step(S, Frame < 15 ? R : Frame < 40 ? D : U); }
    const FMatchState Expected = S;
    S = Snapshot;
    Step(S, R);
    TestTrue(TEXT("Restored second tap matches every field"), S == ActiveDash);
    for (int32 Frame = 0; Frame < 90; ++Frame) { Step(S, Frame < 15 ? R : Frame < 40 ? D : U); }
    TestTrue(TEXT("Replay final state matches every field"), S == Expected);
    S = ActiveDash;
    for (int32 Frame = 0; Frame < 90; ++Frame) { Step(S, Frame < 15 ? R : Frame < 40 ? D : U); }
    TestTrue(TEXT("Restore active dash also matches"), S == Expected);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDashFacingTest, "ProjW.Simulation.DashFacingSemantics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDashFacingTest::RunTest(const FString& Parameters)
{
    FMatchState S = Initial();
    Step(S, R);
    Step(S);
    // Opponent can cross in the air; the raw tap remains valid, but facing changes.
    S.Fighter2.bGrounded = false;
    S.Fighter2.PositionY = 100 * Unit;
    S.Fighter2.PositionX = S.Fighter1.PositionX - 100 * Unit;
    Step(S);
    TestTrue(TEXT("Opponent crossing updates simulation facing"), S.Fighter1.Facing == EFighterFacing::Left);
    Step(S, R);
    TestTrue(TEXT("Same raw right taps classify using second-press facing"), S.Fighter1.CurrentState == EFighterState::BackDash);
    TestEqual(TEXT("Travel remains world right"), S.Fighter1.DashDirection, int8(1));
    S.Fighter2.PositionX = S.Fighter1.PositionX + 100 * Unit;
    Step(S);
    Step(S, L);
    TestEqual(TEXT("Later facing change cannot turn active dash"), S.Fighter1.VelocityX, Sim::BackDashSpeed);

    S = Initial();
    Step(S, R);
    Step(S, L | R);
    Step(S, R); // Releasing Left while holding Right isn't a Right press.
    TestTrue(TEXT("SOCD release is not a second tap"), S.Fighter1.CurrentState == EFighterState::Walking);
    Step(S, D);
    Step(S, R);
    TestTrue(TEXT("Crouch clears dash candidate"), S.Fighter1.CurrentState == EFighterState::Walking);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMixedMovementTest, "ProjW.Simulation.MixedMovementRepeatability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMixedMovementTest::RunTest(const FString& Parameters)
{
    FMatchState A = Initial();
    FMatchState B = A;
    bool ForwardSeen = false, BackSeen = false, CrouchSeen = false, JumpSeen = false, PushSeen = false;
    for (uint32 Frame = 0; Frame < 20000; ++Frame)
    {
        const uint32 Phase = Frame % 240;
        const uint16 P1 = Phase == 1 || Phase == 31 || Phase >= 230 ? 0
            : Phase == 0 || (Phase >= 2 && Phase < 25) ? R
            : Phase == 30 || (Phase >= 32 && Phase < 50) ? L
            : Phase >= 60 && Phase < 80 ? D
            : Phase == 90 ? U | R : Phase > 90 && Phase < 130 ? R
            : Phase >= 140 && Phase < 180 ? L : R;
        const uint16 P2 = Phase < 140 ? L : Phase == 150 ? U : R;
        Step(A, P1, P2);
        Step(B, P1, P2);
        if (!(A == B)) { AddError(FString::Printf(TEXT("Mixed replay diverged at %u"), Frame)); return false; }
        TestTrue(TEXT("Both remain within stage"), InBounds(A.Fighter1) && InBounds(A.Fighter2));
        if (A.Fighter1.bGrounded && A.Fighter2.bGrounded)
        {
            const int64 Gap = static_cast<int64>(A.Fighter1.PositionX) - A.Fighter2.PositionX;
            TestTrue(TEXT("Mixed grounded bodies never overlap"), Gap >= 2 * Sim::PushHalfWidth || Gap <= -2 * Sim::PushHalfWidth);
            PushSeen |= Gap == 2 * Sim::PushHalfWidth || Gap == -2 * Sim::PushHalfWidth;
        }
        ForwardSeen |= A.Fighter1.CurrentState == EFighterState::ForwardDash;
        BackSeen |= A.Fighter1.CurrentState == EFighterState::BackDash;
        CrouchSeen |= A.Fighter1.CurrentState == EFighterState::Crouching;
        JumpSeen |= A.Fighter1.CurrentState == EFighterState::Jumping;
    }
    TestTrue(TEXT("Mixed sequence exercised forward dash"), ForwardSeen);
    TestTrue(TEXT("Mixed sequence exercised backdash"), BackSeen);
    TestTrue(TEXT("Mixed sequence exercised crouch"), CrouchSeen);
    TestTrue(TEXT("Mixed sequence exercised jump"), JumpSeen);
    TestTrue(TEXT("Mixed sequence exercised body contact"), PushSeen);
    TestEqual(TEXT("Exactly 20000 simulation frames"), A.SimulationFrame, uint64(20000));
    return !HasAnyErrors();
}
#endif
