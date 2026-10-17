# Milestone 3A: frame input and grounded movement

Launch the editor with `-d3d11`, open `/Engine/Maps/Entry`, press Play in Selected
Viewport (one player, Standalone) and click the viewport. No scene construction
is needed. Shift+F1 releases the mouse; Escape stops PIE. Renderer settings and
Milestone 2's fixed-step driver are unchanged.

## Controls

A/D: raw Left/Right. W: Up (new press jumps). S: Down (grounded crouch).
Double tap the facing-relative forward direction to forward dash; double tap
backward to backdash. J/K/L: Light/Medium/Heavy, U: Special, I/O: Assist1/Assist2.
Action buttons are recorded but inert. Player 2 remains neutral; it can be pushed.

Holding W through landing no longer repeats a jump. Release and press again.
The jump impulse, gravity and airborne horizontal speed are otherwise unchanged.

## Integer configuration

One world unit (Unreal cm) is 1024 integer subunits. Velocities are subunits/frame;
gravity is subunits/frame squared. All gameplay advances at 60 frames/second.

| Constant | Value |
| --- | --- |
| ForwardWalkSpeed | 4096 (4 world units/frame) |
| BackwardWalkSpeed | 3072 (3 world units/frame) |
| WalkSpeed (existing airborne speed) | 4096 |
| JumpSpeed | 12288 |
| Gravity | -512 |
| DashInputWindowFrames | 12, inclusive press-to-press difference |
| ForwardDashDurationFrames | 12 |
| ForwardDashSpeed | 10240 (10 units/frame) |
| BackDashDurationFrames | 10 |
| BackDashSpeed | 8192 (8 units/frame) |
| StageLeftBoundary / StageRightBoundary | -512000 / 512000 (-500 / +500 units) |
| PushHalfWidth | 20480 (20 units) |
| MinFighterX / MaxFighterX | -491520 / 491520 (-480 / +480 units) |

Bounds inset the center by the push half-width, keeping the whole body in the
stage. All fighters, including airborne fighters, are clamped horizontally.
There are no Actor/physics collision walls. Dash displacement without constraints
is 120 world units forward or 80 backward. The first walking tap precedes this.

## Authoritative input memory and snapshots

`FFighterInput` remains a two-byte raw held-button mask. `FFighterInputEdges::From`
computes Held=current, Pressed=current & ~previous, Released=previous & ~current.
This calculation runs only per simulation frame, never from Unreal key events.

Each `FFighterState` now contains `PreviousInput`, `PressedInput`, `ReleasedInput`,
`LastTapDirection`, `TapAgeFrames` and `DashDirection`. After a completed frame,
PreviousInput holds that frame's input and becomes the next call's previous input.
The other two masks describe the last completed frame's edges for debugging.
LastTapDirection is raw world Left (-1), Right (+1), or no candidate (0).
TapAgeFrames increments per simulation frame and stops at window+1 (13), so old
taps cannot become valid again by counter overflow. DashDirection locks travel
in world space on dash entry and is cleared outside a dash.

All memory lives in the copied match state. The simulation has no hidden mutable
history. Copy/restore also restores held-key edge interpretation, a pending first
tap, or a dash in progress. Defaulted equality compares every field, including
input memory, without comparing struct padding. This is not a wire serializer.

## Dash recognition and priority

Two newly pressed, unopposed instances of the SAME RAW horizontal direction,
with a release between them, trigger a dash if their frame indices differ by at
most 12. A held key does not generate another press. Neutral release retains the
candidate. An opposite-direction press replaces it. Left+Right cancels horizontal
intent and clears the candidate; releasing one while holding the other is not a
fresh raw press. Airborne/crouch/jump frames and active dash frames clear candidates.
The successful pair is consumed; a held second tap cannot retrigger.

Classification uses facing on entry to the second-press frame. Thus Right,
release, facing reversal, Right triggers BACKdash when the fighter now faces
Left. Relative Forward then Forward using different raw directions does not form
a same-direction pair. Once started, travel direction is locked; later facing
changes do not turn the dash. No general motion parser or input buffer is added.

An active dash overrides all movement inputs until its fixed duration expires.
New Up presses during dash are consumed, not buffered into a jump. Outside dash,
a grounded Up press has priority over Down; Down has priority over walk and dash
recognition. Crouching prevents self-directed horizontal movement, but opponents
can still push a crouched fighter. Air control remains the original raw-direction
speed. Neutral stops self-directed walking immediately; Left+Right is neutral.

StateFrame is 0 on the first frame in a new state and increments on each subsequent
frame in the same state. An N-frame dash moves on state frames 0 through N-1 and
exits on the next call. On exit, current inputs select idle/walk/crouch; a new Up
press can initiate a jump. Boundary/body contact does not shorten dash duration.
Walking against a wall/body can remain Walking despite zero resolved velocity.

