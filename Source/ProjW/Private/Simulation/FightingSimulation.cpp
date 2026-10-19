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

    bool IsReactionStun(EFighterState State)
    {
        return State == EFighterState::Hitstun || State == EFighterState::Blockstun
            || State == EFighterState::ParryStun;
    }

    uint64 IntegerRoot(uint64 Value)
    {
        uint64 Root = 0, Bit = uint64(1) << 62;
        while (Bit > Value) Bit >>= 2;
        while (Bit != 0)
        {
            if (Value >= Root + Bit) { Value -= Root + Bit; Root = (Root >> 1) + Bit; }
            else Root >>= 1;
            Bit >>= 2;
        }
        return Root;
    }

    struct FAxis { int32 X, Z; };
    FAxis Toward(const FFighterState& Fighter, const FFighterState& Opponent)
    {
        // Legal positions are stage-bounded. Clamp malformed/restored inputs
        // before squaring so even INT32 extremes cannot overflow signed int64.
        const int64 DX = FMath::Clamp(static_cast<int64>(Opponent.PositionX) - Fighter.PositionX,
            -2LL * Sim::MaxFighterX, 2LL * Sim::MaxFighterX);
        const int64 DZ = FMath::Clamp(static_cast<int64>(Opponent.PositionZ) - Fighter.PositionZ,
            -2LL * Sim::MaxFighterZ, 2LL * Sim::MaxFighterZ);
        const uint64 Length = IntegerRoot(static_cast<uint64>(DX * DX + DZ * DZ));
        if (Length == 0) return {static_cast<int32>(Fighter.Facing) * Sim::AxisScale, 0};
        return {static_cast<int32>(DX * Sim::AxisScale / static_cast<int64>(Length)),
            static_cast<int32>(DZ * Sim::AxisScale / static_cast<int64>(Length))};
    }

    int32 Project(int32 Speed, int32 Component)
    { return static_cast<int32>(static_cast<int64>(Speed) * Component / Sim::AxisScale); }

    FAxis WorldRightAxis(const FAxis& Axis)
    { return Axis.X < 0 ? FAxis{-Axis.X, -Axis.Z} : Axis; }

    constexpr int32 PivotAngleWeights[Sim::PivotDurationFrames] =
        {4, 6, 8, 10, 10, 8, 6, 4, 2, 0, 0, 0};

    void ApplyPivotOrbit(FFighterState& Fighter, const FFighterState& Opponent, uint32 Frame)
    {
        // 8 degrees total, divided by 58 deterministic weights. Q20 trig series.
        constexpr int64 TrigScale = 1 << 20;
        constexpr int64 TotalAngleQ20 = 146408;
        const int64 Angle = static_cast<int64>(Fighter.PivotDirection)
            * TotalAngleQ20 * PivotAngleWeights[Frame] / 58;
        const int64 Sine = Angle - Angle * Angle * Angle / (6 * TrigScale * TrigScale);
        const int64 Cosine = TrigScale - Angle * Angle / (2 * TrigScale);
        const int64 DX = static_cast<int64>(Fighter.PositionX) - Opponent.PositionX;
        const int64 DZ = static_cast<int64>(Fighter.PositionZ) - Opponent.PositionZ;
        Sim::ConstrainToArena(static_cast<int64>(Opponent.PositionX)
            + (DX * Cosine - DZ * Sine) / TrigScale,
            static_cast<int64>(Opponent.PositionZ) + (DX * Sine + DZ * Cosine) / TrigScale,
            Fighter.PositionX, Fighter.PositionZ);
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

    int32 HorizontalDirection(const FFighterInput& Input)
    {
        return static_cast<int32>(Input.IsHeld(EFighterInputButton::Right))
            - static_cast<int32>(Input.IsHeld(EFighterInputButton::Left));
    }

    EMoveId SelectPressedMove(const FFighterInput& Pressed)
    {
        // Priority is explicit and independent of enum values.
        if (Pressed.IsHeld(EFighterInputButton::Heavy)) { return EMoveId::Heavy; }
        if (Pressed.IsHeld(EFighterInputButton::Medium)) { return EMoveId::Medium; }
        if (Pressed.IsHeld(EFighterInputButton::Light)) { return EMoveId::Light; }
        if (Pressed.IsHeld(EFighterInputButton::GroundRupture)) { return EMoveId::GroundRupture; }
        return EMoveId::None;
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
        const int64 IntendedX = static_cast<int64>(Fighter.PositionX) + Fighter.VelocityX;
        const int64 IntendedZ = static_cast<int64>(Fighter.PositionZ) + Fighter.VelocityZ;
        if (Sim::ConstrainToArena(IntendedX, IntendedZ, Fighter.PositionX, Fighter.PositionZ))
        {
            // Remove only outward radial momentum. Tangential/inward motion remains
            // available on later frames; no reflected (bouncing) component exists.
            const int64 Radius2 = static_cast<int64>(Fighter.PositionX) * Fighter.PositionX
                + static_cast<int64>(Fighter.PositionZ) * Fighter.PositionZ;
            const auto RemoveOutward = [&](int32& X, int32& Z)
            {
                const int64 Dot = static_cast<int64>(X) * Fighter.PositionX
                    + static_cast<int64>(Z) * Fighter.PositionZ;
                if (Dot > 0 && Radius2 > 0)
                {
                    X -= static_cast<int32>(Dot * Fighter.PositionX / Radius2);
                    Z -= static_cast<int32>(Dot * Fighter.PositionZ / Radius2);
                }
            };
            RemoveOutward(Fighter.CommittedJumpVelocityX, Fighter.CommittedJumpVelocityZ);
            RemoveOutward(Fighter.PendingKnockbackVelocityX, Fighter.PendingKnockbackVelocityZ);
        }
    }

    void SetLocalVelocity(FFighterState& Fighter, const FAxis& TowardOpponent, int32 WorldDirection, int32 Speed)
    {
        const FAxis Right = WorldRightAxis(TowardOpponent);
        Fighter.VelocityX = Project(WorldDirection * Speed, Right.X);
        Fighter.VelocityZ = Project(WorldDirection * Speed, Right.Z);
    }

    void StepFighter(FFighterState& Fighter, const FFighterState& Opponent, FFighterInput Input)
    {
        Sim::ConstrainToArena(Fighter.PositionX, Fighter.PositionZ,
            Fighter.PositionX, Fighter.PositionZ);
        const FAxis Axis = Toward(Fighter, Opponent);
        const FFighterInputEdges Edges = FFighterInputEdges::From(Input, Fighter.PreviousInput);
        Fighter.PreviousInput = Edges.Held;
        Fighter.PressedInput = Edges.Pressed;
        Fighter.ReleasedInput = Edges.Released;
        if (Fighter.HitstopFramesRemaining > 0)
        {
            --Fighter.HitstopFramesRemaining;
            return;
        }
        bool bEnteredReaction = false;
        if ((Fighter.PendingReaction == EPendingReaction::Hit && Fighter.CurrentState != EFighterState::Hitstun)
            || (Fighter.PendingReaction == EPendingReaction::Block && Fighter.CurrentState != EFighterState::Blockstun)
            || (Fighter.PendingReaction == EPendingReaction::Parry && Fighter.CurrentState != EFighterState::ParryStun))
        {
            Fighter.CurrentState = Fighter.PendingReaction == EPendingReaction::Hit ? EFighterState::Hitstun
                : Fighter.PendingReaction == EPendingReaction::Block ? EFighterState::Blockstun : EFighterState::ParryStun;
            Fighter.StateFrame = 0;
            Fighter.CurrentMoveId = EMoveId::None;
            Fighter.MovePhase = EMovePhase::Startup;
            Fighter.PivotDirection = 0;
            Fighter.ParryHeight = EParryHeight::None;
            Fighter.AttackAxisX = 0;
            Fighter.AttackAxisZ = 0;
            bEnteredReaction = true;
        }
        if (IsReactionStun(Fighter.CurrentState))
        {
            Fighter.bGrounded = true;
            Fighter.PositionY = 0;
            Fighter.VelocityY = 0;
            Fighter.VelocityX = Fighter.PendingKnockbackVelocityX;
            Fighter.VelocityZ = Fighter.PendingKnockbackVelocityZ;
            MoveHorizontal(Fighter);
            if (bEnteredReaction)
            {
                // Entry is StateFrame 0; this call performs the first
                // knockback step without incrementing the state timer.
            }
            else if (Fighter.StateFrame < static_cast<uint32>(Fighter.PendingReactionFrames) - 1u)
            {
                SetState(Fighter, Fighter.CurrentState);
            }
            else
            {
                Fighter.PendingReaction = EPendingReaction::None;
                Fighter.PendingReactionFrames = 0;
                Fighter.PendingKnockbackVelocityX = 0;
                Fighter.PendingKnockbackVelocityZ = 0;
                Fighter.VelocityX = 0;
                Fighter.VelocityZ = 0;
                const int32 Direction = HorizontalDirection(Input);
                const bool bDown = Input.IsHeld(EFighterInputButton::Down);
                SetState(Fighter, bDown ? EFighterState::Crouching : Direction ? EFighterState::Walking : EFighterState::Idle);
            }
            return;
        }
        if (Fighter.CurrentState == EFighterState::ParrySuccess)
        {
            Fighter.ParryHeight = EParryHeight::None;
            SetState(Fighter, EFighterState::Idle);
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

        if (Fighter.CurrentState == EFighterState::Parrying)
        {
            Fighter.VelocityX = Fighter.VelocityY = Fighter.VelocityZ = 0;
            ClearTap(Fighter);
            if (Fighter.StateFrame < Sim::ParryTotalFrames - 1u) Next = EFighterState::Parrying;
            else { Fighter.ParryHeight = EParryHeight::None; Next = EFighterState::Idle; }
        }
        else if (Fighter.CurrentState == EFighterState::Pivoting)
        {
            Fighter.VelocityX = 0;
            Fighter.VelocityZ = 0;
            Fighter.VelocityY = 0;
            ClearTap(Fighter);
            if (Fighter.StateFrame < Sim::PivotDurationFrames - 1u)
            {
                ApplyPivotOrbit(Fighter, Opponent, Fighter.StateFrame + 1u);
                Next = EFighterState::Pivoting;
            }
            else
            {
                Fighter.PivotDirection = 0;
                Next = EFighterState::Idle;
            }
        }
        else if (IsAttack(Fighter.CurrentState))
        {
            Fighter.bGrounded = true;
            Fighter.PositionY = 0;
            Fighter.VelocityX = 0;
            Fighter.VelocityZ = 0;
            Fighter.VelocityY = 0;
            ClearTap(Fighter);
            const FMoveDefinition* Move = Sim::FindMoveDefinition(Fighter.CurrentMoveId);
            if (!Move)
            {
                Fighter.CurrentMoveId = EMoveId::None;
                Fighter.MovePhase = EMovePhase::Startup;
                Fighter.AttackAxisX = Fighter.AttackAxisZ = 0;
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
                Fighter.AttackAxisX = Fighter.AttackAxisZ = 0;
                Next = bDown ? EFighterState::Crouching : Direction ? EFighterState::Walking : EFighterState::Idle;
            }
        }
        else if (bDashing)
        {
            Next = Fighter.CurrentState;
            ClearTap(Fighter);
            SetLocalVelocity(Fighter, Axis, Fighter.DashDirection, Next == EFighterState::ForwardDash
                ? Sim::ForwardDashSpeed : Sim::BackDashSpeed);
        }
        else if (Fighter.CurrentState == EFighterState::JumpStartup)
        {
            ClearTap(Fighter);
            Fighter.DashDirection = 0;
            Fighter.VelocityX = 0;
            Fighter.VelocityZ = 0;
            Fighter.VelocityY = 0;
            if (Fighter.StateFrame < Sim::JumpStartupFrames - 1u)
            {
                Next = EFighterState::JumpStartup;
            }
            else
            {
                Fighter.bGrounded = false;
                Fighter.VelocityX = Fighter.CommittedJumpVelocityX;
                Fighter.VelocityZ = Fighter.CommittedJumpVelocityZ;
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
                const FAxis Right = WorldRightAxis(Axis);
                Fighter.CommittedJumpVelocityZ = Project(Fighter.CommittedJumpVelocityX, Right.Z);
                Fighter.CommittedJumpVelocityX = Project(Fighter.CommittedJumpVelocityX, Right.X);
                Fighter.VelocityX = Fighter.CommittedJumpVelocityX;
                Fighter.VelocityZ = Fighter.CommittedJumpVelocityZ;
                Fighter.VelocityY = Sim::DoubleJumpVerticalSpeed;
                Next = EFighterState::DoubleJumping;
                bFreshAerialImpulse = true;
            }
            else
            {
                Fighter.VelocityX = Fighter.CommittedJumpVelocityX;
                Fighter.VelocityZ = Fighter.CommittedJumpVelocityZ;
                Next = Fighter.bDoubleJumpUsed ? EFighterState::DoubleJumping : EFighterState::Jumping;
            }
        }
        else
        {
            Fighter.DashDirection = 0;
            Fighter.CommittedJumpVelocityX = 0;
            Fighter.CommittedJumpVelocityZ = 0;
            Fighter.bDoubleJumpUsed = false;

            const bool bPivotPositive = Edges.Pressed.IsHeld(EFighterInputButton::PivotPositive);
            const bool bPivotNegative = Edges.Pressed.IsHeld(EFighterInputButton::PivotNegative);
            const bool bHighParry = Edges.Pressed.IsHeld(EFighterInputButton::HighParry);
            const bool bLowParry = Edges.Pressed.IsHeld(EFighterInputButton::LowParry);
            const EMoveId PressedMove = SelectPressedMove(Edges.Pressed);
            if (bPivotPositive != bPivotNegative)
            {
                ClearTap(Fighter);
                Fighter.VelocityX = 0;
                Fighter.VelocityZ = 0;
                Fighter.VelocityY = 0;
                Fighter.PivotDirection = bPivotPositive ? 1 : -1;
                ApplyPivotOrbit(Fighter, Opponent, 0);
                Next = EFighterState::Pivoting;
            }
            else if (bHighParry != bLowParry)
            {
                ClearTap(Fighter);
                Fighter.VelocityX = Fighter.VelocityY = Fighter.VelocityZ = 0;
                Fighter.ParryHeight = bHighParry ? EParryHeight::High : EParryHeight::Low;
                Next = EFighterState::Parrying;
            }
            else if (PressedMove != EMoveId::None)
            {
                ClearTap(Fighter);
                Fighter.VelocityX = 0;
                Fighter.VelocityZ = 0;
                Fighter.VelocityY = 0;
                Fighter.CurrentMoveId = PressedMove;
                Fighter.MovePhase = EMovePhase::Startup;
                Fighter.bCurrentMoveConnected = false;
                Fighter.AttackAxisX = Axis.X;
                Fighter.AttackAxisZ = Axis.Z;
                Next = EFighterState::Attack;
            }
            else if (bJumpPressed)
            {
                ClearTap(Fighter);
                Fighter.CommittedJumpVelocityX = SelectJumpVelocityX(Direction, Fighter.Facing, false);
                const FAxis Right = WorldRightAxis(Axis);
                Fighter.CommittedJumpVelocityZ = Project(Fighter.CommittedJumpVelocityX, Right.Z);
                Fighter.CommittedJumpVelocityX = Project(Fighter.CommittedJumpVelocityX, Right.X);
                Fighter.VelocityX = 0;
                Fighter.VelocityZ = 0;
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
                    SetLocalVelocity(Fighter, Axis, Fighter.DashDirection, Next == EFighterState::ForwardDash
                        ? Sim::ForwardDashSpeed : Sim::BackDashSpeed);
                }
                else if (bDown)
                {
                    Fighter.VelocityX = 0;
                    Fighter.VelocityZ = 0;
                    Next = EFighterState::Crouching;
                }
                else
                {
                    SetLocalVelocity(Fighter, Axis, Direction, Direction == static_cast<int32>(Fighter.Facing)
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
                Fighter.VelocityZ = 0;
                Fighter.VelocityY = 0;
                Fighter.CommittedJumpVelocityX = 0;
                Fighter.CommittedJumpVelocityZ = 0;
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

    void ResolvePush(FFighterState& First, FFighterState& Second)
    {
        const int64 DX = static_cast<int64>(Second.PositionX) - First.PositionX;
        const int64 DZ = static_cast<int64>(Second.PositionZ) - First.PositionZ;
        const int64 Distance = static_cast<int64>(IntegerRoot(static_cast<uint64>(DX * DX + DZ * DZ)));
        const int64 Deficit = static_cast<int64>(2 * Sim::PushHalfWidth) - Distance;
        if (Deficit <= 0) { return; }
        const int64 UX = Distance ? DX * Sim::AxisScale / Distance
            : static_cast<int32>(First.Facing) * Sim::AxisScale;
        const int64 UZ = Distance ? DZ * Sim::AxisScale / Distance : 0;
        const int64 FirstShift = Deficit / 2;
        const int64 SecondShift = Deficit - FirstShift;
        Sim::ConstrainToArena(static_cast<int64>(First.PositionX) - FirstShift * UX / Sim::AxisScale,
            static_cast<int64>(First.PositionZ) - FirstShift * UZ / Sim::AxisScale,
            First.PositionX, First.PositionZ);
        Sim::ConstrainToArena(static_cast<int64>(Second.PositionX) + SecondShift * UX / Sim::AxisScale,
            static_cast<int64>(Second.PositionZ) + SecondShift * UZ / Sim::AxisScale,
            Second.PositionX, Second.PositionZ);
        // If one side was bounded, transfer its unfulfilled separation to the
        // other side along the same local axis. A fixed pass count is deterministic.
        for (int32 Pass = 0; Pass < 3; ++Pass)
        {
            const int64 RemainingX = static_cast<int64>(Second.PositionX) - First.PositionX;
            const int64 RemainingZ = static_cast<int64>(Second.PositionZ) - First.PositionZ;
            const int64 Remaining = 2LL * Sim::PushHalfWidth
                - static_cast<int64>(IntegerRoot(static_cast<uint64>(RemainingX * RemainingX + RemainingZ * RemainingZ)));
            if (Remaining <= 0) break;
            const int32 OldFirstX = First.PositionX, OldFirstZ = First.PositionZ;
            Sim::ConstrainToArena(static_cast<int64>(First.PositionX) - (Remaining + 1) * UX / Sim::AxisScale,
                static_cast<int64>(First.PositionZ) - (Remaining + 1) * UZ / Sim::AxisScale,
                First.PositionX, First.PositionZ);
            const int64 ActualFirstX = static_cast<int64>(OldFirstX) - First.PositionX;
            const int64 ActualFirstZ = static_cast<int64>(OldFirstZ) - First.PositionZ;
            const int64 Achieved = (ActualFirstX * UX + ActualFirstZ * UZ) / Sim::AxisScale;
            if (Achieved < Remaining)
            {
                Sim::ConstrainToArena(static_cast<int64>(Second.PositionX)
                    + (Remaining - Achieved + 1) * UX / Sim::AxisScale,
                    static_cast<int64>(Second.PositionZ)
                    + (Remaining - Achieved + 1) * UZ / Sim::AxisScale,
                    Second.PositionX, Second.PositionZ);
            }
        }
    }

    // Per-frame results only. A future ruleset can classify a valid geometric
    // contact here without changing move progression or pair resolution.
    enum class EContactOutcome : uint8 { None = 0, Hit = 1, Block = 2, Parry = 3 };

    struct FContactResult
    {
        EContactOutcome Outcome = EContactOutcome::None;
        const FMoveDefinition* Move = nullptr;
    };

    bool HasValidGroundedOverlap(const FFighterState& Attacker, const FFighterState& Defender,
        const FMoveDefinition* Move)
    {
        if (Attacker.bCurrentMoveConnected || !Defender.bGrounded || !Move) return false;
        const FSimBox Hurtbox = Sim::GetHurtbox(Defender);
        if (Move->HitVolumeShape == EHitVolumeShape::HorizontalCircle)
        {
            FSimRadialVolume Volume;
            return Sim::GetMoveRadialVolume(Attacker, Volume) && Volume.Overlaps(Hurtbox);
        }
        FSimOrientedBox Volume;
        return Sim::GetMoveHitVolume(Attacker, Volume) && Volume.Overlaps(Hurtbox);
    }

    FContactResult ClassifyContact(const FFighterState& Attacker, const FFighterState& Defender)
    {
        const FMoveDefinition* Move = Sim::FindMoveDefinition(Attacker.CurrentMoveId);
        if (!HasValidGroundedOverlap(Attacker, Defender, Move)) { return {}; }
        if (Defender.CurrentState == EFighterState::Parrying
            && Defender.StateFrame < Sim::ParryActiveFrames
            && ((Defender.ParryHeight == EParryHeight::High && Move->ParryHeight == EParryHeight::High)
                || (Defender.ParryHeight == EParryHeight::Low && Move->ParryHeight == EParryHeight::Low)))
        {
            return {EContactOutcome::Parry, Move};
        }
        const bool bCanGuard = Defender.CurrentState == EFighterState::Idle
            || Defender.CurrentState == EFighterState::Walking
            || Defender.CurrentState == EFighterState::Crouching;
        const EFighterInputButton Away = Defender.Facing == EFighterFacing::Right
            ? EFighterInputButton::Left : EFighterInputButton::Right;
        const bool bCrouching = Defender.PreviousInput.IsHeld(EFighterInputButton::Down);
        const bool bHeightMatches = bCrouching ? Move->GuardHeight == EGuardHeight::Low
            : Move->GuardHeight == EGuardHeight::High;
        return {bCanGuard && Defender.PreviousInput.IsHeld(Away) && bHeightMatches
            ? EContactOutcome::Block : EContactOutcome::Hit, Move};
    }
}

bool FFightingSimulation::IsInsideArena(int32 X, int32 Z)
{
    const uint64 AX = X < 0 ? static_cast<uint64>(-static_cast<int64>(X)) : static_cast<uint64>(X);
    const uint64 AZ = Z < 0 ? static_cast<uint64>(-static_cast<int64>(Z)) : static_cast<uint64>(Z);
    return AX * AX + AZ * AZ <= static_cast<uint64>(FighterCenterRadiusSquared);
}

bool FFightingSimulation::ConstrainToArena(int64 IntendedX, int64 IntendedZ, int32& OutX, int32& OutZ)
{
    // Cap malformed int64 intended steps before squaring. A pair of signed
    // int32 magnitudes squared fits uint64, including INT32_MIN on both axes.
    const int64 X = FMath::Clamp(IntendedX,
        static_cast<int64>(std::numeric_limits<int32>::min()),
        static_cast<int64>(std::numeric_limits<int32>::max()));
    const int64 Z = FMath::Clamp(IntendedZ,
        static_cast<int64>(std::numeric_limits<int32>::min()),
        static_cast<int64>(std::numeric_limits<int32>::max()));
    const uint64 AX = X < 0 ? static_cast<uint64>(-X) : static_cast<uint64>(X);
    const uint64 AZ = Z < 0 ? static_cast<uint64>(-Z) : static_cast<uint64>(Z);
    const uint64 Squared = AX * AX + AZ * AZ;
    if (Squared <= static_cast<uint64>(FighterCenterRadiusSquared))
    {
        OutX = static_cast<int32>(X);
        OutZ = static_cast<int32>(Z);
        return false;
    }
    // Ceiling integer sqrt makes the scale at most Radius / true distance.
    // Truncating both components toward zero therefore always lands inside.
    uint64 Length = IntegerRoot(Squared);
    if (Length * Length < Squared) ++Length;
    OutX = static_cast<int32>(X * FighterCenterRadius / static_cast<int64>(Length));
    OutZ = static_cast<int32>(Z * FighterCenterRadius / static_cast<int64>(Length));
    return true;
}

const FMoveDefinition* FFightingSimulation::FindMoveDefinition(EMoveId MoveId)
{
    static constexpr FMoveDefinition LightMove{
        5, 3, 9, 50, 6, 14, 6 * SubunitsPerWorldUnit,
        EGuardHeight::High, EParryHeight::High, 5, 10, 3 * SubunitsPerWorldUnit,
        44 * SubunitsPerWorldUnit, 55 * SubunitsPerWorldUnit,
        28 * SubunitsPerWorldUnit, 25 * SubunitsPerWorldUnit,
        4 * SubunitsPerWorldUnit, EHitVolumeShape::OrientedBox, 0};
    static constexpr FMoveDefinition MediumMove{
        7, 3, 12, 80, 7, 17, 7 * SubunitsPerWorldUnit,
        EGuardHeight::Low, EParryHeight::High, 8, 13, 4 * SubunitsPerWorldUnit,
        50 * SubunitsPerWorldUnit, 55 * SubunitsPerWorldUnit,
        35 * SubunitsPerWorldUnit, 25 * SubunitsPerWorldUnit,
        30 * SubunitsPerWorldUnit, EHitVolumeShape::OrientedBox, 0};
    static constexpr FMoveDefinition HeavyMove{
        10, 4, 16, 120, 9, 20, 9 * SubunitsPerWorldUnit,
        EGuardHeight::High, EParryHeight::High, 12, 16, 5 * SubunitsPerWorldUnit,
        66 * SubunitsPerWorldUnit, 55 * SubunitsPerWorldUnit,
        44 * SubunitsPerWorldUnit, 25 * SubunitsPerWorldUnit,
        6 * SubunitsPerWorldUnit, EHitVolumeShape::OrientedBox, 0};
    static constexpr FMoveDefinition GroundRuptureMove{
        18, 5, 24, 100, 8, 18, 5 * SubunitsPerWorldUnit,
        EGuardHeight::Low, EParryHeight::High, 10, 16, 3 * SubunitsPerWorldUnit,
        0, 15 * SubunitsPerWorldUnit, 0, 15 * SubunitsPerWorldUnit, 0,
        EHitVolumeShape::HorizontalCircle, GroundRuptureRadius};
    switch (MoveId)
    {
        case EMoveId::Light: return &LightMove;
        case EMoveId::Medium: return &MediumMove;
        case EMoveId::Heavy: return &HeavyMove;
        case EMoveId::GroundRupture: return &GroundRuptureMove;
        default: return nullptr;
    }
}

FSimBox FFightingSimulation::GetHurtbox(const FFighterState& Fighter)
{
    return {Fighter.PositionX - HurtboxHalfWidth, Fighter.PositionX + HurtboxHalfWidth,
        Fighter.PositionY, Fighter.PositionY + HurtboxHeight,
        Fighter.PositionZ - HurtboxHalfExtentZ, Fighter.PositionZ + HurtboxHalfExtentZ};
}

bool FSimOrientedBox::Overlaps(const FSimBox& Other) const
{
    if (MinY >= Other.MaxY || MaxY <= Other.MinY) return false;
    constexpr int64 Q = FFightingSimulation::AxisScale;
    const int64 DX = static_cast<int64>(Other.MinX) + Other.MaxX - 2LL * CenterX;
    const int64 DZ = static_cast<int64>(Other.MinZ) + Other.MaxZ - 2LL * CenterZ;
    const int64 HalfX = static_cast<int64>(Other.MaxX) - Other.MinX;
    const int64 HalfZ = static_cast<int64>(Other.MaxZ) - Other.MinZ;
    const auto StrictAxis = [&](int64 X, int64 Z)
    {
        const int64 CenterDistance = FMath::Abs(DX * X + DZ * Z) * Q;
        const int64 OurRadius = static_cast<int64>(HalfForward) * FMath::Abs(static_cast<int64>(AxisX) * X + static_cast<int64>(AxisZ) * Z)
            + static_cast<int64>(HalfLateral) * FMath::Abs(-static_cast<int64>(AxisZ) * X + static_cast<int64>(AxisX) * Z);
        const int64 TheirRadius = (HalfX * FMath::Abs(X) + HalfZ * FMath::Abs(Z)) * Q;
        return CenterDistance < OurRadius * 2 + TheirRadius;
    };
    return StrictAxis(Q, 0) && StrictAxis(0, Q)
        && StrictAxis(AxisX, AxisZ) && StrictAxis(-AxisZ, AxisX);
}

bool FSimRadialVolume::Overlaps(const FSimBox& Other) const
{
    if (MinY >= Other.MaxY || MaxY <= Other.MinY || Radius <= 0) return false;
    const int64 ClosestX = FMath::Clamp(static_cast<int64>(CenterX),
        static_cast<int64>(Other.MinX), static_cast<int64>(Other.MaxX));
    const int64 ClosestZ = FMath::Clamp(static_cast<int64>(CenterZ),
        static_cast<int64>(Other.MinZ), static_cast<int64>(Other.MaxZ));
    const int64 DX = static_cast<int64>(CenterX) - ClosestX;
    const int64 DZ = static_cast<int64>(CenterZ) - ClosestZ;
    const int64 R = Radius;
    return DX * DX + DZ * DZ < R * R;
}

FSimBox FSimOrientedBox::Bounds() const
{
    constexpr int64 Q = FFightingSimulation::AxisScale;
    const int32 EX = static_cast<int32>((static_cast<int64>(HalfForward) * FMath::Abs(AxisX)
        + static_cast<int64>(HalfLateral) * FMath::Abs(AxisZ) + Q - 1) / Q);
    const int32 EZ = static_cast<int32>((static_cast<int64>(HalfForward) * FMath::Abs(AxisZ)
        + static_cast<int64>(HalfLateral) * FMath::Abs(AxisX) + Q - 1) / Q);
    return {CenterX - EX, CenterX + EX, MinY, MaxY, CenterZ - EZ, CenterZ + EZ};
}

bool FFightingSimulation::GetMoveHitVolume(const FFighterState& Fighter, FSimOrientedBox& OutBox)
{
    if (Fighter.CurrentState != EFighterState::Attack || Fighter.MovePhase != EMovePhase::Active) return false;
    const FMoveDefinition* Move = FindMoveDefinition(Fighter.CurrentMoveId);
    if (!Move || Move->HitVolumeShape != EHitVolumeShape::OrientedBox) return false;
    const int32 AX = Fighter.AttackAxisX || Fighter.AttackAxisZ
        ? Fighter.AttackAxisX : static_cast<int32>(Fighter.Facing) * AxisScale;
    const int32 AZ = Fighter.AttackAxisX || Fighter.AttackAxisZ ? Fighter.AttackAxisZ : 0;
    OutBox = {Fighter.PositionX + static_cast<int32>(static_cast<int64>(AX) * Move->HitboxOffsetX / AxisScale),
        Fighter.PositionZ + static_cast<int32>(static_cast<int64>(AZ) * Move->HitboxOffsetX / AxisScale),
        Fighter.PositionY + Move->HitboxOffsetY - Move->HitboxHalfHeight,
        Fighter.PositionY + Move->HitboxOffsetY + Move->HitboxHalfHeight,
        AX, AZ, Move->HitboxHalfWidth, Move->HitboxHalfExtentZ};
    return true;
}

bool FFightingSimulation::GetMoveRadialVolume(const FFighterState& Fighter, FSimRadialVolume& OutVolume)
{
    if (Fighter.CurrentState != EFighterState::Attack || Fighter.MovePhase != EMovePhase::Active) return false;
    const FMoveDefinition* Move = FindMoveDefinition(Fighter.CurrentMoveId);
    if (!Move || Move->HitVolumeShape != EHitVolumeShape::HorizontalCircle) return false;
    OutVolume = {Fighter.PositionX, Fighter.PositionZ,
        Fighter.PositionY + Move->HitboxOffsetY - Move->HitboxHalfHeight,
        Fighter.PositionY + Move->HitboxOffsetY + Move->HitboxHalfHeight, Move->HitRadius};
    return true;
}

bool FFightingSimulation::GetMoveHitbox(const FFighterState& Fighter, FSimBox& OutBox)
{
    FSimOrientedBox Volume;
    if (!GetMoveHitVolume(Fighter, Volume)) return false;
    OutBox = Volume.Bounds();
    return true;
}

void FFightingSimulation::SimulateFrame(FMatchState& State,
    const FFighterInput& Player1Input, const FFighterInput& Player2Input)
{
    const int32 Start1 = State.Fighter1.PositionX;
    const int32 Start2 = State.Fighter2.PositionX;
    const int32 StartZ1 = State.Fighter1.PositionZ;
    const int32 StartZ2 = State.Fighter2.PositionZ;
    const FFighterState Before1 = State.Fighter1;
    const FFighterState Before2 = State.Fighter2;
    const bool bFirstIsLeft = Start1 < Start2 || (Start1 == Start2
        && (State.Fighter1.Facing == EFighterFacing::Right
            || State.Fighter2.Facing != EFighterFacing::Right));
    StepFighter(State.Fighter1, Before2, Player1Input);
    StepFighter(State.Fighter2, Before1, Player2Input);
    const FContactResult Contact12 = ClassifyContact(State.Fighter1, State.Fighter2);
    const FContactResult Contact21 = ClassifyContact(State.Fighter2, State.Fighter1);
    const bool bContact12 = Contact12.Outcome != EContactOutcome::None;
    const bool bContact21 = Contact21.Outcome != EContactOutcome::None;
    if (bContact12) State.Fighter1.bCurrentMoveConnected = true;
    if (bContact21) State.Fighter2.bCurrentMoveConnected = true;
    if (bContact12 || bContact21)
    {
        const FMoveDefinition* Move1 = Contact12.Move;
        const FMoveDefinition* Move2 = Contact21.Move;
        const uint8 Freeze12 = Contact12.Outcome == EContactOutcome::Parry
            ? ParryHitstopFrames : Move1 ? Move1->HitstopFrames : 0;
        const uint8 Freeze21 = Contact21.Outcome == EContactOutcome::Parry
            ? ParryHitstopFrames : Move2 ? Move2->HitstopFrames : 0;
        const uint8 SharedHitstop = bContact12 && bContact21
            ? FMath::Max(Freeze12, Freeze21) : bContact12 ? Freeze12 : Freeze21;
        if (Contact12.Outcome == EContactOutcome::Hit) { ++State.HitEventCount; }
        if (Contact21.Outcome == EContactOutcome::Hit) { ++State.HitEventCount; }
        if (bContact12)
        {
            const bool bBlocked = Contact12.Outcome == EContactOutcome::Block;
            if (Contact12.Outcome == EContactOutcome::Parry)
            {
                State.Fighter2.CurrentState = EFighterState::ParrySuccess;
                State.Fighter1.PendingReaction = EPendingReaction::Parry;
                State.Fighter1.PendingReactionFrames = ParryStunFrames;
                State.Fighter1.PendingKnockbackVelocityX = State.Fighter1.PendingKnockbackVelocityZ = 0;
            }
            else
            {
                State.Fighter2.Health = FMath::Max(bBlocked && State.Fighter2.Health > 0 ? 1 : 0,
                    State.Fighter2.Health - (bBlocked ? Move1->ChipDamage : Move1->Damage));
                State.Fighter2.PendingReaction = bBlocked ? EPendingReaction::Block : EPendingReaction::Hit;
                State.Fighter2.PendingReactionFrames = bBlocked ? Move1->BlockstunFrames : Move1->HitstunFrames;
                const int32 Speed = bBlocked ? Move1->BlockPushbackSpeed : Move1->KnockbackSpeed;
                const FAxis Reaction = Toward(State.Fighter1, State.Fighter2);
                State.Fighter2.PendingKnockbackVelocityX = Project(Speed, Reaction.X);
                State.Fighter2.PendingKnockbackVelocityZ = Project(Speed, Reaction.Z);
            }
        }
        if (bContact21)
        {
            const bool bBlocked = Contact21.Outcome == EContactOutcome::Block;
            if (Contact21.Outcome == EContactOutcome::Parry)
            {
                State.Fighter1.CurrentState = EFighterState::ParrySuccess;
                State.Fighter2.PendingReaction = EPendingReaction::Parry;
                State.Fighter2.PendingReactionFrames = ParryStunFrames;
                State.Fighter2.PendingKnockbackVelocityX = State.Fighter2.PendingKnockbackVelocityZ = 0;
            }
            else
            {
                State.Fighter1.Health = FMath::Max(bBlocked && State.Fighter1.Health > 0 ? 1 : 0,
                    State.Fighter1.Health - (bBlocked ? Move2->ChipDamage : Move2->Damage));
                State.Fighter1.PendingReaction = bBlocked ? EPendingReaction::Block : EPendingReaction::Hit;
                State.Fighter1.PendingReactionFrames = bBlocked ? Move2->BlockstunFrames : Move2->HitstunFrames;
                const int32 Speed = bBlocked ? Move2->BlockPushbackSpeed : Move2->KnockbackSpeed;
                const FAxis Reaction = Toward(State.Fighter2, State.Fighter1);
                State.Fighter1.PendingKnockbackVelocityX = Project(Speed, Reaction.X);
                State.Fighter1.PendingKnockbackVelocityZ = Project(Speed, Reaction.Z);
            }
        }
        State.Fighter1.HitstopFramesRemaining = SharedHitstop;
        State.Fighter2.HitstopFramesRemaining = SharedHitstop;
    }
    if (State.Fighter1.bGrounded && State.Fighter2.bGrounded
        && State.Fighter1.HitstopFramesRemaining == 0 && State.Fighter2.HitstopFramesRemaining == 0)
    {
        if (GetHurtbox(State.Fighter1).Overlaps(GetHurtbox(State.Fighter2)))
        {
            ResolvePush(bFirstIsLeft ? State.Fighter1 : State.Fighter2,
                bFirstIsLeft ? State.Fighter2 : State.Fighter1);
        }
    }
    State.Fighter1.VelocityX = Bounded(static_cast<int64>(State.Fighter1.PositionX) - Start1,
        std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max());
    State.Fighter2.VelocityX = Bounded(static_cast<int64>(State.Fighter2.PositionX) - Start2,
        std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max());
    State.Fighter1.VelocityZ = Bounded(static_cast<int64>(State.Fighter1.PositionZ) - StartZ1,
        std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max());
    State.Fighter2.VelocityZ = Bounded(static_cast<int64>(State.Fighter2.PositionZ) - StartZ2,
        std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max());
    // Preserve the established aerial wall report: a final partial outward
    // step reports zero horizontal velocity once all committed motion is gone.
    // A surviving tangent component continues to report its actual movement.
    for (FFighterState* Fighter : {&State.Fighter1, &State.Fighter2})
    {
        if (!Fighter->bGrounded && Fighter->CommittedJumpVelocityX == 0
            && Fighter->CommittedJumpVelocityZ == 0)
        {
            Fighter->VelocityX = 0;
            Fighter->VelocityZ = 0;
        }
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
