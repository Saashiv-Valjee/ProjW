# Milestone 2: playable debug harness

This records the original Milestone 2 harness. Current movement/input semantics
are documented in [Milestone3A.md](Milestone3A.md); Milestone 3A adds grounded
crouch, dashes, stage/body constraints and press-only jumping to this harness.

Launch `E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe E:\ProjW\ProjW.uproject -d3d11`.
The project opens the engine Entry map; press Play (Selected Viewport) and click the
viewport for keyboard focus. Shift+F1 releases the mouse; Escape ends PIE. Use one
local player / standalone net mode. No assets, Blueprint setup or Actor placement
are needed. If an older map remains open, open `/Engine/Maps/Entry`; maps with a
World Settings game mode override must use `DebugFightingGameMode` for this harness.

## Controls

| Key | Frame input |
| --- | --- |
| A / D | Left / Right (together cancel) |
| W | Up / Jump (holding repeats after landing, as in Milestone 1) |
| S | Down |
| J / K / L | Light / Medium / Heavy |
| U | Special |
| I / O | Assist1 / Assist2 |

Down and all action buttons are recorded but have no gameplay effect. Player 1 is
blue, player 2 is orange and always receives neutral input. White protrusions point
forward. Fighters can cross freely; there is no body collision or stage boundary.

## Ownership and coordinates

`ADebugFightingGameMode` selects the controller/HUD, disables the default pawn and
spawns one `ADebugMatchDriver`. The driver owns `FMatchState`. Each render tick adds
elapsed time to `FFixedSimulationClock`, which returns the number of fixed 1/60 s
steps due. For each step the controller builds a fresh `FFighterInput` from held
keys and the driver calls `FFightingSimulation::SimulateFrame`. No elapsed time
enters the simulation. The existing simulation and its tests are unchanged.

The clock limits catch-up to eight steps per render tick, discarding excessive
elapsed time after hitches. It never skips simulation frame numbers. Consequently
extreme hitches slow local simulation relative to wall time; ordinary 30/60/144/240
FPS scheduling produces the same 60 simulation steps per second. The HUD shows
the number of catch-up clamps. This is a local debug policy, not a network clock.

The controller reads Unreal's latest processed held-key state once per simulation
step. Multiple catch-up steps can see the same key state. Very brief taps between
render input polls can be missed; timestamped input buffering is future work.
Frame-rate independence assumes identical per-simulation-frame inputs.

Visual mapping (one simulation world unit = one Unreal cm):

```text
Unreal X = PositionX / 1024.0
Unreal Y = 0
Unreal Z = PositionY / 1024.0
```

Fighter Actor origins are at the feet. Bodies extend from Z=0 to Z=100 relative
to their feet; the floor top is world Z=0. Visual yaw is 0 degrees for Right and
180 for Left. Both Actors are updated from the final state after each render
tick, without interpolation. No Actor transform, physics or collision feeds back
into the simulation. Actor collisions and overlap generation are disabled.

The fixed orthographic camera is at (0,1300,160), looking along -Y, with width
1200 cm and a constrained 16:9 aspect ratio. It frames the starting fighters and
the full jump arc. The stage is a plain engine cube with a directional light.
Movement is deliberately unbounded, so prolonged walking can leave the camera
and eventually the visual floor. Restart PIE to reset; there are no gameplay walls.

The Canvas HUD displays frame, raw integer position/velocity, grounded state,
fighter state, state frame, facing, last simulated input mask, render FPS and
catch-up clamp count. It is development UI only.

## Validation commands

```powershell
& 'E:\UE_5.8\Engine\Build\BatchFiles\Build.bat' ProjWEditor Win64 Development '-Project=E:\ProjW\ProjW.uproject' -WaitMutex -NoHotReloadFromIDE
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\ProjW\ProjW.uproject' -d3d11 -unattended -nop4 -nosound '-ExecCmds=Automation RunTests ProjW.' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=E:\ProjW\Saved\Automation\Milestone2' '-abslog=E:\ProjW\Saved\Logs\Milestone2Tests.log'
```

Close the editor before rebuilding if it locks the project DLL. Reports and logs
are generated under ignored `Saved/`. Clock tests cover render rates, fractional
accumulation, hitches and invalid elapsed values. They do not prove keyboard focus
or visual quality. Manually verify walk, opposing directions, repeated jump,
air movement, landing and facing on crossing. Try `t.MaxFPS 30`, `t.MaxFPS 60`
and `t.MaxFPS 144` in the console (`t.MaxFPS 0` removes the cap).

DX11 remains a machine-specific launch workaround; renderer settings are unchanged.

## Recorded validation

- `ProjWEditor Win64 Development`: build succeeded with UE 5.8.3 / MSVC 14.44.
- All five tests passed: `ProjW.Simulation.MovementInvariants`,
  `ProjW.Simulation.Repeatability`, `ProjW.Simulation.SnapshotReplay`,
  `ProjW.DebugHarness.RenderRateIndependence`, and
  `ProjW.DebugHarness.HitchProtection`. Test process exited with status 0.
- An offscreen DX11 `-game` run of 300 benchmark render frames exited with status
  0. Its captured image was inspected: blue P1 starts left, orange P2 starts right,
  both stand on the floor with markers facing inward, and the debug HUD is visible.
  Capture: `Saved/Screenshots/WindowsEditor/HighresScreenshot00002.png`.
- Interactive PIE keyboard focus, manual key presses, and control feel were not
  verified automatically. No cross-platform determinism claim is made.

Recommended next milestone: frame-indexed input recording/playback and input
buffering, using this harness to reproduce manual movement sessions. No subsequent
milestone is implemented here.
