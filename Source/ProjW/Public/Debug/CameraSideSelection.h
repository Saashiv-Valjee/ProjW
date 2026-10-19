#pragma once

#include "CoreMinimal.h"

namespace ProjW::Presentation
{
    // A combat axis has two equivalent side-on camera yaws. Keep the current
    // presented side by selecting the candidate with the shortest wrapped turn.
    inline double SelectContinuousSideOnYaw(double SideOnYawDegrees, double PresentedYawDegrees)
    {
        const double CandidateA = FMath::UnwindDegrees(SideOnYawDegrees);
        const double CandidateB = FMath::UnwindDegrees(SideOnYawDegrees + 180.0);
        const double DeltaA = FMath::FindDeltaAngleDegrees(PresentedYawDegrees, CandidateA);
        const double DeltaB = FMath::FindDeltaAngleDegrees(PresentedYawDegrees, CandidateB);
        return PresentedYawDegrees + (FMath::Abs(DeltaA) <= FMath::Abs(DeltaB) ? DeltaA : DeltaB);
    }
}
