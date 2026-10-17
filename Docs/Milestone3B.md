# Milestone 3B: deterministic jumping and aerial movement

Milestone 3B preserves the fixed 60 Hz simulation, compact per-frame input, grounded
movement, dashes, stage bounds, grounded push, and presentation-only Unreal harness.
It replaces the temporary immediate jump and unrestricted air steering with an
explicit fighting-game jump sequence. Launch with `-d3d11`, press Play in the Entry
map, and click the viewport. No scene construction or Blueprint setup is required.

## Controls and states

A/D remain raw Left/Right, S crouches, and a fresh W press requests a jump. Grounded
W enters `JumpStartup`; releasing W and pressing it once while airborne immediately
enters `DoubleJumping`. Holding W never creates another edge. J/K/L, U, I/O remain
inert action inputs. Existing double-tap A/D dashes remain grounded-only.

The explicit states are `Idle`, `Walking`, `Crouching`, `ForwardDash`, `BackDash`,
`JumpStartup`, `Jumping`, and `DoubleJumping`. No Landing state is added. `StateFrame`
remains 0 on entry and increments on every subsequent frame in the same state.

## Integer constants

One world unit/Unreal centimetre is 1024 simulation subunits. Velocity is subunits
per simulation frame; gravity is subunits per frame squared.

| Constant | Integer value | World units |
| --- | ---: | ---: |
| JumpStartupFrames | 4 | 4 frames |
| JumpVerticalSpeed | 12288 | 12/frame |
| Gravity | -512 | -0.5/frame² |
| NeutralJumpHorizontalSpeed | 0 | 0/frame |
| ForwardJumpHorizontalSpeed | 5120 | 5/frame |
| BackwardJumpHorizontalSpeed | 4096 | 4/frame |
| DoubleJumpVerticalSpeed | 11264 | 11/frame |
| NeutralDoubleJumpHorizontalSpeed | 0 | 0/frame |
| ForwardDoubleJumpHorizontalSpeed | 5120 | 5/frame |
| BackwardDoubleJumpHorizontalSpeed | 4096 | 4/frame |

`JumpSpeed` remains a compatibility alias for `JumpVerticalSpeed`; new simulation
code and tests use the explicit name. Existing stage center limits remain -491520
and +491520 subunits (-480/+480 world units).

## Jump startup and direction commitment

The grounded Up-pressed call enters `JumpStartup` at `StateFrame=0`. Startup frames
0, 1, 2, and 3 are four complete grounded, zero-self-movement simulation frames.
On the following call, the fighter launches exactly once, becomes airborne, enters
`Jumping` at `StateFrame=0`, moves by the configured horizontal and vertical launch
velocities, and exposes those exact velocities. Gravity begins on the next airborne
call. Jump startup cannot walk, crouch, dash, or be shortened by held inputs.
Grounded push may still displace a fighter because pair resolution is external to
self-directed movement.

Horizontal launch intent is sampled and committed on the initial Up-pressed frame:

- Left+Right or neither selects neutral velocity 0.
- A raw direction equal to entering `Facing` selects forward speed.
- The opposite raw direction selects backward speed.
- The resulting exact world-space velocity is stored in authoritative state.

Inputs during startup cannot change it. An active dash consumes Up edges and cannot
be jump-cancelled. A fresh Up press from idle, walk, or crouch outranks grounded
crouch/walk and a newly recognized dash.

## Airborne momentum and double jump

There is no continuous air steering. On every ordinary airborne frame, horizontal
velocity is restored from `CommittedJumpVelocityX`, independent of current held
Left/Right and independent of later facing changes. This remains true after an
airborne side crossover.

The first actual launch makes one double jump available. A fresh airborne Up press
while `bDoubleJumpUsed` is false immediately:

1. sets `bDoubleJumpUsed=true`;
2. samples current raw horizontal direction against current authoritative facing;
3. replaces `CommittedJumpVelocityX` and horizontal velocity with the selected
   neutral/forward/backward double-jump velocity;
4. resets vertical velocity to exactly `DoubleJumpVerticalSpeed`;
5. enters `DoubleJumping` at `StateFrame=0` and moves by those reset velocities.

Gravity resumes on the following frame. Further Up presses before landing do not
reset velocity or create another jump. There is no double jump during startup,
air dash, triple jump, variable jump height, or character-specific jump count.

## Landing, bounds, crossing, and facing

Normal and double jumps integrate integer gravity until position reaches or passes
ground. Landing clamps Y to exactly zero, clears both velocities and committed jump
velocity, sets grounded, resets `bDoubleJumpUsed=false`, and returns directly to
crouch/walk/idle based on current held Down/horizontal input. There is no landing
recovery. Holding Up through landing has no Pressed edge and cannot start a new jump.

Every aerial horizontal move uses the existing deterministic center bounds. A step
that reaches or attempts to pass either boundary clamps to that boundary, reports
horizontal velocity zero immediately, clears committed horizontal momentum, and
does not bounce. Vertical integration continues normally.

