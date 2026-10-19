# Milestone 5G: High/Low Parry and GroundRupture Geometry Prototype

Milestone 5G adds one deterministic Combat Lab defense and one broad-coverage geometry experiment. Block remains reliable and concessionary, Pivot remains a spatial answer decided by actual geometry, and High/Low Parry is a committed timing-and-height read. These are prototype rules, not final balance or a selected production combat grammar.

## Authoritative simulation

`FFighterInput` stays a two-byte per-frame button mask. Bits 12 and 13 are distinct `HighParry` and `LowParry`; bit 14 is the dedicated `GroundRupture` input. F/G and Numpad 6 drive Player 1. Player 2 can now be driven locally with arrows, H/J/K normals, T GroundRupture, R/Y High/Low Parry, and Z/X Pivot, so the Combat Lab comparison does not require an AI. Numpad 7/8/9 and Q/E retain their accepted Player 1 mappings.

Parry is explicit rollback state: `EFighterState::Parrying`, `ParryHeight`, and integer `StateFrame`. A fresh High/Low press from grounded Idle, Walking, or Crouching enters frame 0. Frames 0–3 are active; frames 4–13 are committed recovery; the action exits after 14 frames. It cannot move, attack, jump, dash, Pivot, reverse, Block, or buffer another action. A correct matching contact classifies as transient `Parry`; a wrong-height or recovery contact follows the existing Hit/Block path. Guard height and Parry height are separate move metadata.

Successful Parry deals no damage or chip to the defender and does not put the defender in Hitstun or Blockstun. Both fighters receive 6 shared hitstop frames, then the attacker enters 18 ParryStun frames and the defender returns to actionable neutral. No invulnerability, displacement, meter reward, auto-riposte, or combo scripting is added. Contact directions remain classified before either consequence is resolved. With one mutually exclusive action state per fighter, the reachable Parry exchange is one-way Hit/Parry; a fighter cannot simultaneously attack and Parry to create a two-way Hit/Parry or Parry/Hit trade. Existing Hit/Hit trades remain unchanged.

The new immutable `GroundRupture` definition uses 18 startup, 5 active, and 24 recovery frames; 100 damage; 8 hitstop; 18 hitstun; moderate 5-unit knockback; Low Block metadata; 10 chip; 16 Blockstun; and 3-unit block pushback. Its vertical interval is 0–30 world units and its horizontal radius is exactly 112 world units. A small reusable `FSimRadialVolume` checks the closest point on the hurtbox AABB using integer squared distance with strict `< radius²`; tangent-only contact misses. The move uses geometry alone: Pivot is caught when its hurtbox remains inside the circle and evades it when it leaves. There is no `AoE`, `CannotPivot`, or `CanOnlyParry` semantic flag. Existing normal geometry and balance data remain unchanged.

## Debug presentation and controls

The HUD identifies Milestone 5G, displays Parry active/recovery/success and ParryStun, reports GroundRupture phase, and retains health bars. Greybox fighters use distinct high/low Parry poses and a brief success/Stun color. During an active GroundRupture, an orange debug ring is drawn from the move definition's authoritative center and radius. All of these are presentation only.

- Player 1: A/D move and double-tap dash, W jump, S crouch, Q/E Pivot, R/F/C Light/Medium/Heavy, V GroundRupture, T/G High/Low Parry.
- Player 2: arrow keys move and double-tap dash, Numpad 4/6 Pivot, Numpad 7/8/9 Light/Medium/Heavy, Numpad 5 GroundRupture, Numpad 1/2 High/Low Parry.
- Block remains directional for both players: hold Away for standing Block, or Down+Away for crouching Block. The screen-relative mapper translates physical left/right to the simulation's canonical horizontal directions; the simulation then derives Away from the fighter's current facing, so crossover changes which physical side is Away. Camera state is not sent to simulation.

## Automated coverage

`ParryAndRuptureTests.cpp` adds:

