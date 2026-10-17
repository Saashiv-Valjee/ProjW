#include "Simulation/FightingSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
    using Sim = FFightingSimulation;
    constexpr int32 Unit = Sim::SubunitsPerWorldUnit;

    FFighterInput Buttons(uint16 Mask) { return {Mask}; }
    FFighterInput Button(EFighterInputButton Value) { return Buttons(static_cast<uint16>(Value)); }
    FFighterInput Guard(bool bCrouch, bool bDefenderOnRight = true)
    {
        return Buttons(static_cast<uint16>(bDefenderOnRight ? EFighterInputButton::Right : EFighterInputButton::Left)
            | (bCrouch ? static_cast<uint16>(EFighterInputButton::Down) : 0));
    }
    void Step(FMatchState& State, FFighterInput P1 = {}, FFighterInput P2 = {})
    {
        Sim::SimulateFrame(State, P1, P2);
    }
    FMatchState Active(EMoveId Id, bool bDefenderOnRight = true)
    {
        FMatchState State;
        const FMoveDefinition& Move = *Sim::FindMoveDefinition(Id);
        State.Fighter1.PositionX = 0;
        State.Fighter2.PositionX = (bDefenderOnRight ? 1 : -1) * Move.HitboxOffsetX;
        State.Fighter1.Facing = bDefenderOnRight ? EFighterFacing::Right : EFighterFacing::Left;
        State.Fighter2.Facing = bDefenderOnRight ? EFighterFacing::Left : EFighterFacing::Right;
        State.Fighter1.CurrentState = EFighterState::Attack;
        State.Fighter1.CurrentMoveId = Id;
        State.Fighter1.MovePhase = EMovePhase::Active;
        return State;
    }
    void Replay(FAutomationTestBase& Test, const TCHAR* Label, const FMatchState& Snapshot)
    {
        FMatchState A = Snapshot;
        FMatchState B = Snapshot;
        for (int32 Frame = 0; Frame < 80; ++Frame)
        {
            const FFighterInput P2 = Frame % 17 == 0 ? Guard(false) : FFighterInput{};
            Step(A, {}, P2);
            Step(B, {}, P2);
        }
        Test.TestTrue(Label, A == B);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockDefinitionsTest, "ProjW.Simulation.Block.Definitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockDefinitionsTest::RunTest(const FString&)
{
    struct FExpected { EMoveId Id; EGuardHeight Height; int32 Chip; uint8 Frames; int32 Push; };
    const FExpected Cases[] = {
        {EMoveId::Light, EGuardHeight::High, 5, 10, 3 * Unit},
        {EMoveId::Medium, EGuardHeight::Low, 8, 13, 4 * Unit},
        {EMoveId::Heavy, EGuardHeight::High, 12, 16, 5 * Unit}};
    for (const FExpected& Expected : Cases)
    {
        const FMoveDefinition& Move = *Sim::FindMoveDefinition(Expected.Id);
        TestEqual(TEXT("guard height"), static_cast<uint8>(Move.GuardHeight), static_cast<uint8>(Expected.Height));
        TestEqual(TEXT("chip"), Move.ChipDamage, Expected.Chip);
        TestEqual(TEXT("blockstun frames"), Move.BlockstunFrames, Expected.Frames);
        TestEqual(TEXT("block pushback"), Move.BlockPushbackSpeed, Expected.Push);
        TestTrue(TEXT("block pushback weaker than hit knockback"), Move.BlockPushbackSpeed < Move.KnockbackSpeed);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockGuardTest, "ProjW.Simulation.Block.GuardDirectionHeight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockGuardTest::RunTest(const FString&)
{
    for (const EMoveId Id : {EMoveId::Light, EMoveId::Medium, EMoveId::Heavy})
    {
        const FMoveDefinition& Move = *Sim::FindMoveDefinition(Id);
        for (bool bRight : {false, true})
        {
            for (bool bCrouch : {false, true})
            {
                FMatchState State = Active(Id, bRight);
                Step(State, {}, Guard(bCrouch, bRight));
                const bool bCorrect = bCrouch == (Move.GuardHeight == EGuardHeight::Low);
                TestEqual(TEXT("facing-relative height result"), State.Fighter2.PendingReaction,
                    bCorrect ? EPendingReaction::Block : EPendingReaction::Hit);
                TestEqual(TEXT("exact blocked or hit health"), State.Fighter2.Health,
                    1000 - (bCorrect ? Move.ChipDamage : Move.Damage));
                TestEqual(TEXT("only hits increment hit count"), State.HitEventCount, bCorrect ? uint32(0) : uint32(1));
            }
            FMatchState Toward = Active(Id, bRight);
            Step(Toward, {}, Button(bRight ? EFighterInputButton::Left : EFighterInputButton::Right));
            TestEqual(TEXT("toward never guards"), Toward.Fighter2.PendingReaction, EPendingReaction::Hit);
            FMatchState Unguarded = Active(Id, bRight);
            Step(Unguarded);
            TestEqual(TEXT("ordinary hit damage unchanged"), Unguarded.Fighter2.Health, 1000 - Move.Damage);
        }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockLifecycleTest, "ProjW.Simulation.Block.Lifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockLifecycleTest::RunTest(const FString&)
{
    for (const EMoveId Id : {EMoveId::Light, EMoveId::Medium, EMoveId::Heavy})
    {
        const FMoveDefinition& Move = *Sim::FindMoveDefinition(Id);
        FMatchState State = Active(Id);
        const FFighterInput Correct = Guard(Move.GuardHeight == EGuardHeight::Low);
        Step(State, {}, Correct);
        TestEqual(TEXT("block hitstop"), State.Fighter2.HitstopFramesRemaining, Move.HitstopFrames);
        TestEqual(TEXT("captured block reaction"), State.Fighter2.PendingReaction, EPendingReaction::Block);
        TestEqual(TEXT("captured block duration"), State.Fighter2.PendingReactionFrames, Move.BlockstunFrames);
        TestEqual(TEXT("captured block pushback"), State.Fighter2.PendingKnockbackVelocityX, Move.BlockPushbackSpeed);
        TestTrue(TEXT("contact consumed"), State.Fighter1.bCurrentMoveConnected);
        Replay(*this, TEXT("pending block hitstop snapshot replays"), State);
        const uint32 FrozenFrame = State.Fighter2.StateFrame;
        for (uint8 Frame = 0; Frame < Move.HitstopFrames; ++Frame)
        {
            Step(State, {}, Correct);
            TestEqual(TEXT("freeze retains state frame"), State.Fighter2.StateFrame, FrozenFrame);
            TestEqual(TEXT("freeze retains health"), State.Fighter2.Health, 1000 - Move.ChipDamage);
        }
        TestEqual(TEXT("final frozen call leaves zero hitstop"), State.Fighter2.HitstopFramesRemaining, uint8(0));
        const int32 XBeforeReaction = State.Fighter2.PositionX;
        Step(State, {}, Correct);
        TestEqual(TEXT("first blockstun state"), State.Fighter2.CurrentState, EFighterState::Blockstun);
        TestEqual(TEXT("first blockstun frame"), State.Fighter2.StateFrame, uint32(0));
        TestEqual(TEXT("first block pushback step"), State.Fighter2.PositionX - XBeforeReaction, Move.BlockPushbackSpeed);
        Replay(*this, TEXT("blockstun snapshot replays"), State);
        for (uint8 Frame = 1; Frame < Move.BlockstunFrames; ++Frame)
        {
            Step(State, {}, Buttons(static_cast<uint16>(EFighterInputButton::Up)
                | static_cast<uint16>(EFighterInputButton::Light) | static_cast<uint16>(EFighterInputButton::Down)));
            TestEqual(TEXT("blockstun locks state"), State.Fighter2.CurrentState, EFighterState::Blockstun);
            TestEqual(TEXT("blockstun exact state frame"), State.Fighter2.StateFrame, static_cast<uint32>(Frame));
        }
        Step(State);
        TestEqual(TEXT("actionable after exact duration"), State.Fighter2.CurrentState, EFighterState::Idle);
        TestEqual(TEXT("reaction clears"), State.Fighter2.PendingReaction, EPendingReaction::None);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockChipWallTest, "ProjW.Simulation.Block.ChipWallPushback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockChipWallTest::RunTest(const FString&)
{
    for (const EMoveId Id : {EMoveId::Light, EMoveId::Medium, EMoveId::Heavy})
    {
        const FMoveDefinition& Move = *Sim::FindMoveDefinition(Id);
        for (bool bRight : {false, true})
        {
            FMatchState State = Active(Id, bRight);
            State.Fighter2.Health = 2;
            const FFighterInput Correct = Guard(Move.GuardHeight == EGuardHeight::Low, bRight);
            Step(State, {}, Correct);
            TestEqual(TEXT("chip cannot KO"), State.Fighter2.Health, 1);
            FMatchState ZeroHealth = Active(Id, bRight);
            ZeroHealth.Fighter2.Health = 0;
            Step(ZeroHealth, {}, Correct);
            TestEqual(TEXT("chip cannot resurrect zero health"), ZeroHealth.Fighter2.Health, 0);
            TestEqual(TEXT("pushback direction captured"), State.Fighter2.PendingKnockbackVelocityX,
                (bRight ? 1 : -1) * Move.BlockPushbackSpeed);
            TestEqual(TEXT("attacker has no block recoil"), State.Fighter1.VelocityX, 0);
            const int32 Captured = State.Fighter2.PendingKnockbackVelocityX;
            State.Fighter1.Facing = bRight ? EFighterFacing::Left : EFighterFacing::Right;
            for (uint8 Frame = 0; Frame < Move.HitstopFrames; ++Frame) Step(State);
            TestEqual(TEXT("later facing cannot change captured pushback"), State.Fighter2.PendingKnockbackVelocityX, Captured);
            Step(State);
            TestEqual(TEXT("reaction pushes away from attacker"), State.Fighter2.VelocityX, Captured);
        }
    }
    for (bool bRight : {false, true})
    {
        FMatchState Wall = Active(EMoveId::Heavy, bRight);
        const int32 WallX = bRight ? Sim::MaxFighterX : Sim::MinFighterX;
        Wall.Fighter2.PositionX = WallX - (bRight ? 1 : -1) * Unit;
        Wall.Fighter1.PositionX = Wall.Fighter2.PositionX - (bRight ? 1 : -1) * 60 * Unit;
        const FFighterInput Correct = Guard(false, bRight);
        Step(Wall, {}, Correct);
        TestEqual(TEXT("wall contact blocks"), Wall.Fighter2.PendingReaction, EPendingReaction::Block);
        for (uint8 Frame = 0; Frame < Sim::FindMoveDefinition(EMoveId::Heavy)->HitstopFrames; ++Frame) Step(Wall);
        Step(Wall);
        TestEqual(TEXT("block pushback clamps at wall"), Wall.Fighter2.PositionX, WallX);
        TestEqual(TEXT("block pushback stops at wall"), Wall.Fighter2.PendingKnockbackVelocityX, 0);
        for (int32 Frame = 0; Frame < 40; ++Frame) Step(Wall);
        TestTrue(TEXT("no stage escape"), Wall.Fighter2.PositionX >= Sim::MinFighterX && Wall.Fighter2.PositionX <= Sim::MaxFighterX);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockConsumptionTest, "ProjW.Simulation.Block.ContactConsumption",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockConsumptionTest::RunTest(const FString&)
{
    FMatchState Block = Active(EMoveId::Light);
    Step(Block, {}, Guard(false));
    const int32 Health = Block.Fighter2.Health;
    for (int32 Frame = 0; Frame < 4; ++Frame) Step(Block, {}, Guard(false));
    TestEqual(TEXT("later active frames cannot repeat chip"), Block.Fighter2.Health, Health);
    FMatchState Miss = Active(EMoveId::Light);
    Miss.Fighter2.PositionX = 200 * Unit;
    Step(Miss, {}, Guard(false));
    TestFalse(TEXT("miss does not consume"), Miss.Fighter1.bCurrentMoveConnected);
    FMatchState Edge = Active(EMoveId::Light);
    Edge.Fighter2.PositionX = 96 * Unit;
    Step(Edge, {}, Guard(false));
    TestFalse(TEXT("strict edge touch does not consume"), Edge.Fighter1.bCurrentMoveConnected);
    FMatchState Air = Active(EMoveId::Light);
    Air.Fighter2.bGrounded = false;
    Air.Fighter2.CurrentState = EFighterState::Jumping;
    Air.Fighter2.PositionY = 40 * Unit;
    Step(Air, {}, Guard(false));
    TestFalse(TEXT("airborne overlap does not consume"), Air.Fighter1.bCurrentMoveConnected);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockCommittedTest, "ProjW.Simulation.Block.CommittedDefender",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockCommittedTest::RunTest(const FString&)
{
    for (const EFighterState Committed : {EFighterState::Attack, EFighterState::ForwardDash,
        EFighterState::BackDash, EFighterState::JumpStartup, EFighterState::Hitstun, EFighterState::Blockstun})
    {
        FMatchState State = Active(EMoveId::Light);
        State.Fighter2.CurrentState = Committed;
        if (Committed == EFighterState::Attack)
        {
            State.Fighter2.CurrentMoveId = EMoveId::Heavy;
            State.Fighter2.MovePhase = EMovePhase::Startup;
        }
        if (Committed == EFighterState::Hitstun || Committed == EFighterState::Blockstun)
        {
            State.Fighter2.PendingReaction = Committed == EFighterState::Hitstun
                ? EPendingReaction::Hit : EPendingReaction::Block;
            State.Fighter2.PendingReactionFrames = 5;
        }
        if (Committed == EFighterState::ForwardDash || Committed == EFighterState::BackDash)
        {
            State.Fighter2.DashDirection = 1;
        }
        Step(State, {}, Guard(false));
        TestEqual(TEXT("committed defender cannot guard"), State.Fighter2.PendingReaction, EPendingReaction::Hit);
        TestEqual(TEXT("committed defender takes full damage"), State.Fighter2.Health, 950);
    }
    FMatchState Air = Active(EMoveId::Light);
    Air.Fighter2.bGrounded = false;
    Air.Fighter2.CurrentState = EFighterState::Jumping;
    Air.Fighter2.PositionY = 40 * Unit;
    Step(Air, {}, Guard(false));
    TestEqual(TEXT("airborne defender has no valid contact"), Air.Fighter2.PendingReaction, EPendingReaction::None);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlockTradeTest, "ProjW.Simulation.Block.ReachableTradeSymmetry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBlockTradeTest::RunTest(const FString&)
{
    for (bool bSwap : {false, true})
    {
        FMatchState State;
        State.Fighter1.PositionX = -40 * Unit;
        State.Fighter2.PositionX = 40 * Unit;
        State.Fighter1.Facing = EFighterFacing::Right;
        State.Fighter2.Facing = EFighterFacing::Left;
        State.Fighter1.CurrentState = EFighterState::Attack;
        State.Fighter2.CurrentState = EFighterState::Attack;
        State.Fighter1.CurrentMoveId = bSwap ? EMoveId::Heavy : EMoveId::Light;
        State.Fighter2.CurrentMoveId = bSwap ? EMoveId::Light : EMoveId::Heavy;
        State.Fighter1.MovePhase = EMovePhase::Active;
        State.Fighter2.MovePhase = EMovePhase::Active;
        Step(State, {}, {});
        TestTrue(TEXT("both attacks consume"), State.Fighter1.bCurrentMoveConnected && State.Fighter2.bCurrentMoveConnected);
        TestEqual(TEXT("both hits counted"), State.HitEventCount, uint32(2));
        TestEqual(TEXT("both pending hit"), State.Fighter1.PendingReaction, EPendingReaction::Hit);
        TestEqual(TEXT("other pending hit"), State.Fighter2.PendingReaction, EPendingReaction::Hit);
        TestEqual(TEXT("shared max hitstop 1"), State.Fighter1.HitstopFramesRemaining, uint8(9));
        TestEqual(TEXT("shared max hitstop 2"), State.Fighter2.HitstopFramesRemaining, uint8(9));
        TestEqual(TEXT("fighter 1 receives incoming damage"), State.Fighter1.Health, bSwap ? 950 : 880);
        TestEqual(TEXT("fighter 2 receives incoming damage"), State.Fighter2.Health, bSwap ? 880 : 950);
    }
    // Both contact directions require an active attack. A fighter in Attack is
    // ineligible to guard, so mixed and double Block trades are unreachable in 5E.
    return !HasAnyErrors();
}
#endif
