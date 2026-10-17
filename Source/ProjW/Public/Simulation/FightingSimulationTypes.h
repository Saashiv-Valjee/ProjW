#pragma once

#include "CoreTypes.h"
#include <type_traits>

enum class EFighterInputButton : uint16
{
    Left = 1 << 0, Right = 1 << 1, Up = 1 << 2, Down = 1 << 3,
    Light = 1 << 4, Medium = 1 << 5, Heavy = 1 << 6, Special = 1 << 7,
    Assist1 = 1 << 8, Assist2 = 1 << 9
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
    LightAttackStartup,
    LightAttackActive,
    LightAttackRecovery
};

struct FSimBox
{
    int32 MinX = 0, MaxX = 0, MinY = 0, MaxY = 0;
    constexpr bool Overlaps(const FSimBox& Other) const
    { return MinX < Other.MaxX && MaxX > Other.MinX && MinY < Other.MaxY && MaxY > Other.MinY; }
};

// X is horizontal; Y is height on the fixed gameplay plane. Ground is Y = 0.
// Positions: 1024 integer subunits/world unit. Velocities: subunits/frame.
// StateFrame is zero on entry, incremented on each subsequent frame in that state.
struct FFighterState
{
    int32 PositionX = 0;
    int32 PositionY = 0;
    int32 VelocityX = 0;
    int32 VelocityY = 0;
    EFighterFacing Facing = EFighterFacing::Right;
    bool bGrounded = true;
    int32 Health = 1000;
    EFighterState CurrentState = EFighterState::Idle;
    uint32 StateFrame = 0;

    // Last completed frame's held mask; supplies previous input on the next step.
    FFighterInput PreviousInput;
    FFighterInput PressedInput;
    FFighterInput ReleasedInput;
    // Minimal raw tap history: -1 left, +1 right, 0 no candidate.
    int8 LastTapDirection = 0;
    uint8 TapAgeFrames = 0;
    // World-space direction locked on dash entry, zero outside a dash.
    int8 DashDirection = 0;
    // Exact world-space horizontal velocity committed for the pending/current jump.
    int32 CommittedJumpVelocityX = 0;
    // False during startup/normal jump; true after the one allowed double jump.
    bool bDoubleJumpUsed = false;
    bool bLightAttackHit = false;
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
