# Milestone 4A: deterministic light attack and combat geometry

Light uses Numpad 7; Numpad 8 and Numpad 9 are collected as Medium and Heavy but remain inert. Inputs are still compact held masks, with Pressed edges derived per simulation frame.

The lifecycle is exactly five startup calls (StateFrame 0..4), three active calls (0..2), and nine recovery calls (0..8). The first Light press enters startup at frame 0; the next call after startup enters active frame 0. The attack returns to idle, walking, or crouching on the call after recovery frame 8. Held Light is never buffered or retriggered.

Authoritative boxes use integer subunits (1024 per world unit) and positive-area overlap: touching an edge is not overlap. Each hurtbox is centered at PositionX, starts at PositionY, has half-width 24 units and height 100 units. The Light hitbox is active only in LightAttackActive, has half-width 28 units, height 50 units, center offset 44 units forward and 55 units above PositionY, and mirrors from authoritative Facing.

Each fighter owns `bLightAttackHit`; `FMatchState::HitEventCount` increments once per successful attack/opponent pair. Collision is resolved symmetrically after both fighters have advanced, so simultaneous attacks can both register. No damage or reactions are applied.

Debug boxes are presentation-only DrawDebugBox output from simulation geometry. No Unreal collision, physics, animation, or sockets participate. State and counters are copied with snapshots.

Focused coverage now includes:

- `ProjW.Simulation.Attack.LightLifecycle`
- `ProjW.Simulation.Attack.Geometry`
- `ProjW.Simulation.Attack.InputRules`
- `ProjW.Simulation.Attack.TimingMirroring`
- `ProjW.Simulation.Attack.HitCases`
- `ProjW.Simulation.Attack.SnapshotPhases`
- `ProjW.Simulation.Attack.MixedRepeatability`

These cover lifecycle and StateFrame timing, held-versus-pressed behavior, actionable and non-actionable states, attack movement lock, active-only hitboxes, both facing directions, strict overlap semantics, one-hit limits, late hits, misses, simultaneous hits, snapshots during startup/active-before-hit/active-after-hit/recovery, and long deterministic mixed input sequences.

The manually run initial result was 32 total tests: 29 passed and 3 failed. Diagnosis found no gameplay implementation defect. `Attack.HitCases` started the two trade attacks on different simulation frames, leaving Fighter 2 in startup when Fighter 1 became active; the test now starts both inputs in the same frame. `Attack.LightLifecycle` counted the call entering recovery as one of the nine recovery frames; it now performs the required following exit call. `MovementInvariants` used `0x3f0`, which includes the newly actionable Light bit; the legacy inert-input comparison now uses `0x3e0` (Medium through Assist2) while preserving its movement assertions.

The Editor target rebuilt successfully after these test-only corrections. A new command-line automation result was not obtained: the prior UE 5.8 Zen startup failure remains an environment issue, repeatedly launching `zenserver.exe` with an empty `--data-dir` and failing on port 8558. No final pass/fail/skip count is claimed here beyond the manually observed 32/29/3 result. No gameplay behavior was changed.

Manual checks should cover Numpad focus, startup/active/recovery timing, mirrored attacks, misses, and simultaneous attacks. Damage, hitstun, hitstop, knockback, blocking, animation, networking, and rollback remain future work.
