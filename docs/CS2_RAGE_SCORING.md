# CS2 Rage scoring

The `threat-v2` default is a two-level 0–100 ranking model. `legacy` keeps the
previous 15-degree local-eye-point estimate and relative-maximum damage formula
exactly; the new settings are saved but ignored in that model. Scoring never
changes the frozen Reader contract, recoil evaluation, shared movement strategy,
target inclusion policy, foreground/session qualification or finite-click rules.

## Model and settings

Total = threat share × threat + complementary share × outgoing damage score.
The existing `cs2RageThreatWeight` remains 70% by default.

Threat = clamp(aim share × aim + complementary share × incoming damage score
− low-magazine deduction − reload deduction, 0, 100).

| Setting | Default | Meaning |
|---|---|---|
| cs2RageScoreModel | threat-v2 | New model or legacy comparison |
| cs2RageAimWeight | 60 | Positive threat share; incoming damage gets 40% |
| cs2RageLowAmmoPenalty | 10 | Deduct points for known valid magazine counts 0 or 1 |
| cs2RageReloadPenalty | 60 | Deduct points for confirmed reloading |
| cs2RageHealthNormalization | true | Independently compare health-aware/relative damage |
| cs2RageStableTarget | true | Independently enable the switch margin |
| cs2RageSwitchMargin | 1 | 0–100 score points, inclusive retention boundary |

All numeric settings accept finite 0–100 values. The deductions stack and never
make threat negative. A count above one, absent/sentinel magazine or unknown
reload does not deduct points. Unknown is still recorded, not reported as a full
magazine or a confirmed non-reload. These starting weights are editable model
choices, not empirically proven optimal parameters.

Health-aware damage score is `100 * D / (D + recipient health)`, implemented
without an overflowing sum. Raw uncapped damage is retained and always chooses
the best bone within one person. The curve is a sorting score, not a hit
probability. Disabling it uses separate current maxima for outgoing/incoming
damage. Legacy always keeps its former outgoing normalization.

## Local-body geometry

The complete current local resource Hitbox pose is prepared once per analysis.
Every enemy uses its current entity position plus view offset, and the existing
qualified eye basis (network angle fallback), to generate one forward direction.
An exact intersection with any local body Hitbox receives 100 aim points.

Outside the body, `TargetIntersector::directionProximity` computes the minimum
relative expansion needed for that ray to reach each shape. Capsules/spheres use
analytic forward-ray/segment distance divided by radius; boxes minimize the
six-face piecewise-linear normalized extent. Reciprocal expansion gives a
continuous proximity score below 100. This naturally changes with distance,
stance, signed scale and raw verified transforms. It is a relative geometric
proxy for angular proximity, not an exact angular-distance solver. There is no
fixed 15-degree cutoff in v2 and no unbounded surface-ray search. Backward rays
score zero; missing/invalid complete poses remain unknown. Existing `first`
intersection and inset math are unchanged.

## Incoming damage

The enemy's active handle is resolved only in the same snapshot weapon list.
Weapon definition, valid clip and reload state belong to that enemy, not the
local gun. The local recipient uses current health, armor, helmet and team.
Each finite resource Hitbox center seeds one ray; the exact first local Hitbox
intersection supplies the impact point and group. The existing batch ballistics
port calculates range, armor and static penetration effects. This is a finite
potential-damage estimate, not exhaustive maximization over the body's surface.

Only complete finite results produce an incoming damage value. Stopped sampled
paths contribute known zero; an unknown path is not converted into zero. Even
all stopped finite samples do not prove that every body edge is protected.
The entire instantaneous firearm threat is set to zero only for complete
matching zero damage upper bounds under the provider's model. Definitions outside
the reviewed firearm list remain unknown, not harmless non-firearms. For explicitly included
teammates, reverse predictions carry the same conditional friendly-damage rule;
real teams are preserved and the result is labelled an assumption.

Missing one positive threat term renormalizes the known positive share. If no
configured positive evidence exists, ranking explicitly degrades to outgoing
damage. The UI exposes this status. No such fallback grants missing movement or
shot qualification.

## Selection, lifecycle and inspection

The largest raw-damage bone wins within each selected person, including at 100%
threat weighting. Hysteresis applies only between different valid people. A
challenger ahead by at most **one point** keeps the current target; a larger lead
switches immediately, without a dwell timer. A dead, missing or otherwise
invalid previous target is excluded against the current frame even if a recent
background result still contains it. Final-shot selection uses the same margin
and current eligibility, so a stabilized target can actually fire.

Score policy is carried by Worker jobs/results and checked with generation,
source, map, handles and sample age. Configuration changes clear analysis,
movement and pending shot eligibility. Work stays on the existing bounded
background Worker (one running, one replaceable pending, one latest result).
No geometry or DLL calls are added to drawing or device dispatch.

The optional `cs2Rage.scoring` DTO carries the last evaluated model, aim and
incoming/outgoing scores, raw incoming damage, deductions, threat/total, weapon,
ammo, reload, degraded and stabilized flags. The expandable latest-score panel
uses this data directly and shows unknown as `—`; it never fabricates measurements.
Saved Rage activation and existing target filters are retained during upgrade.

### Visible formula reference

The scoring settings contain a directly visible **Current scoring formulas**
section, requested by the user on 2026-10-31. It follows the accepted saved
settings, rather than uncommitted input drafts or the last evaluated target.
The displayed total/threat coefficients, deductions and strict switch margin
update with those settings. Health-aware and relative-maximum damage equations,
legacy facing, and disabled retention each show their applicable rule.

The reference defines the hitbox proximity score, each damage recipient's health,
the zero/one-round and confirmed-reload conditions, missing-evidence weight
renormalization, the outgoing-only fallback and the legacy zero-facing fallback.
It distinguishes uncapped predicted damage from ranking points and hit
probability. It shares the scoring settings' disclosure and remains readable in
Chinese and English. Unlike general help text, these formulas are visible by the
user's explicit request. This is UI documentation; it adds no JavaScript scoring,
ray queries, native policy changes or implicit setting writes.

## Validation ownership

Native regressions cover the formulas, deductions, unknown values, legacy model,
three geometric primitive types, visual scaling, actual enemy weapon/local armor
direction, current-target invalidation, the inclusive one-point boundary and
current-frame firing under hysteresis. UI tests cover saved selection and
independent controls, defaults, invalid input and real score breakdowns.
Final paired native/Web/bridge/package validation and release are coordinated by
the main thread; source version alone does not establish deployment.
