# Milestone 5V: Combat Lab Visualisation Pass

Milestone 5V adds temporary, presentation-only character and arena visuals so
the accepted 5G simulation can be judged without keeping the detailed HUD open.
It does not change inputs, authoritative state, move definitions, timings,
geometry, contact rules, or rollback behavior.

## Character assets

The project originally had no character assets under `Content` (only
`Collections` and `Developers`). UE 5.8's installed high-quality mannequin
template contained usable assets at
`E:\UE_5.8\Templates\TemplateResources\High\Characters\Content\Mannequins`.
The 128 `.uasset` packages (125.3 MB) were copied intact to
`Content/Characters/Mannequins`, preserving their `/Game` package paths and
dependencies. The source assets include:

- Manny mesh: `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`.
- Quinn mesh: `/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`.
- Animation assets:
  - `/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Bwd`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Left`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Right`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Bwd`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_WallJump`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03`
  - `/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack`
  - `/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01`
  - `/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01`.

P1 uses Manny and P2 Quinn. Their bundled character materials remain intact;
small blue/orange shoulder markers reinforce team identity. The original
unarmed attack and locomotion clips are reused; the rifle-named hit-react clips
are used without weapon assets. These are generic third-person template
animations, not authored fighting-game content.

## Presentation mapping

`ADebugFighterActor` contains a small simulation-to-visual selection function.
Its collision-disabled skeletal component follows the authoritative position
and facing. Single-node animations ignore root motion, use no notifies, and do
not update match state. Locomotion/idle clips loop; attack clips sample their
timeline from the move's authoritative phase/frame, so hitstop holds the sampled
pose. The concrete mapping is:

| Simulation state/action | Animation / presentation |
| --- | --- |
| Idle | `MM_Idle`, looped. |
| Walking toward/away from opponent | `MF_Unarmed_Walk_Fwd` / `MF_Unarmed_Walk_Bwd`, looped according to the velocity dot local axis. |
| Crouching | `MM_Idle`, lower/compressed in-place silhouette. |
| JumpStartup / Jumping | `MM_Jump`, sampled startup / looped airborne. |
| DoubleJumping | `MM_WallJump`, looped airborne. |
| ForwardDash / BackDash | `MF_Unarmed_Jog_Fwd` / `MF_Unarmed_Jog_Bwd`, looped. |
| Light / Medium / Heavy | `MM_Attack_01` / `MM_Attack_02` / `MM_Attack_03`, sampled over authoritative startup, active, and recovery frames. |
| GroundRupture | `MM_ChargedAttack`, sampled over its authoritative phase; floor pulse only during active frames. |
| Standing / crouching Block | `MM_Idle`, neutral or lower silhouette, with a restrained upper/lower guard panel. |
| Standing / crouching Blockstun | `MM_HitReact_Front_Lgt_01`, sampled by `StateFrame`, with the guard panel retained. |
| Pivot counter-clockwise / clockwise | `MF_Unarmed_Walk_Left` / `MF_Unarmed_Walk_Right`, sampled over Pivot frames; directional floor ring. |
| High / Low Parry | `MM_Idle`, upper/lower colored interception marker; Low Parry also lowers the silhouette. |
| ParrySuccess | Held `MM_Idle` pose and brief green accent. |
| ParryStun | `MM_HitReact_Front_Hvy_01`, sampled by `StateFrame`, amber accent. |
| Hitstun | `MM_HitReact_Front_Lgt_01`, sampled by `StateFrame`, red impact accent. |

All movement clips are configured as in-place presentation; actor transforms
continue to follow simulation state. Animation cannot move fighters or change
authoritative state.

Light, Medium, Heavy, and GroundRupture map to three generic attacks and a
charged attack. The selected sequence is sampled across the complete authored
startup/active/recovery duration; the active phase also receives a small visual
extension cue. The GroundRupture floor pulse appears only on authoritative
active frames. Its exact 112-unit authoritative radius remains visible when
debug geometry is enabled.

No template block or parry clips were present. Standing and crouching Block use
a restrained guard panel at upper/lower height; Blockstun reuses a light hit
reaction with the guard panel retained. High and Low Parry use visibly
different upper/lower colored markers and distinct generic pose selections.
ParrySuccess gets a brief green visual accent, ParryStun an amber accent, and
Hitstun a red hit reaction. These simple cues distinguish the actions but are
not bespoke defensive poses or production effects.

## Arena and controls

The isolated floor is now a larger, non-colliding training-room floor with
eight perimeter pillars. A 480-world-unit radius disc marks the authoritative
arena edge. A separate broken ring at radius 455 marks the conservative fighter
center limit, and concentric/depth spokes make small Pivot displacement easier
to see. All environmental meshes have collision disabled; simulation bounds
remain the only gameplay limits. The accepted predominantly side-on camera,
Pivot hold, and recenter behavior are unchanged.

- **F2** toggles the detailed simulation HUD. Health bars remain visible.
- **F3** toggles hurt volumes, attack volumes, GroundRupture radius, and the
  local/presented combat-axis lines. Stage-boundary/depth markings stay visible.
- Current debug input layout: P1 A/D/W/S, Q/E, R/F/C, V, T/G; P2 arrows,
  Numpad 4/6, 7/8/9, 5, and 1/2. See `Docs/Milestone5G.md` for the exact
  action table and diagnostic input path.

## Limitations and validation

The visual component uses stock UE mannequin assets and simple geometric cues.
There are no bespoke guard, parry, Pivot, GroundRupture, or fighter-specific
combat animations; no animation notify, root-motion, skeletal collision,
physics, or animation-to-simulation path is used. Verify model scale, floor
contact, clip quality, and readability in the DX11 editor before treating this
placeholder pass as human-accepted.

## Manual DX11 playtest

Launch the project with `-d3d11`, click the viewport, and keep the detailed HUD
off initially. Exercise P1 against local P2 and judge the character silhouettes
without reading state text:

1. Walk forward/back, crouch, jump, double jump, and dash in both directions.
2. Use R/F/C for Light/Medium/Heavy, then V for GroundRupture;
   compare the visible windup, active moment, and recovery against state timing.
3. Use Q/E for both Pivots; confirm the stride is in-place while the model
   follows the simulation path and the camera retains its accepted hold/recenter.
4. Hold Away for standing Block and Down+Away for crouching Block. Use T/G for
   High/Low Parry; confirm upper/lower accents, successful-Parry flash, and the
   contrasting ParryStun and Hitstun reactions.
5. Confirm P1/P2 remain recognizable and team-marked, the stage is visibly
   larger than the 480-unit arena, the 480 edge differs from the dashed 455
   fighter-center limit, and no environment mesh obstructs gameplay.
6. Press F3 to compare clean presentation with hurt/attack/radial volumes and
   combat axes. Press F2 to bring back the detailed simulation diagnostics.

Human visual acceptance remains open; the smoke launch verifies startup and
asset resolution but does not replace this playtest.

## Validation

Final verification on 2026-10-17:

- `ProjWEditor Win64 Development`: succeeded.
- Canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1`:
  95 discovered, 95 passed, 0 warnings, 0 failed, and 0 skipped/not-run.
