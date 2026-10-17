#include "Simulation/FightingSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
    using Sim = FFightingSimulation;
    constexpr int32 U = Sim::SubunitsPerWorldUnit;
    FFighterInput Press(EFighterInputButton Button) { return {static_cast<uint16>(Button)}; }
    void Step(FMatchState& State, FFighterInput P1 = {}, FFighterInput P2 = {})
    { Sim::SimulateFrame(State, P1, P2); }
    bool Inside(const FFighterState& Fighter)
    {
        if (!Sim::IsInsideArena(Fighter.PositionX, Fighter.PositionZ)) return false;
        for (const int32 SX : {-1, 1}) for (const int32 SZ : {-1, 1})
        {
            const int64 X = static_cast<int64>(Fighter.PositionX) + SX * Sim::HurtboxHalfWidth;
            const int64 Z = static_cast<int64>(Fighter.PositionZ) + SZ * Sim::HurtboxHalfExtentZ;
            if (X * X + Z * Z > Sim::ArenaRadiusSquared) return false;
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularProjectionTest, "ProjW.Simulation.Arena.CircleProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCircularProjectionTest::RunTest(const FString&)
{
    int32 X = 7, Z = 9;
    TestFalse(TEXT("center unchanged"), Sim::ConstrainToArena(0, 0, X, Z));
    TestEqual(TEXT("center X"), X, 0); TestEqual(TEXT("center Z"), Z, 0);
    TestFalse(TEXT("inside unchanged"), Sim::ConstrainToArena(100 * U, 200 * U, X, Z));
    TestEqual(TEXT("inside X"), X, 100 * U); TestEqual(TEXT("inside Z"), Z, 200 * U);
    TestFalse(TEXT("exact fighter-center boundary unchanged"), Sim::ConstrainToArena(Sim::FighterCenterRadius, 0, X, Z));
    TestEqual(TEXT("axial center boundary X"), X, Sim::FighterCenterRadius);
    const int32 DiagonalX = Sim::FighterCenterRadius * 3 / 5;
    const int32 DiagonalZ = Sim::FighterCenterRadius * 4 / 5;
    TestFalse(TEXT("exact 3-4-5 center boundary unchanged"), Sim::ConstrainToArena(DiagonalX, DiagonalZ, X, Z));
    TestEqual(TEXT("diagonal boundary X"), X, DiagonalX);
    TestEqual(TEXT("diagonal boundary Z"), Z, DiagonalZ);
    TestTrue(TEXT("raw floor edge cannot contain full fighter footprint"),
        Sim::ConstrainToArena(Sim::ArenaRadius, 0, X, Z));
    TestEqual(TEXT("floor-edge request projects to footprint-safe center"), X, Sim::FighterCenterRadius);
    for (const int32 Sign : {-1, 1})
    {
        TestTrue(TEXT("outside X projected"), Sim::ConstrainToArena(Sign * (Sim::FighterCenterRadius + U), 0, X, Z));
        TestEqual(TEXT("X mirror"), X, Sign * Sim::FighterCenterRadius);
        TestEqual(TEXT("X projection Z"), Z, 0);
        TestTrue(TEXT("outside Z projected"), Sim::ConstrainToArena(0, Sign * (Sim::FighterCenterRadius + U), X, Z));
        TestEqual(TEXT("Z mirror"), Z, Sign * Sim::FighterCenterRadius);
        TestEqual(TEXT("Z projection X"), X, 0);
    }
    Sim::ConstrainToArena(400 * U, 400 * U, X, Z);
    TestEqual(TEXT("45-degree projection symmetric"), X, Z);
    TestTrue(TEXT("45-degree projection inside"), Sim::IsInsideArena(X, Z));
    TestTrue(TEXT("45-degree projected point remains near fighter-center edge"), X > 320 * U);
    Sim::ConstrainToArena(std::numeric_limits<int32>::min(),
        std::numeric_limits<int32>::max(), X, Z);
    TestTrue(TEXT("malformed extremes project safely"), Sim::IsInsideArena(X, Z));
    TestTrue(TEXT("malformed extremes preserve quadrant"), X < 0 && Z > 0);
    const int64 FootX = static_cast<int64>(X) + (X > 0 ? 24 : -24) * U;
    const int64 FootZ = static_cast<int64>(Z) + (Z > 0 ? 5 : -5) * U;
    TestTrue(TEXT("hurtbox corner remains inside unchanged 480-unit floor"),
        FootX * FootX + FootZ * FootZ <= Sim::ArenaRadiusSquared);
    Sim::ConstrainToArena(Sim::ArenaRadius, 0, X, Z);
    const int64 EdgeCornerX = static_cast<int64>(X) + 24 * U;
    const int64 EdgeCornerZ = 5 * U;
    TestTrue(TEXT("complete hurtbox footprint fits at arena boundary"),
        EdgeCornerX * EdgeCornerX + EdgeCornerZ * EdgeCornerZ <= Sim::ArenaRadiusSquared);
    FMatchState WalkingAtEdge;
    WalkingAtEdge.Fighter1.PositionX = Sim::FighterCenterRadius - 4 * U;
    WalkingAtEdge.Fighter2.PositionX = -300 * U;
    for (int32 I = 0; I < 10; ++I)
    {
        Step(WalkingAtEdge, Press(EFighterInputButton::Right));
        TestTrue(TEXT("walking keeps complete hurtbox footprint on floor"), Inside(WalkingAtEdge.Fighter1));
    }
    TestEqual(TEXT("walking center stops at footprint-safe radial boundary"),
        WalkingAtEdge.Fighter1.PositionX, Sim::FighterCenterRadius);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularMotionTest, "ProjW.Simulation.Arena.CircleMotion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCircularMotionTest::RunTest(const FString&)
{
    for (const int32 Sign : {-1, 1})
    {
        FMatchState Pivot;
        Pivot.Fighter1.PositionX = 430 * U;
        Pivot.Fighter1.PositionZ = Sign * 180 * U;
        Pivot.Fighter2.PositionX = 300 * U;
        Pivot.Fighter2.PositionZ = 0;
        Pivot.Fighter2.Facing = EFighterFacing::Left;
        const int32 StartZ = Pivot.Fighter1.PositionZ;
        Step(Pivot, Press(Sign > 0 ? EFighterInputButton::PivotPositive : EFighterInputButton::PivotNegative));
        for (int32 I = 1; I < Sim::PivotDurationFrames; ++I)
        {
            TestTrue(TEXT("both Pivot directions stay in disc"), Inside(Pivot.Fighter1));
            Step(Pivot);
        }
        TestTrue(TEXT("Pivot produces orbital movement at edge"), Pivot.Fighter1.PositionZ != StartZ);
        TestTrue(TEXT("Pivot final position inside disc"), Inside(Pivot.Fighter1));
    }
    FMatchState Walk;
    Walk.Fighter1.PositionX = 300 * U; Walk.Fighter1.PositionZ = 300 * U;
    Walk.Fighter2.PositionX = 330 * U; Walk.Fighter2.PositionZ = 330 * U;
    Walk.Fighter2.Facing = EFighterFacing::Left;
    for (int32 I = 0; I < 30; ++I)
    {
        Step(Walk, Press(EFighterInputButton::Right));
        TestTrue(TEXT("diagonal walk remains in disc"), Inside(Walk.Fighter1) && Inside(Walk.Fighter2));
    }
    TestTrue(TEXT("diagonal walk retains depth movement"), Walk.Fighter1.PositionZ > 300 * U);

    FMatchState Dash;
    Dash.Fighter1.PositionX = 300 * U; Dash.Fighter1.PositionZ = 300 * U;
    Dash.Fighter2.PositionX = 330 * U; Dash.Fighter2.PositionZ = 330 * U;
    Dash.Fighter2.Facing = EFighterFacing::Left;
    Step(Dash, Press(EFighterInputButton::Right)); Step(Dash);
    Step(Dash, Press(EFighterInputButton::Right));
    TestEqual(TEXT("diagonal dash actually starts"), Dash.Fighter1.CurrentState, EFighterState::ForwardDash);
    TestTrue(TEXT("dash reaches boundary safely"), Inside(Dash.Fighter1));
    for (int32 I = 0; I < 20; ++I)
    {
        Step(Dash);
        TestTrue(TEXT("dash never escapes disc"), Inside(Dash.Fighter1));
    }

    FMatchState Air;
    Air.Fighter1.PositionX = 300 * U; Air.Fighter1.PositionZ = 300 * U;
    Air.Fighter2.PositionX = 330 * U; Air.Fighter2.PositionZ = 330 * U;
    Air.Fighter2.Facing = EFighterFacing::Left;
    Air.Fighter1.PositionX = 300 * U; Air.Fighter1.PositionZ = 300 * U;
    Step(Air, {static_cast<uint16>(static_cast<uint16>(EFighterInputButton::Up)
        | static_cast<uint16>(EFighterInputButton::Right))});
    bool bAirMovedInDepth = false;
    for (int32 I = 0; I < 100; ++I)
    {
        Step(Air);
        bAirMovedInDepth |= !Air.Fighter1.bGrounded && Air.Fighter1.VelocityZ != 0;
        TestTrue(TEXT("airborne horizontal motion stays in disc"), Inside(Air.Fighter1));
    }
    TestTrue(TEXT("airborne movement exercised depth"), bAirMovedInDepth);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCircularCombatTest, "ProjW.Simulation.Arena.CircleCombatReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCircularCombatTest::RunTest(const FString&)
{
    for (const bool bBlock : {false, true})
    {
        FMatchState State;
        State.Fighter1.PositionX = 410 * U;
        State.Fighter2.PositionX = 460 * U;
        State.Fighter1.CurrentState = EFighterState::Attack;
        State.Fighter1.CurrentMoveId = EMoveId::Light;
        State.Fighter1.MovePhase = EMovePhase::Active;
        State.Fighter2.Facing = EFighterFacing::Left;
        Step(State, {}, bBlock ? Press(EFighterInputButton::Right) : FFighterInput{});
        TestEqual(TEXT("edge contact captured"), State.Fighter2.PendingReaction,
            bBlock ? EPendingReaction::Block : EPendingReaction::Hit);
        const FMatchState Snapshot = State;
        bool bAtEdge = false;
        for (int32 I = 0; I < 60; ++I)
        {
            Step(State);
            TestTrue(TEXT("reaction stays in disc"), Inside(State.Fighter2));
            bAtEdge |= State.Fighter2.PositionX == Sim::FighterCenterRadius;
        }
        TestTrue(TEXT("reaction reaches circular edge"), bAtEdge);
        TestEqual(TEXT("outward reaction momentum stops"), State.Fighter2.PendingKnockbackVelocityX, 0);
        TestEqual(TEXT("reaction recovers"), State.Fighter2.CurrentState, EFighterState::Idle);
        const FMatchState Expected = State;
        State = Snapshot;
        for (int32 I = 0; I < 60; ++I) Step(State);
        TestTrue(TEXT("boundary reaction snapshot replay exact"), State == Expected);
    }
    FMatchState Push;
    Push.Fighter1.PositionX = Sim::FighterCenterRadius - 10 * U;
    Push.Fighter2.PositionX = Sim::FighterCenterRadius - 2 * U;
    Push.Fighter2.Facing = EFighterFacing::Left;
    Step(Push);
    TestTrue(TEXT("body push first legal"), Inside(Push.Fighter1));
    TestTrue(TEXT("body push second legal"), Inside(Push.Fighter2));
    TestEqual(TEXT("outer body stays at footprint-safe edge"), Push.Fighter2.PositionX, Sim::FighterCenterRadius);
    TestTrue(TEXT("blocked body separation transferred inward"),
        Push.Fighter2.PositionX - Push.Fighter1.PositionX >= 2 * Sim::PushHalfWidth);
    FMatchState DiagonalPush;
    DiagonalPush.Fighter1.PositionX = 310 * U;
    DiagonalPush.Fighter1.PositionZ = 310 * U;
    DiagonalPush.Fighter2.PositionX = 316 * U;
    DiagonalPush.Fighter2.PositionZ = 316 * U;
    DiagonalPush.Fighter2.Facing = EFighterFacing::Left;
    Step(DiagonalPush);
    TestTrue(TEXT("diagonal body push first legal"), Inside(DiagonalPush.Fighter1));
    TestTrue(TEXT("diagonal body push second legal"), Inside(DiagonalPush.Fighter2));
    TestTrue(TEXT("diagonal body push separates both coordinates"),
        DiagonalPush.Fighter1.PositionX < 310 * U && DiagonalPush.Fighter1.PositionZ < 310 * U
        && DiagonalPush.Fighter2.PositionX > 316 * U && DiagonalPush.Fighter2.PositionZ > 316 * U);
    return !HasAnyErrors();
}
#endif
