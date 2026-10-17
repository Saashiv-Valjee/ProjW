#include "Simulation/FightingSimulation.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
namespace { FFighterInput B(EFighterInputButton Button) { return {static_cast<uint16>(Button)}; } }
namespace {
void Setup(FMatchState& S, int32 X1 = -100, int32 X2 = -40) { S.Fighter1.PositionX = X1 * FFightingSimulation::SubunitsPerWorldUnit; S.Fighter2.PositionX = X2 * FFightingSimulation::SubunitsPerWorldUnit; S.Fighter2.Facing = EFighterFacing::Right; }
void StartAttack(FMatchState& S) { FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {}); }
void EnterActive(FMatchState& S) { StartAttack(S); for (int32 I=1; I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++I) FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {}); FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {}); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackLifecycleTest, "ProjW.Simulation.Attack.LightLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAttackLifecycleTest::RunTest(const FString&)
{
    FMatchState S; S.Fighter1.PositionX = -100 * FFightingSimulation::SubunitsPerWorldUnit; S.Fighter2.PositionX = 300 * FFightingSimulation::SubunitsPerWorldUnit; S.Fighter2.Facing = EFighterFacing::Left;
    FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {});
    TestTrue(TEXT("startup entry"), S.Fighter1.CurrentState == EFighterState::Attack); TestEqual(TEXT("startup move"), S.Fighter1.CurrentMoveId, EMoveId::Light); TestEqual(TEXT("startup phase"), S.Fighter1.MovePhase, EMovePhase::Startup); TestEqual(TEXT("startup frame"), S.Fighter1.StateFrame, uint32(0));
    for (int32 I=1; I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++I) { FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {}); TestEqual(TEXT("startup timing"), S.Fighter1.CurrentState, EFighterState::Attack); TestEqual(TEXT("startup phase timing"), S.Fighter1.MovePhase, EMovePhase::Startup); TestEqual(TEXT("startup frame"), S.Fighter1.StateFrame, uint32(I)); }
    FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {}); TestTrue(TEXT("active entry"), S.Fighter1.CurrentState == EFighterState::Attack); TestEqual(TEXT("active phase"), S.Fighter1.MovePhase, EMovePhase::Active); TestEqual(TEXT("active frame"), S.Fighter1.StateFrame, uint32(0));
    FSimBox Box; TestTrue(TEXT("active hitbox"), FFightingSimulation::GetMoveHitbox(S.Fighter1, Box)); TestEqual(TEXT("complete miss"), S.HitEventCount, uint32(0));
    for (int32 I=1; I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->ActiveFrames; ++I) { FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {}); TestTrue(TEXT("active state"), S.Fighter1.CurrentState == EFighterState::Attack); TestEqual(TEXT("active phase timing"), S.Fighter1.MovePhase, EMovePhase::Active); TestEqual(TEXT("active frame timing"), S.Fighter1.StateFrame, uint32(I)); TestEqual(TEXT("still miss"), S.HitEventCount, uint32(0)); }
    for (int32 I=0; I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->RecoveryFrames; ++I) { FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {}); TestEqual(TEXT("recovery phase"), S.Fighter1.MovePhase, EMovePhase::Recovery); TestEqual(TEXT("recovery frame"), S.Fighter1.StateFrame, uint32(I)); }
    FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {});
    TestTrue(TEXT("returns neutral"), S.Fighter1.CurrentState == EFighterState::Idle); TestEqual(TEXT("move clears after recovery"), S.Fighter1.CurrentMoveId, EMoveId::None); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackGeometryTest, "ProjW.Simulation.Attack.Geometry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAttackGeometryTest::RunTest(const FString&)
{
    FSimBox A{0,10,0,10}; TestTrue(TEXT("positive overlap"), A.Overlaps({9,20,9,20})); TestFalse(TEXT("separated"), A.Overlaps({10,20,0,10})); TestFalse(TEXT("edge touch"), A.Overlaps({10,20,0,10})); TestTrue(TEXT("one subunit"), A.Overlaps({9,11,9,11})); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackInputRulesTest, "ProjW.Simulation.Attack.InputRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAttackInputRulesTest::RunTest(const FString&)
{
    for (EFighterState Initial : {EFighterState::Idle, EFighterState::Walking, EFighterState::Crouching})
    {
        FMatchState S; Setup(S); S.Fighter1.CurrentState = Initial; S.Fighter1.bGrounded = true; StartAttack(S); TestTrue(TEXT("actionable starts Light"), S.Fighter1.CurrentState == EFighterState::Attack);
    }
    for (EFighterState Initial : {EFighterState::JumpStartup, EFighterState::Jumping, EFighterState::DoubleJumping, EFighterState::ForwardDash, EFighterState::BackDash, EFighterState::Hitstun})
    {
        FMatchState S; Setup(S); S.Fighter1.CurrentState = Initial; S.Fighter1.bGrounded = Initial != EFighterState::Jumping && Initial != EFighterState::DoubleJumping; StartAttack(S); TestFalse(TEXT("non-actionable rejects Light"), S.Fighter1.CurrentState == EFighterState::Attack);
    }
    for (EMovePhase Phase : {EMovePhase::Startup, EMovePhase::Active, EMovePhase::Recovery})
    {
        FMatchState S; Setup(S); S.Fighter1.CurrentState = EFighterState::Attack; S.Fighter1.CurrentMoveId = EMoveId::Light; S.Fighter1.MovePhase = Phase; StartAttack(S); TestEqual(TEXT("attack phase cannot retrigger"), S.Fighter1.CurrentMoveId, EMoveId::Light); TestEqual(TEXT("attack remains locked"), S.Fighter1.CurrentState, EFighterState::Attack);
    }
    FMatchState S; Setup(S); StartAttack(S); for (int32 I=0; I<40; ++I) FFightingSimulation::SimulateFrame(S, B(EFighterInputButton::Light), {}); TestTrue(TEXT("held Light does not retrigger"), S.Fighter1.CurrentState == EFighterState::Idle);
    FFightingSimulation::SimulateFrame(S, {}, {}); StartAttack(S); TestTrue(TEXT("release then press retriggers"), S.Fighter1.CurrentState == EFighterState::Attack); TestEqual(TEXT("new move begins"), S.Fighter1.CurrentMoveId, EMoveId::Light); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackTimingAndMirroringTest, "ProjW.Simulation.Attack.TimingMirroring", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAttackTimingAndMirroringTest::RunTest(const FString&)
{
    FMatchState S; Setup(S, 0, 300); StartAttack(S); FSimBox Box; TestFalse(TEXT("startup has no box"), FFightingSimulation::GetMoveHitbox(S.Fighter1, Box)); for (int32 I=1; I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++I) { FFightingSimulation::SimulateFrame(S, {}, {}); TestFalse(TEXT("startup has no box"), FFightingSimulation::GetMoveHitbox(S.Fighter1, Box)); } FFightingSimulation::SimulateFrame(S, {}, {}); TestTrue(TEXT("active box"), FFightingSimulation::GetMoveHitbox(S.Fighter1, Box)); const int32 RightMin = Box.MinX; for (int32 I=0; I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->ActiveFrames; ++I) { TestTrue(TEXT("all active frames have box"), FFightingSimulation::GetMoveHitbox(S.Fighter1, Box)); FFightingSimulation::SimulateFrame(S, {}, {}); } TestFalse(TEXT("recovery has no box"), FFightingSimulation::GetMoveHitbox(S.Fighter1, Box)); S.Fighter1.Facing = EFighterFacing::Left; S.Fighter1.CurrentState = EFighterState::Attack; S.Fighter1.MovePhase = EMovePhase::Active; TestTrue(TEXT("left box exists"), FFightingSimulation::GetMoveHitbox(S.Fighter1, Box)); TestTrue(TEXT("left mirrors right"), Box.MinX < S.Fighter1.PositionX && RightMin > S.Fighter1.PositionX); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackHitCasesTest, "ProjW.Simulation.Attack.HitCases", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAttackHitCasesTest::RunTest(const FString&)
{
    FMatchState S; Setup(S); EnterActive(S); TestEqual(TEXT("hit registers once"), S.HitEventCount, uint32(1)); for (int32 I=0; I<2; ++I) { FFightingSimulation::SimulateFrame(S, {}, {}); TestEqual(TEXT("no duplicate active hit"), S.HitEventCount, uint32(1)); }
    FMatchState Miss; Setup(Miss, -100, 300); EnterActive(Miss); TestEqual(TEXT("complete miss"), Miss.HitEventCount, uint32(0));
    FMatchState Late; Setup(Late, -100, 300); EnterActive(Late); Late.Fighter2.PositionX = -20 * FFightingSimulation::SubunitsPerWorldUnit; FFightingSimulation::SimulateFrame(Late, {}, {}); TestEqual(TEXT("late active hit"), Late.HitEventCount, uint32(1));
    FMatchState Trade; Setup(Trade, -40, 40); Trade.Fighter1.Facing = EFighterFacing::Right; Trade.Fighter2.Facing = EFighterFacing::Left;
    FFightingSimulation::SimulateFrame(Trade, B(EFighterInputButton::Light), B(EFighterInputButton::Light));
    for (int32 I=1; I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++I) FFightingSimulation::SimulateFrame(Trade, B(EFighterInputButton::Light), B(EFighterInputButton::Light));
    FFightingSimulation::SimulateFrame(Trade, B(EFighterInputButton::Light), B(EFighterInputButton::Light));
    TestEqual(TEXT("symmetric trade"), Trade.HitEventCount, uint32(2)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackSnapshotPhasesTest, "ProjW.Simulation.Attack.SnapshotPhases", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAttackSnapshotPhasesTest::RunTest(const FString&)
{
    FMatchState S; Setup(S); StartAttack(S); const FMatchState Startup = S; for (int32 I=1; I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->StartupFrames; ++I) FFightingSimulation::SimulateFrame(S, {}, {}); const FMatchState ActiveBefore = S; FFightingSimulation::SimulateFrame(S, {}, {}); const FMatchState ActiveAfter = S; for (int32 I=1; I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->ActiveFrames; ++I) FFightingSimulation::SimulateFrame(S, {}, {}); const FMatchState Recovery = S;
    for (const FMatchState& Snapshot : {Startup, ActiveBefore, ActiveAfter, Recovery}) { FMatchState A = Snapshot, BState = Snapshot; for (int32 I=0; I<20; ++I) { FFightingSimulation::SimulateFrame(A, {}, {}); FFightingSimulation::SimulateFrame(BState, {}, {}); } TestTrue(TEXT("snapshot replay identical"), A == BState); } return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAttackMixedRepeatabilityTest, "ProjW.Simulation.Attack.MixedRepeatability", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAttackMixedRepeatabilityTest::RunTest(const FString&)
{
    FMatchState A, BState; Setup(A, -100, 100); Setup(BState, -100, 100); for (uint32 Frame=0; Frame<30000; ++Frame) { uint16 Buttons = (Frame%97<30 ? static_cast<uint16>(EFighterInputButton::Light) : 0) | (Frame%211<70 ? static_cast<uint16>(EFighterInputButton::Right) : 0) | (Frame%503==0 ? static_cast<uint16>(EFighterInputButton::Up) : 0); FFighterInput In{Buttons}; FFightingSimulation::SimulateFrame(A, In, {}); FFightingSimulation::SimulateFrame(BState, In, {}); } TestTrue(TEXT("long mixed sequence deterministic"), A == BState); return true;
}
#endif