- Unreal process exit code: 0. Runner script exit code: 0.
- Final current-source report: `Saved/Automation/CodexValidation-20261017-231103`;
  log: `Saved/Logs/CodexValidation-20261017-231103.log`.
- DX11 `UnrealEditor.exe -game` smoke launch reached `Debug match ready`, exited
  0, and loaded both mannequin meshes without skeletal-material or asset-load
  warnings (`Saved/Logs/5V-DX11-Smoke-20261017-2312.log`). The restricted
  identity still logged denied writes to user-local Zen security configuration;
  project-local DDC/shader paths worked and the harness started normally.
- An earlier complete run recorded one test warning because the restricted
  environment could not reach EOS during `Move.FoundationDefinition`; no test
  assertion failed. The final full run completed without warnings.

## Revision A manual-playtest corrections (2026-10-18)

This revision addresses the reported pillar obstruction and forward/back confusion, strengthens the placeholder crouch silhouette, and adds a presentation-only diagnostic for the reported black-screen issue. The authoritative simulation, its movement speeds, combat rules, arena dimensions, and deterministic tests were not changed for this revision.

### Crouch asset audit and pose

The bundled mannequin assets were searched for crouch, crouched locomotion, squat, duck, and low-stance clips; none suitable for this use were found. The available `MM_Idle` remains the pose source. The debug actor now uses a wider, compressed, lowered mesh silhouette and lowers the team marker while crouching. This is a deliberately simple component-transform approximation; no authoritative Y position, hurt volume, grounded state, or gameplay dimension changes. It is not a jointed knee-bent crouch animation. A proper crouch pose would require authoring/configuring a mannequin pose or animation, which is outside this safe presentation-only pass. Human review should confirm that the stronger silhouette reads clearly and that the feet remain acceptably planted.

### Arena clutter

All eight tall perimeter pillars and their bases were removed. The broad flat training floor, 480-unit playable-circle marking, fighter-center-limit ring, and floor/depth reference markings remain. No replacement tall scenery was added, and authoritative circular bounds are unchanged.

### Forward/back diagnosis and correction

The authoritative simulation maps Left/Right onto its canonical local horizontal axis (`WorldRightAxis`) and compares that direction with simulation facing for forward/back state. It has no camera or rendered-transform input. The player controller previously passed raw A/D (and arrow-key) direction through unchanged. After the combat plane rotated, the displayed camera screen-right could oppose the simulation's canonical world-right axis. Thus screen-direction controls and the displayed plane disagreed. The visual actor also used movement classification that could remain in a walk action when the projected velocity was effectively zero. Fighter yaw was set from the live opponent vector, with no explicit shortest-angle normalization.

