# Combat design direction

This document records the current combat identity and questions for later prototypes. Labels such as **Implemented**, **Design direction**, **Prototype hypothesis**, and **Undecided** mark their status. It is not a source of new simulation rules.

## Currently accepted implementation

**Implemented.** The prototype has a deterministic fixed-60-Hz integer simulation with copyable/restorable match state; grounded walking, dashes, jumps and double jump; Light, Medium, and Heavy grounded normals; startup/active/recovery; simulation 3D hit/hurt volumes; damage, hitstop, hitstun, knockback, and simultaneous trades. Valid grounded 3D overlap is classified transiently as `Hit` or `Block` after guard evaluation, and both directions are classified before consequences. Unlike successful contacts share maximum hitstop. The 5E Combat Lab prototype adds standing/crouching guard, chip, Blockstun, and defender block pushback. The accepted 5F Combat Lab prototype adds an authoritative circular disc arena in horizontal X/Z space, a hurtbox-safe fighter-center margin, committed two-direction orbital Pivot without invulnerability, and a derived local combat axis. The user accepted the physical Pivot, camera hold/recenter, rotating combat plane, and circular arena after the edge correction. Presentation consumes simulation state. High/Low Parry and the GroundRupture geometry experiment are planned for 5G; neither implies a production AoE category, free depth locomotion, combo system, or combat AI.

## Combat identity

**Design direction.** This is a modern tactical 2.5D fighter. The arena is conceptually three-dimensional, but most ordinary interaction should read along a focused combat axis, retaining the immediacy and accessibility of a modern 2D fighter. It is not a commitment to unrestricted free-roaming 3D movement or to copying any particular fighter.

Combat should be visually stable and readable, with comparatively low simultaneous-event density and high decision density. Avoid unexplained displacement, excessive side switching, constant overlapping assists or effects, jittery silhouettes, routine very long noninteractive combos, and execution barriers that keep less technical players out of the tactical game. Players should broadly understand an opponent's commitment, available response families, initiative, and why an exchange resolved as it did. The central uncertainty should be what the opponent chooses, not whether one is secretly allowed to act.

## Bounded optimization and freedom of style

**Design direction.** Reward mastery without making one reasonable style overwhelmingly superior in ordinary competitive play. Among strategically legitimate competing choices, the better choice should usually give an incremental edge. A rough comparison on the order of **~20% rather than multiples of effectiveness** is a balancing target, not an exact bound, gameplay constant, runtime invariant, or universal effectiveness formula.

Correct punishes, telegraphed hard counters, major resource expenditure, strong conditioning, and plainly unreasonable responses can produce much larger differences. Do not flatten meaningful counters or make bad decisions artificially competitive. Future rules and balance tools should avoid baking one playstyle into the engine as intrinsically superior; manual mastery should improve expected outcomes while leaving legitimate alternative styles viable.

## Combat decision philosophy

**Design direction.** Movement leads to commitment, a readable response window, a defensive choice, a clear result, then changed initiative, geometry, or resources and the next decision. A meaningful offensive commitment should create an interesting defensive decision, and its resolution should create the next decision. Depth should come from spacing, timing, reads, adaptation, conditioning, and meaningful branches rather than a sequence of arbitrary rapid guesses.

Future attacks may differ in trajectory, coverage, tracking, guard relationship, commitment, recovery, positional effect, response vulnerability, and resource interaction as well as frame timing and damage. These are possible design dimensions, not approved production `FMoveDefinition` fields. Tag and assist actions should be readable strategic commitments rather than continuous screen noise; a roughly four-second cooldown or shared team resource is only an exploratory reference. Ordinary combos should often return players to interactive decisions, while longer optimized or resource-heavy routes may still exist.

## 3D arena and 2D readability

**Design direction.** Ordinary neutral stays organized around the fighter-to-fighter combat axis. **Implemented prototype.** The simulation has integer X/Y/Z positions and an authoritative circular disc of radius 480 world units centered at horizontal X/Z origin. Fighter centers are constrained to radius 455 units so the entire deterministic 24-by-5-unit hurtbox footprint remains inside the unchanged floor disc. The circle has no privileged combat orientation. The fighter pair derives a transient local horizontal axis; walking, dash, jump momentum, attacks, body push, and contact reactions use it. Pivot makes a small bounded orbital movement around the opponent. During either fighter's full 12-frame Pivot, the debug camera holds the prior presentation yaw so the physical off-plane motion stays visible. After the last Pivot exits, the camera smoothly recenters to the new local axis over about 10 presentation frames. This view-only transition never changes fighter coordinates. Repeated Pivots can progressively rotate the established local plane around the arena. There are no permanent parallel depth lanes or privileged world-Z-zero plane. The camera remains predominantly side-on and never determines gameplay. Arena geometry permits spatial interactions while the normal control grammar remains effectively 2D along the local axis; the disc does not authorize unrestricted free locomotion. **Undecided:** final arena size, camera framing, and attack tracking.

