# Batch 37 — Original Nogova barns and farm buildings

From `origin/main` at `63d6da0`: all four pending texture keys on five models,
19 visual LODs and 176 original placements are assessed. **17 exact-model
regions** add existing rough PBR/reflector paths; eight of them also admit the
existing GI receiver contract. No whole-source identity, renderer, shader,
ray budget, material mask, normal reconstruction or gate change.

| Original model under `o/hous/` | LODs / placements | New regions | Added highest-detail / alternative prerequisites |
| --- | --- | --- | --- |
| `stodola.p3d` | 3 / 73 | 689–691 | 16 / 58 |
| `stodola2.p3d` | 4 / 24 | 692 | 12 / 32 |
| `stodola3.p3d` | 3 / 35 | 693–695 | 48 / 86 |
| `zluty_statek.p3d` | 4 / 43 | 696–697, 699, 703 | 66 / 130 |
| `zluty_statek_in.p3d` | 5 / 1 | 704–707, 709, 712 | 64 / 146 |

These are **206 / 452** additional prerequisites, not projected coverage or
simultaneous triangles. Accepted complete highest-detail faces total about
177.56 m² in model space, including all inspected sides, not visible screen area.
The [source audit](evidence/material-modernization/batch-37/source-audit.json)
reuses Batch 06's pixels, material identities and geometry/binding evidence.
All older regions, including the rejected-receiver gate on **529**, are unchanged.

Barn gables and separate timber door/edge faces use roughness .85, F0 .04,
metallic zero and retain ambient. Photographed knots, dark holes, braces and
small fittings remain original artwork; no new transparency or metal mask is
invented. These add useful timber reflectors, not barn-wall GI reception.
Existing timber, roof tiles and masonry retain their earlier identities.

Farm blank plaster and original grey mineral ledge/foundation rows use .90
roughness, F0 .04 and metallic zero. Original repeated U on the blank wall
still samples plaster. The two interior door stiles require original ClampV
and qualify only at LOD1/2: their highest-detail edge overshoot lacks that flag.
All 508 authored door alternatives remain original. This tiny border addition
is not near interior-door relighting.

Substantial window-adjacent plaster was actually built and rendered. The first
24-region trial produces [bright wedges with PBR On and GI Off](evidence/material-modernization/batch-37/trial-zluty_statek_in-front-day-PBR-on-GI-off.png).
An independently built [ambient-preserving variant](evidence/material-modernization/batch-37/ambient-zluty_statek_in-front-day-PBR-on-GI-off.png)
retains the wedges. Five lower plaster/frame regions, then upper-window right
strips 698/708, are withdrawn. Regions 696/706 narrow from U≤.676 to **U≤.502**,
retaining ten complete blank-wall triangles per near LOD while excluding the
upper-window left jambs. The [final full-resolution control](evidence/material-modernization/batch-37/accepted-final-zluty_statek_in-front-day-PBR-on-GI-off.png)
clears those demonstrated wedges. This is a tested raster rejection and useful
partial adoption, not a generic mixed-atlas or uncertain-coefficients objection.

The remaining **2,998 source-unknown alternatives** have explicit dispositions.
For example, the farm's original face14 spans U .0068–3.9181 / V −.5079–.5102
across timber and masonry; its complete fan covers 63.74 m². Barn3's face31
covers 56.78 m² and crosses the wood/mineral row at V .5028. The interior farm's
middle-side bound splits original faces21/42 in LOD2–4, whose full V extends
through .9961/1.0006 into the foundation. Original fan bounds, areas, samplers,
authored lighting, residual border cases and rejected candidate boxes are
enumerated in the audit. No original UV, polygon, clamp, lighting or aperture
is repaired or bypassed to close the assessment.

