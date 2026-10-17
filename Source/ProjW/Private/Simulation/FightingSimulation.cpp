#include "Simulation/FightingSimulation.h"

#include <limits>

namespace
{
    // Wide intermediates prevent signed overflow; saturation defines world limits.
    int32 AddBounded(int32 A, int32 B)
    {
        const int64 Sum = static_cast<int64>(A) + B;
        if (Sum > std::numeric_limits<int32>::max())
        {
            return std::numeric_limits<int32>::max();
        }
        if (Sum < std::numeric_limits<int32>::min())
        {
            return std::numeric_limits<int32>::min();
        }
        return static_cast<int32>(Sum);
    }

    void StepFighter(FFighterState& Fighter, const FFighterInput& Input)
    {
        const int32 Direction = static_cast<int32>(Input.IsHeld(EFighterInputButton::Right))
            - static_cast<int32>(Input.IsHeld(EFighterInputButton::Left));
        Fighter.VelocityX = Direction * FFightingSimulation::WalkSpeed;
        if (Fighter.bGrounded && Input.IsHeld(EFighterInputButton::Up))
        {
            Fighter.bGrounded = false;
            Fighter.VelocityY = FFightingSimulation::JumpSpeed;
        }

        Fighter.PositionX = AddBounded(Fighter.PositionX, Fighter.VelocityX);
        if (!Fighter.bGrounded)
        {
            Fighter.VelocityY = AddBounded(Fighter.VelocityY, FFightingSimulation::Gravity);
            Fighter.PositionY = AddBounded(Fighter.PositionY, Fighter.VelocityY);
        }
        if (Fighter.bGrounded || Fighter.PositionY <= 0)
        {
            Fighter.PositionY = 0;
            Fighter.VelocityY = 0;
            Fighter.bGrounded = true;
        }

        const EFighterState NextState = !Fighter.bGrounded ? EFighterState::Jumping
            : (Direction != 0 ? EFighterState::Walking : EFighterState::Idle);
        // Unsigned counters have defined modulo wraparound.
        Fighter.StateFrame = NextState == Fighter.CurrentState ? Fighter.StateFrame + 1u : 0u;
        Fighter.CurrentState = NextState;
    }
}

void FFightingSimulation::SimulateFrame(FMatchState& State,
    const FFighterInput& Player1Input, const FFighterInput& Player2Input)
{
    StepFighter(State.Fighter1, Player1Input);
    StepFighter(State.Fighter2, Player2Input);
    if (State.Fighter1.PositionX < State.Fighter2.PositionX)
    {
        State.Fighter1.Facing = EFighterFacing::Right;
        State.Fighter2.Facing = EFighterFacing::Left;
    }
    else if (State.Fighter1.PositionX > State.Fighter2.PositionX)
    {
        State.Fighter1.Facing = EFighterFacing::Left;
        State.Fighter2.Facing = EFighterFacing::Right;
    }
    ++State.SimulationFrame;
}
