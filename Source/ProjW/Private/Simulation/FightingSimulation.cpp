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

    bool IsAttack(EFighterState State)
    {
        return State == EFighterState::Attack;
    }

    bool IsHitstun(EFighterState State) { return State == EFighterState::Hitstun; }

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

    int32 HorizontalDirection(const FFighterInput& Input)
    {
        return static_cast<int32>(Input.IsHeld(EFighterInputButton::Right))
            - static_cast<int32>(Input.IsHeld(EFighterInputButton::Left));
    }

    int32 SelectJumpVelocityX(int32 Direction, EFighterFacing Facing, bool bDoubleJump)
    {
        if (Direction == 0)
        {
            return bDoubleJump ? Sim::NeutralDoubleJumpHorizontalSpeed
                : Sim::NeutralJumpHorizontalSpeed;
        }
        const bool bForward = Direction == static_cast<int32>(Facing);
        const int32 Speed = bDoubleJump
            ? (bForward ? Sim::ForwardDoubleJumpHorizontalSpeed : Sim::BackwardDoubleJumpHorizontalSpeed)
            : (bForward ? Sim::ForwardJumpHorizontalSpeed : Sim::BackwardJumpHorizontalSpeed);
        return Direction * Speed;
    }

    void MoveHorizontal(FFighterState& Fighter)
    {
        const int64 Intended = static_cast<int64>(Fighter.PositionX) + Fighter.VelocityX;
        Fighter.PositionX = Bounded(Intended, Sim::MinFighterX, Sim::MaxFighterX);
        if (Intended < Sim::MinFighterX || Intended > Sim::MaxFighterX)
        {
            Fighter.VelocityX = 0; // Stage walls stop committed momentum; no bounce.
            Fighter.CommittedJumpVelocityX = 0;
        }
    }

    void StepFighter(FFighterState& Fighter, FFighterInput Input)
    {
        const FFighterInputEdges Edges = FFighterInputEdges::From(Input, Fighter.PreviousInput);
        Fighter.PreviousInput = Edges.Held;
        Fighter.PressedInput = Edges.Pressed;
        Fighter.ReleasedInput = Edges.Released;
        if (Fighter.HitstopFramesRemaining > 0)
        {
            --Fighter.HitstopFramesRemaining;
            return;
        }
        bool bEnteredHitstun = false;
        if (Fighter.bPendingHitReaction && !IsHitstun(Fighter.CurrentState))
        {
            Fighter.CurrentState = EFighterState::Hitstun;
            Fighter.StateFrame = 0;
            Fighter.CurrentMoveId = EMoveId::None;
            Fighter.MovePhase = EMovePhase::Startup;
            bEnteredHitstun = true;
        }
        if (IsHitstun(Fighter.CurrentState))
        {
            Fighter.bGrounded = true;
            Fighter.PositionY = 0;
            Fighter.VelocityY = 0;
            Fighter.VelocityX = Fighter.PendingKnockbackVelocityX;
            MoveHorizontal(Fighter);
            if (bEnteredHitstun)
            {
                // Entry is StateFrame 0; this call performs the first
                // knockback step without incrementing the state timer.
            }
            else if (Fighter.StateFrame < static_cast<uint32>(Fighter.PendingHitstunFrames) - 1u)
            {
                SetState(Fighter, EFighterState::Hitstun);
            }
            else
            {
                Fighter.bPendingHitReaction = false;
                Fighter.PendingHitstunFrames = 0;
                Fighter.PendingKnockbackVelocityX = 0;
                Fighter.VelocityX = 0;
                const int32 Direction = HorizontalDirection(Input);
                const bool bDown = Input.IsHeld(EFighterInputButton::Down);
                SetState(Fighter, bDown ? EFighterState::Crouching : Direction ? EFighterState::Walking : EFighterState::Idle);
            }
            return;
        }
        if (Fighter.LastTapDirection != 0 && Fighter.TapAgeFrames <= Sim::DashInputWindowFrames)
        {
            ++Fighter.TapAgeFrames;
        }

        const int32 Direction = HorizontalDirection(Input);
        const bool bDown = Input.IsHeld(EFighterInputButton::Down);
        const bool bJumpPressed = Edges.Pressed.IsHeld(EFighterInputButton::Up);
        const bool bWasDash = IsDash(Fighter.CurrentState);
        const uint32 DashDuration = Fighter.CurrentState == EFighterState::ForwardDash
            ? Sim::ForwardDashDurationFrames : Sim::BackDashDurationFrames;
        bool bDashing = Fighter.bGrounded && bWasDash && Fighter.StateFrame < DashDuration - 1u;
        EFighterState Next = EFighterState::Idle;
        bool bFreshAerialImpulse = false;
        bool bMovePhaseChanged = false;

        if (IsAttack(Fighter.CurrentState))
        {
            Fighter.bGrounded = true;
            Fighter.PositionY = 0;
            Fighter.VelocityX = 0;
            Fighter.VelocityY = 0;
            ClearTap(Fighter);
            const FMoveDefinition* Move = Sim::FindMoveDefinition(Fighter.CurrentMoveId);
            if (!Move)
            {
                Fighter.CurrentMoveId = EMoveId::None;
                Fighter.MovePhase = EMovePhase::Startup;
                Next = EFighterState::Idle;
            }
            else if (Fighter.MovePhase == EMovePhase::Startup)
            {
                if (Fighter.StateFrame < Move->StartupFrames - 1u) Next = EFighterState::Attack;
                else { Fighter.MovePhase = EMovePhase::Active; bMovePhaseChanged = true; Next = EFighterState::Attack; }
            }
            else if (Fighter.MovePhase == EMovePhase::Active)
            {
                if (Fighter.StateFrame < Move->ActiveFrames - 1u) Next = EFighterState::Attack;
                else { Fighter.MovePhase = EMovePhase::Recovery; bMovePhaseChanged = true; Next = EFighterState::Attack; }
            }
            else if (Fighter.MovePhase == EMovePhase::Recovery && Fighter.StateFrame < Move->RecoveryFrames - 1u)
            {
                Next = EFighterState::Attack;
            }
            else
            {
                Fighter.CurrentMoveId = EMoveId::None;
                Fighter.MovePhase = EMovePhase::Startup;
                Next = bDown ? EFighterState::Crouching : Direction ? EFighterState::Walking : EFighterState::Idle;
            }
        }
        else if (bDashing)
        {
            Next = Fighter.CurrentState;
            ClearTap(Fighter);
            Fighter.VelocityX = Fighter.DashDirection * (Next == EFighterState::ForwardDash
                ? Sim::ForwardDashSpeed : Sim::BackDashSpeed);
        }
        else if (Fighter.CurrentState == EFighterState::JumpStartup)
        {
            ClearTap(Fighter);
            Fighter.DashDirection = 0;
            Fighter.VelocityX = 0;
            Fighter.VelocityY = 0;
            if (Fighter.StateFrame < Sim::JumpStartupFrames - 1u)
            {
                Next = EFighterState::JumpStartup;
            }
            else
            {
                Fighter.bGrounded = false;
                Fighter.VelocityX = Fighter.CommittedJumpVelocityX;
                Fighter.VelocityY = Sim::JumpVerticalSpeed;
                Next = EFighterState::Jumping;
                bFreshAerialImpulse = true;
            }
        }
        else if (!Fighter.bGrounded)
        {
            ClearTap(Fighter);
            Fighter.DashDirection = 0;
            if (bJumpPressed && !Fighter.bDoubleJumpUsed)
            {
                Fighter.bDoubleJumpUsed = true;
                Fighter.CommittedJumpVelocityX = SelectJumpVelocityX(Direction, Fighter.Facing, true);
                Fighter.VelocityX = Fighter.CommittedJumpVelocityX;
                Fighter.VelocityY = Sim::DoubleJumpVerticalSpeed;
                Next = EFighterState::DoubleJumping;
                bFreshAerialImpulse = true;
            }
            else
            {
                Fighter.VelocityX = Fighter.CommittedJumpVelocityX;
                Next = Fighter.bDoubleJumpUsed ? EFighterState::DoubleJumping : EFighterState::Jumping;
            }
        }
        else
        {
            Fighter.DashDirection = 0;
            Fighter.CommittedJumpVelocityX = 0;
            Fighter.bDoubleJumpUsed = false;

            // A fresh jump press outranks crouch, walking, and a new dash command.
            if (Edges.Pressed.IsHeld(EFighterInputButton::Light))
            {
                ClearTap(Fighter);
                Fighter.VelocityX = 0;
                Fighter.VelocityY = 0;
                Fighter.CurrentMoveId = EMoveId::Light;
                Fighter.MovePhase = EMovePhase::Startup;
                Fighter.bCurrentMoveConnected = false;
                Next = EFighterState::Attack;
            }
            else if (bJumpPressed)
            {
                ClearTap(Fighter);
                Fighter.CommittedJumpVelocityX = SelectJumpVelocityX(Direction, Fighter.Facing, false);
                Fighter.VelocityX = 0;
                Fighter.VelocityY = 0;
                Next = EFighterState::JumpStartup;
            }
            else
            {
                if (!bDown)
                {
                    const bool bDirectionPressed = Direction != 0 && Edges.Pressed.IsHeld(
                        Direction > 0 ? EFighterInputButton::Right : EFighterInputButton::Left);
                    if (bDirectionPressed)
                    {
                        if (Fighter.LastTapDirection == Direction
                            && Fighter.TapAgeFrames <= Sim::DashInputWindowFrames)
                        {
                            Fighter.DashDirection = static_cast<int8>(Direction);
                            Next = Direction == static_cast<int32>(Fighter.Facing)
                                ? EFighterState::ForwardDash : EFighterState::BackDash;
                            bDashing = true;
                            ClearTap(Fighter);
                        }
                        else
                        {
                            Fighter.LastTapDirection = static_cast<int8>(Direction);
                            Fighter.TapAgeFrames = 0;
                        }
                    }
                    if (Input.IsHeld(EFighterInputButton::Left)
                        && Input.IsHeld(EFighterInputButton::Right))
                    {
                        ClearTap(Fighter);
                    }
                }
                else
                {
                    ClearTap(Fighter);
                }

                if (bDashing)
                {
                    Fighter.VelocityX = Fighter.DashDirection * (Next == EFighterState::ForwardDash
                        ? Sim::ForwardDashSpeed : Sim::BackDashSpeed);
                }
                else if (bDown)
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
        }

        MoveHorizontal(Fighter);
        if (!Fighter.bGrounded)
        {
            // A launch/reset frame exposes the exact configured impulse. Gravity
            // begins on the following airborne simulation frame.
            if (!bFreshAerialImpulse)
            {
                Fighter.VelocityY = AddBounded(Fighter.VelocityY, Sim::Gravity);
            }
            Fighter.PositionY = AddBounded(Fighter.PositionY, Fighter.VelocityY);
            if (Fighter.PositionY <= 0)
            {
                Fighter.PositionY = 0;
                Fighter.VelocityX = 0;
                Fighter.VelocityY = 0;
                Fighter.CommittedJumpVelocityX = 0;
                Fighter.bGrounded = true;
                Fighter.bDoubleJumpUsed = false;
                Next = bDown ? EFighterState::Crouching
                    : Direction != 0 ? EFighterState::Walking : EFighterState::Idle;
            }
        }
        else
        {
            Fighter.PositionY = 0;
            Fighter.VelocityY = 0;
        }
        SetState(Fighter, Next);
        if (bMovePhaseChanged) Fighter.StateFrame = 0;
    }

    void ResolvePush(FFighterState& Left, FFighterState& Right)
    {
        const int64 Deficit = static_cast<int64>(2 * Sim::PushHalfWidth)
            - (static_cast<int64>(Right.PositionX) - Left.PositionX);
        if (Deficit <= 0) { return; }
        int64 L = static_cast<int64>(Left.PositionX) - Deficit / 2;
        int64 R = static_cast<int64>(Right.PositionX) + (Deficit - Deficit / 2);
        if (L < Sim::MinFighterX) { R += Sim::MinFighterX - L; L = Sim::MinFighterX; }
        if (R > Sim::MaxFighterX) { L -= R - Sim::MaxFighterX; R = Sim::MaxFighterX; }
        Left.PositionX = static_cast<int32>(L);
        Right.PositionX = static_cast<int32>(R);
    }
}

