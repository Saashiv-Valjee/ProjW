# Character Asset Inventory

Inventory date: 2026-10-19. This is a file/metadata investigation for choosing a
Fencer v0 presentation source. No assets were imported, duplicated, retargeted,
or edited. No gameplay, simulation, controls, or accepted camera/Pivot behavior
were changed.

## Executive Summary

- The imported packs are at `/Game/ParagonYin` and
  `/Game/ParagonCountess`. A third `/Game/ParagonTerra` pack is also present,
  but was not assessed as a Fencer/Brawler candidate.
- **Countess is the stronger Fencer v0 starting hypothesis**: her asset set is
  an aristocratic female character with sword-oriented materials, weapon bones,
  and weapon base/tip sockets. This is a thematic inference from asset names
  and structure, not a visual approval. The built-in sword setup is dual-blade
  flavored, not a rapier. Yin is also female and has a humanoid rig, but her
  pack is organized around a whip and martial-arts moves.
- Best authored crouch: **MISSING**. No crouch/squat/kneel animation was
  positively identified by inventory labels. The existing procedural crouch
  remains the known temporary weakness; it was not modified.
- Best authored Pivot replacements: **MISSING** for both directions. The
  Paragon `Jog_*_Pivot` clips are locomotion direction changes/turn pivots by
  naming, not verified short sidesteps that layer cleanly over the accepted
  simulation path.
- Best thrust candidate: external Motifect
  `External/MotifectCombat/Animations/sword_thrust_forward.fbx`. The pack
  README describes humanoid FBX clips, but this is not a verified fencing or
  rapier action. Countess also has sword-attack sequences, but filenames do not
  establish a straight thrust.
- Best parry candidate: external Motifect
  `External/MotifectCombat/Animations/sword_parry_and_riposte.fbx`; its actual
  line, timing, and content are unverified. No credible separate high-line and
  low-line pair was found.
- Asset Registry inspection found 493 animation sequences across Yin,
  Countess, and the UE mannequin directories searched (162 Yin, 234 Countess,
  97 mannequin), plus 13 Montages. The inventory is selective, not a dump of
  every clip.
- None of the 34 Fencer v0 slots earns EXCELLENT/GOOD because the clips were
  not visually previewed in this investigation. Based on names/metadata only,
  **19 are USABLE WITH EDITING and 15 are MISSING**. “Usable” means a plausible
  source to preview/retarget, not an approved final animation.

## Evidence and Confidence

Verified by filesystem and Unreal Asset Registry: package locations, asset
classes, exact asset paths, sequence-to-Skeleton references for sampled clips,
mesh-to-Skeleton/PhysicsAsset references, animation lengths and root-motion
flags for sampled clips, and mesh socket names/bones. The registry query
covered Yin, Countess, and mannequin directories; it did not load or visually
play every clip.

Inferred from asset names, pack organization, and metadata: likely action use,
looping behavior in the source AnimBlueprint, character aesthetic, and
retargeting potential. An AnimSequence does not itself prove that a state is
looped; its source AnimBlueprint may decide that. No pose-by-pose viewport
inspection, motion-capture review, or rendered comparison was completed, so
ratings are provisional. Confirm timing, actual pose, root translation, prop
dependence, and readability by previewing the chosen clips before integration.

## Character Assets

### Yin

Primary package folder: `/Game/ParagonYin/Characters/Heroes/Yin`.

