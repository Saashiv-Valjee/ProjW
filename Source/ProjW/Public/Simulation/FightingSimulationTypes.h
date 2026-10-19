#pragma once

#include "CoreTypes.h"
#include <type_traits>

enum class EFighterInputButton : uint16
{
    Left = 1 << 0, Right = 1 << 1, Up = 1 << 2, Down = 1 << 3,
    Light = 1 << 4, Medium = 1 << 5, Heavy = 1 << 6, Special = 1 << 7,
    Assist1 = 1 << 8, Assist2 = 1 << 9,
    PivotPositive = 1 << 10, PivotNegative = 1 << 11,
    HighParry = 1 << 12, LowParry = 1 << 13, GroundRupture = 1 << 14
};

// One player's held buttons for exactly one simulation frame. No device polling.
// A future wire format should encode Buttons explicitly, with a defined byte order.
struct FFighterInput
{
    uint16 Buttons = 0;
    bool operator==(const FFighterInput&) const = default;

    constexpr bool IsHeld(EFighterInputButton Button) const
    {
        return (Buttons & static_cast<uint16>(Button)) != 0;
    }
};
static_assert(sizeof(FFighterInput) == 2, "Frame input must remain compact");

// Edges derive only from adjacent simulation frames, for every raw button bit.
struct FFighterInputEdges
{
    FFighterInput Held;
    FFighterInput Pressed;
    FFighterInput Released;
    static constexpr FFighterInputEdges From(FFighterInput Current, FFighterInput Previous)
    {
        return {Current,
            FFighterInput{static_cast<uint16>(Current.Buttons & ~Previous.Buttons)},
            FFighterInput{static_cast<uint16>(Previous.Buttons & ~Current.Buttons)}};
    }
};

enum class EFighterFacing : int8 { Left = -1, Right = 1 };
enum class EMoveId : uint8 { None = 0, Light = 1, Medium = 2, Heavy = 3, GroundRupture = 4 };
enum class EMovePhase : uint8 { Startup = 0, Active = 1, Recovery = 2 };
enum class EGuardHeight : uint8 { High = 0, Low = 1 };
enum class EParryHeight : uint8 { None = 0, High = 1, Low = 2 };
enum class EHitVolumeShape : uint8 { OrientedBox = 0, HorizontalCircle = 1 };
enum class EPendingReaction : uint8 { None = 0, Hit = 1, Block = 2, Parry = 3 };
enum class EFighterState : uint8
{
    Idle,
    Walking,
    Crouching,
    ForwardDash,
    BackDash,
    JumpStartup,
    Jumping,
    DoubleJumping,
    Attack,
    Hitstun,
    Blockstun,
    Pivoting,
    Parrying,
    ParrySuccess,
    ParryStun
};

struct FSimBox
{
    int32 MinX = 0, MaxX = 0, MinY = 0, MaxY = 0, MinZ = 0, MaxZ = 0;
    constexpr bool Overlaps(const FSimBox& Other) const
    { return MinX < Other.MaxX && MaxX > Other.MinX && MinY < Other.MaxY && MaxY > Other.MinY
        && MinZ < Other.MaxZ && MaxZ > Other.MinZ; }
};

// Horizontal orientation uses signed fixed-point unit vectors (scale 16384).
// Contact tests use this box directly; its enclosing AABB is presentation only.
struct FSimOrientedBox
{
    int32 CenterX = 0, CenterZ = 0, MinY = 0, MaxY = 0;
    int32 AxisX = 16384, AxisZ = 0;
    int32 HalfForward = 0, HalfLateral = 0;
    bool Overlaps(const FSimBox& Other) const;
    FSimBox Bounds() const;
};

// A vertical interval swept by a horizontal circle. Integer circle/AABB tests
// use the closest point and strict squared-distance comparison.
struct FSimRadialVolume
{
    int32 CenterX = 0, CenterZ = 0, MinY = 0, MaxY = 0, Radius = 0;
    bool Overlaps(const FSimBox& Other) const;
};

// X/Z are the horizontal arena plane; the fighter pair defines the local axis. Y is height.
// Positions: 1024 integer subunits/world unit. Velocities: subunits/frame.
// StateFrame is zero on entry, incremented on each subsequent frame in that state.
struct FFighterState
{
    static constexpr int32 MaxHealth = 1000;
    int32 PositionX = 0;
    int32 PositionY = 0;
    int32 PositionZ = 0;
    int32 VelocityX = 0;
    int32 VelocityY = 0;
    int32 VelocityZ = 0;
    EFighterFacing Facing = EFighterFacing::Right;
    bool bGrounded = true;
    int32 Health = MaxHealth;
    EFighterState CurrentState = EFighterState::Idle;
    uint32 StateFrame = 0;
    EMoveId CurrentMoveId = EMoveId::None;
    EMovePhase MovePhase = EMovePhase::Startup;

    // Last completed frame's held mask; supplies previous input on the next step.
    FFighterInput PreviousInput;
    FFighterInput PressedInput;
    FFighterInput ReleasedInput;
    // Minimal raw tap history: -1 left, +1 right, 0 no candidate.
    int8 LastTapDirection = 0;
    uint8 TapAgeFrames = 0;
    // World-space direction locked on dash entry, zero outside a dash.
    int8 DashDirection = 0;
    // Signed orbital direction locked on Pivot entry; zero outside Pivot.
    int8 PivotDirection = 0;
    EParryHeight ParryHeight = EParryHeight::None;
    // Exact world-space horizontal velocity committed for the pending/current jump.
    int32 CommittedJumpVelocityX = 0;
    int32 CommittedJumpVelocityZ = 0;
    // Local horizontal attack axis captured on commitment (fixed-point scale 16384).
    int32 AttackAxisX = 0;
    int32 AttackAxisZ = 0;
    // False during startup/normal jump; true after the one allowed double jump.
    bool bDoubleJumpUsed = false;
    bool bCurrentMoveConnected = false;
    uint8 HitstopFramesRemaining = 0;
    EPendingReaction PendingReaction = EPendingReaction::None;
    uint8 PendingReactionFrames = 0;
    int32 PendingKnockbackVelocityX = 0;
    int32 PendingKnockbackVelocityZ = 0;
    bool operator==(const FFighterState&) const = default;
};

// All mutable authoritative state lives here. Copy assignment saves/restores it.
// Compare/serialize fields explicitly; struct padding is not gameplay state.
struct FMatchState
{
    uint64 SimulationFrame = 0;
    FFighterState Fighter1;
    FFighterState Fighter2;
    uint32 HitEventCount = 0;
    bool operator==(const FMatchState&) const = default;
};
static_assert(std::is_trivially_copyable<FMatchState>::value, "Snapshots must be value-copyable");
