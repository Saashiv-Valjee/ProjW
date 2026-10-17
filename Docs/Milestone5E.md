# Milestone 5E: Blocking Prototype

Milestone 5E implements the first defensive family from the 5D combat grammar as a **Combat Lab prototype**. It gives grounded defenders a reliable, concessionary response to existing normals. Its values and High/Low assignments are tuning hypotheses, not accepted production balance. Light, Medium, and Heavy startup, active, recovery, full-hit damage, hitstop, hitstun, and hit knockback remain unchanged.

## Exact prototype rules

There is no Block button. Hold facing-relative Away while grounded for standing guard; hold Down + Away for crouching guard. Only grounded Idle, Walking, and Crouching fighters may guard. An attack, dash, jump startup, airborne state, Hitstun, or Blockstun cannot guard or buffer a guard. The simulation reads authoritative facing and the current frame's held input. Standing guard blocks only prototype High; crouching guard blocks only prototype Low. Wrong-height guard resolves to a normal Hit.

| Move | Prototype guard height | Chip | Blockstun | Defender block pushback |
| --- | --- | ---: | ---: | ---: |
| Light | High | 5 | 10 frames | 3 × 1024 subunits/frame |
| Medium | Low | 8 | 13 frames | 4 × 1024 subunits/frame |
| Heavy | High | 12 | 16 frames | 5 × 1024 subunits/frame |

The move's existing HitstopFrames applies on Block. Chip clamps health at 1 and cannot KO; an actual Hit retains its existing zero-health behavior. Only the defender gets block pushback. The signed integer velocity is captured at contact from relative fighter positions, survives hitstop, and stops without bouncing at stage bounds. There is no attacker block recoil.

## Contact and reaction lifecycle

The 5C transient contact classifier still checks grounded strict hitbox/hurtbox overlap first. It now returns `None`, `Hit`, or `Block`; a valid overlap becomes `Block` only when the defender is eligible, holds Away, and chooses the matching height. No overlap, strict edge touch, or an airborne-invalid target does not consume an attack. A Hit or Block sets the same one-contact-per-move latch; later active frames cannot apply repeated damage or chip.

Both contact directions are classified before either consequence. Successful contacts use the maximum incoming move hitstop as a shared freeze. Each defender independently receives full damage or nonlethal chip and a captured `Hit` or `Block` pending reaction. `EPendingReaction` and `PendingReactionFrames` replace the former pending-hit boolean and hitstun-only duration; `None`, `Hit`, and `Block` are the only values. Hitstop pauses state progression, then the first Hitstun or Blockstun step is `StateFrame` 0. The reaction lasts exactly its stored frame count, disallows new actions, and returns to an ordinary grounded state. Match state remains trivially copyable and exactly replayable. Contact results and definition pointers stay transient.

The current single action state means simultaneous two-way contact requires both fighters to be attacking. Attackers cannot guard, so reachable simultaneous contacts are Hit/Hit. Mixed Hit/Block and Block/Block trades are **unreachable under 5E's explicit attack lock**; no exception was added to fabricate them. Mirrored one-way Block cases and reachable unlike-move Hit/Hit trades test assignment symmetry. If later mechanics permit attacking and guarding concurrently, they must explicitly define and test the mixed cases then. Initiative is not authoritative state: attack recovery, blockstun, and spacing produce it.

## Tests and validation

`BlockingPrototypeTests.cpp` adds seven focused tests: `Definitions`, `GuardDirectionHeight`, `Lifecycle`, `ChipWallPushback`, `ContactConsumption`, `CommittedDefender`, and `ReachableTradeSymmetry`. They cover both facings, all three High/Low assignments, wrong-height full Hits, exact chip and nonlethal clamp, hitstop and Blockstun frames, no actions during Blockstun, captured directional pushback, wall clamp/no bounce, consumed and invalid contacts, committed-state rejection, unlike Hit/Hit trade, and snapshots during pending Block hitstop and Blockstun. Existing tests cover unchanged non-guarded movement and hit behavior.

`ProjWEditor Win64 Development` built successfully on the final source. The canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1` run discovered 72 tests: 72 passed, zero warnings, zero failed, and zero skipped/not run. Unreal process and runner exited 0. Report: `Saved/Automation/CodexValidation-20261017-195757`; log: `Saved/Logs/CodexValidation-20261017-195757.log`. Known restricted-environment Unreal startup messages are separate from project test results.

## Human Combat Lab check

The manual question is whether safe defence feels acceptable while still motivating a stronger defensive read, **not whether balance is already accepted**. Check whether standing/crouching guard is understandable, chip makes passive defence costly without making it pointless, Blockstun communicates attacker initiative, the next decision is readable, pushback relieves point-blank pressure, Block feels safer than guessing, and the interaction remains calm while leaving conceptual room for Pivot and Parry.

Launch with `-d3d11`. The current debug controller drives P1 only and P2 remains neutral, so a human can inspect P1's HUD guard interpretation, standing/crouching input transitions, ordinary unguarded attacks, and unchanged movement/attack display. Incoming Block, chip, Blockstun, pushback, wrong-height Hits, and contact consumption are automation-verified but **cannot yet be directly exercised against an attacking P2 in the current manual harness**. A later local-two-player or minimal opponent-driver milestone can expose those checks interactively; 5E does not add a multiplayer input system.

## Scope and files

Changed: `FightingSimulationTypes.h`, `FightingSimulation.h`, `FightingSimulation.cpp`, `DebugFightingHUD.cpp`, existing tests mechanically updated for the renamed pending-reaction fields, `BlockingPrototypeTests.cpp`, `AGENTS.md`, `CombatDesign.md`, `Roadmap.md`, and this document. No Pivot/Dodge, depth movement, 3D combat geometry, Parry, AoE, throws, guard meter/break, cancels, combos, Pulse Combo, specials, meter, tag/assist gameplay, AI, or production animation is implemented.
