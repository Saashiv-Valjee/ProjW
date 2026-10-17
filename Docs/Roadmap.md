# Provisional roadmap

This sequence is a planning aid, not a fixed delivery contract. Later stages may move as dependencies and playtest findings become clearer.

| Milestone | Scope | Status |
| --- | --- | --- |
| 5A | Generic Move Foundation | Done |
| 5B | Three Normal Attacks | Done |
| 5C | Combat Lab Foundation and design contract | Current |
| 5D | Defence Prototype A: conventional | Planned experiment |
| 5E | Defence Prototype B: directional/active | Planned experiment |
| 5F | Defence Prototype C: hybrid/layered | Planned experiment |
| 5G | Prototype comparison and selection | Provisional |
| 5H | Chosen defence integration | Provisional |

Subsequent work, in provisional order: command history and motion inputs; cancels; combo structure; specials; meter; tag and assist systems; rollback architecture; local two-player play; networking and rollback; character presentation and animation pipeline; combat AI and a sparring opponent; vertical slice; content and tooling. This ordering may change. In particular, readable greybox character animation should precede judging an AI sparring opponent, even if production character presentation comes later.

Combat AI should wait until the chosen defence and basic offensive structure can be exercised and a human can visually understand the opponent's actions. It need not wait for final models. See `CombatDesign.md` for the design constraints.