- `Parry.Definitions`: compact input, distinct bits, exact timing/consequence constants, GroundRupture data and independent height metadata.
- `Parry.ExactLifecycleAndLocks`: both Parry choices, exact frames 0–13, committed action locks, no buffering, and no movement.
- `Parry.HeightMatrix`: correct and wrong Parry heights against Light, Medium, Heavy, and GroundRupture.
- `Parry.ConsequencesAndReplay`: zero defender consequence, shared freeze, ParryStun transfer, defender recovery, and snapshot replay.
- `Parry.FailureAndRecoveryVulnerability`: last active frame, wrong height, Away-held failure, pre-active/recovery vulnerability, and no fallback Block.
- `Parry.HeavyDefenseComparison`: neutral Hit, concessionary Block, matching HighParry, and geometric Heavy whiff via Pivot.
- `Parry.SnapshotDeterminism`: exact repeated mixed-input simulation and intermediate snapshot restore/replay.
- `GroundRupture.StrictRadialGeometry`: near/far, strict tangency, one-subunit interior, diagonal, vertical separation, X/Z rotation invariance, and actual move contacts.
- `GroundRupture.ExactLifecycle`: exact 18/5/24 timing and radial volume visibility only on active frames.
- `GroundRupture.DefenseComparison`: Low Block chip, High Parry, rejection of Low Parry, nearby Pivot caught inside the real radius, and a farther Pivot that escapes.

`Combat.Stress100K` and `Pivot.Mixed30K` retain their original total lengths and exact two-state repeatability. Both use an explicit deterministic opening to exercise successful HighParry and GroundRupture before their existing mixed patterns; they continue asserting their prior movement/combat coverage. `FightingSimulationTests.cpp` compares the added ParryHeight field as part of its explicit state audit.

## Validation

Final verification on 2026-10-17:

