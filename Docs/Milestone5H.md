# Milestone 5H: Combined Combat Lab attack geometry

5H explores how three deliberately different, committed attack geometries interact with the accepted Block, Pivot, and height-specific Parry prototypes. The moves are immutable data entries in the existing move table. They use the existing state machine, captured attack axis, oriented-box contact, guard/Parry heights, hitstop, reactions, and recovery. No anti-Pivot or invulnerability rule was added. The user has accepted the gameplay and presentation pass; this remains a test prototype, not final balance or a chosen combat grammar. The procedural crouch is a known temporary presentation weakness as the project moves toward authored character animation.

## Prototype data

All dimensions are world units in the existing 1024-subunit simulation. Each attack has the same height center/half-height convention used by current normals; contact requires strict overlap in all oriented horizontal and vertical axes.

| Move | Start / active / recovery | Damage | Hitstop / hitstun | Knockback | Guard / Parry | Offset X / Y | Forward half extent | Lateral half extent | Vertical half extent |
|---|---:|---:|---:|---:|---|---:|---:|---:|---:|
| NarrowThrust | 16 / 3 / 22 | 90 | 8 / 19 | 8 | High / High | 60 / 58 | 42 | 2 | 24 |
| BroadSweep | 13 / 4 / 20 | 85 | 7 / 18 | 6 | High / High | 52 / 55 | 40 | 24 | 18 |
| SlowCommittedStrike | 24 / 3 / 32 | 130 | 10 / 23 | 10 | High / High | 68 / 76 | 34 | 12 | 34 |

Blockstun/chip/pushback are respectively 15/9/4 for Thrust, 14/8/3 for Sweep, and 19/13/5 for Slow Strike (chip and speed in world units where applicable). GroundRupture retains its attack data and radial geometry and is Low-Parryable; it is the sole Low-Parry target. Light, Medium, Heavy, NarrowThrust, BroadSweep, and SlowCommittedStrike are High-Parry targets. Per the clarified combat rule, all linear attacks have standing-height volumes and can be ducked; BroadSweep's defining difference is its broad lateral coverage. Medium's guard metadata is High so standing Away can block it, consistent with its standing-height geometry.

NarrowThrust has the narrowest lateral width and long forward coverage. BroadSweep has substantially wider lateral coverage and uses standing-height placement with a short vertical extent. SlowCommittedStrike has the longest startup and recovery, a raised volume, and intermediate width. All volumes are oriented to the attacker's captured axis and never track after commitment.

## Controls and presentation

Existing controls remain: P1 Light/Medium/Heavy on R/F/C and GroundRupture on V; P2 uses numpad 7/8/9 and 5. New controls are P1 Z = NarrowThrust, X = BroadSweep, B = SlowCommittedStrike; P2 Numpad 0 = NarrowThrust, Numpad Decimal = BroadSweep, Numpad Add = SlowCommittedStrike. The physical input layer maps these keys onto currently unused Special/Assist input bits, preserving the compact two-byte per-frame input and leaving the old move bindings intact. No Special or Assist system was added.

The debug geometry view uses the existing oriented red attack boxes and green 3D hurt boxes; GroundRupture retains its radial visualization. Placeholder clips reuse the existing unarmed jab for Thrust, Heavy attack for Sweep, and charged attack for Slow Strike. They are sampled from the authoritative move phase and cannot drive movement or gameplay.

## Automated coverage

`CombatArchetypeTests.cpp` checks all three definitions and selected input IDs; normal hit, appropriate Block and Parry, wrong Parry, NarrowThrust's strict geometric Pivot miss, BroadSweep's same-timing overlap/catch, Slow Strike hit and outside-range miss with whiff recovery, and snapshot/replay. Existing suites continue to cover Light/Medium/Heavy, GroundRupture, Block, Pivot, Parry, and deterministic long runs. The NarrowThrust Pivot case starts at 110 units on the attack axis with both commitments on adjacent input frames: the defender completes its Pivot before active contact, the hurtbox is outside the narrow oriented volume, and hit count remains zero. The comparable BroadSweep case retains actual oriented-volume overlap and produces a hit.

