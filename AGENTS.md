# Engineering rules

These rules apply throughout this repository. The project is a high-quality Unreal Engine 5.8 prototype for a stylized 2.5D tag fighting game, with gameplay direction inspired by modern fighting games such as 2XKO and Street Fighter.

## Core architecture

Unreal Engine is primarily responsible for rendering, skeletal animation, audio, VFX, camera, UI, and asset/content management. Our own C++ simulation layer is authoritative over fighting-game gameplay and must be designed from the beginning for eventual rollback netcode.

1. Authoritative gameplay advances at exactly **60 simulation frames per second**.
2. Gameplay logic must never depend on rendering frame rate or variable `DeltaTime`.
3. Keep simulation and presentation separated.
4. Authoritative gameplay state must be explicit and designed to become snapshot-able and restorable. All state affecting future simulation results must be accounted for by the rollback state model.
5. Given identical initial state and identical inputs, simulation must produce identical results.
6. Avoid nondeterministic floating-point behaviour in authoritative simulation where practical. Prefer integer or fixed-point representations for rollback-sensitive movement and state.
7. Unreal physics must **not** be authoritative for fighter movement, hit detection, hitboxes/hurtboxes, knockback, or other rollback-sensitive gameplay.
8. Do not use `CharacterMovementComponent` as the authoritative fighter simulation.
9. Represent inputs per simulation frame in a form suitable for eventual network transmission.
10. Moves must be data-driven rather than individually hard-coded into fighter controllers.
11. Hitboxes and hurtboxes are simulation constructs rather than Unreal physics collision.
12. Rendering and animation consume simulation state; they do not determine simulation state.
13. Core gameplay systems must be testable without requiring presentation or rendering.
14. C++ owns deterministic gameplay systems. Blueprint is appropriate for presentation, assets, animation configuration, VFX, UI, and content-facing work.
15. Prefer small modular systems over giant Actor classes.
16. Explain major architectural changes before implementing them.
17. Deterministic simulation behaviour must have automated tests.
18. The long-term rollback architecture must permit this sequence:

    ```text
    save state
        -> predict remote input
        -> simulate
        -> receive actual input
        -> restore previous state if prediction was wrong
        -> resimulate
    ```

## Long-term game requirements

The eventual game should support:

- 2.5D fighting along a local fighter-to-fighter combat axis.
- 3D skeletal characters.
- Modern fighting-game movement.
- Normals and special moves.
- Combos and cancels.
- Blocking.
- Meter.
- Tag teams.
- Assists.
- 2-player teams / 2v2.
- Rollback networking.
- Training mode.
- Stylized high-quality presentation.

These are long-term requirements, not instructions to implement these features now. Implement only the scope of the current task while preserving these architectural requirements.

## Combat design contract

Preserve the fixed-60-Hz deterministic, rollback-conscious simulation and read
the latest milestone documents before changing accepted combat behavior. Read
`Docs/CombatDesign.md` before substantial combat-system work.

The current target is a modern tactical 2.5D fighter: a conceptually 3D arena
with strong 2D combat readability, visually stable interactions, relatively few
simultaneous events, and deep, informed decisions. Favor legible commitments and
initiative over repeated blind guesses or hidden turn structures. Block,
spatial Pivot/Dodge, and height-specific High/Low Parry are distinct response
families, not stronger versions of one action. Block is now a Combat Lab
prototype: facing-relative standing/crouching guard, prototype High/Low move
assignments, chip, Blockstun, and block pushback. Its values and guard taxonomy
are not final balance. Combat simulation now has deterministic integer X/Y/Z
space. The fighter pair defines a derived local horizontal combat axis; ordinary
movement stays effectively one-dimensional along it. Pivot makes a small
orbital displacement around the opponent, without free 3D roaming or a
privileged world-Z-zero plane. The Combat Lab arena is an authoritative circular
disc in horizontal X/Z space, centered at the origin with a 480-world-unit
radius. Fighter centers use a deterministic 25-unit hurtbox-footprint margin
so the complete gameplay footprint stays within that floor boundary. Human
playtesting accepted the physical Pivot, camera hold/recenter, and rotating
local combat plane after this edge fix. The arena geometry permits diagonal
relationships but does not grant free 3D
locomotion or a privileged world-X combat orientation. Diagonal attacks must
follow their committed local axis with coherent deterministic geometry. Pivot
physically moves a fighter out of the currently presented 2D plane, so the
debug camera holds presentation yaw throughout either fighter's Pivot and then
smoothly recenters to the new local combat axis. Repeated Pivots may rotate
that plane around the circular arena. Camera transition state is presentation
only and never moves authoritative fighters. Pivot has no intrinsic
invulnerability: avoidance comes from strict 3D hit/hurt non-overlap, not
arbitrary dodgeability flags. Camera and fighter yaw are presentation only.
Prototype widths and timing are not final. High/Low Parry is now a committed,
height-specific active defense with punishable recovery; GroundRupture is a
low radial-geometry experiment, not a semantic AoE category. These 5G Combat
Lab values are not final balance. Preserve rollback-safe state, deterministic
3D overlap, independent guard/parry classifications, the
classify-both-contacts-then-resolve path, and captured pending reactions.

Preserve meaningful alternative styles through bounded optimization. The rough
20% comparison for reasonable choices is a balancing aim, never a simulation
constant or guarantee. A future Pulse-style assisted combo path should let
players with modest execution ability participate in tactical play while manual
routing retains value. Keep future tag and assist actions strategic and readable.
Avoid speculative authoritative fields or premature mechanics. Existing accepted
milestone behavior remains authoritative until explicitly changed.

Make turn ownership and available opportunities legible even when defensive
outcomes are uncertain. The accepted Combat Lab principles are Block as reliable
concession, Pivot as a geometric/spatial response, Parry as a committed
height-specific response, and Pivotability emerging from geometry rather than
semantic dodge permissions. These are prototype principles, not final
production balance or rules.

## Temporary development launch workaround

On the current development machine, the project must currently be launched with **`-d3d11`**. UE 5.8's DX12 ray-tracing initialization is causing `DXGI_ERROR_DEVICE_REMOVED` / `DXGI_ERROR_DRIVER_INTERNAL_ERROR` on this machine.

Example (substitute the installed editor path):

```powershell
& '<UE_5.8 installation>\Engine\Binaries\Win64\UnrealEditor.exe' 'E:\ProjW\ProjW.uproject' -d3d11
```

This is a temporary, machine-specific development workaround. It must not become an architectural dependency or constrain the deterministic gameplay simulation. The launch argument overrides the current configured DX12 default; this documentation does not change renderer configuration.

## Validation

Codex automation runs under the restricted `codexsandboxoffline` identity. Direct
`UnrealEditor-Cmd` automation can fail because the normal user Zen/DDC and shader
working directories are not writable by that identity. Do not use the old raw
Unreal automation command. After simulation or gameplay changes, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1
```

Do not report a milestone as test-passing unless this complete suite executes
successfully. Do not work around failures by weakening tests. Human/manual
gameplay validation remains separate from automated validation.