Original `Resistance/R01WarCry.Noe` placements use copied profiles and the muted
direct developer harness, native 1920×1080/TAA100%, PBR Off / auto-GI Off /
auto-GI On, 09:00 and 17:30, advancing-simulation camera translation/turns,
verified CPU toggles/restoration and 0/8/22/52/92-metre retreats. This is not
physical traversal, Steam-route acceptance or mission completion. Terrain,
neighbors and vegetation obscure some far retreats; original LOD artwork and
silhouette changes remain. Not every runtime LOD selection or placement is
certified. The first barn's house-blocked scout is excluded.

New target native GI reception is **unobserved**. Captured CPU-lit reception is
demonstrated: the final normal-route interior-farm front has 4,387 CPU facade
receiver pixels versus zero in the baseline; the forced-CPU closed farm has
4,402. Normal hardware-T&L operation can contain CPU fallback. These family
counters include scenery and are not per-model masks or coverage percentages.
Independent bounded probes identify new reflector endpoints separately from
reception; identities and raw examples are in the
[qualification receipt](evidence/material-modernization/batch-37/qualification.json).
Observed new identities are 690, 692, 693, 696, 697 and 699; the other accepted
identities were not independently hit in these bounded probes. An eligible GPU
GI hit does not establish native vertex transformation. The
[motion](evidence/material-modernization/batch-37/motion-controls.jpg),
[CPU](evidence/material-modernization/batch-37/CPU-controls.jpg) and
[distance](evidence/material-modernization/batch-37/distance-controls.jpg)
sheets retain the original details and the explicitly obstructed far views.

RTX 4060 Ti / driver 617.14; automatic PBR, one-bounce GI and RT sun/cloud
shadows, AO/water/volumetrics off. Each view warms 10 seconds, then uses GI
Off/On/On/Off with 4-second settling and 6-second sampling. Means of paired
interval medians, whole-scene milliseconds, baseline → final:

| Original view | GPU frame, GI Off | GPU frame, GI On | Trace + filter + temporal |
| --- | ---: | ---: | ---: |
| Large barn | 2.221 → 2.189 | 2.365 → 2.365 | .466 → .473 |
| Closed farm | 2.207 → 2.250 | 2.368 → 2.280 | .387 → .394 |
| Interior farm | 5.040 → 8.055 | 8.658 → 9.280 | 1.630 → 1.721 |

The sampled GI scopes rise about .007/.007/.091 ms. The interior-farm intervals
vary materially, including GI-off GPU frame medians 6.401 and 9.708 ms in the
final run. Its GI-on CPU command medians average 22.038 → 23.785 ms; additional
material ranges have CPU cost. Full-frame differences are not isolated GI or
material costs, and the negative closed-farm delta is not a speedup claim.
No builds, tests or censuses run during timing; forced-CPU performance is not
separately measured.

Vulkan/tools and GL33 builds pass. Focused Vulkan: **97 cases / 303,813
assertions**; GL33: **96 / 303,773**. Surface/road ordering: 17 / 52 each;
Vulkan policy: 690 checks; Python inventory/census: 19 tests. Fresh 726-model
censuses retain 1,711 authored / 1,708 runtime levels, reproduce historical
section prerequisites and preserve all old qualifications, geometry, UVs,
normals, flags and lighting metadata.
Synchronization validation reports **zero errors and warnings across 149,944
presented frames**, including trials, scouting and idle frames; the final
accepted executable presents 33,068. Live GL33 has zero changed original-world
ROI pixels across all five views / 25 unsupported PBR commands. All 270 original
input hashes and 67 settings/fixture/input hashes match; private repositories,
SDKs and preserved snapshots remain untouched. A trial's interrupted last CPU
restoration/capture is excluded; the final paired CPU controls and restoration
pass. Full runtime hashes, intervals, source dispositions and limits remain in
the receipt.

All five source assessments close, not full-model modernization. Global
qualified prerequisites become **24,372 / 48,048**. The ledger has **373 complete /
111 partial / 31 pending / 211 retained-existing** dispositions: **142 models /
400 distinct pending keys** remain. Shared barn-atlas users remain pending;
only `zlutstat01` and `dverein01` leave the global pending-key set. The original
726-model inventory is unchanged. Stop after Batch 37.