## Manual scenarios

For a two-player local test, set fighters approximately 110 units apart on a straight combat axis and use F3 to show geometry. P1 attacks; P2 responds.

**A — Thrust:** P1 presses Z; P2 tests Away Block, T High Parry, Q/E Pivot, and no defense. Check the red box against the green hurt volume and whether Pivot's miss is visibly spatial.

**B — Sweep:** P1 presses X; P2 tests Away Block, T High Parry, Pivot, and no defense. Down+Away should duck under the standing-height volume; the broad red volume should still cover the comparable Pivot path.

**C — GroundRupture:** P1 presses V; P2 tests crouch Block, G Low Parry, T High Parry, and Pivot against its radial volume. GroundRupture remains the Low-Parryable radial prototype; High Parry should fail. The attack values and radial geometry remain unchanged.

**Crouch test:** hold Down against Light, Medium, Heavy, NarrowThrust, BroadSweep, and SlowCommittedStrike. Their standing-height volumes should pass above the crouching hurtbox. GroundRupture remains the low radial exception and can overlap it.

**D — Slow Strike:** P1 presses B; P2 tries Block, T High Parry, Pivot when spacing permits, and simply moving away during the long startup. Judge whether commitment creates a decision without making one response mandatory.

## Open questions and limitations

Prototype frame data, damage, spacing, Parry reward/failure cost, Pivot reward/failure cost, and animation choices are unbalanced. Placeholder clips are not bespoke fighting animations. The prototype does not prove a final defensive triangle or establish the best Pivot role, reactability, broad-attack coverage method, or the final need for a rotating 3D combat plane. Gameplay and presentation are human-accepted for this Combat Lab stage; the procedural crouch remains a known temporary visual weakness for later authored-animation work.

## Clarification and final validation (2026-10-19)

The accepted pairing clarification is that GroundRupture is the only move
that can be answered by Low Parry; every other attack requires High Parry.
Light, Medium, and Heavy are standing-height attacks and can be ducked. They
use High Block metadata so standing Away blocks them; Down + Away does not
block them because their volumes strictly miss the crouching hurtbox. Ground
Rupture remains the low radial exception. BroadSweep keeps broad lateral
coverage but uses standing-height geometry, High Block, and High Parry so it
can be ducked. Existing normal damage, timing, volume, and reaction values
are unchanged; Medium's guard-height metadata was aligned with the clarified
standing-height rule. Crouching hurtbox height is
30 world units, against standing-normal lower edges at 30 units, so strict
vertical face contact misses.

No authored crouch animation clip exists in the local Manny/Quinn mannequin
animation set. The earlier whole-mesh squash has been replaced with a
presentation-only articulated pose: the current source animation is copied to
a poseable mesh, then pelvis, thigh, calf, and spine offsets are applied with a
short render-time blend. The authoritative actor position, crouching hurtbox,
simulation frame, and combat state do not use this pose. This is a procedural
placeholder rather than a bespoke authored clip; inspect the knee bend, floor
contact, and blend in the manual pass.

The final current-source `ProjWEditor Win64 Development` build succeeded
(UBT process exit 0). The final canonical complete suite discovered 101 tests:
101 passed, 0 warnings, 0 failed, 0 skipped/not-run. Unreal process exit was 0
and runner script exit was 0. Final report:
`Saved/Automation/CodexValidation-20261019-121329`; log:
`Saved/Logs/CodexValidation-20261019-121329.log`.

The final-build DX11 `UnrealEditor.exe -game` smoke forced D3D11, reached
`Debug match ready`, shut down D3D11 normally, and exited 0. No asset or skeletal
animation errors were logged. Log:
`Saved/Logs/5H-DX11-Smoke-Verified.log`. This confirms startup and asset resolution, not a substitute for the user's human acceptance of the 5H gameplay/presentation. The crouch pose remains the acknowledged temporary presentation weakness.