| Asset role | Verified asset path / finding |
|---|---|
| Base skeletal mesh | `/Game/ParagonYin/Characters/Heroes/Yin/Meshes/Yin` |
| Base Skeleton | `/Game/ParagonYin/Characters/Heroes/Yin/Meshes/Yin_Skeleton` |
| Base PhysicsAsset | `/Game/ParagonYin/Characters/Heroes/Yin/Meshes/Yin_Extents` |
| Other physics helpers | `.../Meshes/Yin_ShadowCyl` is also a PhysicsAsset-class asset; intended runtime role not established here. |
| Animation Blueprint | `/Game/ParagonYin/Characters/Heroes/Yin/Yin_AnimBlueprint` |
| Rig-related assets | `/Game/ParagonYin/Characters/Heroes/Yin/Meshes/Yin_SkeletonRig` is Registry class `Rig`; it is not an IK Retargeter. `/Game/ParagonYin/Characters/Heroes/Yin/Animations/AnimBP_Yin_Rigging` is an AnimBlueprint. |
| Skeletal skin variants | `/Game/ParagonYin/Characters/Heroes/Yin/Skins/CryptGoddess/Meshes/Yin_CryptGoddess`; `/Game/ParagonYin/Characters/Heroes/Yin/Skins/WindGauntlet/Meshes/Yin_WindGauntlet`. Verify their Skeleton assignments before selection. |
| Weapon structure | Main mesh has whip bones including `whip_base`, `whip_01`, `whip_05`, `whip_09`, `whip_013`, and `whip_015`; sockets include `WhipAimSocket` on `whip_base`, `WhipAimRotation` on `whip_01`, and whip FX endpoints. This is a character-specific prop chain, not a rapier rig. |
| Materials/textures | Character textures are under `/Game/ParagonYin/Characters/Heroes/Yin/Textures`; examples include `T_Yin_Skin_RGBAO`, `T_Yin_Cloth_RGBAO`, `T_Yin_Armor_RGBAO`, and skirt textures. Whip materials include `/Game/ParagonYin/Characters/Heroes/Yin/Materials/M_Yin_Whip_Core` and `M_Yin_WhipHandle`. |
| VFX/audio | Yin FX are under `/Game/ParagonYin/FX/Particles/Yin`; e.g. `/Game/ParagonYin/FX/Particles/Yin/Abilities/WhipCrack/FX/p_Yin_WhipCrack_Animtrail`. `/Game/ParagonYin/Audio/Yin` exists, but its internal cue organization was not inspected. |

### Countess

Primary package folder: `/Game/ParagonCountess/Characters/Heroes/Countess`.

| Asset role | Verified asset path / finding |
|---|---|
| Base skeletal mesh | `/Game/ParagonCountess/Characters/Heroes/Countess/Meshes/SM_Countess` |
| Base Skeleton | `/Game/ParagonCountess/Characters/Heroes/Countess/Meshes/S_Countess_Skeleton` |
| Base PhysicsAsset | `/Game/ParagonCountess/Characters/Heroes/Countess/Meshes/PA_Countess_Phat_Extents` |
| Other physics helpers | `.../Meshes/PA_Countess_Phat_CylShadows`; Shogun skin has `.../Skins/Tier2/Shogun/Meshes/PA_Countess_Shogun_Physics` and `PA_Count_Shogun_Cloth_Physics`. |
| Animation Blueprint | `/Game/ParagonCountess/Characters/Heroes/Countess/Countess_AnimBlueprint` |
| Rig-related asset | `/Game/ParagonCountess/Characters/Heroes/Countess/Meshes/Paragon_Proto_Retarget` is Registry class `Rig`, not an IK Retargeter. |
| Skeletal skin variants | `.../Skins/Tier1/Count_Carnivale/Meshes/SM_Countess_Carnivale`, `.../Count_Gold/Meshes/SM_CountessGilded`, `.../Count_RedRevo/Meshes/SM_Countess_RedRevo`, and `.../Skins/Tier2/Shogun/Meshes/SM_Countess_Shogun`. Verify Skeleton assignments before use. |
| Weapon structure | Main mesh exposes `weapon_l` and `weapon_r` bones and sockets `FX_WeaponBase_L`, `FX_WeaponTip_L`, `FX_WeaponBase_R`, `FX_WeaponTip_R`, each attached to the corresponding weapon bone. This is useful for presentation attachment, but does not establish a separate rapier mesh. No standalone rapier mesh was identified in the character `Meshes` directory. |
| Materials/textures | Character materials and textures are under `/Game/ParagonCountess/Characters/Heroes/Countess/Materials` and `/Textures`. Sword-related examples include `M_Lace_sword`, `M_Countess_Sword_Holster`, `T_Countess_lace_sword_normals_DEMO`, and `T_Countess_Sword_EmissiveMask_RGB`. These are material/texture assets, not a standalone rapier. |
| VFX/audio | Countess attack FX live under `/Game/ParagonCountess/FX/Particles/Abilities`; examples include `/Game/ParagonCountess/FX/Particles/Abilities/BlinkStrike/FX/P_Countess_BlinkStrike_HitFX` and `/Game/ParagonCountess/FX/Particles/Abilities/BladeSiphon/FX/p_Countess_BladeSiphon_HitFX`. Character audio is under `/Game/ParagonCountess/Characters/Heroes/Vamp/Sounds`; sampled files include `SoundWaves/Countess_Ability_LMB_Engage_010` and `Countess_Effort_Ability_Primary_01`. Their intended move mapping was not inspected. |

