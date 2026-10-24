# Batch 41 — Grouped building assessments and intact material surfaces

From `main` at `4f2ba34`, **25 source assessments close** across 97 authored
visual LODs / 330 original placements. **Eight models gain rendering coverage**;
assessment closure also includes concrete decisions to retain original rendering.
The ledger becomes **405 complete / 79 partial / 31 pending / 211 retained-existing**,
leaving **110 unresolved models / 351 distinct pending keys**, down from 135 / 384.
Overall modernization remains ongoing.

| Closed family | Original models (`.p3d`) |
| --- | --- |
| Classic buildings, 12 | `data3d/budova1`, `budova2`, `budova3`, `budova4`, `budova4_in`, `budova5`, `hospital`, `bouda1`, `bouda2`, `bouda2_vnitrek`, `bouda3`, `dum01` |
| Nogova multistory houses, 10 | `o/hous/dum_patr_nizky_prujezd`, `dum_patrovy01`, `dum_patrovy01c`, `dum_patrovy01d`, `dum_patrovy01prujezd`, `dum_patrovy02`, `dum_patrovy03`, `dum_patrovy04`, `dum_patrovy05`, `dum_patrovy06` |
| Nogova arcades, 3 | `o/hous/dum_podloubi02`, `dum_podloubi03`, `dum_podloubi03klaster` |

The [source receipt](evidence/material-modernization/batch-41/source-audit.json)
groups all 135 initial unresolved models by shared physical sources and original
geometry/rendering constraints. Generic black fills do not merge unrelated
buildings into one assessment. The scoped 41 pending source identities receive
per-model dispositions, complete-fan checks and original-input provenance.
Town/chimney evidence from Batches 13/29/40 is reused. Previously failed gable
and vault/return regions remain withdrawn, without new rendering trials.

Eighteen exact-model regions, IDs **831–848**, add **800 highest-detail / 1,462
alternative-LOD triangle prerequisites** across eight models / 35 LODs / 142
placements. Global prerequisites become **29,231 / 57,163**.

| Model under `data3d/` | Added near / alternative prerequisites | Approximate near mesh area |
| --- | ---: | ---: |
| `bouda1.p3d` | 4 / 8 | 45.9 m² |
| `budova1.p3d` | 8 / 24 | 48.5 m² |
| `budova2.p3d` | 4 / 4 | 38.7 m² |
| `budova3.p3d` | 84 / 132 | 25.1 m² |
| `budova4.p3d` | 144 / 236 | 54.8 m² |
| `budova4_in.p3d` | 144 / 352 | 54.8 m² |
| `budova5.p3d` | 20 / 50 | 29.4 m² |
| `hospital.p3d` | 392 / 656 | 98.3 m² |

The approximately **396 m²** comprises 219 m² of plaster/foundations, 77 m² of
chimney/terrace minerals and 99 m² of coatings. Five coating regions preserve
original ambient; mineral regions use unchanged reception gates. Roughness .90,
F0 .04 and metallic zero are conservative approximations. Area includes buried,
hidden and underside mesh, excludes placement multiplication, and is not visible
screen coverage. The terrace source also covers slab/canopy faces, not solely
walkable paving. Alternative LODs are mutually exclusive geometry.

Broad roof faces cross unresolved hatch/patch artwork; broad facade fans include
windows, doors, fixtures or photographic canvas. Conservative clean bounds often
select no complete original fan. Other retained cases have explicit opening,
coating/substrate, sampler, original lighting or small-fragment limits. Arcade
relief trials already cut original fans; tiny out-of-bound balustrade details
do not justify reopening those reviews. Original assets, geometry, lighting,
transparency, samplers, rendering gates and GL33 behavior are preserved.

Six matched original Everon developer-camera views cover day/dusk PBR Off,
PBR On/GI Off and GI On. Four representative forced-CPU controls verify the
lighting toggles and restoration, including GI-off raster checks. Closer
[shared-family controls](evidence/material-modernization/batch-41/closer-controls.jpg)
supplement obstructed views. [Movement/LOD controls](evidence/material-modernization/batch-41/movement-LOD-controls.jpg)
cover advancing-simulation translation/turns and 0/8/22/52/92 m retreats; the
hospital's 92 m view is tree-obscured. No new wedges or GI seams were observed
in the reviewed controls. This is not physical traversal or certification of
every placement/LOD.

Controls on the fourteen unchanged rules demonstrate new normal-route facade
and terrace/mineral reception, plus captured CPU-lit reception. The lower-house
control changes from zero to **19,848 CPU facade-family pixels**; its normal
route remains gated in the sampled views. The final narrowed runtime repeats
shared-family day/dusk and hospital CPU raster/GI
[controls](evidence/material-modernization/batch-41/final-chimney-controls.jpg).
The enlarged chimney source check removes 190 near photo-margin/cap
triangles, about 13 m², before final qualification; original body fans remain
intact. Scene-family counters include scenery and are not identity masks or
coverage percentages. Independent reflector evidence is recorded separately
for unchanged and final narrowed rules in the qualification receipt.

RTX 4060 Ti / driver 617.14, native 1080p/TAA100%, auto PBR, one-bounce GI,
RT sun/cloud shadows; AO/water/volumetrics off. Each baseline/final runtime
warms ten seconds, then samples GI Off/On/On/Off with four-second settling and
six-second intervals. No builds, tests or censuses run during timing. Values
below are means of paired interval medians, whole-scene milliseconds.

| Original Everon view | GPU GI Off, before → after | GPU GI On | Trace + filter + history, GI On | CPU recording, GI On |
| --- | ---: | ---: | ---: | ---: |
| Barracks (`budova1`) | 4.123 → 4.093 | 4.315 → 4.379 | 0.520 → 0.535 | 10.431 → 10.560 |
| Hospital | 2.083 → 2.028 | 2.244 → 2.256 | 0.302 → 0.309 | 3.400 → 3.570 |

No significant GPU frame regression is established in these bounded samples;
GI-on lit draws/frame increase 980 → 984 / 302 → 328, with CPU recording
increases of .13 / .17 ms. The added ranges have a small submission/tracing cost.
Raw intervals, draw/bind counts, runtime hashes, native/CPU channels and probes are in the
[qualification receipt](evidence/material-modernization/batch-41/qualification.json).

Vulkan/tools and GL33 builds pass. Focused Vulkan tests pass **100 cases /
548,420 assertions**, GL33 **99 / 548,380**; road/surface ordering passes **17 /
52** in each build, Vulkan policy **690 checks**, inventory/census **19 Python
tests**. Both fresh 726-model censuses preserve all historical qualifications,
geometry, UVs, normals, flags and lighting. Vulkan synchronization validation
records zero errors/warnings. Live GL33 preserves three world ROIs exactly
across 15 unsupported PBR commands. All 270 original-input hashes and 67
settings/fixture/input hashes match; private DLSS remains clean, remote-free and
preserved with its SDKs and snapshots.
The existing missing EOS overlay-manifest loader message occurs in baseline and
final runs; startup succeeds, and it is outside the engine validation totals.

The most useful next groups are **25 classic house variants / 393 placements**
sharing facade atlases, **17 Nogova civic/village buildings / 298 placements**,
and the **four farm buildings / 131 placements**. Eight grave models sharing
two keys and four ruin-wall models sharing two keys offer efficient follow-up
assessment closure using prior evidence. The remaining gates, street fixtures,
industrial structures and props are grouped in the ledger; new coverage must
continue to earn its own source and rendering evidence.