- `ProjWEditor Win64 Development`: succeeded.
- Canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1`: 95 discovered, 95 passed, 0 test warnings, 0 failed, and 0 skipped/not-run.
- Unreal process exit code: 0. Runner script exit code: 0.
- Both `ProjW.Simulation.Combat.Stress100K` and `ProjW.Simulation.Pivot.Mixed30K` passed at their original exact lengths with their HighParry and GroundRupture exercise assertions.
- Report: `Saved/Automation/CodexValidation-20261017-225229`; log: `Saved/Logs/CodexValidation-20261017-225229.log`.
- The complete log includes restricted-environment Unreal Trace, user-local Zen security-config write denial, and offline EOS connection messages; these did not become automation test warnings or prevent successful test execution.

Automated validation does not replace a manual DX11 playtest. Human acceptance remains open for Parry timing/readability and GroundRupture’s on-screen radial cue and Block/Pivot/Parry comparison.

## Revision: human input path and two-sided controls (2026-10-18)

Human playtesting reported that Block and Parry did not work and that the old
debug controls were awkward for two-sided local testing. The first source-level
mismatch addressed was the debug layout: the former P1 attacks were on the
numpad and the former P2 attacks/defenses were spread across H/J/K, T, R/Y,
and Z/X. Those mappings did not match the requested left-side P1/right-side
P2 layout. This is a control-path correction; it does not establish that the
original human report was caused only by the layout. No authoritative Block or
Parry rule was changed, and no simulation mechanics defect was demonstrated.

`DebugInputMapping.h` centralizes distinct physical key identifiers and the
presentation-only left/right-to-simulation mapping. The controller polls the
new P1 and P2 keys independently, maps both players through the same helper,
and passes only `FFighterInput` masks into simulation. Away remains
facing-relative after crossing: facing Right means simulation Left is Away;
facing Left means simulation Right is Away. Camera/combat-plane screen
alignment selects physical screen-left/right mapping, with the existing
five-degree hysteresis and a one-frame neutralized direction on mapping flips
to avoid manufacturing dash-tap edges. Parry key-down maps to HighParry or
LowParry held bits; `FFighterInputEdges::From` produces `Pressed` only when
the bit changes from unheld to held. Continued key hold therefore cannot
re-enter Parry while its committed action runs.

The presentation diagnostics now expose raw P1/P2 key samples, translated
button masks, authoritative state/frame/facing, Away/guard recognition,
High/Low Parry press edges and active/recovery phase, incoming move/height,
and the resulting Hit/Block/Parry contact. They are not part of rollback state.
Old combat aliases were removed. The exact current layout is:

| Action | P1 (left side) | P2 (right side) |
|---|---|---|
| Move / jump / crouch | A/D / W / S | Left/Right / Up / Down arrows |
| Pivot CCW / CW | Q / E | Numpad 4 / 6 |
| Light / Medium / Heavy | R / F / C | Numpad 7 / 8 / 9 |
| GroundRupture | V | Numpad 5 |
| High / Low Parry | T / G | Numpad 1 / 2 |
| Standing / crouching Block | Away / Down+Away | Away / Down+Away |

`ProjW.Presentation.DebugInputMapping` was added. It checks all new key
bindings and their distinct Numpad identities, action-bit translation for both
players, one-shot Parry press-edge behavior under a held key, Away before/after
crossing, Down+Away, and rotated camera/combat-axis mapping. The complete
current-source build and suite were run after the source changes:

- `ProjWEditor Win64 Development`: succeeded, process exit 0.
- `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1`: 97
  discovered; 96 passed; 1 test completed with warnings; 0 failed; 0
  skipped/not-run. Unreal process exit 0; script exit 0. The warning is the
  restricted environment's offline EOS SDK configuration request during
  `ProjW.Simulation.Attack.AirborneRejected` (11 captured log warning entries
  from the same failed connection to `api.epicgames.dev`); no assertion
  failed.
- Report: `Saved/Automation/CodexValidation-20261018-210132/index.json`;
  log: `Saved/Logs/CodexValidation-20261018-210132.log`.
- DX11 smoke launch reached `Debug match ready` in
  `Saved/Logs/5G-HumanInput-DX11.log`. However, the execution environment had
  no usable foreground desktop (`GetForegroundWindow` returned null), so the
  attempted OS key events did not reach Unreal. No live Block, Parry,
  crossover, Pivot/recenter, Num Lock, wrong-height, or timing result is
  claimed. This run therefore cannot identify the first actual keyboard-path
  divergence or prove the original human-play root cause. The new diagnostic
  path emitted only its all-keys-up baseline sample. Manual DX11 validation by
  a person with a focused game window remains required; 5G is not
  human-accepted.

## Still out of scope

These rules are not final balance. Parry remains a Combat Lab prototype; GroundRupture is one geometric test move, not a production AoE category. Tag, assists, meter, combos, cancels, additional attacks, AI, networking, animation, and production presentation are not added.

## Final defensive correctness investigation (2026-10-18)

The post-control-repair human report was that Light appeared parryable, Medium
could not be blocked, and no attack appeared sidestep-able. No defensive
simulation rule or attack geometry was changed in this pass. The previous
`ProjW.Simulation.Parry.HeightMatrix` already exercises all eight required
cases: High Parry succeeds against Light/Heavy, Low Parry succeeds against
Medium/GroundRupture, and each wrong-height pairing follows ordinary Hit. It
asserts no defender damage and attacker ParryStun on success. This rules out a
height-classification defect in that tested contact path. Its setup places the
attack directly in Active before pressing Parry, however, so it does not prove
human timing. The 4-frame window (66.7 ms) is unchanged while startup is 5
frames for Light, 7 for Medium, 10 for Heavy, and 18 for GroundRupture. A
press timed to one attack's contact will not automatically align with another
attack's later contact. The observed Light-only result is therefore consistent
with a timing/readability mismatch, but this could not be confirmed with a
controlled live keyboard run in this environment.

The previous GroundRupture defense test already proved Low Block chip at a
close range. This pass adds `ProjW.Simulation.Block.LowGuardHeightMatrix` to
explicitly assert Medium and GroundRupture against both standing Away and
crouching Down+Away. Standing guard must take full Hit damage for these Low
attacks; crouching Away must produce Block, chip only, Blockstun, and positive
pushback. Thus standing Block against Medium is intended to fail; it does not
show a defect. The new test also confirms the expected crouching-Block result.

Pivot remains a physical 12-frame orbit targeting about 8 degrees, with no
invulnerability or move-type dodge flags. For a fighter/opponent center radius
`r`, full Pivot's maximum lateral center displacement is approximately
`r * sin(8°)`. At each move's outer center-to-center overlap distance, the
straight-axis estimates are:

| Move | Lateral attack half-extent | Hurt half-extent | Outer overlap distance | Full-Pivot lateral displacement | Geometry finding |
|---|---:|---:|---:|---:|---|
| Light | 4 | 5 | about 96 | about 13.4 | A miss is geometrically possible at long range if Pivot is completed before contact; close range or late Pivot may still overlap. |
| Medium | 30 | 5 | about 109 | about 15.2 | The 35-unit combined lateral extent exceeds full displacement throughout normal range; ordinary Pivot is not expected to evade it. |
| Heavy | 6 | 5 | about 134 | about 18.7 | A miss is possible with sufficient spacing and early enough Pivot. `Parry.HeavyDefenseComparison` already demonstrates a deterministic geometric Heavy whiff at 85-unit spacing. |
| GroundRupture | horizontal circle, radius 112 | hurtbox extends 24 X / 5 Z | center can remain inside the radial threat | orbit preserves center radius | Pivot does not escape when sufficiently inside the circle. Only a narrow tangent-edge case can change overlap through the axis-aligned hurtbox extent; there is no guaranteed radial dodge. |

Distances and displacement are world units, rounded; the calculation assumes a
straight initial pair axis and a complete orbit. Real contact depends on
startup, the frame the defender commits, arena projection, and strict oriented
overlap. The result that Medium is difficult or impossible to sidestep and
GroundRupture catches Pivot is a geometry finding for this prototype, not a
reason to rebalance it in 5G. The current test suite has a positive Heavy
whiff and near/far GroundRupture checks; no geometry values or dodge semantics
were altered.

The diagnostic DX11 launch reached the game process, but this environment had
no active foreground desktop: Win32 `GetForegroundWindow` returned null and
`SetForegroundWindow` failed. Thus no keyboard events reached the game, and
live P1/P2 Parry timing, Low Block, and Pivot results remain unverified here.
The earlier report that only Light appeared parryable cannot be conclusively
classified as a live timing/readability failure from this run. The simulation
height matrix demonstrates that all four correct-height contacts can Parry
when the 4-frame window overlaps contact; it also proves wrong-height contacts
fail. No timing window or attack startup was changed.

### Current-source validation

- `ProjWEditor Win64 Development`: succeeded (UBT exit 0).
- Canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1`:
  98 discovered, 98 passed, 0 warnings, 0 failed, 0 skipped/not-run. Unreal
  process exit 0; runner script exit 0.