### Skeleton relationship

Yin uses `Yin_Skeleton`, Countess uses `S_Countess_Skeleton`, and Manny/Quinn
use `SK_Mannequin`: these are separate Skeleton assets, not one identical
shared asset. Inspected mesh sockets show familiar humanoid base names
(`root`, `pelvis`, `spine_03`, `upperarm_r`, `hand_r`, feet) plus pack-specific
extensions: Yin's whip chain and Countess's left/right weapon bones. This is
evidence for a humanoid retargeting path, not proof of identical hierarchy,
bone orientation, proportions, or plug-and-play compatibility. Best current
classification: separate Skeleton assets with a shared-looking humanoid core
and pack-specific extensions, so treat them as likely compatible through
retargeting, not identical skeletons. The full hierarchy was not extracted;
compare complete bone trees and bind poses in IK Rig before relying on the
retarget setup.

## Existing Animation Sources

### Yin

Registry found 162 AnimSequences, eight Montages, five BlendSpaces, and three
AimOffset blend spaces under the inspected Yin character tree. Useful named
groups include combat idle (`/Game/ParagonYin/Characters/Heroes/Yin/Animations/Idle_Combat`), forward/back combat jog
(`Jog_Fwd_Combat`, `Jog_Bwd_Combat`), ordinary jog start/stop and circle
variants, attack A/B/C/D fast/medium/slow and recovery sequences, jump start,
apex and land, directional hit reactions, and stun start/idle. The
`/Game/ParagonYin/Characters/Heroes/Yin/Animations/Jog_Left_Pivot` and `/Game/ParagonYin/Characters/Heroes/Yin/Animations/Jog_Right_Pivot` are locomotion pivot candidates,
not confirmed short evasions. The attack and whip-specific sets are source
material for Yin; they do not establish Fencer suitability.

Representative Montage: `/Game/ParagonYin/Characters/Heroes/Yin/Animations/E_Ability_Attack_A_Montage`.
Representative BlendSpace: `/Game/ParagonYin/Characters/Heroes/Yin/Animations/BlendSpaces/JogCombat1D`.
Representative AimOffset: `/Game/ParagonYin/Characters/Heroes/Yin/Animations/AimOffsets/AO_Idle_Blendspace`.

Verified examples: `/Game/ParagonYin/Characters/Heroes/Yin/Animations/Idle_Combat`,
`.../Jog_Fwd_Combat`, `.../Primary_Attack_D_Fast`, `.../Jump_Start`,
`.../Jump_Land`, and `.../HitReact_Front` reference `Yin_Skeleton`.
Sampled `Idle_Combat`, `Jog_Fwd_Combat`, `Jog_Left_Pivot`, `HitReact_Front`,
`Jump_Start`, and `Jump_Land` have `enable_root_motion=False`. Their durations
are respectively 10.133 s, 1.9 s, 4.233 s, 0.9 s, 0.267 s, and 0.2 s. These
are metadata readings, not an assessment of clip content or loop points.

