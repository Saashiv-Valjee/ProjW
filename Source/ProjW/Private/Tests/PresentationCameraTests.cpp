#include "Debug/CameraSideSelection.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContinuousCameraSideTest, "ProjW.Presentation.CameraSideContinuity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FContinuousCameraSideTest::RunTest(const FString&)
{
    using ProjW::Presentation::SelectContinuousSideOnYaw;

    const double ReversedAxisYaw = SelectContinuousSideOnYaw(90.0, -90.0);
    TestTrue(TEXT("reversed pair axis keeps equivalent current side"),
        FMath::Abs(FMath::FindDeltaAngleDegrees(-90.0, ReversedAxisYaw)) < 0.001);
    TestTrue(TEXT("crossing does not request a half-turn"),
        FMath::Abs(FMath::FindDeltaAngleDegrees(-90.0, ReversedAxisYaw)) < 90.0);

    const double SmallTurnYaw = SelectContinuousSideOnYaw(-78.0, -90.0);
    TestTrue(TEXT("small plane rotation selects nearby side"),
        FMath::Abs(FMath::FindDeltaAngleDegrees(-90.0, SmallTurnYaw) - 12.0) < 0.001);

    const double WrappedYaw = SelectContinuousSideOnYaw(179.0, -179.0);
    TestTrue(TEXT("wraparound uses the two-degree shortest turn"),
        FMath::Abs(FMath::FindDeltaAngleDegrees(-179.0, WrappedYaw) - -2.0) < 0.001);
    return !HasAnyErrors();
}
#endif
