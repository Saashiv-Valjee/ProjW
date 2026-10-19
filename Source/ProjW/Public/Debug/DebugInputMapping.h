#pragma once

#include "Simulation/FightingSimulationTypes.h"
#include "InputCoreTypes.h"

namespace ProjW::DebugInput::Keys
{
    inline const FKey& P1Left() { return EKeys::A; }
    inline const FKey& P1Right() { return EKeys::D; }
    inline const FKey& P1Jump() { return EKeys::W; }
    inline const FKey& P1Crouch() { return EKeys::S; }
    inline const FKey& P1PivotCCW() { return EKeys::Q; }
    inline const FKey& P1PivotCW() { return EKeys::E; }
    inline const FKey& P1Light() { return EKeys::R; }
    inline const FKey& P1Medium() { return EKeys::F; }
    inline const FKey& P1Heavy() { return EKeys::C; }
    inline const FKey& P1GroundRupture() { return EKeys::V; }
    inline const FKey& P1HighParry() { return EKeys::T; }
    inline const FKey& P1LowParry() { return EKeys::G; }

    inline const FKey& P2Left() { return EKeys::Left; }
    inline const FKey& P2Right() { return EKeys::Right; }
    inline const FKey& P2Jump() { return EKeys::Up; }
    inline const FKey& P2Crouch() { return EKeys::Down; }
    inline const FKey& P2PivotCCW() { return EKeys::NumPadFour; }
    inline const FKey& P2PivotCW() { return EKeys::NumPadSix; }
    inline const FKey& P2Light() { return EKeys::NumPadSeven; }
    inline const FKey& P2Medium() { return EKeys::NumPadEight; }
    inline const FKey& P2Heavy() { return EKeys::NumPadNine; }
    inline const FKey& P2GroundRupture() { return EKeys::NumPadFive; }
    inline const FKey& P2HighParry() { return EKeys::NumPadOne; }
    inline const FKey& P2LowParry() { return EKeys::NumPadTwo; }
}

// Presentation-only sample of physical keys. These values are polled from the
// local PlayerController and translated before entering the simulation.
struct FDebugPhysicalInput
{
    bool bScreenLeft = false;
    bool bScreenRight = false;
    bool bJump = false;
    bool bCrouch = false;
    bool bPivotCounterClockwise = false;
    bool bPivotClockwise = false;
    bool bLight = false;
    bool bMedium = false;
    bool bHeavy = false;
    bool bGroundRupture = false;
    bool bHighParry = false;
    bool bLowParry = false;
};

namespace ProjW::DebugInput
{
    inline void AddScreenDirections(FFighterInput& Input, const FDebugPhysicalInput& Physical,
        double ScreenRightAlignment, int8& PreviousMappingSign)
    {
        constexpr double AlignmentHysteresis = 0.08715574274765817; // sin(5 degrees)
        int8 MappingSign = PreviousMappingSign;
        if (ScreenRightAlignment > AlignmentHysteresis) MappingSign = 1;
        else if (ScreenRightAlignment < -AlignmentHysteresis) MappingSign = -1;
        else if (MappingSign == 0) MappingSign = ScreenRightAlignment < 0.0 ? -1 : 1;

        bool bMappedLeft = MappingSign > 0 ? Physical.bScreenLeft : Physical.bScreenRight;
        bool bMappedRight = MappingSign > 0 ? Physical.bScreenRight : Physical.bScreenLeft;
        if (PreviousMappingSign != 0 && MappingSign != PreviousMappingSign
            && (Physical.bScreenLeft || Physical.bScreenRight) && !Physical.bJump)
        {
            // Avoid treating a camera-plane mapping change during a held
            // direction as a fresh dash tap. This is presentation-only.
            bMappedLeft = true;
            bMappedRight = true;
        }
        PreviousMappingSign = MappingSign;
        if (bMappedLeft) Input.Buttons |= static_cast<uint16>(EFighterInputButton::Left);
        if (bMappedRight) Input.Buttons |= static_cast<uint16>(EFighterInputButton::Right);
    }

    inline FFighterInput BuildPlayer1Input(const FDebugPhysicalInput& Physical,
        double ScreenRightAlignment, int8& PreviousMappingSign)
    {
        FFighterInput Input;
        AddScreenDirections(Input, Physical, ScreenRightAlignment, PreviousMappingSign);
        const auto Add = [&Input](bool bDown, EFighterInputButton Button)
        { if (bDown) Input.Buttons |= static_cast<uint16>(Button); };
        Add(Physical.bJump, EFighterInputButton::Up);
        Add(Physical.bCrouch, EFighterInputButton::Down);
        Add(Physical.bPivotCounterClockwise, EFighterInputButton::PivotPositive);
        Add(Physical.bPivotClockwise, EFighterInputButton::PivotNegative);
        Add(Physical.bLight, EFighterInputButton::Light);
        Add(Physical.bMedium, EFighterInputButton::Medium);
        Add(Physical.bHeavy, EFighterInputButton::Heavy);
        Add(Physical.bGroundRupture, EFighterInputButton::GroundRupture);
        Add(Physical.bHighParry, EFighterInputButton::HighParry);
        Add(Physical.bLowParry, EFighterInputButton::LowParry);
        return Input;
    }

    inline FFighterInput BuildPlayer2Input(const FDebugPhysicalInput& Physical,
        double ScreenRightAlignment, int8& PreviousMappingSign)
    {
        // Both sides share the exact movement/action translation. Separate
        // physical key polling in the controller prevents overlapping aliases.
        return BuildPlayer1Input(Physical, ScreenRightAlignment, PreviousMappingSign);
    }
}