### Countess

Registry found 234 AnimSequences, four Montages, nine BlendSpaces, and two
AimOffset blend spaces in the inspected Countess tree. Named groups include
neutral idle variants (`Idle_Straight`, `Idle_Relaxed`, `Idle_Pose`),
`Jog_Fwd_Combat`/`Jog_Bwd_Combat`, jog start/stop/circle/pivot,
sprint start/stop/pivot, jump start/apex/land/recovery, normal/fast/slow
primary attacks and recovery variants, hit reactions, stun, and death. This
is the broadest candidate set for a playable prototype using the Countess mesh.
Its sequence names and sword sockets support a sword-character hypothesis,
but do not prove that any attack is a fencing thrust or visually appropriate.
Representative Montage: `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Primary_Attack_A_Normal_Montage`.
Representative BlendSpace: `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Blendspaces/JogFwdSlopeLean`.
Representative AimOffset: `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/AimOffsets/Idle_AO`.

Metadata was checked for the idle, forward/back jog, jump start/apex/land,
sprint starts, fast/normal/slow attacks, slow recovery, forward hit reaction,
and stun start candidates in the matrix. All sampled sequences reference
`S_Countess_Skeleton` and report `enable_root_motion=False`. Their durations
range from 0.167 s (`Jump_Land`) to 7.5 s (`Idle_Straight`); the long idle,
looping, and state-transition behavior need visual/AnimBlueprint inspection.

### Manny / Quinn

The project mannequin content is under `/Game/Characters/Mannequins`. Manny
and Quinn simple meshes both reference `/Game/Characters/Mannequins/Meshes/SK_Mannequin`;
their shared physics asset is `/Game/Characters/Mannequins/Rigs/PA_Mannequin`.
The tree contains 97 AnimSequences, including `Anims/Unarmed/MM_Idle`,
`MF_Unarmed_Walk_Fwd`, `MF_Unarmed_Walk_Bwd`, unarmed jog, `MM_Jump`,
`MM_WallJump`, `MM_Land`, `MM_Dash`, `MM_Attack_01/02/03`,
`MM_ChargedAttack`, and rifle-labeled hit reactions. It also includes
`ABP_Unarmed`, rifle/pistol BlendSpaces and AimOffsets, and Control Rig
Blueprints `CR_Mannequin_Body`, `CR_Mannequin_FootIK`, and
`CR_Mannequin_Procedural`. No crouch sequence was positively identified.
These generic unarmed/rifle poses are not a Fencer identity.

### Motifect Combat Motion Pack

The pack is **not imported into Content**. It is available as 40 FBX files under
`External/MotifectCombat/Animations`; its README says each FBX includes a
simple stickman preview mesh and humanoid animation, and retargeting is
required. The pack includes `sword_thrust_forward.fbx`,
`spear_thrust_sequence.fbx`, `sword_parry_and_riposte.fbx`,
`sword_block_high.fbx`, `sword_slash_horizontal.fbx`, sword overhead/diagonal/
spin clips, staff attacks/blocks, Muay Thai guard/kicks, TKD kicks, and judo/
wrestling throws/ground positions. Names are only a first-pass index: actual
FBX motion, source skeleton, prop use, root translation, and loop behavior
were not inspected. These are not Unreal asset paths and cannot be used by the
current project without importing/validating a small subset.

### Other

`/Game/ParagonTerra` is also present, including Terra skeletal mesh/animation
content. It was outside the requested Yin/Countess inventory and is not ranked
for Fencer or Brawler. No other clearly relevant imported external animation
library was identified in this pass.

## Fencer v0 Animation Slot Matrix

Ratings describe whether a source is worth a first preview and editing/
retarget test. They do **not** mean visual approval. Every Paragon choice is
character-specific; using it on another Skeleton still needs validation.

