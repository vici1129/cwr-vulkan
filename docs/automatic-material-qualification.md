# Automatic material qualification prototype

Base: public `ce36d47`. Developer command: `triPBR "auto"`. This enables the
existing enhanced mode plus a shared conservative source/triangle classifier.
`off`, `on` (v3) and `enhanced` keep their existing selection. Automatic adoption
defaults Off, is not persisted, and does not change stock rendering or assets.

**The requested expansion across the pilot building families is not achieved.**
The mechanism is implemented in the renderer, but the original pilot facades
lack a source declaration that distinguishes diffuse wall from painted glass,
doors and hardware. No new texture names were added. Calling every ordinary,
opaque triangle masonry would pass the geometric tests and still be wrong.

## Shared contract

`MaterialQualificationVK.hpp` separates source evidence, triangle eligibility,
and reception. An existing verified stock identity takes precedence. Otherwise
an explicit, complete original `TexMaterial` declaration must specify unit
ambient/diffuse, zero forced diffuse, emission and specular, zero specular
power, and no supplemental maps. It receives a generic source-declared diffuse
identity (256), **not a plaster/concrete identity**. Missing declarations and
legacy defaults never provide this proof. The explicit-declaration acceptance
path is covered by a small unit fixture; the pilot assets do not supply one.

Source color and original geometry normals feed the existing rough dielectric
response (roughness .90, F0 .04, metallic 0) and bounded stock-color transport.
No de-lighting, exposure/gain change, generated mask or replacement map is used.
Gameplay surface `rough` and footstep sounds are not optical material data and
are not used to infer wall identity. UVs are checked for validity, not interpreted
as an invented glass mask. An opaque mixed atlas without a declaration stays
legacy, including any UV subset whose physical identity remains unproven.

Only original ordinary vertices qualify. Nonzero material indices, including
201 (shadow), 205 (75% shadow) and 207 (50% shadow), special light modes and fitted
land hints are rejected. Normals, UVs, positions and triangle area must be usable.
Blended, cutout, shadow/effect, colored, animated, depth-disabled, specular/detail
and unsupported render behavior is rejected. Original traversal and alpha tests
remain in place for rejected triangles; they are not removed from the ray scene.

The native raster and static ray paths consume the same cached triangle ranges,
using the original polygon fans and preserving index order. Rejected triangles
retain legacy shading or zero diffuse transport; adjacent ordinary triangles
can qualify independently. CPU lighting retains its existing per-triangle
captured-material splitting and clipping, using the same source/flag/vertex
rules. Material provenance is cached per texture; explicit section overrides
must supply their own evidence. Dirty geometry/hints invalidate the range cache.
The ray path declines qualification if splitting would exceed the existing
1024-geometry device guarantee, while retaining all original traversal geometry.

Reflector eligibility never requires receiver eligibility. Reception additionally
requires independently captured ambient and direct material inputs plus the
existing opaque-world contract. Packed lit color is never inverted to recover
ambient. Existing material reconstruction, ambient replacement, sky calibration,
sampling, filters and both bounce modes are unchanged; one bounce remains default.

## Original pilot results

Read-only native ODOL classification, finest visual LOD. These are actual fan
triangles, not the older preview inspector's byte-derived `tris` estimate.
The rejection columns give the first applicable reason; categories are disjoint.
Opacity comes from decoded original texture pixels and original render flags.

| Model | Qualified | Untyped source | Render behavior | Authored lighting | Invalid geometry |
| --- | ---: | ---: | ---: | ---: | ---: |
| budova1 | 0 | 260 | 0 | 0 | 0 |
| budova2 | 0 | 226 | 0 | 0 | 0 |
| panelak | 0 | 1294 | 736 | 2343 | 12 |
| panelak2 | 0 | 2108 | 762 | 3007 | 12 |
| dum_mesto3 | 4 | 262 | 0 | 0 | 0 |

The four house roof triangles already have the unchanged verified tile identity;
they are **not new automatic coverage**. Barracks blank gables and apartment
plain-panel regions look useful when inspected by a human, but their source
fields do not establish the identity automatically. Mixed painted windows and
hardware can have the same zero material index, finite normals, opacity and
ordinary sampler flags as a wall. Splitting by those fields cannot isolate them.
Apartment authored lighting/cutout ranges remain rejected. This prototype does
not claim it upgrades the facades, interiors or an entire archetype.

Reproduce a classification using original extracted inputs (no basename fallback
is permitted for qualification texture resolution):

```text
PoseidonTools model inspect <original.p3d> --qualify --sections --texroot <original-texture-root> --material-config <original-CONFIG.BIN>
```

The tool uses the renderer's classifier and reports each section/LOD with
eligible/unknown/render/lighting/geometry counts. Its result describes source
and reflector prerequisites, **not independently captured gameplay ambient**.
Missing exact-path texture inputs fail closed. The supplied base config does
not stand in for all addons; runtime additionally sees the actual loaded config.

## Validation and limits

Current model classifications, runtime identities, receiver/hit counters, GPU
intervals, screenshots, validation receipts and preservation checks are recorded
in [the qualification receipt](evidence/automatic-material-qualification/qualification.json).
Original mission cameras cover Kolgujev barracks, Nogova apartments and
Malden/Everon houses. Captures are unamplified. PBR Off/GI Off shows the original
authored response; enhanced/GI On is the existing baseline; auto/GI Off and
auto/GI On separate material selection from ambient replacement. A second-bounce
capture checks the existing optional mode independently.

