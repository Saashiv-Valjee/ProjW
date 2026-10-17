# Combat design direction

This document separates the accepted prototype from design goals and experiments. It records intent for future decisions; it does not authorize new gameplay in the current milestone.

## Currently accepted

The project has a fixed-60-Hz integer authoritative simulation with copyable match state, grounded movement, dashes, jumps and double jump, Light/Medium/Heavy grounded normals, deterministic boxes, damage, hitstop, hitstun, knockback, and simultaneous trades. The debug harness presents that state. The baseline contact rule is grounded valid overlap → Hit. Accepted milestone behavior remains the reference until explicitly changed.

## Current design goals

Combat should be visually stable and easy to parse. Large changes in position, facing, or silhouette should have legible transitions. Avoid relying on constant sudden displacement, unexplained teleports, frequent side switching, jittery stance changes, or overlapping attacks and effects. Animation readability will eventually be part of gameplay readability.

The desired rhythm is movement → commitment → readable interaction → response → short payoff → repositioning or another decision. This aims for comparatively low event density and high decision density. Depth should come from informed choices, reads, spacing, timing, commitment, adaptation, and opponent conditioning. Frame advantage, pressure, traps, mixups, and delays remain possible, but players should broadly understand the interaction and whose opportunity it is. Hidden frame knowledge and repeated guesses about whether one may act should not be the main source of depth.

Initiative is a design lens, not an authoritative enum: attacker advantage, neutral, defender advantage, and punish should become understandable through gameplay and presentation. Basic defensive success might concede initiative; a stronger informed response might return to neutral; an exceptional response might earn advantage; a committed offensive error might create a visible punish. These are targets for experiments, not current rules.

Explore layered defence so basic survival need not require constant high-risk guessing. Tag and assist actions should be meaningful strategic commitments, with readable, infrequent overlap rather than continuous screen activity. A roughly four-second cooldown or shared resource is only an exploratory reference. Ordinary combos should favor shorter conversions, meaningful branches, and regular returns to interaction; long optimized or resource-heavy routes may still exist.

## Experimental and undecided

Three defensive families are candidates for controlled prototypes:

| Family | Questions to explore |
| --- | --- |
| Conventional 2D | Facing-relative guard, standing/crouching defence, attack levels, blockstun. |
| Directional or active | Attack direction, directional guard, timed deflect/parry/avoid relationships adapted to 2.5D. |
| Hybrid or layered | Basic survival defence plus informed directional responses and precise timed responses. |

A possible outcome ladder is Hit → Basic Block → Strong/Directional Defence → Perfect Defence. Its controls, timings, rewards, and even its existence are undecided. A future move may need orthogonal vertical level and lateral/vector information, but no such properties are approved for the production move definition yet. The Combat Lab contact seam permits later prototypes to compare outcomes without changing the generic move lifecycle. Experiment selection should be explicit and deterministic if introduced; no mutable global rules, per-frame Blueprint reads, or runtime console-variable dependence belongs in authoritative simulation.

## Explicitly not yet decided

No defensive model is selected. There is no final guard scheme, directional taxonomy, parry window, advantage schedule, assist cooldown, tag resource, combo rule, or production animation language. The project should not be treated as a clone of a particular conventional, anime/tag, 3D, or directional fighter. Future experiments and manual readability checks will guide those decisions.

An AI sparring opponent belongs after the core combat language and chosen defence are established, basic offence exists, and a human can visually read the opponent's move, commitment, response, initiative, and result. Readable greybox animation can suffice; final character models are not required. The future AI should reason over authoritative simulation concepts, not pixels or presentation animation state. Its implementation is not specified here.