## Pair resolution and facing order

1. Save pre-step X order. Interpret each player's input using its entering Facing.
2. Independently compute both intended movements and stage-clamp positions.
3. If both fighters are now grounded, preserve their pre-step spatial order and
   enforce center separation of at least 40960 subunits (40 world units).
4. Split the spacing deficit between spatial left/right fighters. For an odd
   deficit, the extra subunit goes to spatial right, never to a fixed player ID.
   If a wall blocks separation, transfer that correction to the other fighter.
5. Report VelocityX as resolved displacement (including being pushed), then face
   each fighter toward the other's final simulation X. Increment SimulationFrame.

The pair solve is independent of which player slot holds the left fighter. It
also prevents intended movements from exchanging grounded sides. Airborne body
push is deliberately absent; fighters can cross while airborne. Landing applies
the same grounded separation rule. With equal starting X, opposite facing breaks
the spatial tie. A malformed equal-X/same-facing snapshot needs a deterministic
Fighter1-left fallback to repair overlap; valid separated grounded play never
uses it. Exact identity symmetry for two indistinguishable coincident fighters
cannot select distinct spatial sides without a tie rule.

All intermediate pair arithmetic uses int64. Stage bounds guarantee enough room
for the pair. These fixed constants must be identical for any future replay/peer.

## Debug presentation

HUD shows all six state names, state frame, grounded, facing, position, velocity,
held/pressed/released masks, simulation frame, render FPS and hitch count. Crouch
shrinks the visual body to 55% height while keeping feet at the same position;
this does not alter authoritative body dimensions or physics. Dashes use their
simulation displacement without animations. Mapping remains X/1024 -> Unreal X,
height Y/1024 -> Unreal Z, Unreal Y=0. The fixed camera frames the bounded stage.

## Validation and limitations

Run `Build.bat ProjWEditor Win64 Development -Project=E:/ProjW/ProjW.uproject
-WaitMutex -NoHotReloadFromIDE`, then the command-line editor with `-d3d11
-unattended -nop4 -nosound`, `-ExecCmds="Automation RunTests ProjW."`,
`-TestExit="Automation Test Queue Empty"` and report export under
`Saved/Automation/Milestone3A`.

All five pre-existing test registrations remain. MovementInvariants now asserts
one jump for held Up, stage clamping instead of unbounded X, and treats only action
buttons as gameplay-inert (Down now crouches). Existing snapshot/repeatability and
render-rate tests compare the expanded state. No tests were deleted.

Focused tests cover all ten input-bit edges, held jump/repress, both facings' walk
speeds, crouch/priority, both dash types/durations/window limits/held non-retrigger,
stage bounds, push symmetry and corners, snapshot replay between taps and during a
dash, facing reversal semantics and 20000 mixed deterministic frames.

Manual PIE checks still needed: keyboard focus and rapid double taps at different
render rates; crouch visibility; walk/dash contact with neutral P2 and stage edges.
The input collector still samples held hardware state at render cadence, so very
short taps between render polls can be missed. Simulation edges are deterministic
for a supplied per-frame input stream; this does not guarantee identical hardware
sampling across render rates. The eight-step hitch cap remains a local debug clock
policy. No presentation interpolation, air-body constraints, replay file format or
cross-platform determinism verification is added.

Recommended next milestone: Milestone 3B's deliberate jump redesign, specified and
tested separately. No attacks, rollback, networking or subsequent milestone is
implemented here. Changes are left uncommitted for review.

## Recorded validation

`ProjWEditor Win64 Development` built successfully with UE 5.8.3 / MSVC 14.44.
The final full automation run reports 16 succeeded, zero warnings, zero failed,
zero not run, and process exit status 0. Every registered ProjW test ran:

| Test | Result |
| --- | --- |
| ProjW.DebugHarness.HitchProtection | Passed |
| ProjW.DebugHarness.RenderRateIndependence | Passed |
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

An initial run passed 15/16: the mixed-sequence coverage check exposed that its
intended neutral release frames still held Right. Correcting that test sequence
exercised every required movement state; the complete rerun passed. No simulation
assertions were disabled. Final report: `Saved/Automation/Milestone3A/index.html`;
log: `Saved/Logs/Milestone3ATests.log`.

A 300-render-frame offscreen DX11 game smoke run exited with status 0. Its captured
image was inspected: both placeholder fighters, stage, facing markers and the new
Milestone 3A HUD with held/pressed/released masks are visible. Capture:
`Saved/Screenshots/WindowsEditor/HighresScreenshot00003.png`. This neutral-input
smoke run does not verify manual crouch/dash controls or keyboard focus.
