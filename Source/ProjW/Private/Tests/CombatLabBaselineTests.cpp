#include "Simulation/FightingSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
    using Sim = FFightingSimulation;
    constexpr int32 Unit = Sim::SubunitsPerWorldUnit;

    FMatchState ActiveAttack(EMoveId MoveId, int32 DefenderX)
    {
        FMatchState State;
        State.Fighter1.PositionX = 0;
        State.Fighter1.CurrentState = EFighterState::Attack;
        State.Fighter1.CurrentMoveId = MoveId;
        State.Fighter1.MovePhase = EMovePhase::Active;
        State.Fighter2.PositionX = DefenderX;
        State.Fighter2.Facing = EFighterFacing::Left;
        return State;
    }

    void Step(FMatchState& State)
    {
        Sim::SimulateFrame(State, {}, {});
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatLabClassificationTest, "ProjW.Simulation.CombatLab.BaselineClassification",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatLabClassificationTest::RunTest(const FString&)
{
    FMatchState NoOverlap = ActiveAttack(EMoveId::Light, 200 * Unit);
    Step(NoOverlap);
    TestEqual(TEXT("no overlap has no hit"), NoOverlap.HitEventCount, uint32(0));
    TestFalse(TEXT("no overlap leaves move available"), NoOverlap.Fighter1.bCurrentMoveConnected);

    FMatchState Airborne = ActiveAttack(EMoveId::Light, 60 * Unit);
    Airborne.Fighter2.bGrounded = false;
    Airborne.Fighter2.CurrentState = EFighterState::Jumping;
    Airborne.Fighter2.PositionY = 40 * Unit;
    Step(Airborne);
    TestEqual(TEXT("airborne overlap has no hit"), Airborne.HitEventCount, uint32(0));
    TestFalse(TEXT("airborne overlap leaves move available"), Airborne.Fighter1.bCurrentMoveConnected);

    FMatchState Edge = ActiveAttack(EMoveId::Light, 96 * Unit);
    Step(Edge);
    TestEqual(TEXT("strict edge touch has no hit"), Edge.HitEventCount, uint32(0));

    FMatchState Connected = ActiveAttack(EMoveId::Light, 60 * Unit);
    Connected.Fighter1.bCurrentMoveConnected = true;
    Step(Connected);
    TestEqual(TEXT("already connected move has no second hit"), Connected.HitEventCount, uint32(0));
    TestEqual(TEXT("already connected move causes no damage"), Connected.Fighter2.Health, 1000);

    for (const EMoveId MoveId : {EMoveId::Light, EMoveId::Medium, EMoveId::Heavy})
    {
        const FMoveDefinition& Move = *Sim::FindMoveDefinition(MoveId);
        FMatchState Hit = ActiveAttack(MoveId, Move.HitboxOffsetX);
        Step(Hit);
        TestEqual(TEXT("valid grounded overlap is a hit"), Hit.HitEventCount, uint32(1));
        TestTrue(TEXT("hit consumes this move"), Hit.Fighter1.bCurrentMoveConnected);
        TestEqual(TEXT("damage comes from active move"), Hit.Fighter2.Health, 1000 - Move.Damage);
        TestEqual(TEXT("hitstop comes from active move"), Hit.Fighter2.HitstopFramesRemaining, Move.HitstopFrames);
        TestEqual(TEXT("pending hitstun comes from active move"), Hit.Fighter2.PendingReactionFrames, Move.HitstunFrames);
        TestEqual(TEXT("knockback comes from active move"), Hit.Fighter2.PendingKnockbackVelocityX, Move.KnockbackSpeed);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatLabTradeTest, "ProjW.Simulation.CombatLab.BaselineTrade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatLabTradeTest::RunTest(const FString&)
{
    FMatchState State = ActiveAttack(EMoveId::Light, 40 * Unit);
    State.Fighter1.PositionX = -40 * Unit;
    State.Fighter2.CurrentState = EFighterState::Attack;
    State.Fighter2.CurrentMoveId = EMoveId::Heavy;
    State.Fighter2.MovePhase = EMovePhase::Active;
    Step(State);

    TestEqual(TEXT("both potential contacts resolve"), State.HitEventCount, uint32(2));
    TestEqual(TEXT("fighter 1 takes Heavy damage"), State.Fighter1.Health, 880);
    TestEqual(TEXT("fighter 2 takes Light damage"), State.Fighter2.Health, 950);
    TestTrue(TEXT("both attacks connect"), State.Fighter1.bCurrentMoveConnected && State.Fighter2.bCurrentMoveConnected);
    TestEqual(TEXT("fighter 1 captures Heavy hitstun"), State.Fighter1.PendingReactionFrames, uint8(20));
    TestEqual(TEXT("fighter 2 captures Light hitstun"), State.Fighter2.PendingReactionFrames, uint8(14));
    TestEqual(TEXT("fighter 1 receives shared max hitstop"), State.Fighter1.HitstopFramesRemaining, uint8(9));
    TestEqual(TEXT("fighter 2 receives shared max hitstop"), State.Fighter2.HitstopFramesRemaining, uint8(9));

    const FMatchState Snapshot = State;
    for (int32 Frame = 0; Frame < 40; ++Frame) { Step(State); }
    const FMatchState Expected = State;
    State = Snapshot;
    for (int32 Frame = 0; Frame < 40; ++Frame) { Step(State); }
    TestTrue(TEXT("baseline trade snapshot replays exactly"), State == Expected);
    return !HasAnyErrors();
}
#endif
