# CS2 Rage

Rage is an independent CS2 output program. It starts disabled. Enabling it turns
off the stored independent RCS, Triggerbot and manual Aimbot switches, cancels
device calibration, and disarms conflicting desktop movement tests. Disabling
Rage releases its ownership; the switches it turned off remain off. Since
2.8.37, enabling any of Aimbot, Triggerbot or Rage first disables the other two.
The manual Aimbot switch does not disable the shared movement program used by
Rage. See [Aim activation and layout](AIM_ACTIVATION.md).

## Optional foreground check (2.8.39)

Aim General owns `cs2CheckForeground`, default true, shared with Triggerbot. Disabled skips only the local foreground PID comparison; it does not claim remote focus or waive game identity, protocol, source age, session, device, target, ray, damage or click conditions. Toggling clears pending movement/shot proposals and analysis; dispatch rechecks the current policy. The earlier unconditional foreground statements below describe the default enabled policy. Reader and firmware are unchanged.

## Compatibility and precise readiness (2.8.20)

The live Reader already reports game build 14193. The former consumer-side 14192
whitelist incorrectly rejected it before target analysis. Game build is now
provenance, not an admission rule. Frozen schema/ABI/ruleset validation, verified
entity references, field availability, clock identity and sample-age limits all
remain. The Reader owns native address adaptation. Skeleton assets are matched
by the verified profile ID and model identity, not by the build number in an
asset filename. Calibration build numbers likewise retain provenance without
discarding an otherwise valid scale. Parameter-package bytes/rules and DLL flags
are still checked; the source game's build label alone cannot reject them.

Rage readiness distinguishes an unsubscribed Reader, missing current frame,
unverified directory, missing map/local player/eye/mouse mapping, absent or
mismatched sample identity, stale/uncertain/unavailable clock, and invalid or
out-of-range sample timing. There is no generic replacement for these failures.
The frame's build label never grants missing fields or authorizes old samples.

Read-only 14193 observations after the fix verified current mouse mapping,
calibration prerequisites, recoil evaluation and 190 hitboxes for 10 models,
with zero missing/invalid poses. No serial device or gameplay input was used.

## Bone candidates and ranking

The current `threat-v2` default and its independently selectable legacy model,
four-factor scoring, health-aware damage scaling and one-point switch margin
are specified in [CS2_RAGE_SCORING.md](CS2_RAGE_SCORING.md). The 15-degree formula
below describes the retained **legacy** comparison, not the v2 default.

Since 2.8.42, Include targets is in Aim General and applies to Aimbot,
Triggerbot and Rage. The existing `cs2RageEnemies` (default true) and
`cs2RageTeammates` (default false) keys retain saved choices. Older stored
documents may omit either key and use its default.
Both false pauses with `targets-disabled`, without scheduling bone analysis.
The local player and unknown/non-playing teams never qualify. The same typed
filter is used by analysis, current-bone resolution, the explicitly supplied
shared-aim target, and the final first-hit ray. An excluded player in front
still blocks firing at another included player behind it.

The worker carries the filter with each job and result. Changing either switch
clears movement, pending shots and analysis; old-generation/filter results
cannot restore output eligibility. Values are stored only in the CS2 game.

For explicitly included teammates, damage ranking requests the existing DLL's
conditional `teammates_are_enemies` rule with an explicitly assumed ruleset.
The actual player team is preserved, the setting does not change the game or
claim that live friendly fire is enabled, and default damage/ESP callers retain
their original rules. Unsupported override providers return unknown results.
The interface is a generic explicit damage-rule override, not Rage business
logic inside the DLL. The target switches do not change threat mathematics,
movement algorithms, recoil, foreground checks, inset, or finite-click limits.

`rage/analysis` enumerates each unique bone bound by the reviewed model's damage
metadata. Since 2.8.29, it transforms the center of that bone's matching hitbox
into the current world pose and queries this one interior point for visibility
and damage. A bone origin can be near a joint and a ray to it can first hit a
neighboring hitbox; the interior point addresses that mismatch without surface
sampling, adaptive subdivision, or whole-body point search. Missing or invalid
pose information cannot fall back to the raw bone origin. Unknown models, dead,
dormant, excluded, and missing players do not qualify. The reviewed metadata
supplies the hitbox and damage group; they are not guessed from bone IDs.

Since 2.8.28, `cs2RageCoverCheck` is a per-CS2 setting, defaulting to true for
both new and existing configurations. With the switch on, only bones explicitly
visible through the static scene are evaluated and the final shot path must also
be visible. With it off, Rage does not request visibility checks: the existing
ballistics API evaluates eligible bones including paths through static cover.
The current map and the ballistics provider remain necessary in either mode.
The switch and each job/result share the same mode; changing it clears old
analysis, movement and pending shots before fresh analysis can qualify output.

