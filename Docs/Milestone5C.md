# Milestone 5C: Combat Lab Foundation

Milestone 5C makes the existing contact path ready for later defensive experiments while preserving the accepted 5B gameplay. The earlier plan to implement conventional High/Mid/Low blocking in 5C was cancelled so conventional, directional/active, and hybrid defence can be prototyped and compared before integration. This milestone selects no defensive model.

## Contact seam and baseline

The simulation still steps both fighters before resolving contact. `HasValidGroundedOverlap` checks the existing one-contact flag, grounded target rule, active move hitbox, and strict integer AABB overlap. `ClassifyBaselineContact` converts a valid geometric contact to the only semantic outcome currently supported: `Hit`; all other cases yield `None`. Both results are collected before either consequence is applied. The transient result carries the immutable source move definition so Light, Medium, and Heavy continue to supply their own damage, hitstop, hitstun, and knockback. Unlike trades continue to use shared maximum hitstop. There is no event framework or runtime ruleset selector. Future prototypes can introduce a deterministic selection mechanism when their actual input needs are known.

## State audit

The move definitions remain immutable configuration. `EContactOutcome`, `FContactResult`, geometric overlap, and the definition pointer inside each result are local derived values for one frame; they are never stored in `FMatchState`. The HUD's `Combat Rules: Baseline` label is presentation only. **No authoritative gameplay field was added.** Match state remains trivially copyable with complete value equality. The authoritative path has no dynamic allocation, UObject/Blueprint rule lookup, mutable global experiment state, or variable `DeltaTime`; copy/restore and replay remain exact.

## Files and tests

- `FightingSimulation.cpp`: local geometric-contact and baseline-classification seam, preserving pair resolution and consequences.
- `CombatLabBaselineTests.cpp`: `BaselineClassification` covers no overlap, airborne invalid target, strict edge touch, already-connected move, and valid Light/Medium/Heavy contact with the correct source values. `BaselineTrade` covers both contacts before consequences, unlike-move damage/reactions, shared hitstop, and snapshot/replay.
- `DebugFightingHUD.cpp`: identifies the current rules as Baseline without adding speculative defensive display.
- `AGENTS.md`: concise combat design contract and latest-milestone reading rule.
- `CombatDesign.md`: accepted systems, design goals, exploratory defensive families, undecided mechanics, and future AI timing.
- `Roadmap.md`: provisional defence-prototype sequence and later systems.

There is no blocking, parry, deflect, attack-level property, guard input, tag change, combo rule, combat AI, or other new combat behavior in 5C.

## Validation

Final verification on 2026-10-17:

- `ProjWEditor Win64 Development`: succeeded.
- Canonical `powershell -ExecutionPolicy Bypass -File .\Scripts\RunTests.ps1`: 65 discovered, 65 passed, 0 project test warnings, 0 failed, and 0 skipped/not-run.
- Unreal process exit code: 0. Runner script exit code: 0.
- Restricted-environment Unreal Trace, user-local Zen/security and analytics, and offline EOS startup messages were present in the engine log. They did not prevent automation or become project test warnings.

Human DX11 validation remains: movement, Numpad 7/8/9 attacks, debug hitboxes, damage/hitstop/hitstun/knockback, and the HUD should still behave as before. There should be no defensive mechanic yet.