Grounded fighters retain Milestone 3A pair push. If either fighter is airborne,
there is no pair push, so airborne crossing is allowed. After both intended moves,
optional grounded pair resolution, and stage handling, facing updates from final
simulation X positions. Crossing can therefore reverse facing without reversing
existing world-space aerial momentum. A later directional double jump interprets
raw direction using the new facing and may deliberately replace that momentum.

## Authoritative state and snapshots

`FFighterState` adds:

- `CommittedJumpVelocityX` (`int32`): pending launch or active world-space aerial
  horizontal velocity;
- `bDoubleJumpUsed` (`bool`): whether the one airborne reset has been consumed.

`CurrentState`, `StateFrame`, `bGrounded`, position, velocity, facing, and the 3A
input-edge fields already carry the rest of the sequence. All fields are copied by
`FMatchState` and included in defaulted field equality. The simulation remains
stateless outside that value. Restoring during startup, normal air, or after double
jump restores direction commitment, exact timing, input edges, and availability.
Struct padding is still not a serialization format.

## Debug harness

The HUD now names `JumpStartup` and `DoubleJumping` and shows double-jump status in
addition to simulation frame, state frame, position, velocity, grounded, facing,
and held/pressed/released masks. The placeholder briefly squashes to 82% height
during startup; feet remain fixed. Crouch presentation remains 55% height. These
visual transforms never feed simulation. Airborne height still maps simulation Y
to Unreal Z at 1024 subunits per centimetre.

## Automated validation

The final `ProjWEditor Win64 Development` build succeeded with UE 5.8.3 and MSVC
14.44. The full DX11 automation run found 27 tests and reported 27 succeeded, zero
warnings, zero failed, and zero skipped. The command-line process exited status 0.

| Test | Result |
| --- | --- |
| ProjW.DebugHarness.HitchProtection | Passed |
| ProjW.DebugHarness.RenderRateIndependence | Passed |
| ProjW.Simulation.Aerial.CrossingFacing | Passed |
| ProjW.Simulation.Aerial.DirectionalDoubleJump | Passed |
| ProjW.Simulation.Aerial.DirectionalJump | Passed |
| ProjW.Simulation.Aerial.DoubleJump | Passed |
| ProjW.Simulation.Aerial.GroundedInteractions | Passed |
| ProjW.Simulation.Aerial.HeldUp | Passed |
| ProjW.Simulation.Aerial.JumpStartup | Passed |
| ProjW.Simulation.Aerial.MixedRepeatability | Passed |
| ProjW.Simulation.Aerial.NeutralJump | Passed |
| ProjW.Simulation.Aerial.SnapshotReplay | Passed |
| ProjW.Simulation.Aerial.StageBounds | Passed |
| ProjW.Simulation.BackDash | Passed |
| ProjW.Simulation.Crouching | Passed |
| ProjW.Simulation.DashFacingSemantics | Passed |
| ProjW.Simulation.FacingRelativeWalk | Passed |
| ProjW.Simulation.ForwardDash | Passed |
| ProjW.Simulation.HeldJump | Passed |
| ProjW.Simulation.InputEdges | Passed |
| ProjW.Simulation.InputSnapshotReplay | Passed |
| ProjW.Simulation.MixedMovementRepeatability | Passed |
| ProjW.Simulation.MovementInvariants | Passed |
| ProjW.Simulation.PushInteraction | Passed |
| ProjW.Simulation.Repeatability | Passed |
| ProjW.Simulation.SnapshotReplay | Passed |
| ProjW.Simulation.StageBounds | Passed |

The first run passed 24/27. Two new crossing tests had setup assumptions that did
not guarantee their intended side relation, and the new wall test found that the
final partial step into a boundary was reported as velocity rather than zero. The
crossing setups were made explicit and the real wall-reporting defect was fixed.
No unrelated assertion was weakened. Final report:
`Saved/Automation/Milestone3B/index.html`; log:
`Saved/Logs/Milestone3BTests.log`.

A 300-render-frame offscreen DX11 game smoke run exited status 0 and produced
`Saved/Screenshots/WindowsEditor/HighresScreenshot00004.png`. The capture confirmed
the stage, both fighters, facing markers, and HUD double-jump field render. The
stale Milestone 3A HUD heading visible in that capture was corrected to 3B afterward;
the final Editor target was rebuilt successfully. No gameplay code changed after
the passing 27-test run.

## Known limitations and manual checks

Manually verify keyboard focus, four-frame startup feel, neutral/forward/backward
arcs, double-tap dashes after landing, one directional double jump, crossover
facing, crouch-to-jump, and stopping at both walls. Input collection still samples
hardware held state at render cadence; brief taps between render polls can be
missed even though a supplied simulation-frame input stream is deterministic.
There is no input buffering, coyote time, jump squat animation, aerial push,
landing recovery, presentation interpolation, replay file, or cross-platform
determinism verification.

Recommended next milestone: frame-indexed input recording/playback and a bounded
input-history abstraction suitable for command recognition and rollback replay.
No attacks, rollback, networking, or later milestone is implemented here. Changes
remain uncommitted for review.
