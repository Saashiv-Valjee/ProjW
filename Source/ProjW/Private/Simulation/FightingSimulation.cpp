#include "Simulation/FightingSimulation.h"
#include <limits>

namespace
{
    using Sim = FFightingSimulation;

    int32 Bounded(int64 Value, int32 Low, int32 High)
    {
        return static_cast<int32>(Value < Low ? Low : Value > High ? High : Value);
    }

    int32 AddBounded(int32 A, int32 B)
    {
        return Bounded(static_cast<int64>(A) + B,
            std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max());
    }

    bool IsDash(EFighterState State)
    {
        return State == EFighterState::ForwardDash || State == EFighterState::BackDash;
    }

    void ClearTap(FFighterState& Fighter)
    {
        Fighter.LastTapDirection = 0;
        Fighter.TapAgeFrames = 0;
    }

    void SetState(FFighterState& Fighter, EFighterState Next)
    {
        Fighter.StateFrame = Next == Fighter.CurrentState ? Fighter.StateFrame + 1u : 0u;
        Fighter.CurrentState = Next;
    }

    void StepFighter(FFighterState& Fighter, FFighterInput Input)
    {
        const FFighterInputEdges Edges = FFighterInputEdges::From(Input, Fighter.PreviousInput);
        Fighter.PreviousInput = Edges.Held;
        Fighter.PressedInput = Edges.Pressed;
        Fighter.ReleasedInput = Edges.Released;
        if (Fighter.LastTapDirection != 0 && Fighter.TapAgeFrames <= Sim::DashInputWindowFrames)
        {
            ++Fighter.TapAgeFrames; // Stops at window+1, never wraps into validity.
        }
        const int32 Direction = static_cast<int32>(Input.IsHeld(EFighterInputButton::Right))
            - static_cast<int32>(Input.IsHeld(EFighterInputButton::Left));
        const bool Down = Input.IsHeld(EFighterInputButton::Down);
        const bool JumpPressed = Edges.Pressed.IsHeld(EFighterInputButton::Up);
        const bool WasDash = IsDash(Fighter.CurrentState);
        const uint32 Duration = Fighter.CurrentState == EFighterState::ForwardDash
            ? Sim::ForwardDashDurationFrames : Sim::BackDashDurationFrames;
        bool Dashing = Fighter.bGrounded && WasDash && Fighter.StateFrame < Duration - 1u;
        EFighterState Next = EFighterState::Idle;

        if (Dashing)
        {
            Next = Fighter.CurrentState; // No jump/crouch/direction cancels.
            ClearTap(Fighter);
        }
        else if (Fighter.bGrounded && !Down && !JumpPressed)
        {
            // Releasing the other half of Left+Right is not a fresh raw press.
            const bool DirectionPressed = Direction != 0 && Edges.Pressed.IsHeld(
                Direction > 0 ? EFighterInputButton::Right : EFighterInputButton::Left);
            if (DirectionPressed)
            {
                if (Fighter.LastTapDirection == Direction && Fighter.TapAgeFrames <= Sim::DashInputWindowFrames)
                {
                    Fighter.DashDirection = static_cast<int8>(Direction);
                    Next = Direction == static_cast<int32>(Fighter.Facing)
                        ? EFighterState::ForwardDash : EFighterState::BackDash;
                    Dashing = true;
                    ClearTap(Fighter); // Consume pair so holding cannot retrigger.
                }
                else
                {
                    Fighter.LastTapDirection = static_cast<int8>(Direction);
                    Fighter.TapAgeFrames = 0;
                }
            }
            if (Input.IsHeld(EFighterInputButton::Left) && Input.IsHeld(EFighterInputButton::Right))
            {
                ClearTap(Fighter);
            }
        }
        else
        {
            ClearTap(Fighter); // No candidate carried across air/crouch/jump.
        }

        if (Dashing)
        {
            Fighter.VelocityX = Fighter.DashDirection * (Next == EFighterState::ForwardDash
                ? Sim::ForwardDashSpeed : Sim::BackDashSpeed);
        }
        else
        {
            Fighter.DashDirection = 0;
            if (Fighter.bGrounded && JumpPressed)
            {
                Fighter.bGrounded = false;
                Fighter.VelocityY = Sim::JumpSpeed;
            }
            if (!Fighter.bGrounded)
            {
                Fighter.VelocityX = Direction * Sim::WalkSpeed;
                Next = EFighterState::Jumping;
            }
            else if (Down)
            {
                Fighter.VelocityX = 0;
                Next = EFighterState::Crouching;
            }
            else
            {
                Fighter.VelocityX = Direction * (Direction == static_cast<int32>(Fighter.Facing)
                    ? Sim::ForwardWalkSpeed : Sim::BackwardWalkSpeed);
                Next = Direction == 0 ? EFighterState::Idle : EFighterState::Walking;
            }
        }

        Fighter.PositionX = Bounded(static_cast<int64>(Fighter.PositionX) + Fighter.VelocityX,
            Sim::MinFighterX, Sim::MaxFighterX);
        if (!Fighter.bGrounded)
        {
            Fighter.VelocityY = AddBounded(Fighter.VelocityY, Sim::Gravity);
            Fighter.PositionY = AddBounded(Fighter.PositionY, Fighter.VelocityY);
        }
        if (Fighter.bGrounded || Fighter.PositionY <= 0)
        {
            Fighter.PositionY = 0;
            Fighter.VelocityY = 0;
            Fighter.bGrounded = true;
            if (Next == EFighterState::Jumping)
            {
                Next = Down ? EFighterState::Crouching
                    : Direction != 0 ? EFighterState::Walking : EFighterState::Idle;
            }
        }
        SetState(Fighter, Next);
    }