## Defensive grammar

**Design direction.** Block, Pivot/Dodge, and Parry are distinct response families that answer different problems. They are not three power tiers of a single action. Block and Pivot have accepted Combat Lab implementations; High/Low Parry is planned for the 5G prototype and is not final balance.

### Block

**Implemented prototype.** Grounded Idle, Walking, or Crouching fighters may guard by holding facing-relative Away; Down + Away selects crouching guard. Standing guard stops prototype High, crouching guard stops prototype Low; the wrong height takes a full Hit. Light and Heavy are prototype High, Medium prototype Low. A Block consumes one attack contact, applies small nonlethal chip (minimum 1 HP), shared move hitstop, then Blockstun and defender-only integer pushback. Attack recovery, Blockstun, and spacing determine initiative without an initiative state. **Design direction.** Block should remain reliable basic survival but concessionary; advanced defence should not be required merely to survive. **Undecided:** final guard taxonomy and move assignments, chip and KO rules, guard damage, blockstun, pushback, advantage, and exceptions. The 5E numbers are tuning values, not final balance.

### Pivot / Dodge

**Implemented prototype.** Pivot is a 12-frame committed clockwise/counter-clockwise orbit of about 8 degrees when bounds permit. It physically moves the fighter out of the currently presented 2D plane, with no intrinsic invulnerability, no mid-action steering or actions, and no automatic world recentering. The debug camera holds its previous combat orientation during the whole Pivot, then smoothly recenters to the new fighter-to-fighter axis after Pivot exits. A later Pivot holds the camera's current displayed yaw and cancels an unfinished recenter; presentation never gates simulation. The camera transition never changes authoritative positions. A successful Pivot is simply a 3D **non-overlap**, yielding transient contact `None`; there is no Dodge outcome or attack-category permission flag. Attacks commit an oriented local-axis volume; narrow Light/Heavy and broad Medium widths are temporary test profiles. Body push and Hit/Block reactions follow the local pair axis. **Design direction.** Pivotability should emerge primarily from deterministic attack/hurt geometry; future authored volumes may move, expand, sweep, or offset through depth. Block remains the reliable concessionary baseline, while Pivot and future Parry should both stay situational. Attacks should allow multiple reasonable responses instead of exactly one prescribed counter. **Undecided:** final controls, depth dimensions, timing, camera behavior, tracking, reward, and punish windows.

### High / Low Parry

**Design direction.** Parry is an active response especially worth testing against broad AoE-style threats that a narrow spatial escape cannot simply solve. High Parry and Low Parry must remain distinct choices: a High Parry is not the correct read for a parryable Low attack, and vice versa. A ground rupture might invite Low Parry; a high burst might invite High Parry. This is not a universal “AoE loses to Parry” rule. **Undecided:** controls, timing, startup, recovery, failure consequences, reward, advantage, and animation.

## Attack geometry and guard relationship

**Design direction.** Height or guard relationship and spatial geometry or trajectory ask different questions. Standing/crouching Block answers guard relationship; Pivot answers spatial trajectory; High/Low Parry answers the corresponding height-specific active read. A future attack might be High and narrow, Low and narrow, a vertically threatening linear commitment, High AoE, or Low AoE. These are conceptual examples, not a final taxonomy or new move fields. Prefer deterministic physical geometry where it makes spatial behavior clear, rather than reducing all exchanges to arbitrary labels.

## Initiative

**Design direction.** Attacker advantage, neutral, defender advantage, and punish are a lens for evaluating exchanges, not authoritative enum states. Basic defensive success might concede initiative; a stronger informed response might reset to neutral; exceptional defence might earn advantage; a committed mistake might create a clear punish. Future animation, spacing, reactions, sound, effects, and appropriate camera work should help players infer broad opportunity. Frame data can exist for experts without making obscure frame-table knowledge the price of basic understanding.

## Representative combat decision tree

**Illustrative hypothesis, not an automatic counter chart:**

```text
Attacker has initiative
  ordinary pressure -> correct Block -> reduced reward or chip;
                       attacker may retain initiative -> next decision
  committed linear attack -> read its trajectory -> directional Pivot
                           -> spatial whiff -> possible punish or position
  broad AoE commitment -> identify height -> correct High/Low Parry
                       -> possible interruption and defender initiative
```

Timing variation, spacing, delayed options, conditioning, character properties, imperfect information, risk, and positional consequences should keep these branches from becoming solved labels.

## Prototype attack archetypes

These are Combat Lab test concepts, not final character moves. No production damage or frame values are assigned.

