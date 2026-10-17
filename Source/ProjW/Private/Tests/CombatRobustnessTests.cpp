#include "Simulation/FightingSimulation.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
FFighterInput Buttons(uint16 B) { FFighterInput I; I.Buttons = B; return I; }
constexpr uint16 Light = static_cast<uint16>(EFighterInputButton::Light);
void Place(FMatchState& S, int32 A, int32 D)
{
    S.Fighter1.PositionX = A * FFightingSimulation::SubunitsPerWorldUnit;
    S.Fighter2.PositionX = D * FFightingSimulation::SubunitsPerWorldUnit;
    S.Fighter1.Facing = EFighterFacing::Right; S.Fighter2.Facing = EFighterFacing::Left;
}
void ReachActive(FMatchState& S)
{
    FFightingSimulation::SimulateFrame(S, Buttons(Light), {});
    for (int32 I = 1; I < FFightingSimulation::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++I) FFightingSimulation::SimulateFrame(S, Buttons(Light), {});
    FFightingSimulation::SimulateFrame(S, Buttons(Light), {});
}
void ReachTradeActive(FMatchState& S)
{
    FFightingSimulation::SimulateFrame(S, Buttons(Light), Buttons(Light));
    for (int32 I = 1; I < FFightingSimulation::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++I) FFightingSimulation::SimulateFrame(S, Buttons(Light), Buttons(Light));
    FFightingSimulation::SimulateFrame(S, Buttons(Light), Buttons(Light));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatSymmetryTest, "ProjW.Simulation.Combat.Symmetry", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatSymmetryTest::RunTest(const FString&)
{
    FMatchState A, B; Place(A, -100, -40); Place(B, 100, 40); B.Fighter1.Facing=EFighterFacing::Left; B.Fighter2.Facing=EFighterFacing::Right;
    ReachActive(A); ReachActive(B); TestEqual(TEXT("mirrored hit events"), A.HitEventCount, B.HitEventCount);
    TestEqual(TEXT("F1 health mirrored"), A.Fighter1.Health, B.Fighter1.Health); TestEqual(TEXT("F2 health mirrored"), A.Fighter2.Health, B.Fighter2.Health);
    TestEqual(TEXT("F1 attack flag mirrored"), A.Fighter1.bCurrentMoveConnected, B.Fighter1.bCurrentMoveConnected); TestEqual(TEXT("F2 attack flag mirrored"), A.Fighter2.bCurrentMoveConnected, B.Fighter2.bCurrentMoveConnected);
    TestEqual(TEXT("F1 hitstop mirrored"), A.Fighter1.HitstopFramesRemaining, B.Fighter1.HitstopFramesRemaining); TestEqual(TEXT("F2 hitstop mirrored"), A.Fighter2.HitstopFramesRemaining, B.Fighter2.HitstopFramesRemaining);
    TestEqual(TEXT("F1 pending mirrored"), A.Fighter1.bPendingHitReaction, B.Fighter1.bPendingHitReaction); TestEqual(TEXT("F2 pending mirrored"), A.Fighter2.bPendingHitReaction, B.Fighter2.bPendingHitReaction);
    TestEqual(TEXT("knockback signs mirrored"), A.Fighter2.PendingKnockbackVelocityX, -B.Fighter2.PendingKnockbackVelocityX);
    for(int32 I=0;I<8;++I){FFightingSimulation::SimulateFrame(A,{},{});FFightingSimulation::SimulateFrame(B,{},{});} TestEqual(TEXT("hitstun entry mirrored"),A.Fighter2.CurrentState,B.Fighter2.CurrentState);
    for(int32 I=0;I<30;++I){FFightingSimulation::SimulateFrame(A,{},{});FFightingSimulation::SimulateFrame(B,{},{});} TestEqual(TEXT("recovery mirrored"),A.Fighter2.CurrentState,B.Fighter2.CurrentState); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatZeroHealthTest, "ProjW.Simulation.Combat.ZeroHealth", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatZeroHealthTest::RunTest(const FString&)
{ FMatchState S; Place(S,-100,-40); S.Fighter2.Health=1; ReachActive(S); TestEqual(TEXT("health clamps at zero"),S.Fighter2.Health,0); TestEqual(TEXT("single event"),S.HitEventCount,uint32(1)); return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatTradeSnapshotTest, "ProjW.Simulation.Combat.TradeSnapshot", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatTradeSnapshotTest::RunTest(const FString&)
{ FMatchState S; Place(S,-40,40); ReachTradeActive(S); TestEqual(TEXT("trade events"),S.HitEventCount,uint32(2)); TestEqual(TEXT("p1 damage"),S.Fighter1.Health,950); TestEqual(TEXT("p2 damage"),S.Fighter2.Health,950); const FMatchState Snap=S; FMatchState A=Snap,B=Snap; for(int32 I=0;I<40;++I){FFightingSimulation::SimulateFrame(A,{},{});FFightingSimulation::SimulateFrame(B,{},{});} TestTrue(TEXT("trade replay"),A==B); return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatStageEdgesTest, "ProjW.Simulation.Combat.StageEdges", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatStageEdgesTest::RunTest(const FString&)
{ FMatchState S; Place(S,FFightingSimulation::MinFighterX,FFightingSimulation::MinFighterX+40); for(int32 I=0;I<120;++I) FFightingSimulation::SimulateFrame(S,Buttons(static_cast<uint16>(EFighterInputButton::Left)),{}); TestEqual(TEXT("left bound"),S.Fighter1.PositionX,FFightingSimulation::MinFighterX); S.Fighter1.PositionX=FFightingSimulation::MaxFighterX; for(int32 I=0;I<120;++I) FFightingSimulation::SimulateFrame(S,Buttons(static_cast<uint16>(EFighterInputButton::Right)),{}); TestEqual(TEXT("right bound"),S.Fighter1.PositionX,FFightingSimulation::MaxFighterX);
    FMatchState L; Place(L,FFightingSimulation::MinFighterX/FFightingSimulation::SubunitsPerWorldUnit+40,FFightingSimulation::MinFighterX/FFightingSimulation::SubunitsPerWorldUnit); L.Fighter1.Facing=EFighterFacing::Left; L.Fighter2.Facing=EFighterFacing::Right; ReachActive(L); for(int32 I=0;I<200;++I) FFightingSimulation::SimulateFrame(L,{},{}); TestEqual(TEXT("combat left wall"),L.Fighter2.PositionX,FFightingSimulation::MinFighterX); TestEqual(TEXT("left wall velocity zero"),L.Fighter2.PendingKnockbackVelocityX,0); TestTrue(TEXT("left wall hitstun/recovery"),L.Fighter2.CurrentState==EFighterState::Hitstun||L.Fighter2.CurrentState==EFighterState::Idle);
    FMatchState R; Place(R,FFightingSimulation::MaxFighterX/FFightingSimulation::SubunitsPerWorldUnit-40,FFightingSimulation::MaxFighterX/FFightingSimulation::SubunitsPerWorldUnit); R.Fighter1.Facing=EFighterFacing::Right; R.Fighter2.Facing=EFighterFacing::Left; ReachActive(R); for(int32 I=0;I<200;++I) FFightingSimulation::SimulateFrame(R,{},{}); TestEqual(TEXT("combat right wall"),R.Fighter2.PositionX,FFightingSimulation::MaxFighterX); TestEqual(TEXT("right wall velocity zero"),R.Fighter2.PendingKnockbackVelocityX,0); TestTrue(TEXT("right wall hitstun/recovery"),R.Fighter2.CurrentState==EFighterState::Hitstun||R.Fighter2.CurrentState==EFighterState::Idle); return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatInputEdgesReactionTest, "ProjW.Simulation.Combat.InputEdgesThroughReaction", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatInputEdgesReactionTest::RunTest(const FString&)
{ FMatchState S; Place(S,-100,-40); ReachActive(S); const uint16 Held=Light|static_cast<uint16>(EFighterInputButton::Up)|static_cast<uint16>(EFighterInputButton::Right); for(int32 I=0;I<25;++I){FFightingSimulation::SimulateFrame(S,Buttons(Held),{}); TestFalse(TEXT("reaction cannot attack"),S.Fighter2.CurrentState==EFighterState::Attack);} TestTrue(TEXT("reaction completes"),S.Fighter2.CurrentState!=EFighterState::Hitstun); return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatSnapshotTortureTest, "ProjW.Simulation.Combat.SnapshotTorture", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatSnapshotTortureTest::RunTest(const FString&)
{ FMatchState S; Place(S,-100,-40); TArray<FMatchState> Snaps; for(int32 I=0;I<18;++I){FFightingSimulation::SimulateFrame(S,Buttons(I==0?Light:0),{}); if(I<5||I==7||I==10||I==17) Snaps.Add(S);} for(const FMatchState& Snap:Snaps){FMatchState A=Snap,B=Snap; for(int32 I=0;I<80;++I){const FFighterInput In=Buttons((I%19==0)?Light:((I%7<3)?static_cast<uint16>(EFighterInputButton::Right):0)); FFightingSimulation::SimulateFrame(A,In,{});FFightingSimulation::SimulateFrame(B,In,{});} TestTrue(TEXT("snapshot phase replay"),A==B);} return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatStress100KTest, "ProjW.Simulation.Combat.Stress100K", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCombatStress100KTest::RunTest(const FString&)
{
    FMatchState A, B;
    Place(A, -100, -40);
    B = A;
    uint32 Hits=0, Hitstop=0, Hitstun=0, Knockback=0, Bounds=0;
    uint32 Walking=0, Jump=0, DoubleJump=0, Dash=0;

    auto StepAndObserve = [&](uint16 P1, uint16 P2)
    {
        FFightingSimulation::SimulateFrame(A, Buttons(P1), Buttons(P2));
        FFightingSimulation::SimulateFrame(B, Buttons(P1), Buttons(P2));
        Hits += A.HitEventCount > 0;
        Hitstop += A.Fighter1.HitstopFramesRemaining > 0 || A.Fighter2.HitstopFramesRemaining > 0;
        Hitstun += A.Fighter1.CurrentState == EFighterState::Hitstun || A.Fighter2.CurrentState == EFighterState::Hitstun;
        Knockback += A.Fighter1.PendingKnockbackVelocityX != 0 || A.Fighter2.PendingKnockbackVelocityX != 0;
        Bounds += A.Fighter1.PositionX == FFightingSimulation::MinFighterX || A.Fighter1.PositionX == FFightingSimulation::MaxFighterX
            || A.Fighter2.PositionX == FFightingSimulation::MinFighterX || A.Fighter2.PositionX == FFightingSimulation::MaxFighterX;
        Walking += A.Fighter1.CurrentState == EFighterState::Walking || A.Fighter2.CurrentState == EFighterState::Walking;
        Jump += A.Fighter1.CurrentState == EFighterState::Jumping || A.Fighter2.CurrentState == EFighterState::Jumping;
        DoubleJump += A.Fighter1.CurrentState == EFighterState::DoubleJumping || A.Fighter2.CurrentState == EFighterState::DoubleJumping;
        Dash += A.Fighter1.CurrentState == EFighterState::ForwardDash || A.Fighter1.CurrentState == EFighterState::BackDash
            || A.Fighter2.CurrentState == EFighterState::ForwardDash || A.Fighter2.CurrentState == EFighterState::BackDash;
    };

    constexpr uint16 Right = static_cast<uint16>(EFighterInputButton::Right);
    StepAndObserve(Right, 0);
    StepAndObserve(0, 0);
    StepAndObserve(Right, 0); // Proven DoubleTap choreography from GroundedMovementTests.

    for (uint32 I = 3; I < 100000; ++I)
    {
        uint16 P1=0, P2=0;
        const uint32 M=I%240;
        if(M<20)P1|=Right; else if(M<40)P1|=static_cast<uint16>(EFighterInputButton::Left);
        if(M==45||M==90)P1|=static_cast<uint16>(EFighterInputButton::Up);
        if(M==60||M==180)P1|=Light;
        if(M==120||M==122)P1|=Right;
        if(M%37==0)P2|=static_cast<uint16>(EFighterInputButton::Left);
        if(M==75)P2|=Light;
        StepAndObserve(P1, P2);
    }
    TestTrue(TEXT("100k deterministic"),A==B); TestEqual(TEXT("frame count"),A.SimulationFrame,uint64(100000));
    TestTrue(TEXT("stress hit"),Hits>0); TestTrue(TEXT("stress hitstop"),Hitstop>0); TestTrue(TEXT("stress hitstun"),Hitstun>0);
    TestTrue(TEXT("stress knockback"),Knockback>0); TestTrue(TEXT("stress stage bound"),Bounds>0); TestTrue(TEXT("stress walking"),Walking>0);
    TestTrue(TEXT("stress jump"),Jump>0); TestTrue(TEXT("stress double jump"),DoubleJump>0); TestTrue(TEXT("stress dash"),Dash>0);
    return true;
}
#endif






