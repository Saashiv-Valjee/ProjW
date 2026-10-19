#include "Debug/DebugInputMapping.h"
#include "Simulation/FightingSimulation.h"
#include "InputCoreTypes.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDebugInputMappingTest, "ProjW.Presentation.DebugInputMapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDebugInputMappingTest::RunTest(const FString&)
{
    using namespace ProjW::DebugInput;
    using namespace ProjW::DebugInput::Keys;
    auto Has = [](FFighterInput Input, EFighterInputButton Button)
    { return Input.IsHeld(Button); };
    auto KeyIs = [this](const TCHAR* Label, const FKey& Actual, const FKey& Expected)
    { TestTrue(Label, Actual.GetFName() == Expected.GetFName()); };

    KeyIs(TEXT("P1 A maps screen-left"), P1Left(), EKeys::A);
    KeyIs(TEXT("P1 D maps screen-right"), P1Right(), EKeys::D);
    KeyIs(TEXT("P1 W maps jump"), P1Jump(), EKeys::W);
    KeyIs(TEXT("P1 S maps crouch"), P1Crouch(), EKeys::S);
    KeyIs(TEXT("P1 Q maps counter-clockwise Pivot"), P1PivotCCW(), EKeys::Q);
    KeyIs(TEXT("P1 E maps clockwise Pivot"), P1PivotCW(), EKeys::E);
    KeyIs(TEXT("P1 R maps Light"), P1Light(), EKeys::R);
    KeyIs(TEXT("P1 F maps Medium"), P1Medium(), EKeys::F);
    KeyIs(TEXT("P1 C maps Heavy"), P1Heavy(), EKeys::C);
    KeyIs(TEXT("P1 V maps GroundRupture"), P1GroundRupture(), EKeys::V);
    KeyIs(TEXT("P1 T maps HighParry"), P1HighParry(), EKeys::T);
    KeyIs(TEXT("P1 G maps LowParry"), P1LowParry(), EKeys::G);
    KeyIs(TEXT("P1 Z maps NarrowThrust"), P1NarrowThrust(), EKeys::Z);
    KeyIs(TEXT("P1 X maps BroadSweep"), P1BroadSweep(), EKeys::X);
    KeyIs(TEXT("P1 B maps SlowCommittedStrike"), P1SlowCommittedStrike(), EKeys::B);
    KeyIs(TEXT("P2 arrows map movement"), P2Left(), EKeys::Left);
    KeyIs(TEXT("P2 arrows map movement right"), P2Right(), EKeys::Right);
    KeyIs(TEXT("P2 Up maps jump"), P2Jump(), EKeys::Up);
    KeyIs(TEXT("P2 Down maps crouch"), P2Crouch(), EKeys::Down);
    KeyIs(TEXT("P2 NumPad 4 maps counter-clockwise Pivot"), P2PivotCCW(), EKeys::NumPadFour);
    KeyIs(TEXT("P2 NumPad 6 maps clockwise Pivot"), P2PivotCW(), EKeys::NumPadSix);
    KeyIs(TEXT("P2 NumPad 7 maps Light"), P2Light(), EKeys::NumPadSeven);
    KeyIs(TEXT("P2 NumPad 8 maps Medium"), P2Medium(), EKeys::NumPadEight);
    KeyIs(TEXT("P2 NumPad 9 maps Heavy"), P2Heavy(), EKeys::NumPadNine);
    KeyIs(TEXT("P2 NumPad 5 maps GroundRupture"), P2GroundRupture(), EKeys::NumPadFive);
    KeyIs(TEXT("P2 NumPad 1 maps HighParry"), P2HighParry(), EKeys::NumPadOne);
    KeyIs(TEXT("P2 NumPad 2 maps LowParry"), P2LowParry(), EKeys::NumPadTwo);
    KeyIs(TEXT("P2 NumPad 0 maps NarrowThrust"), P2NarrowThrust(), EKeys::NumPadZero);
    KeyIs(TEXT("P2 NumPad Decimal maps BroadSweep"), P2BroadSweep(), EKeys::Decimal);
    KeyIs(TEXT("P2 NumPad Add maps SlowCommittedStrike"), P2SlowCommittedStrike(), EKeys::Add);

    int8 P1Map = 0;
    FDebugPhysicalInput P1;
    P1.bLight = true; P1.bMedium = true; P1.bHeavy = true;
    P1.bGroundRupture = true; P1.bHighParry = true; P1.bLowParry = true;
    P1.bNarrowThrust = true; P1.bBroadSweep = true; P1.bSlowCommittedStrike = true;
    P1.bPivotCounterClockwise = true; P1.bPivotClockwise = true;
    FFighterInput P1Actions = BuildPlayer1Input(P1, 1.0, P1Map);
    TestTrue(TEXT("P1 Light has its own action bit"), Has(P1Actions, EFighterInputButton::Light));
    TestTrue(TEXT("P1 Medium has its own action bit"), Has(P1Actions, EFighterInputButton::Medium));
    TestTrue(TEXT("P1 Heavy has its own action bit"), Has(P1Actions, EFighterInputButton::Heavy));
    TestTrue(TEXT("P1 GroundRupture has its own action bit"), Has(P1Actions, EFighterInputButton::GroundRupture));
    TestTrue(TEXT("P1 HighParry has its own action bit"), Has(P1Actions, EFighterInputButton::HighParry));
    TestTrue(TEXT("P1 LowParry has its own action bit"), Has(P1Actions, EFighterInputButton::LowParry));
    TestTrue(TEXT("P1 Thrust has its own prototype command"), Has(P1Actions, EFighterInputButton::NarrowThrust));
    TestTrue(TEXT("P1 Sweep has its own prototype command"), Has(P1Actions, EFighterInputButton::BroadSweep));
    TestTrue(TEXT("P1 Slow strike has its own prototype command"), Has(P1Actions, EFighterInputButton::SlowCommittedStrike));
    TestTrue(TEXT("P1 Pivot directions remain distinct"), Has(P1Actions, EFighterInputButton::PivotPositive)
        && Has(P1Actions, EFighterInputButton::PivotNegative));

    int8 P2Map = 0;
    FDebugPhysicalInput P2 = P1;
    FFighterInput P2Actions = BuildPlayer2Input(P2, 1.0, P2Map);
    TestEqual(TEXT("P2 action layout maps same independent Light bit"), P2Actions.Buttons, P1Actions.Buttons);

    FDebugPhysicalInput PressHigh;
    PressHigh.bHighParry = true;
    const FFighterInput High = BuildPlayer1Input(PressHigh, 1.0, P1Map);
    const FFighterInputEdges FirstPress = FFighterInputEdges::From(High, {});
    const FFighterInputEdges Held = FFighterInputEdges::From(High, FirstPress.Held);
    TestTrue(TEXT("new HighParry physical press creates Pressed"), FirstPress.Pressed.IsHeld(EFighterInputButton::HighParry));
    TestFalse(TEXT("held HighParry does not retrigger Pressed"), Held.Pressed.IsHeld(EFighterInputButton::HighParry));
    const FFighterInputEdges Released = FFighterInputEdges::From({}, Held.Held);
    const FFighterInputEdges PressedAgain = FFighterInputEdges::From(High, Released.Held);
    TestTrue(TEXT("release then new HighParry press creates one new edge"),
        PressedAgain.Pressed.IsHeld(EFighterInputButton::HighParry));

    FDebugPhysicalInput PressLow;
    PressLow.bLowParry = true;
    int8 P1LowMap = 0;
    const FFighterInput P1LowHeld = BuildPlayer1Input(PressLow, 1.0, P1LowMap);
    const FFighterInputEdges P1LowPress = FFighterInputEdges::From(P1LowHeld, {});
    const FFighterInputEdges P1LowRepeat = FFighterInputEdges::From(P1LowHeld, P1LowPress.Held);
    TestTrue(TEXT("P1 G translation produces one LowParry Pressed edge"),
        P1LowPress.Pressed.IsHeld(EFighterInputButton::LowParry));
    TestFalse(TEXT("holding P1 G does not repeat LowParry Pressed"),
        P1LowRepeat.Pressed.IsHeld(EFighterInputButton::LowParry));

    int8 P2LowMap = 0;
    const FFighterInput P2LowHeld = BuildPlayer2Input(PressLow, 1.0, P2LowMap);
    const FFighterInputEdges P2LowPress = FFighterInputEdges::From(P2LowHeld, {});
    const FFighterInputEdges P2LowRepeat = FFighterInputEdges::From(P2LowHeld, P2LowPress.Held);
    TestTrue(TEXT("P2 Numpad2 translation produces one LowParry Pressed edge"),
        P2LowPress.Pressed.IsHeld(EFighterInputButton::LowParry));
    TestFalse(TEXT("holding P2 Numpad2 does not repeat LowParry Pressed"),
        P2LowRepeat.Pressed.IsHeld(EFighterInputButton::LowParry));

    FMatchState P1LowEntry;
    P1LowEntry.Fighter2.PositionX = 300 * FFightingSimulation::SubunitsPerWorldUnit;
    FFightingSimulation::SimulateFrame(P1LowEntry, P1LowHeld, {});
    TestEqual(TEXT("P1 mapped LowParry enters authoritative Parrying state"),
        P1LowEntry.Fighter1.CurrentState, EFighterState::Parrying);
    TestEqual(TEXT("P1 mapped LowParry captures Low height"),
        P1LowEntry.Fighter1.ParryHeight, EParryHeight::Low);
    FMatchState P2LowEntry;
    P2LowEntry.Fighter1.PositionX = -300 * FFightingSimulation::SubunitsPerWorldUnit;
    FFightingSimulation::SimulateFrame(P2LowEntry, {}, P2LowHeld);
    TestEqual(TEXT("P2 mapped LowParry enters authoritative Parrying state"),
        P2LowEntry.Fighter2.CurrentState, EFighterState::Parrying);
    TestEqual(TEXT("P2 mapped LowParry captures Low height"),
        P2LowEntry.Fighter2.ParryHeight, EParryHeight::Low);

    // Normal horizontal combat axis: before crossing, P1 faces right and
    // P2 faces left. After crossing, both face the opposite way. Away changes
    // physical screen direction, while the key mapper still emits the correct
    // authoritative Left/Right bit.
    int8 P1BeforeMap = 0, P2BeforeMap = 0, P1AfterMap = 0, P2AfterMap = 0;
    FDebugPhysicalInput P1AwayBefore; P1AwayBefore.bScreenLeft = true;
    FDebugPhysicalInput P2AwayBefore; P2AwayBefore.bScreenRight = true;
    const FFighterInput P1Before = BuildPlayer1Input(P1AwayBefore, 1.0, P1BeforeMap);
    const FFighterInput P2Before = BuildPlayer2Input(P2AwayBefore, 1.0, P2BeforeMap);
    TestTrue(TEXT("P1 facing right recognizes screen-left Away before crossing"), Has(P1Before, EFighterInputButton::Left));
    TestTrue(TEXT("P2 facing left recognizes screen-right Away before crossing"), Has(P2Before, EFighterInputButton::Right));

    FDebugPhysicalInput P1AwayAfter; P1AwayAfter.bScreenRight = true;
    FDebugPhysicalInput P2AwayAfter; P2AwayAfter.bScreenLeft = true;
    const FFighterInput P1After = BuildPlayer1Input(P1AwayAfter, 1.0, P1AfterMap);
    const FFighterInput P2After = BuildPlayer2Input(P2AwayAfter, 1.0, P2AfterMap);
    TestTrue(TEXT("P1 facing left recognizes screen-right Away after crossing"), Has(P1After, EFighterInputButton::Right));
    TestTrue(TEXT("P2 facing right recognizes screen-left Away after crossing"), Has(P2After, EFighterInputButton::Left));
    FDebugPhysicalInput P2CrouchAway; P2CrouchAway.bScreenLeft = true; P2CrouchAway.bCrouch = true;
    const FFighterInput P2CrouchGuard = BuildPlayer2Input(P2CrouchAway, 1.0, P2AfterMap);
    TestTrue(TEXT("Down plus Away maps to crouch guard input"), Has(P2CrouchGuard, EFighterInputButton::Down)
        && Has(P2CrouchGuard, EFighterInputButton::Left));

    int8 RotatedMap = 0;
    FDebugPhysicalInput ScreenLeft; ScreenLeft.bScreenLeft = true;
    FDebugPhysicalInput ScreenRight; ScreenRight.bScreenRight = true;
    TestTrue(TEXT("rotated camera maps screen-left to local Right when alignment reverses"),
        Has(BuildPlayer1Input(ScreenLeft, -1.0, RotatedMap), EFighterInputButton::Right));
    TestTrue(TEXT("rotated camera maps screen-right to local Left when alignment reverses"),
        Has(BuildPlayer1Input(ScreenRight, -1.0, RotatedMap), EFighterInputButton::Left));
    TestTrue(TEXT("P2 numpad keys are distinct from the main number row"),
        EKeys::NumPadFive.GetFName() != EKeys::Five.GetFName()
        && EKeys::NumPadOne.GetFName() != EKeys::One.GetFName()
        && EKeys::NumPadFour.GetFName() != EKeys::Left.GetFName());
    return !HasAnyErrors();
}
#endif