const FMoveDefinition* FFightingSimulation::FindMoveDefinition(EMoveId MoveId)
{
    static constexpr FMoveDefinition LightMove{
        5, 3, 9, 50, 6, 14, 6 * SubunitsPerWorldUnit,
        44 * SubunitsPerWorldUnit, 55 * SubunitsPerWorldUnit,
        28 * SubunitsPerWorldUnit, 25 * SubunitsPerWorldUnit};
    return MoveId == EMoveId::Light ? &LightMove : nullptr;
}

FSimBox FFightingSimulation::GetHurtbox(const FFighterState& Fighter)
{
    return {Fighter.PositionX - HurtboxHalfWidth, Fighter.PositionX + HurtboxHalfWidth,
        Fighter.PositionY, Fighter.PositionY + HurtboxHeight};
}

bool FFightingSimulation::GetMoveHitbox(const FFighterState& Fighter, FSimBox& OutBox)
{
    if (Fighter.CurrentState != EFighterState::Attack || Fighter.MovePhase != EMovePhase::Active) return false;
    const FMoveDefinition* Move = FindMoveDefinition(Fighter.CurrentMoveId);
    if (!Move) return false;
    const int32 Sign = static_cast<int32>(Fighter.Facing);
    const int32 CenterX = Fighter.PositionX + Sign * Move->HitboxOffsetX;
    OutBox = {CenterX - Move->HitboxHalfWidth, CenterX + Move->HitboxHalfWidth,
        Fighter.PositionY + Move->HitboxOffsetY - Move->HitboxHalfHeight,
        Fighter.PositionY + Move->HitboxOffsetY + Move->HitboxHalfHeight};
    return true;
}