The correction maps physical screen-left/right to simulation Left/Right before constructing each `FFighterInput`. It uses the displayed camera yaw and the fighter pair's canonical local axis, with a five-degree alignment hysteresis. If the mapping changes while a horizontal key is held, both directions are suppressed for one simulation frame (except during an Up press) to prevent the mapping change from appearing as a fresh dash-tap edge. This is presentation/input-layer state only; camera yaw is not copied into `FMatchState` or used by simulation movement. Visual forward/back selection uses actual velocity projected toward the opponent and an integer-subunit dead zone; near zero projected speed selects Idle. Actor yaw is normalized through the shortest-angle delta. Camera yaw/position are derived from one presentation sample each update, avoiding the earlier independent location lag while yaw was held or recentered.

The DX11 scripted input attempt exercised sustained A/D, repeated D double taps, repeated Q and E Pivots, alternating Q/E, movement after Pivot, a jump at the outer part of the arena, and simultaneous P1/P2 movement/Pivot input. The F4 trace showed the expected world/screen-axis relation and stable camera values during normal and Pivot states. This was a presentation/input mapping issue; no deterministic simulation defect was demonstrated.

### Black-view investigation

F4 toggles a compact readout and logs a diagnostic sample every 60 simulation frames. It includes both fighters' authoritative and actor XYZ, pair center, pair-axis delta/length, displayed and desired yaw, Pivot hold/recenter flags, camera XYZ/rotation/distance, orthographic width/FOV, floor transform, finite-value status, and arena-circle membership. It reports a camera invariant failure or a fighter outside the circle. The diagnostic is presentation-only; it does not alter match state.

The DX11 window capture showed a predominantly black background with both fighters and a narrow floor strip still visible. That appearance also occurs without an action-triggered camera failure because this greybox has no sky or backdrop and its accepted side-on, zero-pitch camera sees the floor nearly edge-on. The reported sudden total blackout was **not reproduced** by the scripted movement/Pivot/jump sequence, so no root cause or corrective change is claimed for that distinct symptom. During the near-edge sequence the sampled fighters remained within the 480-unit circle. At frame 6322, for example, P1 was `(409.29, 17.48, 0.00)` and P2 `(449.29, 17.47, 0.00)` in presentation X/Y/Z; both were inside. Camera was `(429.57, 1317.48, 160.00)`, rotation `(0.000, -90.013, 0.000)`, distance `1300.00`, ortho width `1200.00`; the sample reported finite values and `cameraInvariant=ok`. Across the recorded samples no invalid transform, arena escape, center-distance collapse, pitch, roll, or yaw wrap discontinuity was observed. If the abrupt blackout recurs, press F4 and preserve the corresponding `Saved/Logs/5V-RevisionA-Playtest.log` sample for comparison. The static black backdrop remains a presentation limitation and the sudden blackout remains open for human retest.

### Revision A validation

- `ProjWEditor Win64 Development` built successfully (UBT process exit 0).
- `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1` completed the full suite: 95 discovered, 93 passed, 2 passed with warnings, 0 failed, and 0 skipped/not-run. Unreal process exit 0; runner script exit 0. The two warnings were offline EOS SDK configuration/analytics connection warnings; no project test assertion warned or failed.
- DX11 `UnrealEditor.exe -game` smoke/playtest reached `D3D11RHI` initialization and `Debug match ready`; the diagnostic screenshot and trace were produced. The controlled game process was then closed normally. This confirms startup and the scripted diagnostic path, not that the sudden blackout is fixed.
- Report: `Saved/Automation/CodexValidation-20261018-002728`; log: `Saved/Logs/CodexValidation-20261018-002728.log`. DX11 diagnostic log: `Saved/Logs/5V-RevisionA-Playtest.log`.

Human visual acceptance remains open. In particular, review the squat approximation, screen-direction semantics through recentering, and capture F4 diagnostics if the abrupt all-black view recurs.

## Revision B ? camera/boundary repro and fighter presentation (2026-10-18)

Revision B follows the Revision A scripted repro and keeps the presentation-only scope. It does not change authoritative gameplay, the 455-unit fighter-center bound, the 480-unit circular floor, simulation inputs, move definitions, or tests.

### Black-view diagnosis and correction

The observed dark view was the empty world beyond the greybox stage: the harness had no backdrop. A side-on zero-pitch camera also projected the circular floor and its depth markings nearly edge-on, leaving only a thin floor strip beneath the fighters. The recorded DX11 diagnostics did not show an invalid camera transform, an arena escape, or a fighter becoming invisible: camera distance remained 1300, orthographic width 1200, pitch/roll finite, both fighter centers remained inside the disc, and the floor remained inside the camera's vertical view. Thus the reproducible visual loss was the unbounded dark void and readable floor/depth area, not evidence that the characters or camera target were disappearing. The transient all-black report itself was not reproduced as a full-frame loss.

The harness now adds a quiet, collisionless, camera-aligned emissive color card well outside the 480-unit arena, so it fills the view throughout a full yaw rotation without placing a wall or collider in the stage. The camera keeps its fixed gameplay-height target (Z=160 in the presentation mapping) and now uses a fixed -8-degree pitch. This reveals enough of the floor/depth plane to read the disc while still holding the camera at the stage during jumps. In the final DX11 trace, the peak recorded fighter height was 265.5 units; camera pitch stayed -8 degrees, distance stayed 1300, `floorInView=yes`, `backdrop=on`, and `cameraInvariant=ok`. The backdrop and camera never enter authoritative state.

