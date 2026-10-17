#include "Simulation/FightingSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
    using Sim = FFightingSimulation;
    constexpr int32 Unit = Sim::SubunitsPerWorldUnit;

    FFighterInput Button(EFighterInputButton Value) { return {static_cast<uint16>(Value)}; }
    FFighterInput Combined(EFighterInputButton A, EFighterInputButton B)
    { return {static_cast<uint16>(static_cast<uint16>(A) | static_cast<uint16>(B))}; }
    void Step(FMatchState& State, FFighterInput P1 = {}, FFighterInput P2 = {})
    { Sim::SimulateFrame(State, P1, P2); }
    FMatchState Spaced()
    {
        FMatchState State;
        State.Fighter1.PositionX = 0;
        State.Fighter2.PositionX = 300 * Unit;
        State.Fighter2.Facing = EFighterFacing::Left;
        return State;
    }
    FMatchState Attack(EMoveId Id)
    {
        FMatchState State = Spaced();
        State.Fighter2.PositionX = Sim::FindMoveDefinition(Id)->HitboxOffsetX;
        State.Fighter1.CurrentState = EFighterState::Attack;
        State.Fighter1.CurrentMoveId = Id;
        State.Fighter1.MovePhase = EMovePhase::Active;
        return State;
    }
    FMatchState PivotBeforeActive(EMoveId Id, int32 Direction)
    {
        FMatchState State = Spaced();
        State.Fighter2.PositionX = 85 * Unit;
        const EFighterInputButton AttackButton = Id == EMoveId::Light ? EFighterInputButton::Light
            : Id == EMoveId::Medium ? EFighterInputButton::Medium : EFighterInputButton::Heavy;
        Step(State, Button(AttackButton), Button(Direction > 0
            ? EFighterInputButton::PivotPositive : EFighterInputButton::PivotNegative));
        for (int32 Frame = 1; Frame <= Sim::FindMoveDefinition(Id)->StartupFrames; ++Frame) Step(State);
        return State;
    }
    void Replay(FAutomationTestBase& Test, const TCHAR* Label, const FMatchState& Snapshot)
    {
        FMatchState A = Snapshot;
        FMatchState B = Snapshot;
        for (int32 Frame = 0; Frame < 100; ++Frame)
        {
            const FFighterInput P1 = Frame == 50 ? Button(EFighterInputButton::Light) : FFighterInput{};
            const FFighterInput P2 = Frame == 55 ? Button(EFighterInputButton::PivotNegative) : FFighterInput{};
            Step(A, P1, P2);
            Step(B, P1, P2);
        }
        Test.TestTrue(Label, A == B);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotBoxTest, "ProjW.Simulation.Pivot.Strict3DOverlap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotBoxTest::RunTest(const FString&)
{
    const FSimBox A{0, 10, 0, 10, 0, 10};
    TestTrue(TEXT("one-subunit overlap on all axes"), A.Overlaps({9, 20, 9, 20, 9, 20}));
    TestFalse(TEXT("X separation"), A.Overlaps({11, 20, 0, 10, 0, 10}));
    TestFalse(TEXT("Y separation"), A.Overlaps({0, 10, 11, 20, 0, 10}));
    TestFalse(TEXT("Z separation"), A.Overlaps({0, 10, 0, 10, 11, 20}));
    TestFalse(TEXT("X face touch"), A.Overlaps({10, 20, 0, 10, 0, 10}));
    TestFalse(TEXT("Y face touch"), A.Overlaps({0, 10, 10, 20, 0, 10}));
    TestFalse(TEXT("Z face touch"), A.Overlaps({0, 10, 0, 10, 10, 20}));
    TestFalse(TEXT("reverse Z face touch"), FSimBox{0, 10, 0, 10, 10, 20}.Overlaps(A));
    const FMatchState State = Spaced();
    const FSimBox Hurt = Sim::GetHurtbox(State.Fighter1);
    TestEqual(TEXT("hurt Z minimum"), Hurt.MinZ, -5 * Unit);
    TestEqual(TEXT("hurt Z maximum"), Hurt.MaxZ, 5 * Unit);
    for (const EMoveId Id : {EMoveId::Light, EMoveId::Medium, EMoveId::Heavy})
    {
        FMatchState Active = Attack(Id);
        FSimBox Hitbox;
        TestTrue(TEXT("active hitbox exists"), Sim::GetMoveHitbox(Active.Fighter1, Hitbox));
        const int32 ExpectedZ = Id == EMoveId::Light ? 4 : Id == EMoveId::Medium ? 30 : 6;
        TestEqual(TEXT("prototype move Z minimum"), Hitbox.MinZ, -ExpectedZ * Unit);
        TestEqual(TEXT("prototype move Z maximum"), Hitbox.MaxZ, ExpectedZ * Unit);
        Step(Active);
        TestEqual(TEXT("legacy Z-zero attack still hits"), Active.Fighter2.Health,
            FFighterState::MaxHealth - Sim::FindMoveDefinition(Id)->Damage);
    }
    FMatchState Guarded = Attack(EMoveId::Light);
    Step(Guarded, {}, Button(EFighterInputButton::Right));
    TestEqual(TEXT("legacy Z-zero block still works"), Guarded.Fighter2.PendingReaction, EPendingReaction::Block);
    TestEqual(TEXT("legacy chip unchanged"), Guarded.Fighter2.Health, FFighterState::MaxHealth - 5);
    FSimOrientedBox Diagonal{0, 0, 0, 10 * Unit, 11585, 11585, 10 * Unit, 2 * Unit};
    const FSimBox Corner{7 * Unit, 9 * Unit, 0, 10 * Unit, -9 * Unit, -7 * Unit};
    TestTrue(TEXT("enclosing AABB overlaps corner"), Diagonal.Bounds().Overlaps(Corner));
    TestFalse(TEXT("oriented volume rejects AABB-only corner"), Diagonal.Overlaps(Corner));
    FSimOrientedBox Straight{0, 0, 0, 10 * Unit, Sim::AxisScale, 0, 10 * Unit, 2 * Unit};
    TestFalse(TEXT("oriented strict edge touch"), Straight.Overlaps({10 * Unit, 11 * Unit, 0, 10 * Unit, 0, Unit}));
    TestTrue(TEXT("oriented one-subunit overlap"), Straight.Overlaps({10 * Unit - 1, 11 * Unit, 0, 10 * Unit, 0, Unit}));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotMotionTest, "ProjW.Simulation.Pivot.MotionLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotMotionTest::RunTest(const FString&)
{
    for (const int32 Sign : {-1, 1})
    {
        FMatchState State = Spaced();
        const int64 InitialRadiusSquared = static_cast<int64>(State.Fighter2.PositionX - State.Fighter1.PositionX)
            * (State.Fighter2.PositionX - State.Fighter1.PositionX);
        const FFighterInput Start = Button(Sign > 0 ? EFighterInputButton::PivotPositive : EFighterInputButton::PivotNegative);
        for (uint32 Frame = 0; Frame < Sim::PivotDurationFrames; ++Frame)
        {
            const FFighterInput Input = Frame == 0 ? Start
                : Combined(EFighterInputButton::Light, Sign > 0
                    ? EFighterInputButton::PivotNegative : EFighterInputButton::PivotPositive);
            Step(State, Input);
            TestEqual(TEXT("Pivot state"), State.Fighter1.CurrentState, EFighterState::Pivoting);
            TestEqual(TEXT("Pivot state frame"), State.Fighter1.StateFrame, Frame);
            TestEqual(TEXT("locked Pivot direction"), State.Fighter1.PivotDirection, static_cast<int8>(Sign));
            TestEqual(TEXT("Pivot holds Y"), State.Fighter1.PositionY, 0);
            TestEqual(TEXT("Pivot keeps facing"), State.Fighter1.Facing, EFighterFacing::Right);
            TestEqual(TEXT("Pivot does not start attack"), State.Fighter1.CurrentMoveId, EMoveId::None);
        }
        TestTrue(TEXT("both orbital directions move opposite in Z"), Sign * State.Fighter1.PositionZ < 0);
        TestTrue(TEXT("orbit changes X as well as Z"), State.Fighter1.PositionX > 0);
        const int64 DX = static_cast<int64>(State.Fighter2.PositionX) - State.Fighter1.PositionX;
        const int64 DZ = static_cast<int64>(State.Fighter2.PositionZ) - State.Fighter1.PositionZ;
        TestTrue(TEXT("approximately preserves radius"), FMath::Abs(DX * DX + DZ * DZ - InitialRadiusSquared)
            < 3LL * 300 * Unit * Unit);
        const int32 FinalX = State.Fighter1.PositionX, FinalZ = State.Fighter1.PositionZ;
        Replay(*this, TEXT("Pivot recovery snapshot replays"), State);
        Step(State);
        TestEqual(TEXT("Pivot exits after 12 committed frames"), State.Fighter1.CurrentState, EFighterState::Idle);
        TestEqual(TEXT("Pivot direction clears"), State.Fighter1.PivotDirection, int8(0));
        TestEqual(TEXT("neutral does not recenter X"), State.Fighter1.PositionX, FinalX);
        TestEqual(TEXT("neutral does not recenter Z"), State.Fighter1.PositionZ, FinalZ);
    }
    for (const int32 Sign : {-1, 1})
    {
        FMatchState Diagonal = Spaced();
        Diagonal.Fighter2.PositionX = 210 * Unit;
        Diagonal.Fighter2.PositionZ = 70 * Unit;
        const int32 BeforeX = Diagonal.Fighter1.PositionX, BeforeZ = Diagonal.Fighter1.PositionZ;
        Step(Diagonal, Button(Sign > 0 ? EFighterInputButton::PivotPositive : EFighterInputButton::PivotNegative));
        TestTrue(TEXT("diagonal orbit changes both coordinates"), Diagonal.Fighter1.PositionX != BeforeX
            && Diagonal.Fighter1.PositionZ != BeforeZ);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotAuthoritativeMovementTest, "ProjW.Simulation.Pivot.AuthoritativeMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotAuthoritativeMovementTest::RunTest(const FString&)
{
    FMatchState State;
    State.Fighter1.PositionX = -160 * Unit;
    State.Fighter2.PositionX = 160 * Unit;
    State.Fighter2.Facing = EFighterFacing::Left;
    const int32 P1StartX = State.Fighter1.PositionX;
    const int32 P1StartZ = State.Fighter1.PositionZ;
    const int32 P2StartX = State.Fighter2.PositionX;
    const int32 P2StartZ = State.Fighter2.PositionZ;

    Step(State, Button(EFighterInputButton::PivotPositive));
    TestTrue(TEXT("first Pivot frame moves authoritative X/Z"),
        State.Fighter1.PositionX != P1StartX || State.Fighter1.PositionZ != P1StartZ);
    TestEqual(TEXT("first-frame X result"), State.Fighter1.PositionX, -163825);
    TestEqual(TEXT("first-frame Z result"), State.Fighter1.PositionZ, -3155);
    TestEqual(TEXT("opponent X remains stationary"), State.Fighter2.PositionX, P2StartX);
    TestEqual(TEXT("opponent Z remains stationary"), State.Fighter2.PositionZ, P2StartZ);
    const FMatchState Snapshot = State;

    for (int32 Frame = 1; Frame < 9; ++Frame) Step(State);
    TestEqual(TEXT("end-of-nine-movement-frames X"), State.Fighter1.PositionX, -160650);
    TestEqual(TEXT("end-of-nine-movement-frames Z"), State.Fighter1.PositionZ, -45600);
    for (int32 Frame = 9; Frame < Sim::PivotDurationFrames; ++Frame) Step(State);
    TestEqual(TEXT("end-of-12-frame Pivot X"), State.Fighter1.PositionX, -160650);
    TestEqual(TEXT("end-of-12-frame Pivot Z"), State.Fighter1.PositionZ, -45600);
    TestEqual(TEXT("opponent stays fixed for complete Pivot X"), State.Fighter2.PositionX, P2StartX);
    TestEqual(TEXT("opponent stays fixed for complete Pivot Z"), State.Fighter2.PositionZ, P2StartZ);
    TestTrue(TEXT("final Pivot position remains in disc"), Sim::IsInsideArena(
        State.Fighter1.PositionX, State.Fighter1.PositionZ));

    FMatchState Replayed = Snapshot;
    for (int32 Frame = 1; Frame < Sim::PivotDurationFrames; ++Frame) Step(Replayed);
    TestTrue(TEXT("Pivot snapshot replay reproduces exact final state"), Replayed == State);
    const double DisplacementX = static_cast<double>(State.Fighter1.PositionX - P1StartX) / Unit;
    const double DisplacementZ = static_cast<double>(State.Fighter1.PositionZ - P1StartZ) / Unit;
    const double Displacement = FMath::Sqrt(DisplacementX * DisplacementX + DisplacementZ * DisplacementZ);
    TestTrue(TEXT("standard Pivot has meaningful physical displacement"), Displacement > 44.5 && Displacement < 44.8);
    AddInfo(FString::Printf(TEXT("Standard Pivot P1: (-160, 0) -> first (-159.9853515625, -3.0810546875), final (-156.884765625, -44.53125), displacement %.6f world units; P2 remains (160, 0)."), Displacement));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotEligibilityTest, "ProjW.Simulation.Pivot.EligibilityBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotEligibilityTest::RunTest(const FString&)
{
    for (const EFighterState Actionable : {EFighterState::Idle, EFighterState::Walking, EFighterState::Crouching})
    {
        FMatchState State = Spaced();
        State.Fighter1.CurrentState = Actionable;
        Step(State, Button(EFighterInputButton::PivotPositive));
        TestEqual(TEXT("ordinary grounded state can Pivot"), State.Fighter1.CurrentState, EFighterState::Pivoting);
    }
    for (const EFighterState Committed : {EFighterState::Attack, EFighterState::ForwardDash,
        EFighterState::BackDash, EFighterState::JumpStartup, EFighterState::Hitstun, EFighterState::Blockstun})
    {
        FMatchState State = Spaced();
        State.Fighter1.CurrentState = Committed;
        if (Committed == EFighterState::Attack)
        {
            State.Fighter1.CurrentMoveId = EMoveId::Light;
            State.Fighter1.MovePhase = EMovePhase::Startup;
        }
        if (Committed == EFighterState::ForwardDash || Committed == EFighterState::BackDash)
            State.Fighter1.DashDirection = 1;
        if (Committed == EFighterState::Hitstun || Committed == EFighterState::Blockstun)
        {
            State.Fighter1.PendingReaction = Committed == EFighterState::Hitstun
                ? EPendingReaction::Hit : EPendingReaction::Block;
            State.Fighter1.PendingReactionFrames = 5;
        }
        Step(State, Button(EFighterInputButton::PivotPositive));
        TestTrue(TEXT("committed state rejects Pivot"), State.Fighter1.CurrentState != EFighterState::Pivoting);
        TestEqual(TEXT("rejected Pivot leaves Z"), State.Fighter1.PositionZ, 0);
    }
    FMatchState Air = Spaced();
    Air.Fighter1.bGrounded = false;
    Air.Fighter1.CurrentState = EFighterState::Jumping;
    Air.Fighter1.PositionY = 100 * Unit;
    Step(Air, Button(EFighterInputButton::PivotPositive));
    TestTrue(TEXT("airborne Pivot rejected"), Air.Fighter1.CurrentState != EFighterState::Pivoting);
    FMatchState Frozen = Spaced();
    Frozen.Fighter1.HitstopFramesRemaining = 2;
    Step(Frozen, Button(EFighterInputButton::PivotPositive));
    TestEqual(TEXT("hitstop Pivot rejected"), Frozen.Fighter1.PositionZ, 0);
    Step(Frozen);
    Step(Frozen);
    TestTrue(TEXT("rejected Pivot not buffered"), Frozen.Fighter1.CurrentState != EFighterState::Pivoting);
    for (const int32 Sign : {-1, 1})
    {
        FMatchState Bound = Spaced();
        Bound.Fighter1.PositionX = 430 * Unit;
        Bound.Fighter1.PositionZ = Sign * 180 * Unit;
        Step(Bound, Button(Sign > 0 ? EFighterInputButton::PivotNegative : EFighterInputButton::PivotPositive));
        TestTrue(TEXT("Pivot inside circular arena"), Sim::IsInsideArena(
            Bound.Fighter1.PositionX, Bound.Fighter1.PositionZ));
        for (int32 Frame = 0; Frame < 20; ++Frame)
        {
            Step(Bound);
            TestTrue(TEXT("Pivot remains inside circular arena"), Sim::IsInsideArena(
                Bound.Fighter1.PositionX, Bound.Fighter1.PositionZ));
        }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotContactTest, "ProjW.Simulation.Pivot.GeometryContact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotContactTest::RunTest(const FString&)
{
    for (const int32 Sign : {-1, 1})
    {
        FMatchState Narrow = PivotBeforeActive(EMoveId::Light, Sign);
        TestTrue(TEXT("Pivot moved outside narrow lateral reach"), FMath::Abs(Narrow.Fighter2.PositionZ) > 9 * Unit);
        TestEqual(TEXT("narrow move misses"), Narrow.HitEventCount, uint32(0));
        TestEqual(TEXT("narrow miss causes no damage or chip"), Narrow.Fighter2.Health, FFighterState::MaxHealth);
        TestFalse(TEXT("narrow miss does not consume contact"), Narrow.Fighter1.bCurrentMoveConnected);
        for (int32 Frame = 0; Frame < 3; ++Frame) Step(Narrow);
        TestEqual(TEXT("narrow whiff enters ordinary recovery"), Narrow.Fighter1.MovePhase, EMovePhase::Recovery);
        Replay(*this, TEXT("depth whiff snapshot replays"), Narrow);
        for (int32 Frame = 0; Frame < 4; ++Frame) Step(Narrow);
        TestEqual(TEXT("Pivot completes while attacker remains committed"), Narrow.Fighter2.CurrentState, EFighterState::Idle);
        TestEqual(TEXT("attacker still in ordinary recovery"), Narrow.Fighter1.MovePhase, EMovePhase::Recovery);

        FMatchState Broad = PivotBeforeActive(EMoveId::Medium, Sign);
        TestEqual(TEXT("broad move catches Pivot"), Broad.Fighter2.CurrentState, EFighterState::Pivoting);
        TestEqual(TEXT("broad move deals full damage"), Broad.Fighter2.Health, FFighterState::MaxHealth - 80);
        TestEqual(TEXT("broad contact is Hit"), Broad.Fighter2.PendingReaction, EPendingReaction::Hit);
        TestTrue(TEXT("broad contact consumes attack"), Broad.Fighter1.bCurrentMoveConnected);
        Replay(*this, TEXT("broad-hit Pivot snapshot replays"), Broad);
    }
    FMatchState Block = Attack(EMoveId::Light);
    Step(Block, {}, Button(EFighterInputButton::Right));
    TestEqual(TEXT("Block at Z overlap"), Block.Fighter2.PendingReaction, EPendingReaction::Block);
    FMatchState Wrong = Attack(EMoveId::Medium);
    Step(Wrong, {}, Button(EFighterInputButton::Right));
    TestEqual(TEXT("wrong-height guard is Hit"), Wrong.Fighter2.PendingReaction, EPendingReaction::Hit);
    FMatchState FailedPivot = Attack(EMoveId::Light);
    Step(FailedPivot, {}, Combined(EFighterInputButton::PivotPositive, EFighterInputButton::Right));
    TestEqual(TEXT("Pivot has no invulnerability or auto-Block"), FailedPivot.Fighter2.PendingReaction, EPendingReaction::Hit);
    TestEqual(TEXT("failed Pivot takes full damage"), FailedPivot.Fighter2.Health, FFighterState::MaxHealth - 50);
    FMatchState Recovery = Attack(EMoveId::Light);
    Recovery.Fighter2.CurrentState = EFighterState::Pivoting;
    Recovery.Fighter2.PivotDirection = 1;
    Recovery.Fighter2.StateFrame = 9;
    Recovery.Fighter2.PositionZ = 0;
    Step(Recovery, {}, Button(EFighterInputButton::Right));
    TestEqual(TEXT("Pivot recovery can be hit"), Recovery.Fighter2.PendingReaction, EPendingReaction::Hit);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotBodyPushTest, "ProjW.Simulation.Pivot.BodyPushDepth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotBodyPushTest::RunTest(const FString&)
{
    FMatchState Same = Spaced();
    Same.Fighter1.PositionX = -5 * Unit;
    Same.Fighter2.PositionX = 5 * Unit;
    Step(Same);
    TestEqual(TEXT("same-depth left body push"), Same.Fighter1.PositionX, -20 * Unit);
    TestEqual(TEXT("same-depth right body push"), Same.Fighter2.PositionX, 20 * Unit);
    FMatchState Apart = Spaced();
    Apart.Fighter1.PositionX = -5 * Unit;
    Apart.Fighter2.PositionX = 5 * Unit;
    Apart.Fighter2.PositionZ = 40 * Unit;
    Step(Apart);
    TestEqual(TEXT("separated depth does not push first X"), Apart.Fighter1.PositionX, -5 * Unit);
    TestEqual(TEXT("separated depth does not push second X"), Apart.Fighter2.PositionX, 5 * Unit);
    FMatchState Reversed = Spaced();
    Reversed.Fighter1.PositionX = 5 * Unit;
    Reversed.Fighter2.PositionX = -5 * Unit;
    Reversed.Fighter1.PositionZ = 40 * Unit;
    Step(Reversed);
    TestEqual(TEXT("reversed assignment does not push first X"), Reversed.Fighter1.PositionX, 5 * Unit);
    TestEqual(TEXT("reversed assignment does not push second X"), Reversed.Fighter2.PositionX, -5 * Unit);
    FMatchState Touch = Apart;
    Touch.Fighter2.PositionZ = 40 * Unit;
    Step(Touch);
    TestEqual(TEXT("radially separated bodies do not push"), Touch.Fighter1.PositionX, -5 * Unit);
    FMatchState Diagonal = Spaced();
    Diagonal.Fighter1.PositionX = -5 * Unit;
    Diagonal.Fighter1.PositionZ = -4 * Unit;
    Diagonal.Fighter2.PositionX = 5 * Unit;
    Diagonal.Fighter2.PositionZ = 4 * Unit;
    Step(Diagonal);
    TestTrue(TEXT("diagonal push separates X"), Diagonal.Fighter1.PositionX < -5 * Unit
        && Diagonal.Fighter2.PositionX > 5 * Unit);
    TestTrue(TEXT("diagonal push separates Z"), Diagonal.Fighter1.PositionZ < -4 * Unit
        && Diagonal.Fighter2.PositionZ > 4 * Unit);
    FMatchState DepthTouch = Spaced();
    DepthTouch.Fighter1.PositionX = -5 * Unit;
    DepthTouch.Fighter2.PositionX = 5 * Unit;
    DepthTouch.Fighter1.PositionZ = -5 * Unit;
    DepthTouch.Fighter2.PositionZ = 5 * Unit;
    Step(DepthTouch);
    TestEqual(TEXT("depth face touch does not body-push X"), DepthTouch.Fighter1.PositionX, -5 * Unit);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotLocalAxisTest, "ProjW.Simulation.Pivot.LocalAxisCombat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotLocalAxisTest::RunTest(const FString&)
{
    FMatchState Walk = Spaced();
    Walk.Fighter2.PositionX = 100 * Unit;
    Walk.Fighter2.PositionZ = 50 * Unit;
    Step(Walk, Button(EFighterInputButton::Right));
    TestTrue(TEXT("local walk advances X and Z"), Walk.Fighter1.PositionX > 0 && Walk.Fighter1.PositionZ > 0);
    FMatchState Dash = Spaced();
    Dash.Fighter2.PositionX = 100 * Unit;
    Dash.Fighter2.PositionZ = 50 * Unit;
    Step(Dash, Button(EFighterInputButton::Right)); Step(Dash);
    Step(Dash, Button(EFighterInputButton::Right));
    TestEqual(TEXT("diagonal double tap starts dash"), Dash.Fighter1.CurrentState, EFighterState::ForwardDash);
    TestTrue(TEXT("diagonal dash advances depth"), Dash.Fighter1.VelocityZ > 0);
    FMatchState Hit = Spaced();
    Hit.Fighter2.PositionX = 52 * Unit;
    Hit.Fighter2.PositionZ = 52 * Unit;
    Step(Hit, Button(EFighterInputButton::Light));
    TestTrue(TEXT("attack locks diagonal axis"), Hit.Fighter1.AttackAxisX > 0 && Hit.Fighter1.AttackAxisZ > 0);
    for (int32 Frame = 0; Frame < Sim::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++Frame) Step(Hit);
    TestEqual(TEXT("diagonal attack makes contact"), Hit.Fighter2.PendingReaction, EPendingReaction::Hit);
    TestTrue(TEXT("diagonal reaction has X and Z knockback"), Hit.Fighter2.PendingKnockbackVelocityX > 0
        && Hit.Fighter2.PendingKnockbackVelocityZ > 0);
    Replay(*this, TEXT("diagonal hit snapshot replay"), Hit);
    FMatchState Block = Spaced();
    Block.Fighter2.PositionX = 40 * Unit;
    Block.Fighter2.PositionZ = 40 * Unit;
    for (int32 Frame = 0; Frame <= Sim::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++Frame)
        Step(Block, Frame == 0 ? Button(EFighterInputButton::Light) : FFighterInput{}, Button(EFighterInputButton::Right));
    TestEqual(TEXT("diagonal guard blocks"), Block.Fighter2.PendingReaction, EPendingReaction::Block);
    TestTrue(TEXT("diagonal block push has X and Z"), Block.Fighter2.PendingKnockbackVelocityX > 0
        && Block.Fighter2.PendingKnockbackVelocityZ > 0);
    FMatchState AfterPivot = Spaced();
    AfterPivot.Fighter2.PositionX = 85 * Unit;
    Step(AfterPivot, Button(EFighterInputButton::PivotPositive));
    for (int32 Frame = 1; Frame <= Sim::PivotDurationFrames; ++Frame) Step(AfterPivot);
    const int32 PersistentZ = AfterPivot.Fighter1.PositionZ;
    TestTrue(TEXT("Pivot establishes persistent diagonal"), PersistentZ != 0);
    Step(AfterPivot, Button(EFighterInputButton::Light));
    TestTrue(TEXT("new attack follows established diagonal"), AfterPivot.Fighter1.AttackAxisZ != 0);
    for (int32 Frame = 0; Frame < Sim::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++Frame)
        Step(AfterPivot);
    TestEqual(TEXT("post-Pivot local attack hits"), AfterPivot.Fighter2.PendingReaction, EPendingReaction::Hit);
    TestTrue(TEXT("post-Pivot hit reacts in depth"), AfterPivot.Fighter2.PendingKnockbackVelocityZ > 0);
    TestEqual(TEXT("attacker is not recentered"), AfterPivot.Fighter1.PositionZ, PersistentZ);
    Replay(*this, TEXT("post-Pivot diagonal attack snapshot"), AfterPivot);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotRecenterTest, "ProjW.Simulation.Pivot.PersistentDepth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotRecenterTest::RunTest(const FString&)
{
    FMatchState Neutral = Spaced();
    Neutral.Fighter1.PositionZ = 5 * Unit;
    Neutral.Fighter2.PositionZ = -5 * Unit;
    for (int32 Frame = 0; Frame < 100; ++Frame)
    {
        Step(Neutral);
        TestEqual(TEXT("positive depth persists"), Neutral.Fighter1.PositionZ, 5 * Unit);
        TestEqual(TEXT("negative depth persists"), Neutral.Fighter2.PositionZ, -5 * Unit);
    }
    Replay(*this, TEXT("persistent depth snapshot replays"), Neutral);
    for (const EFighterState Committed : {EFighterState::Attack, EFighterState::ForwardDash,
        EFighterState::Pivoting, EFighterState::Hitstun, EFighterState::Blockstun})
    {
        FMatchState State = Spaced();
        State.Fighter1.PositionZ = 20 * Unit;
        State.Fighter2.PositionZ = -20 * Unit;
        State.Fighter1.CurrentState = Committed;
        if (Committed == EFighterState::Attack)
        {
            State.Fighter1.CurrentMoveId = EMoveId::Heavy;
            State.Fighter1.MovePhase = EMovePhase::Startup;
        }
        if (Committed == EFighterState::ForwardDash) State.Fighter1.DashDirection = 1;
        if (Committed == EFighterState::Pivoting) State.Fighter1.PivotDirection = 1;
        if (Committed == EFighterState::Hitstun || Committed == EFighterState::Blockstun)
        {
            State.Fighter1.PendingReaction = Committed == EFighterState::Hitstun
                ? EPendingReaction::Hit : EPendingReaction::Block;
            State.Fighter1.PendingReactionFrames = 5;
        }
        Step(State);
        TestEqual(TEXT("other fighter never recenters"), State.Fighter2.PositionZ, -20 * Unit);
        if (Committed != EFighterState::Pivoting && Committed != EFighterState::ForwardDash)
            TestEqual(TEXT("committed fighter does not recenter"), State.Fighter1.PositionZ, 20 * Unit);
    }
    FMatchState Frozen = Spaced();
    Frozen.Fighter1.PositionZ = 20 * Unit;
    Frozen.Fighter2.PositionZ = -20 * Unit;
    Frozen.Fighter1.HitstopFramesRemaining = 1;
    Frozen.Fighter2.HitstopFramesRemaining = 1;
    Step(Frozen);
    TestEqual(TEXT("final frozen frame preserves depth 1"), Frozen.Fighter1.PositionZ, 20 * Unit);
    TestEqual(TEXT("final frozen frame preserves depth 2"), Frozen.Fighter2.PositionZ, -20 * Unit);
    Step(Frozen);
    TestEqual(TEXT("depth persists afterward"), Frozen.Fighter1.PositionZ, 20 * Unit);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotSnapshotTest, "ProjW.Simulation.Pivot.SnapshotPhases",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotSnapshotTest::RunTest(const FString&)
{
    FMatchState Motion = Spaced();
    Step(Motion, Button(EFighterInputButton::PivotPositive));
    Replay(*this, TEXT("Pivot movement snapshot"), Motion);
    for (int32 Frame = 1; Frame < 10; ++Frame) Step(Motion);
    TestEqual(TEXT("Pivot recovery frame"), Motion.Fighter1.StateFrame, uint32(9));
    Replay(*this, TEXT("Pivot recovery snapshot"), Motion);
    FMatchState Whiff = PivotBeforeActive(EMoveId::Light, 1);
    Replay(*this, TEXT("narrow depth whiff snapshot"), Whiff);
    FMatchState Hit = PivotBeforeActive(EMoveId::Medium, -1);
    Replay(*this, TEXT("broad Pivot hit snapshot"), Hit);
    FMatchState Neutral = Spaced();
    Neutral.Fighter1.PositionZ = 21 * Unit;
    Step(Neutral);
    TestEqual(TEXT("nonzero neutral depth snapshot"), Neutral.Fighter1.PositionZ, 21 * Unit);
    Replay(*this, TEXT("persistent-depth replay snapshot"), Neutral);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPivotStressTest, "ProjW.Simulation.Pivot.Mixed30K",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPivotStressTest::RunTest(const FString&)
{
    FMatchState A = Spaced();
    A.Fighter2.PositionX = 50 * Unit;
    FMatchState B = A;
    int32 Positive = 0, Negative = 0, Walk = 0, Dash = 0, Jump = 0, AttackCount = 0, BlockCount = 0;
    for (int32 Frame = 0; Frame < 30000; ++Frame)
    {
        const int32 Phase = Frame % 320;
        FFighterInput P1;
        if (Phase == 0) P1 = Button(EFighterInputButton::Light);
        else if (Phase == 30) P1 = Button(EFighterInputButton::PivotPositive);
        else if (Phase == 60) P1 = Button(EFighterInputButton::PivotNegative);
        else if ((Phase >= 90 && Phase <= 96) || Phase == 110 || Phase == 112)
            P1 = Button(EFighterInputButton::Right);
        else if (Phase == 140) P1 = Button(EFighterInputButton::Up);
        else if (Phase == 210) P1 = Button(EFighterInputButton::Medium);
        else if (Phase == 250) P1 = Button(EFighterInputButton::Heavy);
        const FFighterInput P2 = Phase <= 15 ? Button(EFighterInputButton::Right) : FFighterInput{};
        Step(A, P1, P2);
        Step(B, P1, P2);
        if (A != B) { AddError(TEXT("mixed 30K diverged")); break; }
        Positive += A.Fighter1.CurrentState == EFighterState::Pivoting && A.Fighter1.PivotDirection > 0;
        Negative += A.Fighter1.CurrentState == EFighterState::Pivoting && A.Fighter1.PivotDirection < 0;
        Walk += A.Fighter1.CurrentState == EFighterState::Walking;
        Dash += A.Fighter1.CurrentState == EFighterState::ForwardDash || A.Fighter1.CurrentState == EFighterState::BackDash;
        Jump += A.Fighter1.CurrentState == EFighterState::Jumping;
        AttackCount += A.Fighter1.CurrentState == EFighterState::Attack;
        BlockCount += A.Fighter2.PendingReaction == EPendingReaction::Block;
        TestTrue(TEXT("circular bound 1"), Sim::IsInsideArena(A.Fighter1.PositionX, A.Fighter1.PositionZ));
        TestTrue(TEXT("circular bound 2"), Sim::IsInsideArena(A.Fighter2.PositionX, A.Fighter2.PositionZ));
    }
    TestEqual(TEXT("exact stress length"), A.SimulationFrame, uint64(30000));
    TestTrue(TEXT("positive Pivot exercised"), Positive > 0);
    TestTrue(TEXT("negative Pivot exercised"), Negative > 0);
    TestTrue(TEXT("walk exercised"), Walk > 0);
    TestTrue(TEXT("dash exercised"), Dash > 0);
    TestTrue(TEXT("jump exercised"), Jump > 0);
    TestTrue(TEXT("normal attacks exercised"), AttackCount > 0);
    TestTrue(TEXT("Block exercised"), BlockCount > 0);
    TestTrue(TEXT("stress final states exactly equal"), A == B);
    return !HasAnyErrors();
}
#endif
