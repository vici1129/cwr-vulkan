# Batch 38 — Original Nogova village houses

From `origin/main` at `411a493`: all seven pending source keys on four models,
14 visual LODs and 104 original placements are assessed. **24 new exact-model
regions and four existing foundation extensions** add 414 highest-detail /
914 alternative-LOD prerequisites. All 3,465 remaining source-unknown alternatives
have retained original-face or tested rendering dispositions. Assessment
completion is separate from rendering coverage.

| Original model under `o/hous/` | LODs / placements | New regions / extended identity | Added highest-detail / alternative prerequisites |
| --- | --- | --- | --- |
| `domek_sedy.p3d` | 4 / 53 | 714, 716–719 / 519 | 94 / 264 |
| `domek_sedy_bez.p3d` | 3 / 11 | 721, 723–726 / 520 | 98 / 244 |
| `domek_zluty.p3d` | 4 / 39 | 727, 730–731, 734–737 / 521 | 107 / 198 |
| `domek_zluty_bez.p3d` | 3 / 1 | 738, 742–743, 746–749 / 522 | 115 / 208 |

The [source audit](evidence/material-modernization/batch-38/source-audit.json)
reuses Batches 06/37's pixels, physical rows and geometry identities. Plaster,
mineral trim, foundations and paving/roof slabs use roughness .90; the separate
braced timber roof hatch uses .85. F0 remains .04 and metallic zero. These are
rough artistic approximations, not recovered optical measurements. Original
weathering, shadows and small photographed fittings remain artwork; no new
hardware metallic mask is inferred. All accepted regions admit the existing
receiver contract, subject to unchanged runtime gates.

| Pending source under `o/hous/` | Explicit disposition | Remaining source-unknown alternatives |
| --- | --- | ---: |
| `domsedy01.pac` | Four mineral ledge/base regions accepted; eight broad or narrow plaster regions built and rejected. Window/door photographs and original corner fans retained. | 1,246 |
| `domsedy02.pac` | Four complete plaster/cornice regions accepted. Large gables cross glazing; original underside faces repeat through mixed rows. | 318 |
| `domek_zluty1.pac` | Eight trim/mineral regions accepted; thirteen broad or narrow plaster regions built and rejected. Optical and cross-boundary fans retained. | 1,529 |
| `domek_zluty2.pac` | Six chimney plaster, mineral-detail and separate timber-hatch regions accepted. Gable glazing and inseparable full fans retained. | 364 |
| `dverein03.pac` | Each ordinary door quad spans timber, glass and handle: U .0272–.4829 / V .0314–.9979, near area 2.18 m². No wood-only original fan; 384 authored-lit alternatives also retain their gate. | 8 |
| `stodola01.pac` | Extend 519/520 to V≤1.003 with original ClampV required; extend 521/522 to bounded U ±4 in the same stone row. All older qualified triangles keep their identity and response. | 0 |
| `dum_podhradi1_5.pac` | Two exact-model mineral roof/step regions accepted. About 73.90 of 76.48 m² per near model is upward-facing exterior roof slope, using original paving artwork; it is not ground-paving coverage. Other source users remain pending. | 0 |

For example, grey underside faces83/84 span U .0074–.7255 / V .0032–2.0331,
covering 73.90 m² across repeated mixed rows; their counterparts are about .10 m
above them on the accepted mineral roof. Narrow right-jamb and white-band boxes
split original corner fans. The audit enumerates complete retained fans, UV and
position bounds, areas, flags and geometry-only candidate failures across all
LODs. Original windows, doors, transparency, authored lighting, samplers and
geometry gates remain; no whole mixed atlas is promoted.

The first 37-region trial produces clear PBR On / GI Off brightness patches on
the [grey](evidence/material-modernization/batch-38/trial-domek_sedy-front-day-PBR-on-GI-off.png)
and [yellow](evidence/material-modernization/batch-38/trial-domek_zluty-front-day-PBR-on-GI-off.png)
facades; forced-CPU controls expose failures on the closed variants too.
Thirteen broad plaster/window-side regions are withdrawn. Eight additional
complete lintel/sill regions are then built: the open houses again show
[raster wedges](evidence/material-modernization/batch-38/narrow-domek_sedy-front-day-PBR-on-GI-off.png),
while the closed houses show native-GI continuity wedges on the
[grey front](evidence/material-modernization/batch-38/narrow-domek_sedy_bez-front-day-PBR-on-GI-on.png)
and [clear yellow rear](evidence/material-modernization/batch-38/narrow-domek_zluty_bez-left-day-PBR-on-GI-on.png).
Their GI-off CPU controls are distinguished from those GI failures. All eight
withdraw: **17 raster and four native-GI rejections total**, not weathering-based
exclusions. No reflector-only substitution, shader or geometry workaround.
The accepted [grey](evidence/material-modernization/batch-38/final-domek_sedy-front-day-PBR-on-GI-off.png)
and [yellow](evidence/material-modernization/batch-38/final-domek_zluty-front-day-PBR-on-GI-off.png)
controls clear the demonstrated patches. The exact tested 24-region executable
is retained after the rejected narrow experiment; rebuild hashes are separate
in the [qualification receipt](evidence/material-modernization/batch-38/qualification.json).