| Fencer requirement | Best existing candidate | Source | Rating | Gap? |
|---|---|---|---|---|
| Neutral/combat idle | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Idle_Straight` | Countess | USABLE WITH EDITING | Neutral pose candidate; no explicit Countess combat-idle sequence found. |
| Forward movement | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Jog_Fwd_Combat` | Countess | USABLE WITH EDITING | Jog rather than precise fencing advance. |
| Backward movement | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Jog_Bwd_Combat` | Countess | USABLE WITH EDITING | Jog rather than fencing retreat. |
| Crouch | MISSING | — | MISSING | No authored crouch positively identified. |
| Jump | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Jump_Start` + `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Jump_Apex` | Countess | USABLE WITH EDITING | Loop and landing transitions need preview. |
| Landing | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Jump_Land` | Countess | USABLE WITH EDITING | Check blend and root translation. |
| Forward dash | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Sprint_Fwd_Start` | Countess | USABLE WITH EDITING | Sprint transition, not a short in-place fighting dash. |
| Back dash | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Sprint_Bwd_Start` | Countess | USABLE WITH EDITING | Same limitation; must not drive simulation position. |
| Pivot CW | MISSING | — | MISSING | `Jog_Right_Pivot` is only a weak turn/run lead; no verified short weave. |
| Pivot CCW | MISSING | — | MISSING | `Jog_Left_Pivot` is only a weak turn/run lead; no verified short weave. |
| Light | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Primary_Attack_A_Fast_V1` | Countess | USABLE WITH EDITING | Sword-action source; not proven quick rapier poke. |
| Medium | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Primary_Attack_A_Normal` | Countess | USABLE WITH EDITING | Sword-action source; not fencing-specific. |
| Heavy | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Primary_Attack_A_Slow` | Countess | USABLE WITH EDITING | Sword-action source; inspect commitment and recovery. |
| NarrowThrust | `External/MotifectCombat/Animations/sword_thrust_forward.fbx` | Motifect, external | USABLE WITH EDITING | Closest named straight thrust; motion/prop not previewed. |
| BroadSweep | `External/MotifectCombat/Animations/sword_slash_horizontal.fbx` | Motifect, external | USABLE WITH EDITING | Possible broad sword path; not verified against current volume. |
| SlowCommittedStrike | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Primary_Attack_B_Slow` | Countess | USABLE WITH EDITING | Slow sword-action candidate; clip/recovery not previewed. |
| GroundRupture | MISSING | — | MISSING | No credible ground-impact animation identified. |
| High Block | `External/MotifectCombat/Animations/sword_block_high.fbx` | Motifect, external | USABLE WITH EDITING | Name suggests high guard; actual pose/timing unverified. |
| Low Block | MISSING | — | MISSING | No credible low-guard animation identified. |
| High Parry | `External/MotifectCombat/Animations/sword_parry_and_riposte.fbx` | Motifect, external | USABLE WITH EDITING | Candidate only; high-line direction is unverified. |
| Low Parry | MISSING | — | MISSING | No distinct low-line parry candidate. |
| Hit reaction | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Hitreact_Fwd` | Countess | USABLE WITH EDITING | Direction and severity coverage need preview. |
| Block reaction | MISSING | — | MISSING | Generic hit reaction is not verified block recoil. |
| Parry-success attacker reaction / stagger | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Stun_Start` | Countess | USABLE WITH EDITING | Stun candidate; brief stagger readability is unknown. |
| En garde idle | MISSING | — | MISSING | Combat idle is not evidence of fencing guard. |
| Fencing advance | MISSING | — | MISSING | No fencing-specific advance found. |
| Fencing retreat | MISSING | — | MISSING | No fencing-specific retreat found. |
| Quick rapier thrust | MISSING | — | MISSING | Motifect sword thrust is the nearest generic non-rapier candidate. |
| Full fencing lunge | MISSING | — | MISSING | No verified fencing lunge; spear thrust is not a substitute without inspection. |
| High-line parry | MISSING | — | MISSING | Generic sword parry does not establish a high-line action. |
| Low-line parry | MISSING | — | MISSING | No verified low-line action. |
| Riposte | MISSING | — | MISSING | Combined FBX filename is insufficient to establish fencing riposte. |
| Whiff recovery | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Primary_Attack_A_Slow_Recovery` | Countess | USABLE WITH EDITING | Generic sword recovery; retime/pose alignment likely required. |
| Return-to-guard | `/Game/ParagonCountess/Characters/Heroes/Countess/Animations/Primary_Attack_A_Slow_Recovery` | Countess | USABLE WITH EDITING | Possible recovery endpoint; guard pose unverified. |

