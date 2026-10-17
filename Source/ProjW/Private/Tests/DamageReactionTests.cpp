#include "Simulation/FightingSimulation.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
namespace { FFighterInput L() { return {static_cast<uint16>(EFighterInputButton::Light)}; }
void Setup(FMatchState& S, int32 A=-100, int32 D=-40) { S.Fighter1.PositionX=A*FFightingSimulation::SubunitsPerWorldUnit; S.Fighter2.PositionX=D*FFightingSimulation::SubunitsPerWorldUnit; S.Fighter2.Facing=EFighterFacing::Right; }
void Active(FMatchState& S) { FFightingSimulation::SimulateFrame(S,L(),{}); for(int32 I=1;I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->StartupFrames;++I) FFightingSimulation::SimulateFrame(S,L(),{}); FFightingSimulation::SimulateFrame(S,L(),{}); }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageReactionTest,"ProjW.Simulation.Attack.DamageReaction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDamageReactionTest::RunTest(const FString&)
{ FMatchState S; Setup(S); Active(S); TestEqual(TEXT("damage"),S.Fighter2.Health,950); TestEqual(TEXT("hitstop"),S.Fighter1.HitstopFramesRemaining,uint8(6)); for(int32 I=0;I<6;++I){const int32 X=S.Fighter1.PositionX; const uint32 F=S.Fighter1.StateFrame; FFightingSimulation::SimulateFrame(S,L(),{}); TestEqual(TEXT("frozen position"),S.Fighter1.PositionX,X); TestEqual(TEXT("frozen frame"),S.Fighter1.StateFrame,F);} FFightingSimulation::SimulateFrame(S,L(),{}); TestTrue(TEXT("defender hitstun"),S.Fighter2.CurrentState==EFighterState::Hitstun); TestEqual(TEXT("hitstun entry"),S.Fighter2.StateFrame,uint32(0)); for(int32 I=0;I<FFightingSimulation::FindMoveDefinition(EMoveId::Light)->HitstunFrames-1;++I) FFightingSimulation::SimulateFrame(S,{},{}); TestTrue(TEXT("hitstun frame 13"),S.Fighter2.CurrentState==EFighterState::Hitstun); FFightingSimulation::SimulateFrame(S,{},{}); TestTrue(TEXT("hitstun exits"),S.Fighter2.CurrentState==EFighterState::Idle); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockbackDirectionTest,"ProjW.Simulation.Attack.Knockback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FKnockbackDirectionTest::RunTest(const FString&)
{ FMatchState S; Setup(S,-100,-40); Active(S); const int32 Start=S.Fighter2.PositionX; for(int32 I=0;I<6;++I) FFightingSimulation::SimulateFrame(S,{},{}); FFightingSimulation::SimulateFrame(S,{},{}); TestTrue(TEXT("right knockback"),S.Fighter2.PositionX>Start); TestEqual(TEXT("knockback speed"),S.Fighter2.VelocityX,FFightingSimulation::FindMoveDefinition(EMoveId::Light)->KnockbackSpeed); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitstopSnapshotTest,"ProjW.Simulation.Attack.HitstopSnapshot",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHitstopSnapshotTest::RunTest(const FString&)
{ FMatchState S; Setup(S); Active(S); FFightingSimulation::SimulateFrame(S,{},{}); const FMatchState Snap=S; FMatchState A=S,B=S; for(int32 I=0;I<30;++I){FFightingSimulation::SimulateFrame(A,{},{});FFightingSimulation::SimulateFrame(B,{},{});} TestTrue(TEXT("hitstop snapshot replay"),A==B); return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAirborneHitRejectedTest,"ProjW.Simulation.Attack.AirborneRejected",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAirborneHitRejectedTest::RunTest(const FString&)
{ FMatchState S; Setup(S); S.Fighter2.bGrounded=false; S.Fighter2.PositionY=100*FFightingSimulation::SubunitsPerWorldUnit; S.Fighter2.CurrentState=EFighterState::Jumping; Active(S); TestEqual(TEXT("no airborne event"),S.HitEventCount,uint32(0)); TestEqual(TEXT("no airborne damage"),S.Fighter2.Health,1000); TestFalse(TEXT("attack remains available"),S.Fighter1.bCurrentMoveConnected); TestEqual(TEXT("no reaction"),S.Fighter1.HitstopFramesRemaining,uint8(0)); return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitstopPushFrozenTest,"ProjW.Simulation.Attack.HitstopPushFrozen",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHitstopPushFrozenTest::RunTest(const FString&)
{ FMatchState S; Setup(S,-100,-40); Active(S); const int32 A=S.Fighter1.PositionX, D=S.Fighter2.PositionX; for(int32 I=0;I<6;++I){FFightingSimulation::SimulateFrame(S,{},{}); TestEqual(TEXT("attacker frozen"),S.Fighter1.PositionX,A); TestEqual(TEXT("defender frozen"),S.Fighter2.PositionX,D);} return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKnockbackLeftTest,"ProjW.Simulation.Attack.KnockbackLeft",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FKnockbackLeftTest::RunTest(const FString&)
{ FMatchState S; Setup(S,40,-40); S.Fighter1.Facing=EFighterFacing::Left; S.Fighter2.Facing=EFighterFacing::Right; Active(S); for(int32 I=0;I<7;++I) FFightingSimulation::SimulateFrame(S,{},{}); TestEqual(TEXT("left knockback speed"),S.Fighter2.VelocityX,-FFightingSimulation::FindMoveDefinition(EMoveId::Light)->KnockbackSpeed); return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitstunInputLockTest,"ProjW.Simulation.Attack.HitstunInputLock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHitstunInputLockTest::RunTest(const FString&)
{ FMatchState S; Setup(S); Active(S); for(int32 I=0;I<7;++I) FFightingSimulation::SimulateFrame(S,L(),{}); const int32 V=S.Fighter2.VelocityX; for(int32 I=0;I<5;++I) { FFightingSimulation::SimulateFrame(S,{static_cast<uint16>(EFighterInputButton::Left)|static_cast<uint16>(EFighterInputButton::Down)|static_cast<uint16>(EFighterInputButton::Up)|static_cast<uint16>(EFighterInputButton::Light)},{}); TestTrue(TEXT("hitstun remains locked"),S.Fighter2.CurrentState==EFighterState::Hitstun); TestEqual(TEXT("knockback remains committed"),S.Fighter2.VelocityX,V); } return true; }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReactionSnapshotPhasesTest,"ProjW.Simulation.Attack.ReactionSnapshotPhases",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReactionSnapshotPhasesTest::RunTest(const FString&)
{ FMatchState S; Setup(S); Active(S); TArray<FMatchState> Snaps; for(int32 I=0;I<4;++I){FFightingSimulation::SimulateFrame(S,{},{}); Snaps.Add(S);} for(const FMatchState& Snap:Snaps){FMatchState A=Snap,BState=Snap; for(int32 I=0;I<25;++I){FFightingSimulation::SimulateFrame(A,{},{});FFightingSimulation::SimulateFrame(BState,{},{});} TestTrue(TEXT("reaction replay equal"),A==BState);} return true; }
#endif
