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

    FMatchState AttackAt(EMoveId Id, int32 DefenderX = 60 * U)
    {
        FMatchState S;
        S.Fighter1.PositionX = 0;
        S.Fighter1.CurrentState = EFighterState::Attack;
        S.Fighter1.CurrentMoveId = Id;
        S.Fighter1.MovePhase = EMovePhase::Active;
        S.Fighter2.PositionX = DefenderX;
        S.Fighter2.Facing = EFighterFacing::Left;
        return S;
    }

    bool StepIntoParry(FMatchState& S, EFighterInputButton Height)
    {
        Step(S, {}, Button(Height));
        return S.Fighter2.CurrentState == EFighterState::ParrySuccess;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParryDefinitionsTest, "ProjW.Simulation.Parry.Definitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FParryDefinitionsTest::RunTest(const FString&)
{
    TestEqual(TEXT("input remains compact"), sizeof(FFighterInput), size_t(2));
    TestTrue(TEXT("high and low parry use distinct bits"),
        static_cast<uint16>(EFighterInputButton::HighParry) != static_cast<uint16>(EFighterInputButton::LowParry));
    TestEqual(TEXT("14 committed frames"), Sim::ParryTotalFrames, uint8(14));
    TestEqual(TEXT("4 active frames"), Sim::ParryActiveFrames, uint8(4));
    TestEqual(TEXT("10 recovery frames"), Sim::ParryRecoveryFrames, uint8(10));
    TestEqual(TEXT("successful contact freeze"), Sim::ParryHitstopFrames, uint8(6));
    TestEqual(TEXT("attacker stun"), Sim::ParryStunFrames, uint8(18));
    const FMoveDefinition* Rupture = Sim::FindMoveDefinition(EMoveId::GroundRupture);
    TestNotNull(TEXT("GroundRupture definition"), Rupture);
    if (Rupture)
    {
        TestEqual(TEXT("rupture startup"), Rupture->StartupFrames, uint8(18));
        TestEqual(TEXT("rupture active"), Rupture->ActiveFrames, uint8(5));
        TestEqual(TEXT("rupture recovery"), Rupture->RecoveryFrames, uint8(24));
        TestEqual(TEXT("rupture damage"), Rupture->Damage, 100);
        TestEqual(TEXT("rupture radius"), Rupture->HitRadius, 112 * U);
        TestEqual(TEXT("rupture is geometric circle"), Rupture->HitVolumeShape, EHitVolumeShape::HorizontalCircle);
        TestEqual(TEXT("GroundRupture is the Low-Parry target"),
            Rupture->ParryHeight, EParryHeight::Low);
        TestEqual(TEXT("existing Low guard remains distinct"), Rupture->GuardHeight, EGuardHeight::Low);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParryLifecycleTest, "ProjW.Simulation.Parry.ExactLifecycleAndLocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FParryLifecycleTest::RunTest(const FString&)
{
    for (const auto Pair : {TPair<EFighterInputButton, EParryHeight>{EFighterInputButton::HighParry, EParryHeight::High},
        TPair<EFighterInputButton, EParryHeight>{EFighterInputButton::LowParry, EParryHeight::Low}})
    {
        FMatchState S;
        S.Fighter2.PositionX = 300 * U;
        Step(S, Button(Pair.Key));
        TestEqual(TEXT("Parry enters committed state on fresh press"), S.Fighter1.CurrentState, EFighterState::Parrying);
        TestEqual(TEXT("choice captured on entry"), S.Fighter1.ParryHeight, Pair.Value);
        TestEqual(TEXT("active entry frame zero"), S.Fighter1.StateFrame, uint32(0));
        for (uint32 ActiveFrame = 1; ActiveFrame < Sim::ParryActiveFrames; ++ActiveFrame)
        {
            Step(S);
            TestEqual(TEXT("exact active frame"), S.Fighter1.StateFrame, ActiveFrame);
            TestEqual(TEXT("remains committed during active"), S.Fighter1.CurrentState, EFighterState::Parrying);
        }
        // Attempt movement, jump, attack, Pivot, and the opposite Parry while committed.
        const uint16 Attempts = static_cast<uint16>(EFighterInputButton::Right)
            | static_cast<uint16>(EFighterInputButton::Up) | static_cast<uint16>(EFighterInputButton::Light)
            | static_cast<uint16>(EFighterInputButton::PivotPositive)
            | static_cast<uint16>(Pair.Key == EFighterInputButton::HighParry
                ? EFighterInputButton::LowParry : EFighterInputButton::HighParry);
        Step(S, {Attempts});
        TestEqual(TEXT("frame four begins recovery"), S.Fighter1.StateFrame, uint32(4));
        for (uint32 StateFrame = 5; StateFrame < Sim::ParryTotalFrames; ++StateFrame)
        {
            Step(S);
            TestEqual(TEXT("exact recovery state frame"), S.Fighter1.StateFrame, StateFrame);
            TestEqual(TEXT("recovery remains committed"), S.Fighter1.CurrentState, EFighterState::Parrying);
        }
        Step(S);
        TestEqual(TEXT("exits after exactly 14 state frames"), S.Fighter1.CurrentState, EFighterState::Idle);
        TestEqual(TEXT("parry choice clears on exit"), S.Fighter1.ParryHeight, EParryHeight::None);
        TestEqual(TEXT("attempted Light was not buffered"), S.Fighter1.CurrentMoveId, EMoveId::None);
        TestEqual(TEXT("committed parry has no movement"), S.Fighter1.PositionX, 0);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParryHeightMatrixTest, "ProjW.Simulation.Parry.HeightMatrix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FParryHeightMatrixTest::RunTest(const FString&)
{
    struct FCase { EMoveId Move; EFighterInputButton Input; bool bParries; };
    const FCase Cases[] = {
        {EMoveId::Light, EFighterInputButton::HighParry, true},
        {EMoveId::Light, EFighterInputButton::LowParry, false},
        {EMoveId::Medium, EFighterInputButton::HighParry, true},
        {EMoveId::Medium, EFighterInputButton::LowParry, false},
        {EMoveId::Heavy, EFighterInputButton::HighParry, true},
        {EMoveId::Heavy, EFighterInputButton::LowParry, false},
        {EMoveId::GroundRupture, EFighterInputButton::HighParry, false},
        {EMoveId::GroundRupture, EFighterInputButton::LowParry, true}
    };
    for (const FCase& C : Cases)
    {
        for (const bool bPlayer1Defends : {true, false})
        {
            FMatchState S = AttackAt(C.Move);
            FFighterState& Attacker = bPlayer1Defends ? S.Fighter2 : S.Fighter1;
            FFighterState& Defender = bPlayer1Defends ? S.Fighter1 : S.Fighter2;
            if (bPlayer1Defends)
            {
                Attacker.PositionX = 60 * U;
                Attacker.Facing = EFighterFacing::Left;
                Defender.PositionX = 0;
                Defender.Facing = EFighterFacing::Right;
                S.Fighter1.CurrentState = EFighterState::Idle;
                S.Fighter1.CurrentMoveId = EMoveId::None;
                S.Fighter1.MovePhase = EMovePhase::Startup;
                S.Fighter2.CurrentState = EFighterState::Attack;
                S.Fighter2.CurrentMoveId = C.Move;
                S.Fighter2.MovePhase = EMovePhase::Active;
            }
            bool bSuccess = false;
            if (bPlayer1Defends)
            {
                Step(S, Button(C.Input), {});
                bSuccess = Defender.CurrentState == EFighterState::ParrySuccess;
            }
            else
            {
                bSuccess = StepIntoParry(S, C.Input);
            }
            const FString Side = bPlayer1Defends ? TEXT("P1 defends") : TEXT("P2 defends");
            TestEqual(*FString::Printf(TEXT("%s: matching height alone determines Parry result"), *Side),
                bSuccess, C.bParries);
            if (C.bParries)
            {
                TestEqual(*FString::Printf(TEXT("%s: defender takes no damage"), *Side),
                    Defender.Health, FFighterState::MaxHealth);
                TestEqual(*FString::Printf(TEXT("%s: attacker receives ParryStun"), *Side),
                    Attacker.PendingReaction, EPendingReaction::Parry);
            }
            else
            {
                TestEqual(*FString::Printf(TEXT("%s: wrong height is ordinary Hit"), *Side),
                    Defender.Health, FFighterState::MaxHealth - Sim::FindMoveDefinition(C.Move)->Damage);
                TestEqual(*FString::Printf(TEXT("%s: wrong height captures Hit reaction"), *Side),
                    Defender.PendingReaction, EPendingReaction::Hit);
            }
        }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParryConsequencesTest, "ProjW.Simulation.Parry.ConsequencesAndReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FParryConsequencesTest::RunTest(const FString&)
{
    FMatchState S = AttackAt(EMoveId::Light);
    TestTrue(TEXT("one-way Hit/Parry contact is reachable"), StepIntoParry(S, EFighterInputButton::HighParry));
    TestEqual(TEXT("no damage to successful defender"), S.Fighter2.Health, FFighterState::MaxHealth);
    TestEqual(TEXT("no chip on successful defender"), S.Fighter2.PendingReaction, EPendingReaction::None);
    TestEqual(TEXT("no defender Hitstun"), S.Fighter2.CurrentState, EFighterState::ParrySuccess);
    TestEqual(TEXT("no defender Blockstun"), S.Fighter2.PendingKnockbackVelocityX, 0);
    TestEqual(TEXT("shared six frame freeze on attacker"), S.Fighter1.HitstopFramesRemaining, uint8(6));
    TestEqual(TEXT("shared six frame freeze on defender"), S.Fighter2.HitstopFramesRemaining, uint8(6));
    TestEqual(TEXT("attacker ParryStun pending"), S.Fighter1.PendingReaction, EPendingReaction::Parry);
    TestEqual(TEXT("exact attacker stun duration"), S.Fighter1.PendingReactionFrames, uint8(18));

    FMatchState Reversed;
    Reversed.Fighter1.PositionX = 0;
    Reversed.Fighter1.Facing = EFighterFacing::Right;
    Reversed.Fighter2.PositionX = 60 * U;
    Reversed.Fighter2.Facing = EFighterFacing::Left;
    Reversed.Fighter2.CurrentState = EFighterState::Attack;
    Reversed.Fighter2.CurrentMoveId = EMoveId::Light;
    Reversed.Fighter2.MovePhase = EMovePhase::Active;
    Step(Reversed, Button(EFighterInputButton::HighParry));
    TestEqual(TEXT("reversed assignment resolves Parry/Hit direction"),
        Reversed.Fighter1.CurrentState, EFighterState::ParrySuccess);
    TestEqual(TEXT("reversed attacker receives ParryStun"), Reversed.Fighter2.PendingReaction,
        EPendingReaction::Parry);

    const FMatchState Snapshot = S;
    FMatchState A = Snapshot, B = Snapshot;
    for (int32 I = 0; I < 40; ++I) { Step(A); Step(B); }
    TestTrue(TEXT("successful Parry snapshot replay exact"), A == B);
    for (int32 I = 0; I < Sim::ParryHitstopFrames; ++I) Step(S);
    Step(S); // First simulation frame after freeze transfers the pending reaction.
    TestEqual(TEXT("attacker enters ParryStun after shared freeze"), S.Fighter1.CurrentState, EFighterState::ParryStun);
    TestEqual(TEXT("defender returns actionable after shared freeze"), S.Fighter2.CurrentState, EFighterState::Idle);
    for (int32 I = 0; I < Sim::ParryStunFrames; ++I) Step(S);
    TestTrue(TEXT("ParryStun exits after its exact duration"), S.Fighter1.CurrentState != EFighterState::ParryStun);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParryFailureTest, "ProjW.Simulation.Parry.FailureAndRecoveryVulnerability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FParryFailureTest::RunTest(const FString&)
{
    FMatchState LastActive = AttackAt(EMoveId::Light);
    LastActive.Fighter2.CurrentState = EFighterState::Parrying;
    LastActive.Fighter2.ParryHeight = EParryHeight::High;
    LastActive.Fighter2.StateFrame = Sim::ParryActiveFrames - 2;
    Step(LastActive);
    TestEqual(TEXT("last active Parry frame still succeeds"), LastActive.Fighter2.CurrentState,
        EFighterState::ParrySuccess);

    FMatchState S = AttackAt(EMoveId::Light);
    S.Fighter2.CurrentState = EFighterState::Parrying;
    S.Fighter2.ParryHeight = EParryHeight::Low; // Wrong height.
    S.Fighter2.StateFrame = 0;
    Step(S, {}, Button(EFighterInputButton::Right)); // Away is held: Parry does not fall back to Block.
    TestEqual(TEXT("wrong height hits during active window"), S.Fighter2.PendingReaction, EPendingReaction::Hit);
    TestEqual(TEXT("failed parry takes full damage"), S.Fighter2.Health,
        FFighterState::MaxHealth - Sim::FindMoveDefinition(EMoveId::Light)->Damage);

    FMatchState Recovery = AttackAt(EMoveId::Light);
    Recovery.Fighter2.CurrentState = EFighterState::Parrying;
    Recovery.Fighter2.ParryHeight = EParryHeight::High;
    Recovery.Fighter2.StateFrame = Sim::ParryActiveFrames;
    Step(Recovery, {}, Button(EFighterInputButton::Right));
    TestEqual(TEXT("recovery contact is full Hit"), Recovery.Fighter2.PendingReaction, EPendingReaction::Hit);
    TestEqual(TEXT("recovery contact is not Block"), Recovery.Fighter2.CurrentState, EFighterState::Parrying);

    FMatchState Before = AttackAt(EMoveId::Light);
    Before.Fighter2.CurrentState = EFighterState::Parrying;
    Before.Fighter2.ParryHeight = EParryHeight::High;
    Before.Fighter2.StateFrame = 4;
    Step(Before);
    TestEqual(TEXT("late committed parry is vulnerable before active timing"),
        Before.Fighter2.PendingReaction, EPendingReaction::Hit);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundRuptureGeometryTest, "ProjW.Simulation.GroundRupture.StrictRadialGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGroundRuptureGeometryTest::RunTest(const FString&)
{
    constexpr int32 R = Sim::GroundRuptureRadius;
    const FSimRadialVolume Circle{0, 0, 0, 10, R};
    TestTrue(TEXT("near-center rectangle overlaps"), Circle.Overlaps({10, 20, 1, 2, 10, 20}));
    TestFalse(TEXT("strict radial tangent misses"), Circle.Overlaps({R, R + 10, 1, 2, -10, 10}));
    TestTrue(TEXT("one subunit inside radial edge overlaps"), Circle.Overlaps({R - 1, R + 10, 1, 2, -10, 10}));
    TestTrue(TEXT("diagonal corner within radius overlaps"), Circle.Overlaps({70 * U, 80 * U, 1, 2, 70 * U, 80 * U}));
    TestFalse(TEXT("diagonal corner outside radius misses"), Circle.Overlaps({80 * U, 90 * U, 1, 2, 80 * U, 90 * U}));
    TestFalse(TEXT("vertical edge touch misses"), Circle.Overlaps({0, 1, 10, 20, 0, 1}));
    const FSimRadialVolume Rotated{0, 0, 0, 10, R};
    const bool bAxisAligned = Rotated.Overlaps({60 * U, 70 * U, 1, 2, 80 * U, 90 * U});
    const bool bSwappedAxes = Rotated.Overlaps({80 * U, 90 * U, 1, 2, 60 * U, 70 * U});
    TestTrue(TEXT("off-axis rectangle overlaps"), bAxisAligned);
    TestEqual(TEXT("swap X/Z preserves radial result"), bSwappedAxes, bAxisAligned);

    FMatchState Near = AttackAt(EMoveId::GroundRupture, 60 * U);
    Step(Near);
    TestEqual(TEXT("near GroundRupture hits"), Near.HitEventCount, uint32(1));
    FMatchState Far = AttackAt(EMoveId::GroundRupture, 140 * U);
    Step(Far);
    TestEqual(TEXT("outside GroundRupture radius misses"), Far.HitEventCount, uint32(0));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundRuptureLifecycleTest, "ProjW.Simulation.GroundRupture.ExactLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGroundRuptureLifecycleTest::RunTest(const FString&)
{
    FMatchState S = AttackAt(EMoveId::GroundRupture, 300 * U);
    S.Fighter1.CurrentState = EFighterState::Idle;
    S.Fighter1.CurrentMoveId = EMoveId::None;
    Step(S, Button(EFighterInputButton::GroundRupture));
    TestEqual(TEXT("rupture startup enters frame zero"), S.Fighter1.StateFrame, uint32(0));
    FSimRadialVolume Volume;
    TestFalse(TEXT("startup has no radial volume"), Sim::GetMoveRadialVolume(S.Fighter1, Volume));
    for (uint32 Frame = 1; Frame < 18; ++Frame)
    {
        Step(S);
        TestEqual(TEXT("exact startup frame"), S.Fighter1.StateFrame, Frame);
        TestFalse(TEXT("startup remains non-active"), Sim::GetMoveRadialVolume(S.Fighter1, Volume));
    }
    Step(S);
    TestEqual(TEXT("active phase entry frame zero"), S.Fighter1.StateFrame, uint32(0));
    for (uint32 ActiveFrame = 0; ActiveFrame < 5; ++ActiveFrame)
    {
        TestTrue(TEXT("all five active frames expose radial volume"), Sim::GetMoveRadialVolume(S.Fighter1, Volume));
        TestEqual(TEXT("authoritative radius drives volume"), Volume.Radius, Sim::GroundRuptureRadius);
        Step(S);
    }
    TestEqual(TEXT("recovery begins after five active frames"), S.Fighter1.MovePhase, EMovePhase::Recovery);
    TestFalse(TEXT("recovery has no radial volume"), Sim::GetMoveRadialVolume(S.Fighter1, Volume));
    for (int32 Frame = 0; Frame < 24; ++Frame) Step(S);
    TestEqual(TEXT("move exits after 18/5/24 lifecycle"), S.Fighter1.CurrentState, EFighterState::Idle);
    TestEqual(TEXT("completed rupture clears move ID"), S.Fighter1.CurrentMoveId, EMoveId::None);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeavyDefenseComparisonTest, "ProjW.Simulation.Parry.HeavyDefenseComparison",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHeavyDefenseComparisonTest::RunTest(const FString&)
{
    FMatchState Neutral = AttackAt(EMoveId::Heavy, 85 * U);
    Step(Neutral);
    TestEqual(TEXT("neutral defender is hit by Heavy"), Neutral.Fighter2.PendingReaction, EPendingReaction::Hit);

    FMatchState Block = AttackAt(EMoveId::Heavy, 85 * U);
    Step(Block, {}, Button(EFighterInputButton::Right));
    TestEqual(TEXT("matching High Block answers Heavy at a cost"), Block.Fighter2.PendingReaction, EPendingReaction::Block);

    FMatchState Parry = AttackAt(EMoveId::Heavy, 85 * U);
    TestTrue(TEXT("correctly timed HighParry answers Heavy"), StepIntoParry(Parry, EFighterInputButton::HighParry));

    FMatchState Pivot;
    Pivot.Fighter2.PositionX = 85 * U;
    Pivot.Fighter1.CurrentState = EFighterState::Attack;
    Pivot.Fighter1.CurrentMoveId = EMoveId::Heavy;
    Pivot.Fighter1.MovePhase = EMovePhase::Startup;
    Step(Pivot, {}, Button(EFighterInputButton::PivotPositive));
    for (int32 Frame = 1; Frame <= Sim::FindMoveDefinition(EMoveId::Heavy)->StartupFrames; ++Frame) Step(Pivot);
    TestEqual(TEXT("Pivot can evade narrow Heavy through geometry"), Pivot.HitEventCount, uint32(0));
    TestTrue(TEXT("Pivot physically left the committed plane"), FMath::Abs(Pivot.Fighter2.PositionZ) > 10 * U);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundRuptureDefenseComparisonTest, "ProjW.Simulation.GroundRupture.DefenseComparison",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGroundRuptureDefenseComparisonTest::RunTest(const FString&)
{
    FMatchState Block = AttackAt(EMoveId::GroundRupture, 70 * U);
    Step(Block, {}, {static_cast<uint16>(EFighterInputButton::Right) | static_cast<uint16>(EFighterInputButton::Down)});
    TestEqual(TEXT("low Block remains available concession"), Block.Fighter2.PendingReaction, EPendingReaction::Block);
    TestEqual(TEXT("GroundRupture block chip preserved"), Block.Fighter2.Health,
        FFighterState::MaxHealth - Sim::FindMoveDefinition(EMoveId::GroundRupture)->ChipDamage);

    FMatchState Parry = AttackAt(EMoveId::GroundRupture, 70 * U);
    TestFalse(TEXT("HighParry does not answer GroundRupture"), StepIntoParry(Parry, EFighterInputButton::HighParry));
    TestEqual(TEXT("wrong-height HighParry takes ordinary GroundRupture hit"),
        Parry.Fighter2.PendingReaction, EPendingReaction::Hit);

    FMatchState LowParry = AttackAt(EMoveId::GroundRupture, 70 * U);
    TestTrue(TEXT("LowParry answers GroundRupture"), StepIntoParry(LowParry, EFighterInputButton::LowParry));

    FMatchState Pivot = AttackAt(EMoveId::GroundRupture, 70 * U);
    Step(Pivot, {}, Button(EFighterInputButton::PivotPositive));
    TestEqual(TEXT("nearby Pivot remains inside actual radius and is hit"), Pivot.Fighter2.PendingReaction,
        EPendingReaction::Hit);

    FMatchState Outside = AttackAt(EMoveId::GroundRupture, 140 * U);
    Step(Outside, {}, Button(EFighterInputButton::PivotPositive));
    TestEqual(TEXT("Pivot outside actual circle is not magically hit"), Outside.HitEventCount, uint32(0));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLowGuardHeightMatrixTest, "ProjW.Simulation.Block.LowGuardHeightMatrix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLowGuardHeightMatrixTest::RunTest(const FString&)
{
    for (const EMoveId MoveId : {EMoveId::GroundRupture})
    {
        const FMoveDefinition& Move = *Sim::FindMoveDefinition(MoveId);
        FMatchState Standing = AttackAt(MoveId, 70 * U);
        Step(Standing, {}, Button(EFighterInputButton::Right)); // Defender faces Left; Right is Away.
        TestEqual(TEXT("standing guard does not block a Low attack"),
            Standing.Fighter2.PendingReaction, EPendingReaction::Hit);
        TestEqual(TEXT("standing guard takes full Low-attack damage"), Standing.Fighter2.Health,
            FFighterState::MaxHealth - Move.Damage);

        FMatchState Crouching = AttackAt(MoveId, 70 * U);
        const uint16 DownAway = static_cast<uint16>(EFighterInputButton::Down)
            | static_cast<uint16>(EFighterInputButton::Right);
        Step(Crouching, {}, {DownAway});
        TestEqual(TEXT("crouching Away guard blocks a Low attack"),
            Crouching.Fighter2.PendingReaction, EPendingReaction::Block);
        TestEqual(TEXT("crouching Low guard takes chip only"), Crouching.Fighter2.Health,
            FFighterState::MaxHealth - Move.ChipDamage);
        TestEqual(TEXT("crouching Low guard captures Blockstun"), Crouching.Fighter2.PendingReactionFrames,
            Move.BlockstunFrames);
        TestTrue(TEXT("crouching Low guard receives pushback"),
            Crouching.Fighter2.PendingKnockbackVelocityX > 0);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParrySnapshotDeterminismTest, "ProjW.Simulation.Parry.SnapshotDeterminism",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FParrySnapshotDeterminismTest::RunTest(const FString&)
{
    FMatchState A, B;
    Step(A, Button(EFighterInputButton::LowParry));
    Step(B, Button(EFighterInputButton::LowParry));
    Step(A, Button(EFighterInputButton::Light)); Step(B, Button(EFighterInputButton::Light));
    const FMatchState Snap = A;
    bool bSawParry = Snap.Fighter1.CurrentState == EFighterState::Parrying;
    for (int32 I = 0; I < 5000; ++I)
    {
        const FFighterInput P1 = I % 47 == 0 ? Button(EFighterInputButton::HighParry)
            : I % 41 == 0 ? Button(EFighterInputButton::GroundRupture)
            : I % 13 < 4 ? Button(EFighterInputButton::Right) : FFighterInput{};
        const FFighterInput P2 = I % 53 == 0 ? Button(EFighterInputButton::LowParry)
            : I % 37 == 0 ? Button(EFighterInputButton::GroundRupture)
            : I % 17 < 5 ? Button(EFighterInputButton::Left) : FFighterInput{};
        Step(A, P1, P2); Step(B, P1, P2);
        if (A != B) { AddError(FString::Printf(TEXT("repeatability diverged at %d"), I)); return false; }
        bSawParry |= A.Fighter1.CurrentState == EFighterState::Parrying
            || A.Fighter2.CurrentState == EFighterState::Parrying
            || A.Fighter1.CurrentState == EFighterState::ParrySuccess
            || A.Fighter2.CurrentState == EFighterState::ParrySuccess
            || A.Fighter1.CurrentState == EFighterState::ParryStun
            || A.Fighter2.CurrentState == EFighterState::ParryStun;
    }
    FMatchState Replayed = Snap;
    // Replay the same deterministic input sequence from the intermediate snapshot.
    for (int32 I = 0; I < 5000; ++I)
    {
        const FFighterInput P1 = I % 47 == 0 ? Button(EFighterInputButton::HighParry)
            : I % 41 == 0 ? Button(EFighterInputButton::GroundRupture)
            : I % 13 < 4 ? Button(EFighterInputButton::Right) : FFighterInput{};
        const FFighterInput P2 = I % 53 == 0 ? Button(EFighterInputButton::LowParry)
            : I % 37 == 0 ? Button(EFighterInputButton::GroundRupture)
            : I % 17 < 5 ? Button(EFighterInputButton::Left) : FFighterInput{};
        Step(Replayed, P1, P2);
    }
    TestTrue(TEXT("snapshot restore and replay exact"), Replayed == A);
    TestTrue(TEXT("Parry was encountered in deterministic sequence"), bSawParry);
    return !HasAnyErrors();
}

#endif