**Slot count (34 total):** 0 EXCELLENT/GOOD, 19 USABLE WITH EDITING, 15
MISSING. This avoids promoting generic sword clips to fencing-specific slots.

## Highest-Priority Findings

- **Crouch:** MISSING. The source mannequin set and queried Paragon/FBX names
  expose no authored crouch candidate. Motifect `wrestling_guard_position.fbx`
  is ground grappling by name, not a credible standing crouch.
- **Idle and movement:** Countess `Idle_Straight`, `Jog_Fwd_Combat`, and
  `Jog_Bwd_Combat` are the best same-character starting candidates. No
  explicitly named Countess combat idle was found. Verify stance and foot/root
  behavior in the animation viewer.
- **Pivot:** no credible authored CW/CCW weave. Named Paragon locomotion pivots
  are not assumed to be small evasive steps. The accepted simulation
  trajectory continues to own the actor transform.
- **Thrust/lunge:** Motifect `sword_thrust_forward.fbx` is the closest named
  straight thrust. `spear_thrust_sequence.fbx` is a likely long-prop candidate,
  not a rapier lunge. Neither has been visually reviewed.
- **Parry:** `sword_parry_and_riposte.fbx` is worth previewing as generic sword
  defense, but there are no verified separate high/low-line parries or a
  return-to-guard transition.
- **Brawler:** Motifect Muay Thai, TKD, judo and wrestling FBXs are the strongest
  named broad-body/punch/kick/grappling candidates, but remain unimported and
  unpreviewed. Existing character-specific clips do not establish a Brawler
  animation set.

## Missing Fencer Animations

Priority gaps are en garde; fencing advance/retreat; crouch; short left/right
weave suitable for physical Pivot; fast rapier thrust; full fencing lunge;
distinct high-/low-line parries and riposte; low block; ground-impact response
for GroundRupture; and readable block recoil. Generic Countess and Motifect
candidates may cover temporary slots after preview/editing, but do not remove
these identity gaps.

## Retargeting Feasibility

The conceptual `source animation -> source IK Rig -> IK Retargeter -> target IK
Rig/Skeleton` path is appropriate to test humanoid Paragon-to-Paragon or
Motifect-to-Countess transfer. It is not set up in the project today.

- No project `IKRigDefinition` or `IKRetargeter` assets were found in the
  searched Content tree. The engine IKRig plugin is installed. The project has
  mannequin Control Rig Blueprints (`CR_Mannequin_*`) and Paragon assets whose
  Registry class is generic `Rig`; their specialized rig roles were not
  established. Neither is counted as a reusable IK Rig or Retargeter here.
- Yin, Countess and mannequin each reference a different Skeleton asset.
  Inspected socket bones share conventional humanoid names, but full hierarchy,
  reference pose, root orientation and chain mappings have not been diffed.
  Retargetability is plausible, not proven.
- Create/validate source and target IK Rigs and one Retargeter, then compare
  root/pelvis translation and feet on a short idle/walk. Paragon proportions,
  skirts/cloth, Yin's whip chain and Countess's weapon bones need attention.
  Retargeted root motion must be ignored by authoritative actor movement.
- Motifect README describes a humanoid stickman preview skeleton in each FBX;
  source bone names, axes, root behavior and weapon prop tracks were not
  inspected. Import one file only for a diagnostic after selection.

