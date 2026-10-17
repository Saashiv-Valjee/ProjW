#include "Simulation/FightingSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
    FFighterInput LightInput() { return {static_cast<uint16>(EFighterInputButton::Light)}; }
    void PlaceInRange(FMatchState& State)
    {
        State.Fighter1.PositionX = -100 * FFightingSimulation::SubunitsPerWorldUnit;
        State.Fighter2.PositionX = -40 * FFightingSimulation::SubunitsPerWorldUnit;
        State.Fighter2.Facing = EFighterFacing::Left;
    }
    void ReachActive(FMatchState& State)
    {
        const FMoveDefinition& Move = *FFightingSimulation::FindMoveDefinition(EMoveId::Light);
        FFightingSimulation::SimulateFrame(State, LightInput(), {});
        for (uint8 Index = 1; Index < Move.StartupFrames; ++Index)
            FFightingSimulation::SimulateFrame(State, {}, {});
        FFightingSimulation::SimulateFrame(State, {}, {});
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenericDefinitionTest, "ProjW.Simulation.Move.FoundationDefinition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenericDefinitionTest::RunTest(const FString&)
{
    const FMoveDefinition* Move = FFightingSimulation::FindMoveDefinition(EMoveId::Light);
    if (!TestNotNull(TEXT("Light definition exists"), Move)) return false;
    TestEqual(TEXT("startup"), Move->StartupFrames, uint8(5));
    TestEqual(TEXT("active"), Move->ActiveFrames, uint8(3));
    TestEqual(TEXT("recovery"), Move->RecoveryFrames, uint8(9));
    TestEqual(TEXT("damage"), Move->Damage, 50);
    TestEqual(TEXT("hitstop"), Move->HitstopFrames, uint8(6));
    TestEqual(TEXT("hitstun"), Move->HitstunFrames, uint8(14));
    TestEqual(TEXT("knockback"), Move->KnockbackSpeed, 6 * FFightingSimulation::SubunitsPerWorldUnit);
    TestEqual(TEXT("X offset"), Move->HitboxOffsetX, 44 * FFightingSimulation::SubunitsPerWorldUnit);
    TestEqual(TEXT("Y offset"), Move->HitboxOffsetY, 55 * FFightingSimulation::SubunitsPerWorldUnit);
    TestEqual(TEXT("half width"), Move->HitboxHalfWidth, 28 * FFightingSimulation::SubunitsPerWorldUnit);
    TestEqual(TEXT("half height"), Move->HitboxHalfHeight, 25 * FFightingSimulation::SubunitsPerWorldUnit);
    TestNull(TEXT("None has no definition"), FFightingSimulation::FindMoveDefinition(EMoveId::None));
    TestNull(TEXT("Invalid ID has no definition"), FFightingSimulation::FindMoveDefinition(static_cast<EMoveId>(255)));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenericContactTest, "ProjW.Simulation.Move.ContactAndReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenericContactTest::RunTest(const FString&)
{
    FMatchState State; PlaceInRange(State);
    FFightingSimulation::SimulateFrame(State, LightInput(), {});
    TestFalse(TEXT("fresh move has no contact"), State.Fighter1.bCurrentMoveConnected);
    for (int32 Index = 1; Index <= 5; ++Index) FFightingSimulation::SimulateFrame(State, {}, {});
    TestEqual(TEXT("one hit on active entry"), State.HitEventCount, uint32(1));
    TestTrue(TEXT("move marked connected"), State.Fighter1.bCurrentMoveConnected);
    for (int32 Index = 0; Index < 2; ++Index) FFightingSimulation::SimulateFrame(State, {}, {});
    TestEqual(TEXT("remaining active frames do not hit again"), State.HitEventCount, uint32(1));
    for (int32 Index = 0; Index < 40; ++Index) FFightingSimulation::SimulateFrame(State, {}, {});
    TestEqual(TEXT("finished move clears ID"), State.Fighter1.CurrentMoveId, EMoveId::None);
    FFightingSimulation::SimulateFrame(State, LightInput(), {});
    TestEqual(TEXT("new Light move starts"), State.Fighter1.CurrentMoveId, EMoveId::Light);
    TestFalse(TEXT("new move resets contact"), State.Fighter1.bCurrentMoveConnected);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenericReactionCaptureTest, "ProjW.Simulation.Move.ReactionCapture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenericReactionCaptureTest::RunTest(const FString&)
{
    FMatchState State; PlaceInRange(State); ReachActive(State);
    TestEqual(TEXT("hit recorded"), State.HitEventCount, uint32(1));
    TestEqual(TEXT("hitstun captured at contact"), State.Fighter2.PendingHitstunFrames, uint8(14));
    TestEqual(TEXT("knockback captured at contact"), State.Fighter2.PendingKnockbackVelocityX,
        6 * FFightingSimulation::SubunitsPerWorldUnit);
    const FMatchState Snapshot = State;
    FMatchState Replay = Snapshot;
    for (int32 Index = 0; Index < 30; ++Index)
    {
        FFightingSimulation::SimulateFrame(State, {}, {});
        FFightingSimulation::SimulateFrame(Replay, {}, {});
    }
    TestTrue(TEXT("reaction snapshot restores every field"), State == Replay);
    FMatchState ChangedAttacker = Snapshot;
    ChangedAttacker.Fighter1.CurrentMoveId = EMoveId::None;
    for (int32 Index = 0; Index < 7; ++Index) FFightingSimulation::SimulateFrame(ChangedAttacker, {}, {});
    TestEqual(TEXT("reaction still enters Hitstun"), ChangedAttacker.Fighter2.CurrentState, EFighterState::Hitstun);
    TestEqual(TEXT("reaction keeps hitstun duration"), ChangedAttacker.Fighter2.PendingHitstunFrames, uint8(14));
    TestEqual(TEXT("reaction keeps knockback"), ChangedAttacker.Fighter2.PendingKnockbackVelocityX,
        6 * FFightingSimulation::SubunitsPerWorldUnit);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenericTradeTest, "ProjW.Simulation.Move.Trade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenericTradeTest::RunTest(const FString&)
{
    FMatchState State;
    State.Fighter1.PositionX = -40 * FFightingSimulation::SubunitsPerWorldUnit;
    State.Fighter2.PositionX = 40 * FFightingSimulation::SubunitsPerWorldUnit;
    State.Fighter2.Facing = EFighterFacing::Left;
    FFightingSimulation::SimulateFrame(State, LightInput(), LightInput());
    for (int32 Index = 1; Index <= 5; ++Index) FFightingSimulation::SimulateFrame(State, {}, {});
    TestEqual(TEXT("both contacts register"), State.HitEventCount, uint32(2));
    TestEqual(TEXT("fighter 1 damage"), State.Fighter1.Health, 950);
    TestEqual(TEXT("fighter 2 damage"), State.Fighter2.Health, 950);
    TestTrue(TEXT("both moves connected"), State.Fighter1.bCurrentMoveConnected && State.Fighter2.bCurrentMoveConnected);
    TestEqual(TEXT("both reactions captured"), State.Fighter1.PendingHitstunFrames, State.Fighter2.PendingHitstunFrames);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenericSnapshotPhasesTest, "ProjW.Simulation.Move.SnapshotPhases",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenericSnapshotPhasesTest::RunTest(const FString&)
{
    FMatchState Miss;
    Miss.Fighter1.PositionX = -100 * FFightingSimulation::SubunitsPerWorldUnit;
    Miss.Fighter2.PositionX = 300 * FFightingSimulation::SubunitsPerWorldUnit;
    FFightingSimulation::SimulateFrame(Miss, LightInput(), {});
    const FMatchState Startup = Miss;
    for (int32 Index = 0; Index < 5; ++Index) FFightingSimulation::SimulateFrame(Miss, {}, {});
    const FMatchState ActiveBeforeContact = Miss;
    for (int32 Index = 0; Index < 3; ++Index) FFightingSimulation::SimulateFrame(Miss, {}, {});
    const FMatchState Recovery = Miss;

    FMatchState Hit;
    PlaceInRange(Hit);
    ReachActive(Hit);
    const FMatchState ActiveAfterContact = Hit;
    for (int32 Index = 0; Index < 3; ++Index) FFightingSimulation::SimulateFrame(Hit, {}, {});
    const FMatchState Hitstop = Hit;
    for (int32 Index = 0; Index < 3; ++Index) FFightingSimulation::SimulateFrame(Hit, {}, {});
    const FMatchState PendingReaction = Hit;
    FFightingSimulation::SimulateFrame(Hit, {}, {});
    const FMatchState Hitstun = Hit;

    TestEqual(TEXT("startup phase"), Startup.Fighter1.MovePhase, EMovePhase::Startup);
    TestEqual(TEXT("active before contact phase"), ActiveBeforeContact.Fighter1.MovePhase, EMovePhase::Active);
    TestFalse(TEXT("active before contact has no hit"), ActiveBeforeContact.Fighter1.bCurrentMoveConnected);
    TestEqual(TEXT("active after contact phase"), ActiveAfterContact.Fighter1.MovePhase, EMovePhase::Active);
    TestTrue(TEXT("active after contact records hit"), ActiveAfterContact.Fighter1.bCurrentMoveConnected);
    TestEqual(TEXT("recovery phase"), Recovery.Fighter1.MovePhase, EMovePhase::Recovery);
    TestTrue(TEXT("hitstop is frozen"), Hitstop.Fighter2.HitstopFramesRemaining > 0);
    TestTrue(TEXT("pending reaction survives hitstop"), PendingReaction.Fighter2.bPendingHitReaction);
    TestEqual(TEXT("pending reaction follows final frozen frame"), PendingReaction.Fighter2.HitstopFramesRemaining, uint8(0));
    TestEqual(TEXT("reaction enters Hitstun"), Hitstun.Fighter2.CurrentState, EFighterState::Hitstun);

    const FMatchState Snapshots[] = {Startup, ActiveBeforeContact, ActiveAfterContact,
        Recovery, Hitstop, PendingReaction, Hitstun};
    const TCHAR* Labels[] = {TEXT("startup replay"), TEXT("active before contact replay"),
        TEXT("active after contact replay"), TEXT("recovery replay"), TEXT("hitstop replay"),
        TEXT("pending reaction replay"), TEXT("Hitstun replay")};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Snapshots); ++Index)
    {
        FMatchState Replayed = Snapshots[Index];
        for (int32 Frame = 0; Frame < 40; ++Frame) FFightingSimulation::SimulateFrame(Replayed, {}, {});
        const FMatchState Expected = Replayed;
        Replayed = Snapshots[Index];
        for (int32 Frame = 0; Frame < 40; ++Frame) FFightingSimulation::SimulateFrame(Replayed, {}, {});
        TestTrue(Labels[Index], Replayed == Expected);
    }
    return !HasAnyErrors();
}
#endif