- `ProjW.Simulation.Block.LowGuardHeightMatrix` passed, explicitly checking
  both Medium and GroundRupture against standing and crouching Away, including
  full damage versus chip, Blockstun, and pushback.
- Existing `ProjW.Simulation.Parry.HeightMatrix` passed all four matching and
  all four wrong-height cases. Existing `ProjW.Simulation.Parry.HeavyDefenseComparison`
  passed its Heavy geometric Pivot whiff; existing
  `ProjW.Simulation.GroundRupture.DefenseComparison` passed the near-radius
  Pivot catch and far-radius miss.
- The suite completed with zero automation warnings. Engine log startup still
  includes restricted-network EOS/Google connectivity messages; they were not
  test warnings in this run.
- Report: `Saved/Automation/CodexValidation-20261018-235938/index.json`;
  log: `Saved/Logs/CodexValidation-20261018-235938.log`; build log:
  `Saved/Logs/Build5G-Defensive.log`.
- DX11 launch/input attempt log: `Saved/Logs/5G-Defensive-DX11.log`. The
  foreground desktop limitation prevented the requested targeted keyboard
  matrix. Human acceptance remains open.

## Low Parry follow-up (2026-10-19)

Human playtest subsequently reported that High Parry works, while Low Parry
appears to fail for both players against Medium and GroundRupture. Source
inspection found no separate High-only classifier: the same active-window
condition compares `Defender.ParryHeight` with the move's immutable
`ParryHeight` for either height, and both Low moves carry Low metadata. The
P1 G and P2 Numpad2 bindings also translate to the same distinct LowParry
input bit as their respective physical samples.

The previous automated contact matrix covered all matching and wrong-height
cases with P2 defending, but did not repeat the contact matrix with P1
defending. The input-helper test checked that LowParry's bit existed but only
tested Pressed-edge behavior for HighParry. This was a **regression coverage
gap**, not evidence of the runtime failure's root cause. This follow-up
expanded `ProjW.Simulation.Parry.HeightMatrix` to both defender slots and
extended `ProjW.Presentation.DebugInputMapping` to cover G/Numpad2 translation,
one-shot LowParry Pressed edges, held-key non-retrigger, and authoritative Low
Parry state/height entry for both players. Existing Parry lifecycle coverage
still checks four active frames followed by ten recovery frames. No move
metadata, classification logic, timing, or presentation changed.

All deterministic checks pass, including Medium/Low and GroundRupture/Low
successes, all wrong-height failures, and Heavy/High as a control. This proves
the authoritative classifier and both-player simulation assignments work
when the Low input arrives on the contact frame. It does not prove that the
physical G or Numpad2 press arrives during the live four-frame window. The
different startup lengths (Medium 7, GroundRupture 18) make a timing/readability
miss plausible, but the reported failure's exact cause remains **unresolved**
without live physical-key diagnostics at the pressed edge and contact frame.
Do not treat timing as confirmed or widen the window on this evidence.