## Rapier Integration

Countess is the more plausible first mesh. Its main skeletal mesh exposes
`weapon_r` and `weapon_l`, with `FX_WeaponBase_R`/`FX_WeaponTip_R` and
corresponding left sockets. A future single presentation rapier should
probably attach to a dedicated `RapierSocket` parented to `hand_r`, with its
offset tuned against the retargeted wrist pose. Reusing `FX_WeaponBase_R` is
possible for a prototype, but mixes weapon attachment with an FX socket; a
named weapon socket is cleaner. Confirm the mesh's existing weapon rendering
and whether its blade is skinned/hidden before attaching another mesh.

Yin's right hand is extended by whip bones and sockets, making a rapier more
likely to conflict. Neither pack exposes a socket explicitly named for a
rapier. Weapon attachment remains presentation-only; deterministic NarrowThrust
continues to own active frames, hit geometry, and contact. Align the blade near
those frames without using blade pose or root motion for hit detection or
movement.

## Brawler Gap Summary

| Need | Best existing lead | Assessment |
|---|---|---|
| Combat idle / movement | Countess/Yin `Idle_Combat`, `Jog_*_Combat`; Motifect `muay_thai_guard_stance.fbx` | Generic combat movement exists; no Brawler-specific preview. |
| Punch / heavy punch | Motifect `muay_thai_combination.fbx`, `muay_thai_elbow_cut.fbx` | Plausible named strikes; actual punch content unverified. |
| Broad strike | Motifect `muay_thai_roundhouse.fbx`, `tkd_roundhouse_high.fbx` | Large kick arcs, not necessarily broad arm strikes. |
| Committed heavy strike | Motifect `tkd_axe_kick.fbx`, `spear_thrust_sequence.fbx` | Possible large/committed motion; prop and body path unknown. |
| Dodge / Pivot | Paragon `Jog_*_Pivot` or Motifect `muay_thai_push_kick_defense.fbx` | Weak candidates; none confirmed as small in-place dodge. |
| Block / parry / deflect | Motifect `staff_block.fbx`, `sword_block_high.fbx`, `sword_parry_and_riposte.fbx` | Prop-specific guards; likely need posing/retarget edits. |
| GroundRupture-like | Motifect `wrestling_body_slam.fbx`, `staff_overhead_strike.fbx` | No verified ground shockwave/stomp motion. |
| Hit reactions | Yin/Countess `HitReact_*`/`Hitreact_*`; mannequin `MM_HitReact_*` | Reactions exist; severity, direction, and retarget quality need review. |

Overall Brawler asset coverage is **weak-to-moderate**: there are named
unarmed/prop striking clips in external FBX, but none are imported and no
Brawler presentation has been accepted.

## Gameplay / Presentation Boundary

Animations remain presentation. The deterministic 60 Hz simulation remains
authoritative for fighter position, movement, startup/active/recovery, attack
and hurt geometry, hit detection, Block, Parry, Pivot, hitstop, hitstun,
blockstun, knockback, arena boundaries, and state transitions. NarrowThrust
continues to follow its existing move definition. Any eventual rapier pose
should visually align near its active frames; animation must never decide
attack activation, collision, or gameplay movement. Root motion remains
non-authoritative.

## Recommended Next Step

1. Select Countess as the Fencer v0 visual-test mesh (a reversible choice,
   subject to viewport review).
2. Verify one minimal Countess IK Rig and source-to-Countess IK Retargeter;
   compare humanoid proportions and `weapon_r`/hand alignment.
3. Preview/retarget five or six candidates first: Countess combat idle,
   forward/back combat jog, one jump/land pair, Motifect straight thrust, and
   Motifect high-block/parry candidate. Import one Motifect FBX at a time.
4. Test candidates as presentation layered over deterministic transforms, then
   decide which fencing-specific gaps need authored motion.

This inventory does not start Milestone 5I and does not select final production
combat grammar or character animation direction.
