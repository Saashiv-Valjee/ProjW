# Milestone 5D: Combat Grammar Specification

Milestone 5D is specification-first. The accepted 5C Combat Lab has only a baseline `None`/`Hit` classifier; it deliberately deferred blocking and did not select a defensive model. Since then, the intended combat language has become clearer. This milestone records that direction before prototype mechanics can bias the architecture. All accepted gameplay remains unchanged.

## Direction and grammar

The target is a modern tactical 2.5D fighter: conceptually a 3D arena, with ordinary combat organized around a readable 2D-style axis. Depth should enter through deliberate spatial actions, especially a future Pivot, rather than unrestricted free roaming. The game aims for visual stability, low simultaneous-event density, meaningful branches, accessible basic execution, and advanced expression. Commitments and their outcomes should be readable, with broad initiative understandable without hidden turn ambiguity.

Three **distinct proposed response families** now guide future Combat Lab work:

- **Block:** reliable standing/crouching survival, potentially with chip and retained attacker initiative. Exact guard rules, damage, and advantage remain open.
- **Pivot/Dodge:** deliberate movement off a narrow committed attack line through depth. Success should preferably arise from deterministic geometry; broad areas may still cover the escape route. Controls and movement rules remain open.
- **High/Low Parry:** active, height-specific response particularly relevant to broad AoE threats. Choosing High for a parryable Low attack, or Low for High, should not count as the correct read. Timing, reward, and controls remain open.

These are not increasingly powerful versions of one action or a universal Block/Pivot/Parry counter chart. Height or guard relationship and spatial trajectory are different design questions. The [combat design document](CombatDesign.md) includes an illustrative decision tree and six prototype attack archetypes: straight normal, low sweep, axe kick, straight thrust, ground stomp/rupture, and high AoE/burst. It records what each should communicate, possible basic and advanced responses, the question to test, and unresolved details. No production move values are assigned.

## Bounded optimization and accessibility

Reasonable competing choices and styles should remain viable while mastery earns an edge. A rough advantage on the order of **~20% rather than multiples** is a balancing philosophy, not a gameplay constant, universal metric, or guarantee; correct punishes and hard counters may differ far more. A future Pulse-style assisted combo path should make confirmed hits useful for players with modest execution ability without making manual routing meaningless. Its controls, routes, scaling, and rewards remain undecided.

## Evaluation and roadmap

The Combat Lab comparison criteria are readability, information fairness, initiative legibility, decision depth, option diversity, bounded optimization, input burden, spatial clarity, offensive and defensive agency, spectator clarity, dominant-option risk, accessibility, and expert expression. These are questions for controlled prototypes and human playtests, not automated acceptance values today.

The provisional [roadmap](Roadmap.md) now places a Blocking Prototype at 5E, 3D Pivot/Linear Attack Prototype at 5F, High/Low Parry + AoE Prototype at 5G, a combined lab at 5H, and tuning/integration decision at 5I. Later command inputs, cancels, combo structure, assisted combos, specials, resources, tag/assists, rollback, two-player/networking, presentation, AI, and content remain reorderable. Readable greybox presentation may need to move earlier to evaluate Pivot/Parry; production models are not a prerequisite for a later sparring AI.

## Scope and files

Only [AGENTS.md](../AGENTS.md), [CombatDesign.md](CombatDesign.md), [Roadmap.md](Roadmap.md), and this milestone document change for 5D. No gameplay code, tests, move definition, authoritative state, or presentation code changes. Block, Pivot, Parry, depth movement, AoE, chip, initiative state, Pulse combo, AI, and camera behavior remain unimplemented. The existing 65-test suite remains the regression gate.

## Validation

`ProjWEditor Win64 Development` built successfully on the current source (`Build.bat` exit 0; target up to date). The canonical `Scripts/RunTests.ps1` complete-suite run discovered 65 tests: 64 passed cleanly, one passed with warnings, zero failed, and zero skipped/not run. Unreal process and runner script both exited 0. The warned test was `ProjW.Simulation.Normal.InputSelection`; its 11 warnings were EOS SDK HTTP connection/configuration failures in the restricted environment, not failed project assertions. Report: `Saved/Automation/CodexValidation-20261017-194402`; log: `Saved/Logs/CodexValidation-20261017-194402.log`.

Human design review remains necessary for the grammar, prototype priorities, and open mechanics.
