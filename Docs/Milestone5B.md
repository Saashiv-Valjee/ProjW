# Milestone 5B: Three Normal Attacks

Milestone 5B activates Light, Medium, and Heavy as deterministic grounded normal attacks through the generic move foundation from 5A. All three use `EFighterState::Attack`, `CurrentMoveId`, `MovePhase`, `StateFrame`, and `bCurrentMoveConnected`; no per-move state machine or additional authoritative fighter field was added.

## Prototype definitions

All positions and velocities use integer simulation subunits, with 1024 subunits per world unit.

| Move | Startup | Active | Recovery | Damage | Hitstop | Hitstun | Knockback | Hitbox center X | Half-width | Center Y | Half-height |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Light | 5 | 3 | 9 | 50 | 6 | 14 | 6 × 1024 | 44 | 28 | 55 | 25 |
| Medium | 7 | 3 | 12 | 80 | 7 | 17 | 7 × 1024 | 50 | 35 | 55 | 25 |
| Heavy | 10 | 4 | 16 | 120 | 9 | 20 | 9 × 1024 | 66 | 44 | 55 | 25 |

These values are immutable static C++ definitions selected through `EMoveId`. `None` and unknown IDs resolve deterministically to no definition. There is no UObject, Blueprint lookup, hash map, allocation, or runtime-mutated move data in authoritative resolution.

## Input and lifecycle

Numpad 7 maps to Light, Numpad 8 to Medium, and Numpad 9 to Heavy. A fresh per-frame `Pressed` edge selects a move. When several normal buttons are newly pressed together, one explicit selection path applies `Heavy > Medium > Light`; enum ordering is not used as priority.

Normals start from the same Idle, Walking, and Crouching states previously accepted for Light. Movement states that were already locked remain locked. Once an attack begins, later attack presses cannot replace or cancel it, and attack inputs consumed during an attack, hitstop, or Hitstun are not buffered. `StateFrame` remains the phase-local clock for Startup, Active, and Recovery. Hitstop pauses it, every phase begins at frame 0, and the move ID clears to `None` after Recovery.

## Hits, reactions, and trades

Generic integer hitbox generation reads geometry from the current immutable definition and mirrors its X offset from authoritative facing. Strict AABB overlap, grounded-only targets, one successful contact per move, later-active-frame hits after an invalid airborne overlap, zero-health clamping, wall behavior, and captured pending reactions remain unchanged.

Both fighters step before either contact is applied. On an unlike-move trade, each defender receives damage, pending hitstun, and knockback from the incoming attacker's definition. Because hitstop is shared by attacker and defender and simultaneous moves can request different durations, both fighters receive the maximum duration of the two contacts. This rule is independent of fighter evaluation order and preserves the prior Light-vs-Light result.

## Rollback audit

Medium and Heavy require no new authoritative fields. Their differences live entirely in immutable definitions. `FMatchState` remains trivially copyable, value equality still covers all authoritative state, snapshots store no definition pointers, and per-frame lookup requires no allocation. Presentation reads the move ID and generic hitbox but does not affect gameplay. No variable `DeltaTime` enters the simulation.

## Tests

`NormalAttackTests.cpp` adds focused coverage for:

- all exact Light/Medium/Heavy definition values;
- input mapping, actionable-state rules, held input, ignored/bufferless presses, and simultaneous-button priority;
- exact phase lengths and `StateFrame` boundaries for every normal;
- right-facing hits, left-facing mirrored hits, misses, and strict edge-touch rejection;
- one-contact behavior, reset on the next move, exact damage/hitstop/hitstun/knockback, and both knockback directions;
- Light-miss/Medium-hit and Medium-miss/Heavy-hit integer range positions;
- Medium and Heavy airborne-invalid overlap followed by a later valid active-frame hit;
- Light-vs-Heavy and Medium-vs-Heavy trades in both player assignments;
- Medium and Heavy snapshot/replay during Startup, Active, Recovery, hitstop, pending reaction, and Hitstun;
- 30,000-frame exact repeatability containing all three normals, walking, and jumping.

The legacy movement invariant was updated so its inert-input comparison uses only Special, Assist1, and Assist2 now that Medium and Heavy are intentionally actionable. Existing tests were otherwise retained.

Blocking, attack levels, command inputs, cancels, combos, meter, specials, projectiles, animation, VFX, and production UI remain out of scope.

## Validation

Final verification on 2026-10-17:

- `ProjWEditor Win64 Development`: succeeded.
- Canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1`: 63 discovered, 63 passed, 0 test warnings, 0 failed, and 0 skipped/not-run.
- Unreal process exit code: 0. Runner script exit code: 0.
- No project assertion warned or failed. Restricted-environment startup logs may still contain Unreal Trace, user-local Zen/security or analytics, and offline EOS connectivity noise; it did not affect test execution.

Human DX11 gameplay validation remains separate: verify Numpad 7/8/9 mapping, HUD names, visible timing/range/impact differences, mirrored hitboxes, lack of attack cancels, and movement/Light regression behavior.