The retained limits are unchanged: fighter centers clamp to the 455-unit radius, while the visible floor edge remains 480 units. DX11 scripted captures show the floor, its two edge cues, the distant backdrop, and both fighters at the opposite side and at a diagonal boundary position. Repeated jumps at these positions, after Q Pivot and subsequent movement, kept `arenaCenters=inside/inside` and `cameraInvariant=ok`; the trace includes both airborne heights and landing. The same checks were repeated after rotating the pair axis. No boundary escape, black-frame loss, NaN/infinite transform, or actor/camera separation appeared. Captures and full trace are in ignored `Saved/Logs/5V-RevisionB-*.png` and `Saved/Logs/5V-RevisionB-DX11-BoundaryFinal.log` artifacts.

### Character presentation audit and correction

The copied mannequin animation set has walk/jog and attack clips but no suitable dodge, evade, or sidestep transition. Pivot therefore no longer loops the walk-left/right clips. It uses the idle asset with a deterministic 12-frame presentation-only weight shift, weave, and plant curve. The actor heading remains stable during the Pivot; camera yaw holds over the complete Pivot state, then recenters smoothly. Animation root motion remains ignored, and simulation coordinates remain the only source of fighter displacement.

`MM_Attack_02` was also reviewed for the assigned Low Medium. It reads upright and does not communicate a low strike, so the debug actor uses the idle asset with a lowered, compressed forward lunge over the existing simulation phase. This does not alter the move definition, attack timing, hit volume, or state. The copied pack has no suitable low-strike clip. Light and Heavy retain their available attack clips. The existing crouch silhouette approximation and screen-relative A/D input mapping are preserved. The DX11 captures confirm that the Low placeholder changes the body silhouette and that Pivot remains in place at the mesh level; human review is still needed for pose quality.

### Final validation

- `ProjWEditor Win64 Development` built successfully from the final Revision B source; UBT exit code 0.
- `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1` discovered and executed the complete 95-test ProjW suite. Result: 94 passed without warnings, 1 passed with warnings, 0 failed, 0 skipped/not run. Unreal process exit 0; runner script exit 0.
- The single test warning was attached to `ProjW.Simulation.Normal.UnlikeTrades` and consisted of restricted-environment EOS SDK HTTPS connection warnings (`api.epicgames.dev`, connection denied). Its assertions passed; this was not a project test warning or gameplay failure. Other startup-only environment messages included the Unreal Trace Server timeout and user-local Zen security-config write denial; the project-local DDC and shader working directory were writable and initialization completed.
- DX11 `UnrealEditor.exe -game -d3d11` smoke/repro reached `D3D11RHI` initialization and `Debug match ready`, then closed normally. Scripted boundary jumps were observed from the negative-X edge and a diagonal near-edge location, including after Pivot and movement. Diagnostic state remained finite and within the circular bounds. Log: `Saved/Logs/5V-RevisionB-DX11-BoundaryFinal.log`.

Human visual acceptance remains open. The DX11 checks verify that this repro no longer loses the floor/backdrop or camera/fighter invariants and that the boundary remains legal; they do not establish final art quality, Pivot readability, or the Low pose's animation quality.

## Revision C ? fighting stance and Medium visual (2026-10-18)

Revision C addresses the human review that both mannequins showed too much side/back and that Medium's striking leg became unreadable. This is presentation-only. Pivot, its simulation and pose curve, the camera hold/recenter, screen-relative controls, arena/backdrop, camera pitch, Light, Heavy, and all authoritative state remain unchanged.

### Camera-facing fighting stance

The previous yaw aimed each mannequin's authored forward directly toward the opponent, with a small camera bias. That read as side/back surfaces, and a shared offset could turn the second fighter's chest away from the camera. The mesh forward was checked against the bundled forward locomotion clips; the mannequin's authored forward is local +X.

The new visual target is the shortest-angle midpoint between the displayed camera-facing yaw (`displayed camera yaw + 180 degrees`) and the opponent's horizontal X/Z bearing. This keeps the chest substantially open to the camera while leaving the fighter's lead direction halfway toward the opponent. The per-fighter opponent bearing naturally mirrors the stance: with the camera at -90 degrees and the pair on world X, P1 presents at 45 degrees and P2 at 135 degrees. After the combat plane accumulates about 40 degrees of rotation, the equivalent values are about 85 and 175 degrees. These are actor yaw values only; `EFighterFacing`, attack axis, and simulation transforms are not changed. Presentation interpolation uses Unreal's shortest-angle rotation interpolation, so recentering settles without a 180-degree wrap snap.

During Pivot the existing actor-heading hold remains in effect. The fighter does not twist toward the moving opponent vector while the camera holds its plane. After Pivot exits and the camera recenters, the stance smoothly converges to the midpoint for the new displayed plane. The Revision C scripted captures showed both fronts in the initial pair and after repeated Q/E movement and recentering; the F4 trace reported mirrored headings and `cameraInvariant=ok` after the rotated-plane pass.