Original `Resistance/R01WarCry.Noe` placements use copied profiles, muted direct
developer launch, 1920×1080/TAA100%, PBR Off / auto-GI Off / auto-GI On at 09:00
and 17:30, verified CPU toggles/restoration, advancing-simulation camera
translation/turns and 0/8/22/52/92-metre retreats. This is not physical traversal,
Steam-route or mission-completion acceptance. Trees, terrain, neighbors and a
truck obstruct some angles/far retreats; not every runtime LOD or placement is
certified. Original LOD artwork and silhouette differences remain. The
[motion](evidence/material-modernization/batch-38/motion-controls.jpg),
[CPU](evidence/material-modernization/batch-38/CPU-controls.jpg) and
[distance](evidence/material-modernization/batch-38/distance-controls.jpg)
sheets retain these limits.

New **native reception** is observed on the closed variants: the grey front
changes from zero facade/stone samples to 951/28,543 native receiver pixels;
the yellow front gains 5,281 native facade pixels. The open grey/yellow fronts
instead show **captured CPU-lit reception**, including 1,907/5,303 facade pixels
versus zero before. Normal hardware-T&L operation can contain CPU fallback.
These family counters include scenery and are not per-model masks or coverage
percentages. Independent bounded probes hit new reflector identities **724,
726, 738, 742 and 746**. Other admitted identities are not independently hit;
reflector endpoints do not prove reception or native vertex transformation.

RTX 4060 Ti / driver 617.14; automatic PBR, one-bounce GI, RT sun/cloud shadows,
AO/water/volumetrics off. Warm 10 seconds; GI Off/On/On/Off, settle four seconds,
sample six seconds. Means of paired interval medians, whole-scene milliseconds:

| Original front view | GPU frame GI Off, before → after | GPU frame GI On | Trace + filter + temporal |
| --- | ---: | ---: | ---: |
| Grey | 5.726 → 6.181 | 6.246 → 6.830 | .686 → .972 |
| Closed grey | 5.408 → 5.454 | 5.856 → 5.986 | .984 → 1.140 |
| Yellow | 5.830 → 4.848 | 6.251 → 6.963 | .751 → .915 |
| Closed yellow | 3.001 → 3.141 | 3.419 → 3.623 | .806 → .886 |

GI scopes increase .286/.157/.164/.080 ms. GI-on CPU command medians increase
15.301→16.999, 14.193→14.376, 15.711→17.534 and 7.980→8.431 ms. Additional
material ranges have CPU cost. The yellow GI-off decrease is not a speedup
claim; full-frame deltas are not isolated material/GI costs. No builds, tests
or censuses run during timing; forced-CPU performance is not separately timed.

Vulkan/tools and GL33 builds pass. Focused Vulkan: **97 cases / 340,046
assertions**; GL33: **96 / 340,006**. Road/surface ordering: 17 / 52 each;
Vulkan policy: 690 checks; Python inventory/census: 19 tests. Both fresh
726-model censuses retain 1,711 authored / 1,708 runtime LODs, reproduce historical
prerequisites and preserve old qualifications, geometry, UVs, normals, flags and
lighting metadata. Synchronization validation has **zero errors/warnings over
135,344 presented frames**, including trials/scouting/idle; the accepted runtime
presents 43,610. Live GL33 has zero changed world-ROI pixels across four views /
20 unsupported PBR commands. All 270 original input hashes and 67 settings,
fixture and input hashes match. Private repositories, SDKs and preserved
snapshots remain intact; full hashes and raw intervals are in the receipt.

All four assessments close. Global prerequisites become **24,786 / 48,962**;
the ledger has **377 complete / 107 partial / 31 pending / 211 retained-existing**
dispositions, leaving **138 models / 394 distinct pending keys**. Six keys leave
the global pending set; the paving source remains pending on other users.
The original 726-model inventory is unchanged. Stop after Batch 38.
