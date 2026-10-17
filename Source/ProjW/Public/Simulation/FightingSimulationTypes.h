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

    constexpr bool IsHeld(EFighterInputButton Button) const
    {
        return (Buttons & static_cast<uint16>(Button)) != 0;
    }
};
static_assert(sizeof(FFighterInput) == 2, "Frame input must remain compact");

enum class EFighterFacing : int8 { Left = -1, Right = 1 };
enum class EFighterState : uint8 { Idle, Walking, Jumping };

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
};

// All mutable authoritative state lives here. Copy assignment saves/restores it.
// Compare/serialize fields explicitly; struct padding is not gameplay state.
struct FMatchState
{
    uint64 SimulationFrame = 0;
    FFighterState Fighter1;
    FFighterState Fighter2;
};
static_assert(std::is_trivially_copyable<FMatchState>::value, "Snapshots must be value-copyable");
