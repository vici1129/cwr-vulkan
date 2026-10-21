# Batch 04: original-triangle regions in mixed farm atlases

Base: `6f96a39f73ba85c5738a740c0ba4fc47f33a9a5a`. The existing automatic
material path now supports explicitly inspected UV regions on original triangles.
This is **auto-only** (`triPBR 'auto'`), not a whole-atlas promotion or a default
change. Existing v3/enhanced tables and catalog entries remain unchanged.

The first family covers `data3d/statek_kulna.p3d` (54 original placements) and
`data3d/statek_brana.p3d` (23). Original decoded pixels, native triangle positions,
normals, lighting hints and UVs were inspected. Both `budova2_s` images contain
photographed windows; their lower strips contain only rough masonry/plaster.
Existing geometry independently samples those strips on foundations and gate
soffits. Roughness .90, F0 .04, metallic 0 use the existing unmeasured rough-
dielectric approximation, with original color and baked lighting retained.

| Region | Exact model / original texture | Original geometry adopted |
| --- | --- | --- |
| 512 | `statek_kulna` / `data/budova2_s.pac` | Two foundation triangles per visual LOD, UV V >= .675 |
| 513 | `statek_kulna` / `data/budova2_s2.pac` | Two foundation triangles per visual LOD, UV V >= .625 |
| 514 | `statek_brana` / `data/budova2_s.pac` | Masonry soffits, 14/6/2 triangles at resolutions .5/1/1.5 |

The shed has three visual LODs (.5/1/3); the gate's fourth visual LOD (2.25)
does not inherit a region. Across the **two models / seven visual LODs**, 34
original triangles gain prerequisites. The highest-detail increment is 18,
without placement multiplication. These are audit counts, not the objective or
a visible-coverage denominator. Every remaining shed side triangle spans the
window photo and cannot separate physical materials in the original triangulation.
It stays legacy. Mixed gable doors/clutter, gate boards and hardware remain
separate pending cases, not approved from adjacent masonry.

## Evidence and safety contract

The [source audit](evidence/material-modernization/batch-04/source-audit.json)
retains original source SHA256s, decoded dimensions/pixel fingerprints, reviewed
triangle UV bounds/vertical extents, normal prerequisites and all visual-LOD
qualifications. Full original vertex/index/normal dumps stay in the ignored local
cache, not published as reconstructable original mesh buffers.
A regional approval requires all of the following together:

- Exact model key, visual resolution, vertex/section counts and original geometry fingerprint.
- Exact ordered original texture bindings. The existing geometry fingerprint
  includes material indices but not their resource names; this additional gate
  prevents swapped texture assignments from inheriting a rule.
- Exact immutable original texture key, dimensions and decoded RGBA fingerprint.
- Every corner of the whole original triangle inside the inspected convex UV
  rectangle. No wrapping or clipping of a mixed face to manufacture a region.
- All existing ordinary-vertex, valid-normal/position/UV/area, authored-lighting,
  opacity/render flags, static-object and captured ambient/direct prerequisites.

Source identity remains **zero for the whole mixed atlas**. The diagnostic's
`regionalSources` field counts only accepted original triangles. Native raster
and ray submission share the existing cached material ranges. CPU submission
checks original indices/UVs and rejects clipping-generated regional triangles.
Unknown/rejected triangles retain lighting and traversal geometry; the existing
1024-geometry fallback still declines transport rather than dropping geometry.
No shader, map, artwork, original mesh, texture or lighting hint is generated or
edited. Existing catalog/signpost behavior, ray budgets and transport stay intact.

