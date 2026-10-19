# Provisional roadmap

This sequence is a planning aid, not a fixed delivery contract. Later stages may move as dependencies and playtest findings become clearer.

| Milestone | Scope | Status |
| --- | --- | --- |
| 5A | Generic Move Foundation | Done |
| 5B | Three Normal Attacks | Done |
| 5C | Combat Lab Foundation and design contract | Done |
| 5D | Combat Grammar Specification | Done |
| 5E | Blocking Prototype | Done; implemented prototype |
| 5F | Deterministic 3D Combat + Orbital Pivot Prototype | Done; Revision B accepted, circular arena footprint validated |
| 5G | High/Low Parry and Broad Geometry Prototype | Done; human-accepted Combat Lab prototype |
| 5V | Combat Lab Visualisation / Placeholder Presentation | Done; human-accepted Combat Lab presentation |
| 5H | Combined Combat Lab attack-geometry exploration | Done; human-accepted gameplay/presentation, procedural crouch remains a known temporary weakness |
| 5I | Combat Grammar Tuning / Integration Decision | Provisional |

Subsequent work, in provisional order: command history and motion inputs; cancels; combo structure; Pulse-style assisted combos; specials; meter and resources; tag and assist systems; rollback architecture; local two-player play; networking and rollback; character presentation and animation pipeline; combat AI and a sparring opponent; vertical slice; content and tooling. This ordering may change. Minimal readable greybox animation and presentation may need to move earlier to evaluate Pivot and Parry fairly.

Combat AI should wait until the chosen defence and basic offensive structure can be exercised and a human can visually understand the opponent's actions. It need not wait for final models. See `CombatDesign.md` for the design constraints.