| Test archetype | Readable information | Likely basic response | Likely advanced response | Decision to test | Unknowns |
| --- | --- | --- | --- | --- | --- |
| Straight or jab-like normal | Line, reach, recovery | Appropriate standing/crouching Block | Timed challenge, spacing, or read | Can ordinary pressure remain legible and interactive? | Guard relation, advantage, range |
| Low sweep | Low trajectory and commitment | Crouching Block | Spacing, timed response, or punish | Can a low threat be recognized without a blind guess? | Startup cue, recovery, tracking |
| Axe kick / committed linear Heavy | Large vertical line and committed arc | Survival Block if valid | Directional Pivot off the line | Does geometry reward a readable spatial read? | Vertical coverage, tracking, Pivot window |
| Straight thrust / committed linear Heavy | Narrow forward trajectory and recovery | Survival Block if valid | Directional Pivot and positional punish | Does direction matter more than attack label? | Depth width, camera, recovery |
| Ground stomp / rupture | Expanding low area and source | Low-appropriate Block if valid | Low Parry | Can broad low space defeat simple Pivot but reward a height read? | Area growth, timing, escape paths |
| High AoE / overhead burst | Broad high area and anticipation | High-appropriate Block if valid | High Parry | Can a high area preserve a distinct response branch? | Area shape, cue, Parry window |

## Accessibility and Pulse-style assisted combos

**Design requirement for a later milestone.** A player with modest execution ability should be able to turn a confirmed hit into a coherent, competitively useful combo. Basic execution should not be the admission fee for tactical play. Inspired by the accessibility benefit of Pulse Combo, this project should explore its own assisted path rather than copy another game's implementation. Manual routing must still offer meaningful optimization in damage, position, resources, pressure, or character-specific choices. Under bounded optimization, the assisted path should not be intentionally terrible; the manual advantage should often be incremental, not self-defeating for the accessible player. The ~20% idea remains a design target, not a formula. **Undecided:** controls, length, scaling, route selection, damage, meter gain, and resource interactions.

## Visual and aesthetic language

**Design direction.** Presentation should be modern and stylized but tactically readable: stable framing, clear silhouettes and anticipation, visible trajectories and defensive movement, committed recovery, restrained ordinary effects, smooth camera response, and deliberate depth use. Major Pivot success, height-specific Parry, punish, counter-hit, tag commitment, or large resource expenditure may receive stronger audiovisual emphasis. Animation, spatial behavior, and effects should communicate attack meaning where practical, without requiring a UI label for every category. No presentation changes are implemented by this specification.

## Combat Lab evaluation criteria

Later prototypes should be compared under similar situations and with both player and spectator feedback:

| Criterion | Evaluation question |
| --- | --- |
| Readability | Can a player understand what happened and why? |
| Information fairness | Was there enough information for an informed response? |
| Initiative legibility | Is it broadly clear who can meaningfully act next? |
| Decision depth | Are there interesting branches rather than a solved answer? |
| Option diversity | Are multiple reasonable styles viable? |
| Bounded optimization | Does mastery improve outcomes without unnecessarily invalidating legitimate options? |
| Input burden | Are the actions practical on a controller at real speed? |
| Spatial clarity | Does depth add decisions without obscuring position or camera orientation? |
| Offensive agency | Can an attacker condition and adapt beyond choosing a winning label? |
| Defensive agency | Can a defender choose among survival, spatial, timing, and risk responses? |
| Spectator clarity | Can an observer follow the exchange without frame-data knowledge? |
| Dominant-option risk | Does a single response collapse the decision tree? |
| Accessibility | Can players with lower execution ability enter the tactical layer? |
| Expert expression | Do routing, timing, spacing, adaptation, and character mastery retain value? |

## Experimental and undecided mechanics

**Prototype hypotheses.** The exact interaction of Block, spatial Pivot, height-specific Parry, broad attacks, and layered responses still needs playtesting. The 5F prototype adds only the depth extent needed for its attack volumes; moving or shaped volumes remain future experiments. Experiment selection, if added, must be explicit and deterministic, with no mutable global rule changes or per-frame Blueprint reads in authoritative simulation. Readability may require further greybox animation before fair comparison. Health bars are presentation views of authoritative simulation health; no gameplay depends on the HUD.

## Explicitly not yet decided

No final guard rules, chip values, Pivot controls/timing/geometry, depth dimensions, camera behavior, AoE taxonomy, Parry timing or reward, initiative schedule, assist cooldown, tag resource, combo structure, Pulse route, or production animation language is fixed. The 5F Pivot has no invulnerability; later changes to that would require an explicit design decision. The approximate ~20% balancing aim is not an exact rule. A later milestone and playtesting must decide which hypotheses become gameplay.

## Future combat AI

Combat AI belongs after the core combat language and selected defence are established, basic offensive structure exists, and a human can visually read the opponent's move, commitment, response, initiative, and result. Readable greybox animation can suffice; final production models are not required. The future AI should reason from authoritative simulation concepts rather than pixels or presentation animation. Its implementation is not designed here.
