#pragma once

#include "Simulation/FightingSimulationTypes.h"

// Immutable deterministic move data. No UObject or runtime allocation is involved.
struct FMoveDefinition
{
    uint8 StartupFrames;
    uint8 ActiveFrames;
    uint8 RecoveryFrames;
    int32 Damage;
    uint8 HitstopFrames;
    uint8 HitstunFrames;
    int32 KnockbackSpeed;
    EGuardHeight GuardHeight;
    // Independent prototype contact taxonomy; not inferred from block rules.
    EParryHeight ParryHeight;
    int32 ChipDamage;
    uint8 BlockstunFrames;
    int32 BlockPushbackSpeed;
    int32 HitboxOffsetX;
    int32 HitboxOffsetY;
    int32 HitboxHalfWidth;
    int32 HitboxHalfHeight;
    int32 HitboxHalfExtentZ;
    EHitVolumeShape HitVolumeShape;
    int32 HitRadius;
};
static_assert(std::is_trivially_copyable_v<FMoveDefinition>);

// Stateless authoritative simulation. No Actors, world, physics, animation or clock.
class PROJW_API FFightingSimulation
{
public:
    static constexpr int32 FramesPerSecond = 60;
    static constexpr int32 SubunitsPerWorldUnit = 1024;
    static constexpr int32 WalkSpeed = 4 * SubunitsPerWorldUnit;
    static constexpr int32 ForwardWalkSpeed = WalkSpeed;
    static constexpr int32 BackwardWalkSpeed = 3 * SubunitsPerWorldUnit;
    static constexpr int32 DashInputWindowFrames = 12;
    static constexpr int32 ForwardDashDurationFrames = 12;
    static constexpr int32 BackDashDurationFrames = 10;
    static constexpr int32 ForwardDashSpeed = 10 * SubunitsPerWorldUnit;
    static constexpr int32 BackDashSpeed = 8 * SubunitsPerWorldUnit;
    static constexpr int32 PushHalfWidth = 20 * SubunitsPerWorldUnit;
    // The floor remains a 480-unit disc. Keep the complete X/Z hurtbox inside
    // it by constraining fighter centers to a conservative 25-unit footprint
    // radius (ceil(sqrt(24^2 + 5^2))).
    static constexpr int32 ArenaRadius = 480 * SubunitsPerWorldUnit;
    static constexpr int64 ArenaRadiusSquared = static_cast<int64>(ArenaRadius) * ArenaRadius;
    static constexpr int32 FighterFootprintRadius = 25 * SubunitsPerWorldUnit;
    static constexpr int32 FighterCenterRadius = ArenaRadius - FighterFootprintRadius;
    static constexpr int64 FighterCenterRadiusSquared = static_cast<int64>(FighterCenterRadius) * FighterCenterRadius;
    // Axial extrema for legacy callers; these are not independent X/Z bounds.
    static constexpr int32 MinFighterX = -FighterCenterRadius;
    static constexpr int32 MaxFighterX = FighterCenterRadius;
    static constexpr int32 MinFighterZ = -FighterCenterRadius;
    static constexpr int32 MaxFighterZ = FighterCenterRadius;
    static constexpr uint8 PivotDurationFrames = 12;
    static constexpr int32 AxisScale = 16384;
    static constexpr int32 JumpStartupFrames = 4;
    static constexpr int32 JumpVerticalSpeed = 12 * SubunitsPerWorldUnit;
    static constexpr int32 NeutralJumpHorizontalSpeed = 0;
    static constexpr int32 ForwardJumpHorizontalSpeed = 5 * SubunitsPerWorldUnit;
    static constexpr int32 BackwardJumpHorizontalSpeed = 4 * SubunitsPerWorldUnit;
    static constexpr int32 DoubleJumpVerticalSpeed = 11 * SubunitsPerWorldUnit;
    static constexpr int32 NeutralDoubleJumpHorizontalSpeed = 0;
    static constexpr int32 ForwardDoubleJumpHorizontalSpeed = ForwardJumpHorizontalSpeed;
    static constexpr int32 BackwardDoubleJumpHorizontalSpeed = BackwardJumpHorizontalSpeed;
    // Compatibility name for old tests/content; new code uses JumpVerticalSpeed.
    static constexpr int32 JumpSpeed = JumpVerticalSpeed;
    static constexpr int32 Gravity = -SubunitsPerWorldUnit / 2;
    static constexpr int32 HurtboxHalfWidth = 24 * SubunitsPerWorldUnit;
    static constexpr int32 HurtboxHeight = 100 * SubunitsPerWorldUnit;
    static constexpr int32 HurtboxHalfExtentZ = 5 * SubunitsPerWorldUnit;
    static constexpr uint8 ParryActiveFrames = 4;
    static constexpr uint8 ParryRecoveryFrames = 10;
    static constexpr uint8 ParryTotalFrames = ParryActiveFrames + ParryRecoveryFrames;
    static constexpr uint8 ParryHitstopFrames = 6;
    static constexpr uint8 ParryStunFrames = 18;
    static constexpr int32 GroundRuptureRadius = 112 * SubunitsPerWorldUnit;

    // Returns null for None or invalid IDs. Definitions are static and immutable.
    static const FMoveDefinition* FindMoveDefinition(EMoveId MoveId);
    static FSimBox GetHurtbox(const FFighterState& Fighter);
    static bool GetMoveHitbox(const FFighterState& Fighter, FSimBox& OutBox);
    static bool GetMoveHitVolume(const FFighterState& Fighter, FSimOrientedBox& OutBox);
    static bool GetMoveRadialVolume(const FFighterState& Fighter, FSimRadialVolume& OutVolume);
    // Tests fighter-center legality using the conservative hurtbox footprint
    // margin; the visual arena floor remains ArenaRadius.
    static bool IsInsideArena(int32 X, int32 Z);
    // Returns true if the intended position was projected onto the disc.
    static bool ConstrainToArena(int64 IntendedX, int64 IntendedZ, int32& OutX, int32& OutZ);

    // Exactly one 1/60-second gameplay step. Caller owns scheduling and inputs.
    // Left+right cancel. Grounded Up PRESSED starts a fixed jump startup.
    // Airborne momentum is committed; one fresh Up press can replace it via double jump.
    // Interpret using entering facing; classify raw double taps on second press.
    // Intended local-axis steps -> stage clamp -> grounded pair push -> facing update.
    static void SimulateFrame(FMatchState& State,
        const FFighterInput& Player1Input, const FFighterInput& Player2Input);
};