### Medium Low visual

The animation audit searched the copied content tree for kick, low, sweep, attack, melee, combat, crouch, duck, strike, and leg assets. The only attack-named assets found were `/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01`, `MM_Attack_02`, `MM_Attack_03`, and `MM_ChargedAttack`; none is a readable low strike. In particular, `MM_Attack_02` is upright and does not match this Low move, so Medium continues to use `/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle` as its base pose. No new animation asset was introduced.

The failure was not an animation sampling dropout or a gameplay hitbox issue: an upright idle/attack pose was being compressed and pitched as a whole to imply a low lunge. That squeezed the mannequin's legs together and made its own body occlude the striking limb at the active pose. The replacement keeps the mannequin near its normal proportions and adds a presentation-only thigh, shin, and foot progression, built from collision-disabled primitive mesh components. Startup lowers/prepares the body; the active phase extends the low limb toward the opponent in the camera-side local plane; recovery retracts it. The progress reads the existing authoritative move phase and frame, but adds no root motion or simulation state. The team marker is hidden only during this lowered pose so it does not appear detached above the lowered torso. The same local construction mirrors for either player and follows the rotated actor yaw. Revision C captures covered P1, P2, and P2 on the rotated plane; the striking limb remained visible for both. Light and Heavy retained their existing clips and were checked for compatibility. High/Low Parry and standing/crouching Block cues were also checked; no defense redesign was needed.

### Revision C validation

- `ProjWEditor Win64 Development` rebuilt successfully after the final presentation adjustment (UBT exit 0; `Saved/Logs/Build5VRevisionCFinal.log`). No simulation or test source changed.
- The canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1` completed all 95 discovered tests: 95 passed, 0 test warnings, 0 failed, 0 skipped/not-run. Unreal process exit 0; script exit 0. Report: `Saved/Automation/CodexValidation-20261018-012830`; log: `Saved/Logs/CodexValidation-20261018-012830.log`. Startup environment output included the Trace Server timeout and offline EOS connection messages, but no project test warning or failure.
- A final-source DX11 `UnrealEditor.exe -game -d3d11` launch reached `Debug match ready` with project-local DDC and shader working paths (`Saved/Logs/5V-RevisionC-Final-DX11.log`) and was closed normally. The earlier Revision C scripted DX11 checks supplied stance, rotated-plane, P1/P2 Medium, Light/Heavy, defense, and Pivot captures. The final marker-only adjustment was compiled and passed the complete suite; a new interactive visual capture of that last marker visibility tweak could not be produced in the restricted desktop session.

Human visual acceptance remains open. Revision C is not marked human-accepted; the last marker visibility adjustment should be confirmed in the next interactive editor review.
## Revision D: fighter body orientation

Revision D corrects only presentation orientation. Revision C?s camera/opponent midpoint heading left each fighter visibly three-quarter turned away at some camera orientations. The actor root now smoothly aims toward the opponent (and keeps the existing heading lock for the full Pivot); the skeletal mesh is counter-rotated independently to present its anatomical front toward the displayed camera. The mannequin?s front is local -Y, so the camera-facing skeletal yaw uses the shortest root-to-camera heading delta plus 90 degrees. This keeps root-attached combat cues directed at the opponent while the chest and face remain camera-readable. Manny/P1 is mirrored across local X and Quinn/P2 remains unmirrored, reversing their lateral strike pose while preserving the same local -Y front. No authoritative facing, simulation, attack, camera, arena, or Pivot behavior changed.

### Visual checks

Fresh DX11 `-game` captures were taken from the diagnostic harness. At the initial axis, displayed camera yaw was -90 degrees and actor headings were P1 0 / P2 180. Rotated-plane captures measured displayed camera yaw about -2.0 degrees with P1 87.8 / P2 -92.1, then 85.3 degrees with P1 175.1 / P2 -4.8, and 186.7 degrees with P1 -97.0 / P2 83.2. Both mannequins? fronts remained visible across the near-quarter-turn orientations. Final Light captures show P1?s arm extending screen-right toward P2 and P2?s arm extending screen-left toward P1. These checks exposed and corrected the initial incorrect assumption that the mannequin front was local +X; the visible front is local -Y. Capture images are in ignored `Saved/Logs/5V-RevisionD-*.png` files.

The diagnostic Heavy capture showed its forward cue on the opponent side. GroundRupture was captured after its active cue had ended, so that image does not establish its direction or active presentation. Medium and the active GroundRupture cue still need a deliberate human visual check. Revision D does not claim acceptance of those remaining presentation checks. F4 camera/plane diagnostics remained consistent, and the repeated-Pivot camera hold/recenter implementation was not changed. No gameplay or test source was changed for this revision.

### Revision D validation

- `ProjWEditor Win64 Development` built successfully after the final orientation and side-mirroring changes (UBT exit 0; `Saved/Logs/Build5VRevisionD.log`).
- The final canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1` run discovered 95 tests: 95 passed, 0 test warnings, 0 failed, 0 skipped/not-run. Unreal process exit 0; runner script exit 0. Report: `Saved/Automation/CodexValidation-20261018-015658`; log: `Saved/Logs/CodexValidation-20261018-015658.log`.
- The command-line log confirms Zen used the writable project-local `Saved/AutomationDDC/Zen` directory and DDC used `Saved/AutomationDDC`. Restricted startup emitted Trace Server timeout and offline EOS connectivity warnings; they did not produce automation test warnings or alter the green result.
- DX11 smoke launch reached D3D11 initialization and `Debug match ready` (`Saved/Logs/5V-RevisionD-DX11-Final.log`). Human acceptance remains open; inspect Medium and active GroundRupture presentation, plus overall stance readability, in the interactive editor. Revision D is not marked human-accepted.
## Revision E - external fighting idle (2026-10-18)