For each included player, the ballistics API evaluates the eligible bones in a
batch using the current weapon, team, health, armor, and helmet. Only a reached,
build-compatible prediction with finite positive uncapped `healthDamage` can
be selected. Zero or stopped damage cannot qualify, and missing or invalid
values never become zero. The model retains the library's static-scene and
vulnerable-target assumptions, including the documented conditional team rule.

Threat uses a valid current `eye_basis.value` first, otherwise raw network
`eye_angles` (`m_angEyeAngles`). The direction from the enemy's entity position
plus `view_offset` to the local eye is compared to that orientation. The estimate
is `max(0, 1 - angle/15 degrees)`. Missing direction contributes no confirmed
threat score. It is an orientation estimate, not a target-lock field or a claim
about the enemy's final server bullet direction.

Score = threatWeight * threat + (1 - threatWeight) * damage / maximumEligibleDamage.
The requested default is **70% threat / 30% damage**. Weight is editable. Ties
prefer larger damage, then stable identities. Since an enemy's threat score is
the same for its bones, its highest-damage eligible bone is preferred even at
100% threat weight.

## Analysis and movement

`rage/worker` is a dedicated thread with one running job, one replaceable pending
job, and one immutable latest result. Full bone damage traversal and threat/rank
work remain on that thread. Scene loading uses the existing shared providers.
The result owns immutable visibility/ballistics handles for a bounded final
check; loading, file reads and profile validation never run in the output loop.

Movement may use a completed analysis up to 100ms old (also bounded by the user
sample-age limit), with the same Reader/clock/epoch/process/map generation,
connection, local handles, and weapon. It resolves that same bone and hitbox
center from the current matching model and current pose. Current source and the
selected algorithm's complete recoil/input qualification remain mandatory.

Since 2.8.21, Rage composes the same `cs2::OutputProgram` as manual Aimbot.
`aimSettings` is the only parameter decoder: the current algorithm/version or
single-factor comparison, speed, prediction and recoil switches, response delay,
sample age, native/measured mapping, and enabled mouse model all apply directly.
The device binding also uses that speed. There is no independent direct-step
algorithm, fastest-speed override, pending ledger or feedback implementation.
V1/V2/V3/V4, all comparisons, numerical reference and prediction, smoothing,
acceleration, rollback and response observation run through the shared code.
When prediction is enabled, missing velocities pause just as in Aimbot; they do
not silently turn prediction off. The obsolete `cs2RageMaxAgeMs` key is discarded
on load without overwriting the user's `cs2AimMaxAgeMs`.
The stored-document prevalidation explicitly accepts only this retired key in
its old numeric range before normalization; live writes and all other unknown
keys remain rejected. This is covered by a stored-document regression and a
no-device startup using a copy of the user's pre-upgrade configuration.

The user explicitly retained Rage's target and bone policy. It supplies a
current `TargetPoint` to `solveAimAt`; manual Aimbot's part, priority, cone,
distance selection and held side-key intent do not replace Rage's policy.
Enemy/alive/source qualification still applies. Comparison modes preserve the
same selected bone while changing only their registered numerical factor.
Changing the enemy or bone resets motion state before continuing automatically.
Ordinary Aimbot still retains its manual release/hold policy and golden output.
Each packet observes the negotiated interval, signed step, and XY budget.
`ActivationSource::Continuous` is an explicit program activation, separate from
physical mouse and keyboard bindings.

## Final shot qualification

Since 2.8.34, shots no longer require the full ranking result to have the latest
capture number. At 100 Hz, a ranker taking longer than a frame could otherwise
allow continuous motion but permanently fail that equality. The compatible
ranking's existing age limit (the smaller of 100 ms and the shared setting) is
unchanged, and it selects the target/bone only. Its old damage and shot ray
cannot authorize a click.

`rage/shot_gate` validates one selected point with the immutable current snapshot:
the current target/model/pose, weapon and ammo, actual recoil-based ray, included
first player, actual first hitbox's inset, static visibility when enabled, and
one current positive target-damage query. It never repeats full ranking,
all-bone damage traversal or threat calculation. Only already-loaded const
scenes are queried; the DLL explicitly supports concurrent read queries with
independent scratch workspaces. This small final query runs in the output
program's qualification stage and is cached once per snapshot/selection, rather
than delaying the Reader or renderer. A later capture recomputes it.

