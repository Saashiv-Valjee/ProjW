# Milestone 5A: Generic Move Foundation

Milestone 5A migrates the accepted Light attack to a small deterministic move representation without adding attacks or changing Light gameplay. Simulation still advances one fixed 60 Hz frame per call and uses integer subunits (1 world unit = 1024 subunits).

## Representation and phase timing

`EMoveId` has stable values `None = 0` and `Light = 1`. `EFighterState::Attack` combines with `EMovePhase` (`Startup`, `Active`, `Recovery`) and the existing phase-local `StateFrame`. Entry into a phase is frame 0; each subsequent unfrozen simulation call advances that phase's frame. The transition to the next phase resets `StateFrame` to 0. Hitstop pauses this clock. Completing Recovery returns to the input-selected neutral state and clears `CurrentMoveId` to `None`. There is no second move timer or input buffer.

`FMoveDefinition` is immutable static C++ data resolved by a bounded ID lookup. `None` and invalid IDs have no definition. Light's accepted definition is 5 startup, 3 active, and 9 recovery frames; 50 damage; 6 hitstop frames; 14 hitstun frames; and 6 × 1024 subunits/frame knockback. Its integer hitbox is centered 44 world units forward and 55 units above the fighter, with a 28-unit half-width and a **25-unit half-height** (50-unit total height). This preserves the pre-5A `LightHitboxHeight = 50` geometry; the milestone request called 50 a half-height, but the accepted implementation used it as total height. Facing mirrors the X offset. Targets must be grounded, and strict AABB overlap excludes edge contact.

## State and rollback audit

`CurrentMoveId`, `MovePhase`, and `bCurrentMoveConnected` are authoritative fighter snapshot fields. The contact flag resets on a fresh move, prevents a second successful hit by that move, and remains part of the copyable state. `PendingHitstunFrames` is also authoritative: on contact, hitstun duration and knockback are copied from the move definition into the defender's pending reaction. The defender can enter and complete Hitstun after hitstop without consulting the attacker's later move state. These fields have deterministic default values; `FMatchState` remains trivially copyable and its fieldwise equality includes them. No definition pointer or reference is stored in match state.

The move definition is immutable data. The local hitbox, move-definition pointers, and the two contact booleans used in hit resolution are derived transient values. The HUD and debug hitbox drawing are presentation only. Lookup uses no UObject, dynamic allocation, Unreal collision, physics, or `DeltaTime`. Both fighters step before contacts are collected, preserving simultaneous trades and their consequences.

## Files and tests

- `FightingSimulationTypes.h`: move ID/phase and authoritative move/reaction fields.
- `FightingSimulation.h/.cpp`: immutable Light definition, generic attack phases, hitbox, contact, and reaction resolution.
- `DebugFightingHUD.cpp` and `DebugMatchDriver.cpp`: display move ID, phase, contact and existing reaction data; draw the generic hitbox.
- Existing movement/combat test files: migrate names and assertions to the generic state while retaining their behavioral expectations and complete-state checks.
- `GenericMoveFoundationTests.cpp`: new `FoundationDefinition`, `ContactAndReset`, `ReactionCapture`, `Trade`, and `SnapshotPhases` tests. The snapshot test restores and replays from Startup, Active before contact, Active after contact, Recovery, hitstop, pending reaction, and Hitstun.

No Medium/Heavy gameplay, blocking, command inputs, cancels, combos, meter, specials, networking, or production presentation is included.

## Validation

Final verification on 2026-10-17:

- `ProjWEditor Win64 Development` build: succeeded.
- Canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1`: 54 tests discovered, 54 passed, 0 warnings, 0 failed, 0 skipped/not-run.
- Unreal process exit code: 0. Runner script exit code: 0.
- Automation reported no test warnings or project assertion failures. The engine startup log still contains restricted-environment warnings, including an Unreal Trace Server timeout, inaccessible user-local Zen/security and analytics files, and offline EOS connectivity. These did not prevent the suite from running or affect its results.

Manual DX11 gameplay acceptance remains for a human: confirm Numpad 7 timing, visible hitbox/damage/hitstop/Hitstun/knockback, inert Numpad 8/9, and HUD move ID/phase/contact display.
