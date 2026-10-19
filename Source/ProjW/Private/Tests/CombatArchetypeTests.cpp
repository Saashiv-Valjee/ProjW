#include "Simulation/FightingSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
    using Sim = FFightingSimulation;
    constexpr int32 U = Sim::SubunitsPerWorldUnit;

    FFighterInput Button(EFighterInputButton B) { return {static_cast<uint16>(B)}; }
    void Step(FMatchState& S, FFighterInput P1 = {}, FFighterInput P2 = {})
    { Sim::SimulateFrame(S, P1, P2); }

    FMatchState Active(EMoveId Move, int32 DefenderX = 100 * U)
    {
        FMatchState S;
        S.Fighter1.CurrentState = EFighterState::Attack;
        S.Fighter1.CurrentMoveId = Move;
        S.Fighter1.MovePhase = EMovePhase::Active;
        S.Fighter1.AttackAxisX = Sim::AxisScale;
        S.Fighter2.PositionX = DefenderX;
        S.Fighter2.Facing = EFighterFacing::Left;
        return S;
    }

    bool ResolveParry(EMoveId Move, EFighterInputButton Parry)
    {
        FMatchState S = Active(Move);
        Step(S, {}, Button(Parry));
        return S.Fighter2.CurrentState == EFighterState::ParrySuccess;
    }

    bool ResolveBlock(EMoveId Move, bool bLow)
    {
        FMatchState S = Active(Move);
        const uint16 Buttons = static_cast<uint16>(EFighterInputButton::Right)
            | (bLow ? static_cast<uint16>(EFighterInputButton::Down) : 0);
        Step(S, {}, {Buttons});
        return S.Fighter2.PendingReaction == EPendingReaction::Block;
    }

    struct FPivotResult { bool bHit; bool bPivotEnded; bool bPivotMoved; bool bOverlapAtContact; };
    FPivotResult ResolvePivot(EMoveId Move)
    {
        FMatchState S;
        S.Fighter2.PositionX = 110 * U;
        S.Fighter2.Facing = EFighterFacing::Left;
        Step(S, Button(Move == EMoveId::NarrowThrust ? EFighterInputButton::NarrowThrust
            : EFighterInputButton::BroadSweep));
        // Start the same committed 12-frame Pivot at the same time as attack commitment.
        Step(S, {}, Button(EFighterInputButton::PivotPositive));
        for (int32 Frame = 1; Frame < Sim::FindMoveDefinition(Move)->StartupFrames; ++Frame) Step(S);
        FSimOrientedBox Volume;
        const bool bHasVolume = Sim::GetMoveHitVolume(S.Fighter1, Volume);
        const bool bOverlap = bHasVolume && Volume.Overlaps(Sim::GetHurtbox(S.Fighter2));
        return {S.HitEventCount > 0, S.Fighter2.CurrentState != EFighterState::Pivoting,
            S.Fighter2.PositionZ != 0, bOverlap};
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatArchetypeDefinitionsTest, "ProjW.Simulation.CombatLab.ArchetypeDefinitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatArchetypeDefinitionsTest::RunTest(const FString&)
{
    struct FCase { EMoveId Id; EFighterInputButton Input; uint8 Startup, Active, Recovery; int32 Damage, Offset, OffsetY, Forward, Lateral, Vertical; EGuardHeight Guard; EParryHeight Parry; };
    const FCase Cases[] = {
        {EMoveId::NarrowThrust, EFighterInputButton::NarrowThrust, 16, 3, 22, 90, 60, 58, 42, 2, 24, EGuardHeight::High, EParryHeight::High},
        {EMoveId::BroadSweep, EFighterInputButton::BroadSweep, 13, 4, 20, 85, 52, 55, 40, 24, 18, EGuardHeight::High, EParryHeight::High},
        {EMoveId::SlowCommittedStrike, EFighterInputButton::SlowCommittedStrike, 24, 3, 32, 130, 68, 76, 34, 12, 34, EGuardHeight::High, EParryHeight::High}
    };
    for (const FCase& C : Cases)
    {
        const FMoveDefinition* M = Sim::FindMoveDefinition(C.Id);
        if (!TestNotNull(TEXT("prototype move definition exists"), M)) continue;
        TestEqual(TEXT("startup"), M->StartupFrames, C.Startup);
        TestEqual(TEXT("active"), M->ActiveFrames, C.Active);
        TestEqual(TEXT("recovery"), M->RecoveryFrames, C.Recovery);
        TestEqual(TEXT("damage"), M->Damage, C.Damage);
        TestEqual(TEXT("forward offset"), M->HitboxOffsetX, C.Offset * U);
        TestEqual(TEXT("vertical offset"), M->HitboxOffsetY, C.OffsetY * U);
        TestEqual(TEXT("forward half extent"), M->HitboxHalfWidth, C.Forward * U);
        TestEqual(TEXT("lateral half extent"), M->HitboxHalfExtentZ, C.Lateral * U);
        TestEqual(TEXT("vertical half extent"), M->HitboxHalfHeight, C.Vertical * U);
        TestEqual(TEXT("guard height"), M->GuardHeight, C.Guard);
        TestEqual(TEXT("parry height"), M->ParryHeight, C.Parry);
        TestEqual(TEXT("uses committed oriented volume"), M->HitVolumeShape, EHitVolumeShape::OrientedBox);
        FMatchState InputState;
        InputState.Fighter2.PositionX = 300 * U;
        Step(InputState, Button(C.Input));
        TestEqual(TEXT("dedicated prototype input starts its move"), InputState.Fighter1.CurrentMoveId, C.Id);

        FMatchState Snapshot = Active(C.Id);
        const FMatchState Saved = Snapshot;
        for (int32 Frame = 0; Frame < 80; ++Frame) Step(Snapshot);
        const FMatchState Expected = Snapshot;
        Snapshot = Saved;
        for (int32 Frame = 0; Frame < 80; ++Frame) Step(Snapshot);
        TestTrue(TEXT("snapshot restore replays this archetype exactly"), Snapshot == Expected);
    }
    TestEqual(TEXT("NarrowThrust has correct high parry"), ResolveParry(EMoveId::NarrowThrust, EFighterInputButton::HighParry), true);
    TestEqual(TEXT("NarrowThrust rejects wrong low parry"), ResolveParry(EMoveId::NarrowThrust, EFighterInputButton::LowParry), false);
    TestEqual(TEXT("NarrowThrust blocks"), ResolveBlock(EMoveId::NarrowThrust, false), true);
    TestEqual(TEXT("BroadSweep has correct high parry"), ResolveParry(EMoveId::BroadSweep, EFighterInputButton::HighParry), true);
    TestEqual(TEXT("BroadSweep rejects wrong low parry"), ResolveParry(EMoveId::BroadSweep, EFighterInputButton::LowParry), false);
    TestEqual(TEXT("BroadSweep standing-blocks"), ResolveBlock(EMoveId::BroadSweep, false), true);
    TestEqual(TEXT("BroadSweep passes over crouching Block"), ResolveBlock(EMoveId::BroadSweep, true), false);
    TestEqual(TEXT("Slow strike has correct high parry"), ResolveParry(EMoveId::SlowCommittedStrike, EFighterInputButton::HighParry), true);
    TestEqual(TEXT("Slow strike blocks"), ResolveBlock(EMoveId::SlowCommittedStrike, false), true);
    FMatchState WrongThrust = Active(EMoveId::NarrowThrust);
    Step(WrongThrust, {}, Button(EFighterInputButton::LowParry));
    TestEqual(TEXT("wrong Thrust height resolves as ordinary Hit"), WrongThrust.Fighter2.PendingReaction, EPendingReaction::Hit);
    TestEqual(TEXT("wrong Thrust height does not suppress contact"), WrongThrust.HitEventCount, uint32(1));
    FMatchState WrongSweep = Active(EMoveId::BroadSweep);
    Step(WrongSweep, {}, Button(EFighterInputButton::LowParry));
    TestEqual(TEXT("wrong Sweep height resolves as ordinary Hit"), WrongSweep.Fighter2.PendingReaction, EPendingReaction::Hit);
    TestEqual(TEXT("wrong Sweep height does not suppress contact"), WrongSweep.HitEventCount, uint32(1));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatArchetypeGeometryTest, "ProjW.Simulation.CombatLab.ArchetypeGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCombatArchetypeGeometryTest::RunTest(const FString&)
{
    FMatchState Neutral = Active(EMoveId::NarrowThrust, 120 * U);
    Step(Neutral);
    TestEqual(TEXT("no defense is hit by NarrowThrust"), Neutral.HitEventCount, uint32(1));
    for (const TPair<EMoveId, int32> Case : {
        TPair<EMoveId, int32>{EMoveId::NarrowThrust, 110 * U},
        TPair<EMoveId, int32>{EMoveId::BroadSweep, 100 * U},
        TPair<EMoveId, int32>{EMoveId::SlowCommittedStrike, 110 * U}})
    {
        FMatchState Timed;
        Timed.Fighter2.PositionX = Case.Value;
        Timed.Fighter2.Facing = EFighterFacing::Left;
        const EFighterInputButton Attack = Case.Key == EMoveId::NarrowThrust
            ? EFighterInputButton::NarrowThrust : Case.Key == EMoveId::BroadSweep
                ? EFighterInputButton::BroadSweep : EFighterInputButton::SlowCommittedStrike;
        Step(Timed, Button(Attack));
        const uint8 Startup = Sim::FindMoveDefinition(Case.Key)->StartupFrames;
        for (uint8 Frame = 0; Frame < Startup; ++Frame) Step(Timed);
        TestEqual(TEXT("unopposed committed prototype hits at tested range"), Timed.HitEventCount, uint32(1));
    }
    const FPivotResult ThrustPivot = ResolvePivot(EMoveId::NarrowThrust);
    TestFalse(TEXT("NarrowThrust Pivot creates no hit"), ThrustPivot.bHit);
    TestTrue(TEXT("NarrowThrust Pivot ends before active contact"), ThrustPivot.bPivotEnded);
    TestTrue(TEXT("NarrowThrust defender physically moved in depth"), ThrustPivot.bPivotMoved);
    TestFalse(TEXT("NarrowThrust whiff is strict volume non-overlap"), ThrustPivot.bOverlapAtContact);
    const FPivotResult SweepPivot = ResolvePivot(EMoveId::BroadSweep);
    TestTrue(TEXT("same Pivot timing is caught by broad Sweep geometry"), SweepPivot.bHit);
    TestTrue(TEXT("BroadSweep defender physically moved in depth"), SweepPivot.bPivotMoved);
    TestTrue(TEXT("BroadSweep still overlaps the Pivot hurt volume"), SweepPivot.bOverlapAtContact);

    FMatchState Sweep = Active(EMoveId::BroadSweep, 100 * U);
    Step(Sweep);
    TestEqual(TEXT("no defense is hit by BroadSweep"), Sweep.HitEventCount, uint32(1));

    FMatchState SlowHit = Active(EMoveId::SlowCommittedStrike, 110 * U);
    Step(SlowHit);
    TestEqual(TEXT("slow strike reaches intended range"), SlowHit.HitEventCount, uint32(1));
    FMatchState SlowWhiff = Active(EMoveId::SlowCommittedStrike, 150 * U);
    Step(SlowWhiff);
    TestEqual(TEXT("slow strike misses outside committed geometry"), SlowWhiff.HitEventCount, uint32(0));

    FMatchState Whiff = Active(EMoveId::SlowCommittedStrike, 200 * U);
    Step(Whiff);
    TestEqual(TEXT("slow strike advances to active without a target"), Whiff.HitEventCount, uint32(0));
    for (int32 I = 1; I < 24 + 3; ++I) Step(Whiff);
    TestTrue(TEXT("slow strike retains meaningful recovery after whiff"),
        Whiff.Fighter1.CurrentState == EFighterState::Attack && Whiff.Fighter1.MovePhase == EMovePhase::Recovery);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrouchDuckGeometryTest, "ProjW.Simulation.CombatLab.CrouchDuckGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrouchDuckGeometryTest::RunTest(const FString&)
{
    FFighterState CrouchingFighter;
    CrouchingFighter.CurrentState = EFighterState::Crouching;
    TestEqual(TEXT("crouching hurtbox uses its authoritative low height"),
        Sim::GetHurtbox(CrouchingFighter).MaxY, Sim::CrouchingHurtboxHeight);
    for (const EMoveId Move : {EMoveId::Light, EMoveId::Medium, EMoveId::Heavy})
    {
        FMatchState S = Active(Move, 60 * U);
        S.Fighter2.CurrentState = EFighterState::Crouching;
        Step(S, {}, Button(EFighterInputButton::Down));
        TestEqual(TEXT("standing-height normal strictly whiffs over crouch"), S.HitEventCount, uint32(0));
        TestFalse(TEXT("ducking does not consume the attack"), S.Fighter1.bCurrentMoveConnected);
    }
    FMatchState Rupture = Active(EMoveId::GroundRupture, 70 * U);
    Rupture.Fighter2.CurrentState = EFighterState::Crouching;
    Step(Rupture, {}, Button(EFighterInputButton::Down));
    TestEqual(TEXT("low radial GroundRupture overlaps crouch"), Rupture.HitEventCount, uint32(1));
    FMatchState Sweep = Active(EMoveId::BroadSweep, 100 * U);
    Sweep.Fighter2.CurrentState = EFighterState::Crouching;
    Step(Sweep, {}, Button(EFighterInputButton::Down));
    TestEqual(TEXT("standing-height BroadSweep passes over crouch"), Sweep.HitEventCount, uint32(0));
    return !HasAnyErrors();
}
#endif