The click proposal records its original revision and selected point. Dispatch
does not reject solely because a newer capture arrived: it checks that the
source/clock/epoch/connection/map/local player/weapon are compatible, then repeats
current aim and final-shot qualification with that new snapshot. It never
substitutes a new target during dispatch. Proposals also expire within the
smaller of 100 ms and the shared sample-age limit. Death, disappearance, pose or
source changes, missing/zero/stopped damage, focus loss, settings changes and
actual ray misses revoke a proposal. The accepted capture is the one validated
at dispatch, preserving per-capture deduplication. Shared aim qualification can
consume the same immutable observation, avoiding a second source read midway
through one check. Ordinary manual Aimbot retains its existing revision wrapper.

One current shot ray finds the first actual hitbox among players. Since 2.8.30, this hitbox
may belong to another bone or damage group of the selected player: it must still
be entered deeply enough to satisfy that first hitbox's configured inset.
`cs2RageOtherPlayerCheck`, default true for existing and new configurations,
blocks firing when a different person is first on the ray. When disabled, another
person may be hit first only if that person matches the enemy/teammate inclusion
settings; an excluded or unknown-team person with a valid hitbox pose still
blocks firing. The inset is always checked against the actual first hitbox. Switching the setting revokes
old analysis, movement and pending shots. Selection, positive damage at the
chosen point and current identity remain qualified. If a different part or
person is hit first, actual damage can differ from the estimate for the selected
point. This is one final ray, not a search
over hitbox surfaces. With cover checking on,
the path must be explicitly visible in the static scene. With it off, the chosen
current selected point must instead have a fresh valid positive damage prediction, including
penetration; the final ray still has to pass the existing hitbox checks.

Aim → General supplies the shared hitbox inset. The range is **0–90%**, with
**10%** for a new configuration. The check uses the inset of the first hitbox.
Spheres and capsules shrink their radius, and boxes shrink each half extent
around the center.

Current source, game foreground PID, connection/session, physical-left priority,
firmware finite-click capability, cooldown, and per-capture deduplication are
checked before requesting the pulse, then checked again by the device owner at
dispatch. Click requests are finite 5–40ms effects released by existing v7.1.
Final-shot rejection reasons are published instead of leaving the public state
at `tracking`: ray miss, inset, other person, excluded person, cover, damage,
aim qualification, physical-left priority, cooldown, consumed capture and expired
proposal are distinct. Routine waits do not become global faults. A new valid
frame retries automatically; there is no latch requiring an enable toggle.
Focus loss invalidates pending movement and shots. Returning to the foreground
requires fresh qualified data and analysis; no historical shot is replayed.

Rage does not simulate spread or guarantee a server hit. The initial UI exposes
the enable switch, threat/damage weighting, click interval, hold duration,
and source-age limit. Live reasons and request counts use the page context bar;
explanations use control tooltips. PUBG has no verified equivalent damage/hitbox
contract, so it does not advertise Rage or request additional Reader fields.

## Validation

Native synthetic tests cover bone-only enumeration, weighting, same-enemy best
damage, occlusion/unknown, death/model/weapon invalidation, inset edge rejection,
activation isolation, current-frame finite shots, deduplication, foreground loss,
stale source, and stored conflict normalization. The 2.8.34 regression deliberately
holds full ranking in a batch query while new 100 Hz captures and current poses
advance: finite clicks can still qualify from fresh one-point checks. It checks
new captures between proposal/dispatch, one query per snapshot, current zero or
unknown damage, target motion/death, map/weapon/connection/config changes, real
left priority, deduplication and automatic recovery. Actual-DLL synthetic wood
checks exercise the final gate and report its finite-query timing; these are not
real gameplay or worst-case latency guarantees. UI and hidden packaged checks
verify defaults, settings, context feedback, and exclusivity with no devices.
A 2.8.29 no-device regression recreates the reported head/neck posture from a
read-only capture: a ray toward the raw head bone first reaches the neck, while
a ray toward the transformed head center first reaches the head. With the
2.8.30 rule, a sufficiently deep hit on either part of the same target qualifies;
a shallow first hit is still rejected. The other-player switch tests cover the
selected person, another included person, an excluded person, and revocation on
setting changes. The checks also cover current-pose tracking and missing rotation.
The 2.8.28 regressions check both cover modes, stale-result revocation,
zero/unknown/stopped/invalid damage and saved settings across native restarts.
An isolated synthetic wood scene through the real traceline DLL verifies that a
positive-damage penetrating shot is excluded with cover checking on and eligible
with it off. This is an offline fixture, not a live-game guarantee.
Shared-control regression compares Rage and manual Aimbot packet counts for all
20 modes and five speed settings, including prediction/recoil switches,
response delay, measured mapping, learned limits, and rejected output rollback.
Real-game behavior remains for user manual testing.