This presentation-only revision selects an unarmed combat stance from the external Motifect Martial Arts Motion Pack. The source is `External/MotifectCombat`: 40 binary FBX 7400 humanoid clips and `Documentation/README.txt`. The pack's README says each FBX also contains a simple stickman preview mesh and recommends UE5 IK Retargeting. The source rig is a generic biped hierarchy with `Hips`, `Spine1`/`Spine2`/`Chest`, `Neck1`/`Neck2`/`Head`, paired shoulder/arm/forearm/hand and leg/shin/foot/toe chains, plus finger bones. Its names and skeleton differ from the project mannequin.

### Candidate selection

The stance candidates are `muay_thai_guard_stance.fbx`, `wrestling_guard_position.fbx`, `sword_draw_stance.fbx`, and `staff_spin_guard.fbx`. `muay_thai_guard_stance.fbx` was selected for its unarmed, upright striking guard and suitability for an unarmed 2.5D roster. The wrestling pose is a ground/grappling-oriented guard; sword draw and staff spin guard are weapon-specific. The project has no candidate attack or defense clips enabled by this revision.

Other useful future source names include `muay_thai_combination.fbx`, `muay_thai_body_kick.fbx`, `muay_thai_elbow_cut.fbx`, `muay_thai_teep.fbx`, `muay_thai_roundhouse.fbx`, `muay_thai_push_kick_defense.fbx`, `muay_thai_knee_clinch.fbx`, `tkd_front_kick_high.fbx`, `tkd_side_kick.fbx`, `tkd_back_kick.fbx`, `tkd_axe_kick.fbx`, `tkd_spinning_heel_kick.fbx`, `sword_block_high.fbx`, `sword_parry_and_riposte.fbx`, and `wrestling_stand_up_defense.fbx`. They remain source-only in `External`.

### Import, retarget, and presentation

The selected FBX was imported with UE 5.8's legacy FBX skeletal-mesh/animation importer to create its source skeleton and sequence. UE5 IKRig auto-generated retarget definitions were created for the actual source skeleton and `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`; `IKRetargetBatchOperation` produced the Manny-target sequence at `/Game/Characters/CombatLab/Animations/muay_thai_guard_stance_Anim_Manny`. Manny and Quinn share `/Game/Characters/Mannequins/Meshes/SK_Mannequin`, so the same target sequence supports both. Temporary comparison maps, unselected candidate imports, source preview meshes, and intermediate IK Rig/IK Retargeter assets were removed; the source FBXs remain untouched in `External`.

Only ordinary `EVisualAction::Idle` selects the new sequence, with the stock `MM_Idle` as a load fallback. Crouch, Block, Parry, Pivot, attacks, and reactions retain their prior selections. The skeletal component remains in single-node presentation mode, ignores root motion, and does not fire gameplay notifies. The animation cannot change simulation state. Actor-root orientation still follows the opponent, while the existing presentation yaw keeps both chests visible from the camera. P1 uses the established presentation-only local-X skeletal-component reflection (`bMirrorPresentation`); P2 uses the unreflected pose. This does not alter authoritative facing or rotate an entire character 180 degrees.

### Validation and open visual review

`ProjWEditor Win64 Development` succeeded (UBT exit 0). UnrealBuildTool first failed before compilation because its trace backup in the restricted identity's user-local AppData was not writable. The authorized rerun used no ACL or lasting environment changes and compiled `DebugFighterActor.cpp`, `DebugMatchDriver.cpp`, and generated module code.

The canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1` run after the build discovered 95 tests: 95 passed, 0 warnings, 0 failed, and 0 skipped/not-run. Unreal process exit and runner script exit were both 0. Report: `Saved/Automation/CodexValidation-20261018-115303`; log: `Saved/Logs/CodexValidation-20261018-115303.log`.

A side-by-side candidate preview map was attempted for the Muay Thai, wrestling, and sword stances, but its DX11 capture rendered black and could not support a reliable visual ranking. The selection above is therefore based on the source clip names and intended motion described by the pack, plus the successful in-harness preview of the selected Muay Thai sequence; the alternatives were not validated in a usable comparative capture.

The DX11 game launch reached `Debug match ready`. Initial neutral capture: `Saved/Logs/Milestone5V-CombatIdle-DX11.png` (source capture: `Saved/Screenshots/WindowsEditor/HighresScreenshot00005.png`). It shows a bent stance, higher guard, lead side, and a visible chest angle, with Manny and Quinn opposing each other. The restricted windowed session did not accept automated keyboard injection, so a post-Pivot/recenter image and Idle-to-Light/Medium/Heavy transition images were not captured. Those visual checks remain open, along with human review of loop quality and mirroring. This revision is not marked human-accepted.

## Revision F: opponent-facing presentation and animation repair (2026-10-18)

Human review rejected attempts to expose both fighters' chests while preserving
combat facing. The placeholder now prioritizes the simpler rule that each
fighter visually faces the opponent; opposing profiles are acceptable. This
choice does not prohibit authored fighting-game stance posing in production.

### Orientation

The camera-facing skeletal yaw compensation
(`FindDeltaAngleDegrees(UnwrappedYaw, CameraFacingYaw) + 90`) and the
`bMirrorPresentation` P1-only negative local-X mesh scale have been removed.
Both fighters use positive component scale. Each
actor root smoothly targets the X/Z bearing from that fighter to its opponent;
while Pivot is active, the actor heading is held, then opponent-facing updates
resume. No midpoint yaw or camera-facing mesh rotation remains. Inspection and
DX11 captures established that Manny and Quinn's visible anatomical forward is
local +Y. A fixed -90 degree skeletal-component yaw maps local +Y to actor +X,
which points toward the opponent. The camera implementation and Pivot simulation,
timing, displacement path, and pose curve were left unchanged.

### Kick leg and Medium

The old Medium path did not play `MM_Attack_02`; it selected the idle animation
and constructed a fake low-strike leg from three separate primitive mesh
components (`LowStrikeThigh`, `LowStrikeShin`, `LowStrikeFoot`). Their transforms
were rebuilt from a phase curve and their visibility was conditional on the
Medium visual action. This detached helper was the source of the malformed /
vanishing kick limb; it was not a hidden skeletal bone. Source inspection found
no attack code that hides or scales skeletal bones, changes skeletal mesh
visibility, or detaches animated limbs. All three helper components and their
phase-driven placement code have been removed. The intact stock `MM_Attack_02`
sequence now supplies Medium presentation, and the team marker remains visible.
No External combat-pack animation was retargeted.

Light, Medium, and Heavy map simulation startup/active/recovery frames across
the authored clip using the normalized phase frame. Sampling is clamped below
the exact sequence endpoint (`Length - max(KINDA_SMALL_NUMBER, Length *
0.0001)`), avoiding endpoint evaluation while preserving the complete authored
clip interval. Since presentation is sampled from the unchanged simulation
phase each render tick, hitstop continues to hold the same pose. Root motion,
notifies, and gameplay-side animation reads remain disabled. The Muay Thai
combat idle remains the ordinary standing idle, with `MM_Idle` only as its
asset-load fallback. Medium remains authoritative Low; no simulation data or
gameplay code changed.

### DX11 visual review and validation

DX11 captures under `Saved/Logs/5V-OrientationRepair-*.png` show neutral opposing
profiles, inward Light strikes, intact generic Medium and Heavy poses, a Pivot
pose, and a settled diagonal fighter relationship. In particular:

- Neutral: `Saved/Logs/5V-OrientationRepair-Neutral.png`
- Light: `Saved/Logs/5V-OrientationRepair-Light.png`
- Medium: `Saved/Logs/5V-OrientationRepair-Medium.png`
- Heavy: `Saved/Logs/5V-OrientationRepair-Heavy.png`
- Pivot pose and settled diagonal: `Saved/Logs/5V-OrientationRepair-PivotPose.png`
  and `Saved/Logs/5V-OrientationRepair-PivotSettle.png`

These are renderer captures of representative states, not a recorded manual
key-driven sequence. The static DX11 review confirms both characters' facing and
that the full skeletal limbs are present in sampled Light/Medium/Heavy poses;
repeated Q/E input, natural transition timing through every attack phase, and
human judgment of the animation loop remain open playtest checks. Milestone 5V
is not marked human-accepted.

`ProjWEditor Win64 Development` built successfully (UBT exit 0;
`Saved/Logs/Build5V-Final.log`). The canonical
`powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1` run discovered
95 tests: 93 passed, 2 passed with warnings, 0 failed, and 0 skipped/not-run.
Unreal process exit code and runner script exit code were both 0. The two
warnings were EOS SDK configuration / analytics connection failures because
the restricted test environment cannot reach Epic services; no project test
assertions warned or failed. Report:
`Saved/Automation/CodexValidation-20261018-165545`; log:
`Saved/Logs/CodexValidation-20261018-165545.log`.

## Revision G: camera-side continuity and presentation cleanup (2026-10-18)

This revision corrects the earlier black-view diagnosis. Human playtesting
identified that asymmetric scene lighting made the scene appear black when a
fighter crossed the opponent and the camera switched to the opposite side of
the combat plane. The accepted design decision is to keep the established
camera side and let the fighters exchange screen-left/right positions. No
lighting was added to mask the problem.

### Continuous camera side

The old side-on yaw was calculated as
`-90 degrees + atan2(P2.Z - P1.Z, P2.X - P1.X)`. Reversing the ordered pair
vector at a crossover changed that raw value by 180 degrees. The camera
therefore tried to preserve fighter screen ordering by moving to the opposite
side, even though both side-on orientations describe the same combat plane.

The presentation helper now considers the raw side-on yaw and its equivalent
180 degrees away. It unwinds both candidates, measures each shortest wrapped
angular delta from the currently displayed camera yaw, and chooses the closer
candidate. Ordinary crossings consequently keep camera orientation continuous;
fighters can exchange screen sides. This state remains in
`ADebugMatchDriver` and does not enter simulation or rollback state.

Pivot presentation remains hold-during-Pivot followed by a smooth recenter.
The recenter target now uses that same nearest-equivalent selection relative to
the currently displayed yaw. It does not impose a global yaw clamp, so repeated
Pivot can continue to rotate the presented plane around the arena.

The DX11 crossover reproduction ran once with P1 crossing and once with P2
crossing. Before each crossing the presented/raw yaw was -90 degrees; after
the pair axis reversed, the old formula requested +90 degrees (180 degrees
away), while the corrected selection stayed at -90 degrees (0-degree change).
The logged post-crossing X coordinates show the fighters had exchanged order:

- P1 crossing: P1 X `122880`, P2 X `61440`; log
  `Saved/Logs/5V-CameraCross-P1-20261018-180336-155.log`; capture
  `Saved/Logs/5V-CameraCross-P1.png`.
- P2 crossing: P1 X `-61440`, P2 X `-122880`; log
  `Saved/Logs/5V-CameraCross-P2-20261018-180359-514.log`; capture
  `Saved/Logs/5V-CameraCross-P2.png`.

Both logs record `rawBefore=-90`, `rawAfter=90`, `oldDesiredDelta=180`,
`chosenAfter=-90`, and `chosenDelta=0`. The captures were taken in the actual
DX11 game window with deterministic simulated jump inputs; the source-only
reproduction harness was removed afterward. These runs verify the crossover
camera path, but do not replace manual review of natural movement, lighting,
or repeated Pivot input.

### Idle, walking, and attack presentation

The imported Muay Thai idle was rejected in human playtesting and is no longer
selected. Neutral Idle uses the stock mannequin `MM_Idle`; the Muay Thai
source/import remains available but unused. The obsolete camera-facing chest
compensation, midpoint torso orientation, and P1-only skeletal reflection
remain removed. The presentation rule is that each fighter faces the opponent.

Backward walking previously used actual velocity projected onto the
opponent-facing axis to identify direction, but then replaced the backward
walk action with the high-Block pose whenever Away was held. The walking
selection now uses that projected velocity alone: positive toward-opponent
motion chooses forward locomotion and negative motion chooses backward
locomotion for either player. This also works after crossover and Pivot, where
raw screen-key direction can differ from the current opponent direction.

The preceding Medium repair is preserved: the detached artificial low-strike
thigh/shin/foot components are gone, Medium uses the intact generic stock
`MM_Attack_02` sequence, and Medium remains authoritatively Low. No Medium
gameplay data changed.

### Automated checks and validation

`ProjW.Presentation.CameraSideContinuity` tests pair-axis reversal, crossing
continuity, a small combat-plane turn, and shortest-angle wraparound near
±180 degrees. No camera state was added to the simulation to support testing.

`ProjWEditor Win64 Development` built successfully (UBT exit 0;
`Saved/Logs/Build5V-FinalCameraContinuity.log`). The canonical
`powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1` completed
with 96 discovered, 96 passed, 0 warnings, 0 failures, and 0 skipped/not-run.
Unreal process exit code and runner script exit code were both 0. Report:
`Saved/Automation/CodexValidation-20261018-180552`; log:
`Saved/Logs/CodexValidation-20261018-180552.log`.

Human DX11 checks remain open for physically controlled forward/back walking
by both players before and after crossing, walk-back immediately after
crossover, Pivot hold/recenter and several same-direction Pivots, and visual
review of the lighting and attacks. Milestone 5V is not marked human-accepted.

## Human acceptance and closeout (2026-10-19)

The human reviewed the current 5G/5V state and reported “everything is in
place.” Milestone 5V's current presentation is accepted for Combat Lab use:
the mannequin presentation, opponent-facing orientation, current idle/walk
and attack visuals, Heavy presentation, screen-side crossover behavior,
Pivot, camera hold/recenter, circular arena, backdrop/lighting, and controls.
This accepts the current placeholder quality for the prototype; it does not
claim production animation or final presentation polish. Previously rejected
orientation and detached-limb experiments remain removed.

Closeout verification on 2026-10-19 used the accepted 5G/5V source: the editor
build succeeded (UBT exit 0), and the canonical complete suite discovered 98
tests with 98 passed, 0 warnings, 0 failed, and 0 skipped/not-run. Unreal
process and runner script exits were 0. Report:
`Saved/Automation/CodexValidation-20261019-113708`; log:
`Saved/Logs/CodexValidation-20261019-113708.log`.