    void ResolvePush(FFighterState& Left, FFighterState& Right)
    {
        // Spatial roles come from pre-step positions, never fighter update order.
        // Preserve that order even when a proposed dash would cross completely.
        const int64 Deficit = static_cast<int64>(2 * Sim::PushHalfWidth)
            - (static_cast<int64>(Right.PositionX) - Left.PositionX);
        if (Deficit <= 0) { return; }
        int64 L = static_cast<int64>(Left.PositionX) - Deficit / 2;
        int64 R = static_cast<int64>(Right.PositionX) + (Deficit - Deficit / 2);
        // Transfer blocked separation to the other fighter at a wall.
        if (L < Sim::MinFighterX) { R += Sim::MinFighterX - L; L = Sim::MinFighterX; }
        if (R > Sim::MaxFighterX) { L -= R - Sim::MaxFighterX; R = Sim::MaxFighterX; }
        Left.PositionX = static_cast<int32>(L);
        Right.PositionX = static_cast<int32>(R);
    }
}

void FFightingSimulation::SimulateFrame(FMatchState& State,
    const FFighterInput& Player1Input, const FFighterInput& Player2Input)
{
    const int32 Start1 = State.Fighter1.PositionX;
    const int32 Start2 = State.Fighter2.PositionX;
    // Equal-X ties use facing; identical-facing ties need an identity fallback to
    // repair invalid overlapping initialization. Valid separated pairs never use it.
    const bool FirstIsLeft = Start1 < Start2 || (Start1 == Start2
        && (State.Fighter1.Facing == EFighterFacing::Right || State.Fighter2.Facing != EFighterFacing::Right));
    StepFighter(State.Fighter1, Player1Input);
    StepFighter(State.Fighter2, Player2Input);
    if (State.Fighter1.bGrounded && State.Fighter2.bGrounded)
    {
        ResolvePush(FirstIsLeft ? State.Fighter1 : State.Fighter2,
            FirstIsLeft ? State.Fighter2 : State.Fighter1);
    }
    // Report resolved displacement, including wall stops and opponent push.
    State.Fighter1.VelocityX = Bounded(static_cast<int64>(State.Fighter1.PositionX) - Start1,
        std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max());
    State.Fighter2.VelocityX = Bounded(static_cast<int64>(State.Fighter2.PositionX) - Start2,
        std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max());
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