The previous Medium/GroundRupture Low-guard matrix remains valid: standing
Away produces Hit; Down+Away produces Low Block. No new Low Parry contact or
Block mechanics change was needed. Pivot remains the geometry-only Combat Lab
finding recorded above and is deferred; it received no changes.

### Low Parry follow-up validation

- `ProjWEditor Win64 Development`: succeeded (UBT exit 0).
- Canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1`:
  98 discovered, 98 passed, 0 warnings, 0 failed, 0 skipped/not-run. Unreal
  process exit 0; runner script exit 0.
- `Parry.HeightMatrix`, `Parry.ExactLifecycleAndLocks`,
  `Presentation.DebugInputMapping`, and `Block.LowGuardHeightMatrix` all
  passed with no warnings.
- Report: `Saved/Automation/CodexValidation-20261019-011245/index.json`;
  log: `Saved/Logs/CodexValidation-20261019-011245.log`; build log:
  `Saved/Logs/Build5G-LowParry.log`.
- Actual DX11 physical-key validation could not be performed in this
  environment because it has no usable foreground desktop. Therefore P1 G,
  P2 Numpad2, and live Low Parry contacts remain to be checked by a human with
  the game focused. Milestone 5G remains human-unaccepted.

## Parry-height contract correction (2026-10-19)

The human clarification established the intended current contract: Light,
Medium, Heavy, and GroundRupture are all default standing attacks and all must
match High Parry. No Low attack exists yet, so no current move may match Low
Parry. The earlier Low Parry follow-up correctly exposed Medium and
GroundRupture as Low-Parry targets in code, but that metadata encoded the wrong
design rule; the prior tests and stress openings repeated that mistake.

The immutable Medium and GroundRupture definitions now use High Parry metadata.
Their separate Low Block metadata is retained, as this correction changes only
Parry pairing. The full height matrix asserts High succeeds and Low fails for
each existing move, with both players defending. GroundRupture's defense test
also checks this directly. Deterministic long-run tests now exercise successful
High Parry on both sides and no longer require a successful Low Parry. Low
Parry remains an available committed action, but currently has no matching
attack.

### Parry-height correction validation

- `ProjWEditor Win64 Development` succeeded (UBT exit 0).
- The canonical complete suite discovered 98 tests: 98 passed, 0 warnings,
  0 failed, and 0 skipped/not-run. Unreal process exit 0; runner script exit 0.
- `Parry.HeightMatrix` verifies High Parry succeeds and Low Parry falls through
  to ordinary Hit for Light, Medium, Heavy, and GroundRupture, with either
  player defending. `GroundRupture.DefenseComparison` independently confirms
  High success and Low failure. Low Block behavior remains unchanged.
- Both deterministic long runs passed after replacing their Low-Parry success
  choreography with High Parry; neither requires a current Low-Parry target.
- Report: `Saved/Automation/CodexValidation-20261019-013633/index.json`;
  log: `Saved/Logs/CodexValidation-20261019-013633.log`; build log:
  `Saved/Logs/Build5G-ParryHeightCorrection.log`.

## Human acceptance and closeout (2026-10-19)

The human reviewed the current 5G/5V state and reported “everything is in
place.” This is acceptance of the current Combat Lab prototype, not final
production balance or a final combat grammar. Block remains a reliable,
concessionary guard; Pivot remains a physical spatial response with outcomes
determined by geometry and no invulnerability; Parry remains an active,
height-specific commitment with its current timing. Current attacks use High
Parry metadata; Low Parry has no target until a Low attack is deliberately
introduced. GroundRupture remains a deterministic broad/radial prototype with
no semantic AoE or anti-Pivot property. Current geometry does not need to make
every attack an attractive Pivot opportunity. 5H is the next controlled
exploration of attack geometry and the existing defensive grammar.

Closeout verification on 2026-10-19: `ProjWEditor Win64 Development` succeeded
(UBT exit 0). The canonical complete suite discovered 98 tests: 98 passed,
0 warnings, 0 failed, and 0 skipped/not-run. Unreal process exit 0; runner
script exit 0. Report: `Saved/Automation/CodexValidation-20261019-113708`;
log: `Saved/Logs/CodexValidation-20261019-113708.log`; build log:
`Saved/Logs/Build5GV-Closeout.log`.
