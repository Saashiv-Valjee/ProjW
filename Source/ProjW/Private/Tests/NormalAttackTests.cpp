#include "Simulation/FightingSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
    using Sim = FFightingSimulation;
    constexpr int32 Unit = Sim::SubunitsPerWorldUnit;

    struct FExpectedMove
    {
        EMoveId Id;
        EFighterInputButton Button;
        uint8 Startup, Active, Recovery;
        int32 Damage;
        uint8 Hitstop, Hitstun;
        int32 Knockback, OffsetX, HalfWidth;
    };

    constexpr FExpectedMove Moves[] = {
        {EMoveId::Light, EFighterInputButton::Light, 5, 3, 9, 50, 6, 14, 6 * Unit, 44 * Unit, 28 * Unit},
        {EMoveId::Medium, EFighterInputButton::Medium, 7, 3, 12, 80, 7, 17, 7 * Unit, 50 * Unit, 35 * Unit},
        {EMoveId::Heavy, EFighterInputButton::Heavy, 10, 4, 16, 120, 9, 20, 9 * Unit, 66 * Unit, 44 * Unit}
    };

    FFighterInput Input(EFighterInputButton Button)
    {
        return {static_cast<uint16>(Button)};
    }

    FFighterInput Combined(uint16 Buttons) { return {Buttons}; }

    void Step(FMatchState& State, FFighterInput Player1 = {}, FFighterInput Player2 = {})
    {
        Sim::SimulateFrame(State, Player1, Player2);
    }

    FMatchState SpacedState(int32 Fighter1X = 0, int32 Fighter2X = 300 * Unit)
    {
        FMatchState State;
        State.Fighter1.PositionX = Fighter1X;
        State.Fighter2.PositionX = Fighter2X;
        State.Fighter2.Facing = EFighterFacing::Left;
        return State;
    }

    void StartMove(FMatchState& State, const FExpectedMove& Move)
    {
        Step(State, Input(Move.Button));
    }

    void ReachActive(FMatchState& State, const FExpectedMove& Move)
    {
        StartMove(State, Move);
        for (uint8 Frame = 0; Frame < Move.Startup; ++Frame) { Step(State); }
    }

    void PlaceAtHitCenter(FMatchState& State, const FExpectedMove& Move, bool bFacingRight = true)
    {
        State = SpacedState();
        State.Fighter1.PositionX = 0;
        State.Fighter2.PositionX = bFacingRight ? Move.OffsetX : -Move.OffsetX;
        State.Fighter1.Facing = bFacingRight ? EFighterFacing::Right : EFighterFacing::Left;
        State.Fighter2.Facing = bFacingRight ? EFighterFacing::Left : EFighterFacing::Right;
    }

    void ReplaySnapshot(FAutomationTestBase& Test, const TCHAR* Label, const FMatchState& Snapshot)
    {
        FMatchState First = Snapshot;
        FMatchState Second = Snapshot;
        for (int32 Frame = 0; Frame < 80; ++Frame)
        {
            const FFighterInput P1 = Frame == 50 ? Input(EFighterInputButton::Right) : FFighterInput{};
            Step(First, P1);
            Step(Second, P1);
        }
        Test.TestTrue(Label, First == Second);
    }

    void CaptureSnapshots(const FExpectedMove& Move, FMatchState (&Out)[6])
    {
        FMatchState Miss = SpacedState();
        StartMove(Miss, Move);
        Out[0] = Miss; // Startup.
        for (uint8 Frame = 0; Frame < Move.Startup; ++Frame) { Step(Miss); }
        Out[1] = Miss; // Active before contact.
        for (uint8 Frame = 0; Frame < Move.Active; ++Frame) { Step(Miss); }
        Out[2] = Miss; // Recovery.

        FMatchState Hit;
        PlaceAtHitCenter(Hit, Move);
        ReachActive(Hit, Move);
        Out[3] = Hit; // Hitstop with a pending reaction.
        for (uint8 Frame = 0; Frame < Move.Hitstop; ++Frame) { Step(Hit); }
        Out[4] = Hit; // Pending reaction after the final frozen call.
        Step(Hit);
        Out[5] = Hit; // Hitstun.
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNormalDefinitionsTest, "ProjW.Simulation.Normal.Definitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNormalDefinitionsTest::RunTest(const FString&)
{
    for (const FExpectedMove& Expected : Moves)
    {
        const FMoveDefinition* Move = Sim::FindMoveDefinition(Expected.Id);
        if (!TestNotNull(TEXT("move definition exists"), Move)) { continue; }
        TestEqual(TEXT("startup"), Move->StartupFrames, Expected.Startup);
        TestEqual(TEXT("active"), Move->ActiveFrames, Expected.Active);
        TestEqual(TEXT("recovery"), Move->RecoveryFrames, Expected.Recovery);
        TestEqual(TEXT("damage"), Move->Damage, Expected.Damage);
        TestEqual(TEXT("hitstop"), Move->HitstopFrames, Expected.Hitstop);
        TestEqual(TEXT("hitstun"), Move->HitstunFrames, Expected.Hitstun);
        TestEqual(TEXT("knockback"), Move->KnockbackSpeed, Expected.Knockback);
        TestEqual(TEXT("hitbox X offset"), Move->HitboxOffsetX, Expected.OffsetX);
        TestEqual(TEXT("hitbox Y offset"), Move->HitboxOffsetY, 55 * Unit);
        TestEqual(TEXT("hitbox half width"), Move->HitboxHalfWidth, Expected.HalfWidth);
        TestEqual(TEXT("hitbox half height"), Move->HitboxHalfHeight, 25 * Unit);
    }
    TestNull(TEXT("None remains invalid"), Sim::FindMoveDefinition(EMoveId::None));
    TestNull(TEXT("unknown ID remains invalid"), Sim::FindMoveDefinition(static_cast<EMoveId>(255)));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNormalInputSelectionTest, "ProjW.Simulation.Normal.InputSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNormalInputSelectionTest::RunTest(const FString&)
{
    for (const FExpectedMove& Move : Moves)
    {
        FMatchState State = SpacedState();
        StartMove(State, Move);
        TestEqual(TEXT("fresh button maps to move"), State.Fighter1.CurrentMoveId, Move.Id);
        for (int32 Frame = 0; Frame < 80; ++Frame) { Step(State, Input(Move.Button)); }
        TestEqual(TEXT("held button cannot retrigger"), State.Fighter1.CurrentMoveId, EMoveId::None);
        TestEqual(TEXT("held button returns neutral"), State.Fighter1.CurrentState, EFighterState::Idle);

        for (EFighterState Initial : {EFighterState::Idle, EFighterState::Walking, EFighterState::Crouching})
        {
            FMatchState Actionable = SpacedState();
            Actionable.Fighter1.CurrentState = Initial;
            Step(Actionable, Input(Move.Button));
            TestEqual(TEXT("normal starts from each actionable grounded state"),
                Actionable.Fighter1.CurrentMoveId, Move.Id);
        }
        for (EFighterState Initial : {EFighterState::JumpStartup, EFighterState::Jumping,
            EFighterState::DoubleJumping, EFighterState::ForwardDash, EFighterState::BackDash})
        {
            FMatchState LockedState = SpacedState();
            LockedState.Fighter1.CurrentState = Initial;
            if (Initial == EFighterState::Jumping || Initial == EFighterState::DoubleJumping)
            {
                LockedState.Fighter1.bGrounded = false;
                LockedState.Fighter1.PositionY = 100 * Unit;
            }
            Step(LockedState, Input(Move.Button));
            TestTrue(TEXT("normal is rejected from non-actionable movement state"),
                LockedState.Fighter1.CurrentMoveId == EMoveId::None);
        }
    }

    constexpr uint16 L = static_cast<uint16>(EFighterInputButton::Light);
    constexpr uint16 M = static_cast<uint16>(EFighterInputButton::Medium);
    constexpr uint16 H = static_cast<uint16>(EFighterInputButton::Heavy);
    struct FPriorityCase { uint16 Buttons; EMoveId Expected; };
    constexpr FPriorityCase Cases[] = {{L | M, EMoveId::Medium}, {L | H, EMoveId::Heavy},
        {M | H, EMoveId::Heavy}, {L | M | H, EMoveId::Heavy}};
    for (const FPriorityCase& Priority : Cases)
    {
        FMatchState State = SpacedState();
        Step(State, Combined(Priority.Buttons));
        TestEqual(TEXT("explicit simultaneous priority"), State.Fighter1.CurrentMoveId, Priority.Expected);
    }

    FMatchState Locked = SpacedState();
    StartMove(Locked, Moves[1]);
    Step(Locked); // Release Medium while still in startup.
    Step(Locked, Input(EFighterInputButton::Heavy));
    TestEqual(TEXT("another attack cannot replace the current move"), Locked.Fighter1.CurrentMoveId,
        EMoveId::Medium);
    for (int32 Frame = 0; Frame < 80; ++Frame) { Step(Locked); }
    TestEqual(TEXT("ignored attack input is not buffered"), Locked.Fighter1.CurrentMoveId, EMoveId::None);

    FMatchState Reaction;
    PlaceAtHitCenter(Reaction, Moves[0]);
    ReachActive(Reaction, Moves[0]);
    Step(Reaction, {}, Input(EFighterInputButton::Heavy)); // Press during hitstop.
    Step(Reaction);
    for (uint8 Frame = 2; Frame < Moves[0].Hitstop; ++Frame) { Step(Reaction); }
    Step(Reaction); // Enter Hitstun.
    Step(Reaction, {}, Input(EFighterInputButton::Medium)); // Press during Hitstun.
    for (int32 Frame = 0; Frame < 30; ++Frame) { Step(Reaction); }
    TestEqual(TEXT("reaction-time attacks are not buffered"), Reaction.Fighter2.CurrentMoveId, EMoveId::None);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNormalLifecycleTest, "ProjW.Simulation.Normal.ExactLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNormalLifecycleTest::RunTest(const FString&)
{
    for (const FExpectedMove& Move : Moves)
    {
        FMatchState State = SpacedState();
        StartMove(State, Move);
        TestEqual(TEXT("startup entry phase"), State.Fighter1.MovePhase, EMovePhase::Startup);
        TestEqual(TEXT("startup entry frame"), State.Fighter1.StateFrame, uint32(0));
        for (uint32 Frame = 1; Frame < Move.Startup; ++Frame)
        {
            Step(State);
            TestEqual(TEXT("startup phase"), State.Fighter1.MovePhase, EMovePhase::Startup);
            TestEqual(TEXT("startup exact frame"), State.Fighter1.StateFrame, Frame);
        }
        Step(State);
        TestEqual(TEXT("active entry phase"), State.Fighter1.MovePhase, EMovePhase::Active);
        TestEqual(TEXT("active entry frame"), State.Fighter1.StateFrame, uint32(0));
        for (uint32 Frame = 1; Frame < Move.Active; ++Frame)
        {
            Step(State);
            TestEqual(TEXT("active phase"), State.Fighter1.MovePhase, EMovePhase::Active);
            TestEqual(TEXT("active exact frame"), State.Fighter1.StateFrame, Frame);
        }
        Step(State);
        TestEqual(TEXT("recovery entry phase"), State.Fighter1.MovePhase, EMovePhase::Recovery);
        TestEqual(TEXT("recovery entry frame"), State.Fighter1.StateFrame, uint32(0));
        for (uint32 Frame = 1; Frame < Move.Recovery; ++Frame)
        {
            Step(State);
            TestEqual(TEXT("recovery phase"), State.Fighter1.MovePhase, EMovePhase::Recovery);
            TestEqual(TEXT("recovery exact frame"), State.Fighter1.StateFrame, Frame);
        }
        Step(State);
        TestEqual(TEXT("move completes to neutral"), State.Fighter1.CurrentState, EFighterState::Idle);
        TestEqual(TEXT("completed move clears ID"), State.Fighter1.CurrentMoveId, EMoveId::None);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNormalHitGeometryTest, "ProjW.Simulation.Normal.HitMissGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNormalHitGeometryTest::RunTest(const FString&)
{
    for (const FExpectedMove& Move : Moves)
    {
        FMatchState Hit;
        PlaceAtHitCenter(Hit, Move);
        ReachActive(Hit, Move);
        TestEqual(TEXT("right-facing intended-range hit"), Hit.HitEventCount, uint32(1));

        FMatchState LeftHit;
        PlaceAtHitCenter(LeftHit, Move, false);
        ReachActive(LeftHit, Move);
        TestEqual(TEXT("left-facing mirrored hit"), LeftHit.HitEventCount, uint32(1));

        FMatchState Miss = SpacedState(0, Move.OffsetX + Move.HalfWidth + Sim::HurtboxHalfWidth + 1);
        ReachActive(Miss, Move);
        TestEqual(TEXT("outside-range miss"), Miss.HitEventCount, uint32(0));

        FMatchState Edge = SpacedState(0, Move.OffsetX + Move.HalfWidth + Sim::HurtboxHalfWidth);
        ReachActive(Edge, Move);
        TestEqual(TEXT("strict edge touch is not a hit"), Edge.HitEventCount, uint32(0));
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNormalContactReactionTest, "ProjW.Simulation.Normal.ContactReaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNormalContactReactionTest::RunTest(const FString&)
{
    for (const FExpectedMove& Move : Moves)
    {
        FMatchState State;
        PlaceAtHitCenter(State, Move);
        ReachActive(State, Move);
        TestEqual(TEXT("first overlap hits once"), State.HitEventCount, uint32(1));
        TestTrue(TEXT("contact flag set"), State.Fighter1.bCurrentMoveConnected);
        TestEqual(TEXT("exact damage"), State.Fighter2.Health, 1000 - Move.Damage);
        TestEqual(TEXT("exact hitstop"), State.Fighter2.HitstopFramesRemaining, Move.Hitstop);
        TestEqual(TEXT("captured hitstun"), State.Fighter2.PendingHitstunFrames, Move.Hitstun);
        TestEqual(TEXT("positive knockback"), State.Fighter2.PendingKnockbackVelocityX, Move.Knockback);
        for (uint8 Frame = 1; Frame < Move.Active; ++Frame)
        {
            Step(State);
            TestEqual(TEXT("later active calls cannot hit twice"), State.HitEventCount, uint32(1));
        }
        for (uint8 Frame = 0; Frame < Move.Hitstop - (Move.Active - 1); ++Frame) { Step(State); }
        Step(State);
        TestEqual(TEXT("reaction enters Hitstun"), State.Fighter2.CurrentState, EFighterState::Hitstun);
        TestEqual(TEXT("Hitstun entry frame"), State.Fighter2.StateFrame, uint32(0));
        for (uint32 Frame = 1; Frame < Move.Hitstun; ++Frame)
        {
            Step(State);
            TestEqual(TEXT("exact Hitstun state"), State.Fighter2.CurrentState, EFighterState::Hitstun);
            TestEqual(TEXT("exact Hitstun frame"), State.Fighter2.StateFrame, Frame);
        }
        Step(State);
        TestTrue(TEXT("Hitstun exits after exact duration"), State.Fighter2.CurrentState != EFighterState::Hitstun);

        for (int32 Frame = 0; Frame < 80; ++Frame) { Step(State); }
        State.Fighter1.PositionX = 0;
        State.Fighter2.PositionX = Move.OffsetX;
        State.Fighter1.Facing = EFighterFacing::Right;
        State.Fighter2.Facing = EFighterFacing::Left;
        StartMove(State, Move);
        TestFalse(TEXT("new move resets contact"), State.Fighter1.bCurrentMoveConnected);
        for (uint8 Frame = 0; Frame < Move.Startup; ++Frame) { Step(State); }
        TestEqual(TEXT("new move may hit again"), State.HitEventCount, uint32(2));

        FMatchState Reverse;
        PlaceAtHitCenter(Reverse, Move, false);
        ReachActive(Reverse, Move);
        TestEqual(TEXT("left-facing knockback direction"), Reverse.Fighter2.PendingKnockbackVelocityX,
            -Move.Knockback);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNormalRangeAndAirborneTest, "ProjW.Simulation.Normal.RangeAndAirborne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNormalRangeAndAirborneTest::RunTest(const FString&)
{
    const FExpectedMove& Light = Moves[0];
    const FExpectedMove& Medium = Moves[1];
    const FExpectedMove& Heavy = Moves[2];
    const int32 LightReach = Light.OffsetX + Light.HalfWidth + Sim::HurtboxHalfWidth;
    const int32 MediumReach = Medium.OffsetX + Medium.HalfWidth + Sim::HurtboxHalfWidth;

    FMatchState LightMiss = SpacedState(0, LightReach + Unit);
    ReachActive(LightMiss, Light);
    FMatchState MediumHit = SpacedState(0, LightReach + Unit);
    ReachActive(MediumHit, Medium);
    TestEqual(TEXT("Light misses at Medium-only range"), LightMiss.HitEventCount, uint32(0));
    TestEqual(TEXT("Medium hits beyond Light range"), MediumHit.HitEventCount, uint32(1));

    FMatchState MediumMiss = SpacedState(0, MediumReach + Unit);
    ReachActive(MediumMiss, Medium);
    FMatchState HeavyHit = SpacedState(0, MediumReach + Unit);
    ReachActive(HeavyHit, Heavy);
    TestEqual(TEXT("Medium misses at Heavy-only range"), MediumMiss.HitEventCount, uint32(0));
    TestEqual(TEXT("Heavy hits beyond Medium range"), HeavyHit.HitEventCount, uint32(1));

    for (const FExpectedMove* Move : {&Medium, &Heavy})
    {
        FMatchState State;
        PlaceAtHitCenter(State, *Move);
        State.Fighter2.bGrounded = false;
        State.Fighter2.PositionY = 40 * Unit;
        ReachActive(State, *Move);
        TestEqual(TEXT("airborne overlap creates no event"), State.HitEventCount, uint32(0));
        TestFalse(TEXT("airborne overlap preserves contact flag"), State.Fighter1.bCurrentMoveConnected);
        State.Fighter2.bGrounded = true;
        State.Fighter2.PositionY = 0;
        Step(State);
        TestEqual(TEXT("later grounded active overlap connects"), State.HitEventCount, uint32(1));
        TestTrue(TEXT("later valid contact sets flag"), State.Fighter1.bCurrentMoveConnected);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNormalUnlikeTradeTest, "ProjW.Simulation.Normal.UnlikeTrades",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNormalUnlikeTradeTest::RunTest(const FString&)
{
    struct FTrade { const FExpectedMove* First; const FExpectedMove* Second; };
    const FTrade Trades[] = {{&Moves[0], &Moves[2]}, {&Moves[1], &Moves[2]}};
    for (const FTrade& Trade : Trades)
    {
        FMatchState State = SpacedState(-40 * Unit, 40 * Unit);
        const uint8 Delay = Trade.Second->Startup - Trade.First->Startup;
        Step(State, {}, Input(Trade.Second->Button));
        for (uint8 Frame = 1; Frame < Delay; ++Frame) { Step(State); }
        Step(State, Input(Trade.First->Button));
        for (uint8 Frame = 0; Frame < Trade.First->Startup; ++Frame) { Step(State); }
        TestEqual(TEXT("both unlike contacts collected"), State.HitEventCount, uint32(2));
        TestEqual(TEXT("fighter 1 receives second move damage"), State.Fighter1.Health,
            1000 - Trade.Second->Damage);
        TestEqual(TEXT("fighter 2 receives first move damage"), State.Fighter2.Health,
            1000 - Trade.First->Damage);
        TestEqual(TEXT("fighter 1 captures second move hitstun"), State.Fighter1.PendingHitstunFrames,
            Trade.Second->Hitstun);
        TestEqual(TEXT("fighter 2 captures first move hitstun"), State.Fighter2.PendingHitstunFrames,
            Trade.First->Hitstun);
        TestEqual(TEXT("fighter 1 knockback comes from second move"), State.Fighter1.PendingKnockbackVelocityX,
            -Trade.Second->Knockback);
        TestEqual(TEXT("fighter 2 knockback comes from first move"), State.Fighter2.PendingKnockbackVelocityX,
            Trade.First->Knockback);
        TestEqual(TEXT("trade hitstop uses deterministic maximum"), State.Fighter1.HitstopFramesRemaining,
            Trade.Second->Hitstop);
        TestEqual(TEXT("both fighters share trade hitstop"), State.Fighter2.HitstopFramesRemaining,
            Trade.Second->Hitstop);

        FMatchState Reversed = SpacedState(-40 * Unit, 40 * Unit);
        Step(Reversed, Input(Trade.Second->Button), {});
        for (uint8 Frame = 1; Frame < Delay; ++Frame) { Step(Reversed); }
        Step(Reversed, {}, Input(Trade.First->Button));
        for (uint8 Frame = 0; Frame < Trade.First->Startup; ++Frame) { Step(Reversed); }
        TestEqual(TEXT("reversed player IDs still collect both contacts"), Reversed.HitEventCount, uint32(2));
        TestEqual(TEXT("reversed fighter 1 receives first move damage"), Reversed.Fighter1.Health,
            1000 - Trade.First->Damage);
        TestEqual(TEXT("reversed fighter 2 receives second move damage"), Reversed.Fighter2.Health,
            1000 - Trade.Second->Damage);
        TestEqual(TEXT("reversed fighter 1 captures first move hitstun"), Reversed.Fighter1.PendingHitstunFrames,
            Trade.First->Hitstun);
        TestEqual(TEXT("reversed fighter 2 captures second move hitstun"), Reversed.Fighter2.PendingHitstunFrames,
            Trade.Second->Hitstun);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNormalSnapshotReplayTest, "ProjW.Simulation.Normal.SnapshotReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNormalSnapshotReplayTest::RunTest(const FString&)
{
    for (const FExpectedMove* Move : {&Moves[1], &Moves[2]})
    {
        FMatchState Snapshots[6];
        CaptureSnapshots(*Move, Snapshots);
        TestEqual(TEXT("startup snapshot"), Snapshots[0].Fighter1.MovePhase, EMovePhase::Startup);
        TestEqual(TEXT("active snapshot"), Snapshots[1].Fighter1.MovePhase, EMovePhase::Active);
        TestEqual(TEXT("recovery snapshot"), Snapshots[2].Fighter1.MovePhase, EMovePhase::Recovery);
        TestTrue(TEXT("hitstop snapshot"), Snapshots[3].Fighter2.HitstopFramesRemaining > 0);
        TestTrue(TEXT("pending snapshot"), Snapshots[4].Fighter2.bPendingHitReaction);
        TestEqual(TEXT("pending snapshot is after freeze"), Snapshots[4].Fighter2.HitstopFramesRemaining, uint8(0));
        TestEqual(TEXT("Hitstun snapshot"), Snapshots[5].Fighter2.CurrentState, EFighterState::Hitstun);
        const TCHAR* Labels[] = {TEXT("startup replay"), TEXT("active replay"), TEXT("recovery replay"),
            TEXT("hitstop replay"), TEXT("pending replay"), TEXT("Hitstun replay")};
        for (int32 Index = 0; Index < 6; ++Index) { ReplaySnapshot(*this, Labels[Index], Snapshots[Index]); }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNormalMixedDeterminismTest, "ProjW.Simulation.Normal.MixedDeterminism",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNormalMixedDeterminismTest::RunTest(const FString&)
{
    FMatchState First = SpacedState(-300 * Unit, 300 * Unit);
    FMatchState Second = First;
    bool bLight = false, bMedium = false, bHeavy = false, bWalk = false, bJump = false;
    for (uint32 Frame = 0; Frame < 30000; ++Frame)
    {
        const uint32 Phase = Frame % 240;
        FFighterInput P1;
        if (Phase == 0) { P1 = Input(EFighterInputButton::Light); }
        else if (Phase == 40) { P1 = Input(EFighterInputButton::Medium); }
        else if (Phase == 90) { P1 = Input(EFighterInputButton::Heavy); }
        else if (Phase == 150) { P1 = Input(EFighterInputButton::Up); }
        else if (Phase >= 205 && Phase < 225) { P1 = Input(EFighterInputButton::Right); }
        Step(First, P1);
        Step(Second, P1);
        if (!(First == Second))
        {
            AddError(FString::Printf(TEXT("mixed normal replay diverged at frame %u"), Frame));
            return false;
        }
        bLight |= First.Fighter1.CurrentMoveId == EMoveId::Light;
        bMedium |= First.Fighter1.CurrentMoveId == EMoveId::Medium;
        bHeavy |= First.Fighter1.CurrentMoveId == EMoveId::Heavy;
        bWalk |= First.Fighter1.CurrentState == EFighterState::Walking;
        bJump |= First.Fighter1.CurrentState == EFighterState::Jumping;
    }
    TestTrue(TEXT("mixed sequence exercised Light"), bLight);
    TestTrue(TEXT("mixed sequence exercised Medium"), bMedium);
    TestTrue(TEXT("mixed sequence exercised Heavy"), bHeavy);
    TestTrue(TEXT("mixed sequence exercised walking"), bWalk);
    TestTrue(TEXT("mixed sequence exercised jumping"), bJump);
    TestEqual(TEXT("exact focused sequence length"), First.SimulationFrame, uint64(30000));
    return !HasAnyErrors();
}
#endif