| Original mission view | Original PBR/GI Off | Automatic, GI Off | Automatic, GI On |
| --- | --- | --- | --- |
| Kolgujev barracks | [original](evidence/automatic-material-qualification/cain-legacy.png) | [off](evidence/automatic-material-qualification/cain-auto-off.png) | [on](evidence/automatic-material-qualification/cain-auto-on.png) |
| Nogova apartments | [original](evidence/automatic-material-qualification/noe-legacy.png) | [off](evidence/automatic-material-qualification/noe-auto-off.png) | [on](evidence/automatic-material-qualification/noe-auto-on.png) |
| Malden, house 37339 | [original](evidence/automatic-material-qualification/abel-legacy.png) | [off](evidence/automatic-material-qualification/abel-auto-off.png) | [on](evidence/automatic-material-qualification/abel-auto-on.png) |
| Everon, Saint Pierre house | [original](evidence/automatic-material-qualification/eden-legacy.png) | [off](evidence/automatic-material-qualification/eden-auto-off.png) | [on](evidence/automatic-material-qualification/eden-auto-on.png) |

The house cameras target WRP `dum_mesto3` placements 37339 on Malden and 139 on
Everon. The earlier Le Port view contains related `dum_mesto2` houses and was
used only during investigation, not as a `dum_mesto3` acceptance example.

Because no new pilot facade qualifies, these comparisons validate conservative
fallback and existing transport. They cannot prove a new building-surface PBR/GI
gain. GPU timings measure this conservative prototype's current cost, not the
cost of hypothetical broad facade adoption. No global coverage claim is made.

RTX 4060 Ti, driver 617.14, stationary TAA with actual 100% world/output sizes,
RT sunlight/cloud shadows On and AO/volumetrics/water optics Off. Each mode
settles three seconds and measures four; ordering is enhanced/auto/auto/enhanced,
one bounce, probe writes Off. Values average the two interval medians. The raw
interval samples, raster/CPU scopes and status strings are retained in the receipt.

| Scene | GPU frame enhanced / auto (ms) | GI trace enhanced / auto (ms) | CPU scene preparation delta (ms) |
| --- | ---: | ---: | ---: |
| Kolgujev 1080p | 2.471 / 2.505 | .160 / .162 | +.031 |
| Nogova 1080p | 2.442 / 2.457 | .186 / .188 | +.035 |
| Malden 1080p | 5.659 / 5.725 | .315 / .323 | +.037 |
| Everon 1080p | 3.616 / 3.682 | .278 / .280 | +.017 |
| Nogova 1440p | 3.550 / 3.549 | .238 / .240 | +.034 |
| Malden 1440p | 5.846 / 5.874 | .273 / .273 | +.031 |

These short intervals do not isolate tiny GPU deltas from clock/idle variation.
Malden is CPU-bound at about 14 ms of command work; the lower 1440p GI trace
time is not evidence of a resolution speedup. No new pilot facade is admitted,
and these numbers cannot predict the cost of a future broad adoption. Source
evidence is cached per texture; original data-dependent section overrides and
triangle ranges still add a small observed CPU scene-preparation cost.

Both public game/test/tool builds pass. Focused material/shape/shader checks pass with 49,261
assertions in 67 Vulkan cases and 49,221 in 66 GL33 cases; 690 driver-free Vulkan
policy/lifetime checks pass. Eight final-runtime mission/regression/control runs present
41,094 frames with synchronization validation and zero validation errors/warnings.
An existing missing Epic overlay manifest still produces a separate loader
diagnostic; it is also present in the earlier survey logs, not a validation-layer
finding. The policy suite's intentional injected destruction error is not a game
validation finding either.

Ordinary TAA comparison captures are not pixel-identical. The receipt includes
their raw ROI differences without calling them PBR gains. A separate AA-Off
enhanced/auto/auto/enhanced round trip resolves this ambiguity: after settling,
the selected apartment wall, window and dark balcony-interior ROIs have **zero
changed pixels** between [automatic](evidence/automatic-material-qualification/noe-static-auto.png)
and [existing enhanced](evidence/automatic-material-qualification/noe-static-enhanced.png).
The earlier enhanced capture also differs from the later enhanced capture, so
startup/temporal settling must not be attributed to automatic material adoption.
This is a selected-region control, not a full-frame bit-exact replay claim.

The exact-window Pause regression moves the roof's receiver samples from native
2604/CPU 0 to native 0/CPU 2600 and restores native reception. Independently
captured catalog reception remains active (about 49,500-50,000 samples), and the
optional second bounce still produces qualified continuation hits. These are
completed half-resolution samples, not full-resolution pixel counts or coverage
percentages. All 59 original settings/fixture hashes and eight original asset/config
hashes match the fresh preservation snapshot. Private DLSS is clean, remote-free
and untouched; both SDKs and preserved build/runtime snapshots remain present.

The implementation provides reusable qualification and removes the section-wide
authored-lighting veto in the opt-in ray path. Safely expanding these particular
untyped atlases still needs stronger original evidence or a separately authorized
material annotation approach. This result does not weaken the ambiguity gate to
manufacture a positive coverage result.