The fresh [726-model / 1,711-LOD runtime census](evidence/material-modernization/runtime-census.json)
still reproduces every original section prerequisite multiset after explicitly
replaying only enhanced identities 130–151 and regional identities 512–514.
There are zero other geometry, lighting, opacity or rejection-bucket differences.
Use the batch-03 reproduction command plus `--replay-regional-identities 512 513 514`.
That describes this batch's policy snapshot. For the current executable, use the
[batch-05 reproduction command](material-modernization-batch-05.md#runtime-census-tests-and-preservation).
This is not completed optical assessment of every original model.

## Actual gameplay and limits

The direct developer harness runs original Everon `05HeavyMetal.Eden` gameplay,
copied profiles and `--nosound`, with standing terrain-grounded cameras at original
WRP placements. It does not certify a Steam launch or mission completion.
Full-size unamplified evidence:

| View | Before | Final runtime |
| --- | --- | --- |
| Masonry gate soffit | [before](evidence/material-modernization/batch-04/modernization-b04-baseline-extra-eden-1080-gateclose.png) | [after](evidence/material-modernization/batch-04/modernization-b04-new-exact-eden-1080-gateclose.png) |
| Small exposed shed foundation | [before](evidence/material-modernization/batch-04/modernization-b04-baseline-extra-eden-1080-exposed1.png) | [after](evidence/material-modernization/batch-04/modernization-b04-new-exact-eden-1080-exposed1.png) |

The visible change is localized, subtle rough shading/ambient replacement, not
full facade GI. The gate sample has 2,355 native stone-family receiver pixels,
2,384 on the CPU route, and 2,355 again after exact-window native restoration.
The near standing [under-soffit view](evidence/material-modernization/batch-04/modernization-b04-new-exact-eden-1080-gateunder-scout.png)
has 41,244 native stone-family pixels. Bounded grid/phase probes independently
record **three distinct eligible paths hitting identity 514**, proving actual
regional ray endpoints, not merely a nearby-object name. No sampled foundation
identity hit is claimed; the bounded source-specific probes do not establish a
global transport distribution.

The geometry inspection also corrected the initial “wall strip” interpretation:
the shed quads extend **below the main facade**, usually below terrain. A bounded
[terrain survey](evidence/material-modernization/batch-04/foundation-exposure-survey.json)
samples nine points around all 49 Everon shed placements. It ranks candidates,
not exact face visibility. The best candidates have only small exposed areas;
normal standing captures confirm that limit. A same-runtime enhanced/auto control
at the selected hillside view changes stone-family reception from 0 to 258
pixels. Original window-bearing facades remain unchanged and unqualified.
This is not presented as a large normal-gameplay improvement.

Morning, overcast, dusk, moving/settled TAA and FSR Quality, lower-LOD retreats
and two-bounce operation were inspected. Texture resolution, original geometric
LOD changes and photographic shadows remain. MSAA4 validates but retains the
existing spatial-only GI noise; it is not certified as a clean visual result.
Neither extra samples nor altered filters hide that limitation.

## Cost, tests and preservation

RTX 4060 Ti; auto, one bounce, TAA, 100% world resolution, RT sun/cloud shadows;
AO/water optics/volumetrics off. Ten-second warmup, then Off/On/On/Off with four-
second settling and six-second samples. Values average paired interval medians.
Probes are off; no build or other game runs during timing.

| Gate view | GI-on GPU frame, before / after | GI trace, before / after | Trace+filter+history, before / after |
| --- | ---: | ---: | ---: |
| 1920×1080 | 2.906 / 2.936 ms | .134 / .137 ms | .243 / .257 ms |
| 2560×1440 | 3.553 / 3.563 ms | .123 / .131 ms | .245 / .259 ms |

Measured GI scopes add approximately .014 ms in either view; total frame changes
are +.030 / +.010 ms. CPU recording is around 6.8 ms, and Off/raster/clock variation
is comparable to the total-frame differences. No scaling law, speedup or universal
GPU budget is inferred from these small changes.

The **four final paired runs presented 42,776 frames**, synchronization validation
on, zero errors and warnings, with verified shutdown. Another 81,470 exploratory
frames are recorded separately; the pre-binding-hardening executable is not the
final acceptance runtime. The final Vulkan executable SHA256 is
`6D514B3CE7F866BDFBF968162864B065D6B76C23F4E00F1EAEF5FCD1305720CC`.

Public Vulkan game/tools/tests and GL33 game/tests build successfully. Focused
Vulkan tests pass 70 cases / 49,826 assertions; GL33 69 / 49,786; driver-free Vulkan
policy checks 690; Python inventory/census tests 15. Tests retain pixel-fingerprint,
wrong-model/LOD, swapped-binding, mixed-UV, authored-lighting, render-state and
missing captured-input rejection. Live GL33 Off/On/Enhanced/Auto/Off controls
remain unsupported no-ops, with zero changed pixels in the original world ROI.

All 270 original inventory input hashes and 67 retained settings/fixture/input
hashes still match. Private DLSS remains clean and remote-free; SDKs and preserved
build/runtime snapshots remain present. Full counters, runtime identities,
intervals and source-specific probe samples are in the
[qualification receipt](evidence/material-modernization/batch-04/qualification.json).

The ledger now records 31 complete and 32 partial source reviews, 452 awaiting
source review, and 211 retained prerequisite dispositions. The shed and gate
remain partial reviews. Work continues through related physical material families;
no whole-building or 100% rendering-coverage claim is made.