void FFightingSimulation::SimulateFrame(FMatchState& State,
    const FFighterInput& Player1Input, const FFighterInput& Player2Input)
{
    const int32 Start1 = State.Fighter1.PositionX;
    const int32 Start2 = State.Fighter2.PositionX;
    const bool bFirstIsLeft = Start1 < Start2 || (Start1 == Start2
        && (State.Fighter1.Facing == EFighterFacing::Right
            || State.Fighter2.Facing != EFighterFacing::Right));
    StepFighter(State.Fighter1, Player1Input);
    StepFighter(State.Fighter2, Player2Input);
    FSimBox Hitbox;
    const bool bHit12 = GetMoveHitbox(State.Fighter1, Hitbox) && !State.Fighter1.bCurrentMoveConnected
        && State.Fighter2.bGrounded && Hitbox.Overlaps(GetHurtbox(State.Fighter2));
    const bool bHit21 = GetMoveHitbox(State.Fighter2, Hitbox) && !State.Fighter2.bCurrentMoveConnected
        && State.Fighter1.bGrounded && Hitbox.Overlaps(GetHurtbox(State.Fighter1));
    if (bHit12) State.Fighter1.bCurrentMoveConnected = true;
    if (bHit21) State.Fighter2.bCurrentMoveConnected = true;
    if (bHit12 || bHit21)
    {
        const FMoveDefinition* Move1 = bHit12 ? FindMoveDefinition(State.Fighter1.CurrentMoveId) : nullptr;
        const FMoveDefinition* Move2 = bHit21 ? FindMoveDefinition(State.Fighter2.CurrentMoveId) : nullptr;
        if (bHit12) { ++State.HitEventCount; }
        if (bHit21) { ++State.HitEventCount; }
        if (bHit12)
        {
            State.Fighter2.Health = FMath::Max(0, State.Fighter2.Health - Move1->Damage);
            State.Fighter1.HitstopFramesRemaining = Move1->HitstopFrames;
            State.Fighter2.HitstopFramesRemaining = Move1->HitstopFrames;
            State.Fighter2.bPendingHitReaction = true;
            State.Fighter2.PendingHitstunFrames = Move1->HitstunFrames;
            State.Fighter2.PendingKnockbackVelocityX = State.Fighter2.PositionX >= State.Fighter1.PositionX ? Move1->KnockbackSpeed : -Move1->KnockbackSpeed;
        }
        if (bHit21)
        {
            State.Fighter1.Health = FMath::Max(0, State.Fighter1.Health - Move2->Damage);
            State.Fighter2.HitstopFramesRemaining = Move2->HitstopFrames;
            State.Fighter1.HitstopFramesRemaining = Move2->HitstopFrames;
            State.Fighter1.bPendingHitReaction = true;
            State.Fighter1.PendingHitstunFrames = Move2->HitstunFrames;
            State.Fighter1.PendingKnockbackVelocityX = State.Fighter1.PositionX >= State.Fighter2.PositionX ? Move2->KnockbackSpeed : -Move2->KnockbackSpeed;
        }
    }
    if (State.Fighter1.bGrounded && State.Fighter2.bGrounded
        && State.Fighter1.HitstopFramesRemaining == 0 && State.Fighter2.HitstopFramesRemaining == 0)
    {
        ResolvePush(bFirstIsLeft ? State.Fighter1 : State.Fighter2,
            bFirstIsLeft ? State.Fighter2 : State.Fighter1);
    }
    State.Fighter1.VelocityX = Bounded(static_cast<int64>(State.Fighter1.PositionX) - Start1,
        std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max());
    State.Fighter2.VelocityX = Bounded(static_cast<int64>(State.Fighter2.PositionX) - Start2,
        std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max());
    // Contact with a horizontal stage boundary reports zero velocity immediately
    // and stops future committed aerial movement, including a partial final step.
    if (!State.Fighter1.bGrounded && (State.Fighter1.PositionX == MinFighterX
        || State.Fighter1.PositionX == MaxFighterX))
    {
        State.Fighter1.VelocityX = 0;
        State.Fighter1.CommittedJumpVelocityX = 0;
    }
    if (!State.Fighter2.bGrounded && (State.Fighter2.PositionX == MinFighterX
        || State.Fighter2.PositionX == MaxFighterX))
    {
        State.Fighter2.VelocityX = 0;
        State.Fighter2.CommittedJumpVelocityX = 0;
    }
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
