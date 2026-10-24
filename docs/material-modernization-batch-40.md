# Batch 40 — High-throughput Nogova town modernization

From `origin/main` at `9ea17b9`, **all 13 targets gain material coverage** through
27 new exact-model regions and an extension of identity 584. The pass adds
**2,723 highest-detail / 4,341 alternative-LOD prerequisites**, across the scoped
50 visual LODs and 94 original placements. All 13 assessments remain partial.

| Original model under `o/hous/` | Added treatment | Added near / alternative prerequisites |
| --- | --- | ---: |
| `dum_patr_nizky_prujezd.p3d` | Balcony minerals, passage tiles, gutter/downpipe coating | 132 / 146 |
| `dum_patrovy01.p3d` | Balcony minerals and coating | 128 / 134 |
| `dum_patrovy01c.p3d` | Expanded balcony minerals and coating | 134 / 150 |
| `dum_patrovy01d.p3d` | Balcony minerals and coating | 132 / 144 |
| `dum_patrovy01prujezd.p3d` | Balcony minerals and coating | 132 / 148 |
| `dum_patrovy02.p3d` | Gutter/downpipe coating | 240 / 557 |
| `dum_patrovy03.p3d` | Gutter/downpipe coating | 169 / 405 |
| `dum_patrovy04.p3d` | Gutter/downpipe coating | 104 / 208 |
| `dum_patrovy05.p3d` | Gutter/downpipe coating | 121 / 242 |
| `dum_patrovy06.p3d` | Gutter/downpipe coating | 156 / 216 |
| `dum_podloubi02.p3d` | Plaster walls, balustrades, paving | 556 / 874 |
| `dum_podloubi03.p3d` | Plaster walls, balustrades, paving | 502 / 712 |
| `dum_podloubi03klaster.p3d` | Plaster walls and mineral relief | 217 / 405 |

The grouped rules qualify approximately **764 m² of additional near geometry**:
375 m² of arcade plaster/minerals, 137 m² of balcony minerals, 15 m² of passage
tiles, 98 m² of paving and 138 m² of coatings. Areas exclude placement
multiplication and are not screen-coverage percentages. Roughness .90, F0 .04
and metallic zero are conservative approximations, not recovered optical data.
Batch 29/13 pixels, source dispositions and geometry evidence are reused.
The gutter treatment uses a rough coating approximation without guessing a
bare-metal substrate; it does not reopen a demonstrated rendering failure.

Original windows, transparency, artwork, authored lighting, geometry, UVs,
normals and rendering gates remain. Existing roof coverage is reused; no new
roof or timber identity is claimed. Mixed window/shop/gate fans, the rejected
Batch 29 gable, chimney limits and tiny residual fragments remain original.
The [source receipt](evidence/material-modernization/batch-40/source-audit.json)
records the grouped rules and boundaries.

The vault/return group is **withdrawn**, not partially accepted. Initial native
GI controls exposed dark wedges; ambient-preserving rules then failed the forced-
CPU PBR On/GI Off raster control with bright wedges absent when PBR was Off:
[GI trial](evidence/material-modernization/batch-40/trial-vault-GI-on.png),
[CPU raster failure](evidence/material-modernization/batch-40/trial-CPU-GI-off.png).
All 11 rules are removed on all three shared models. The retained blank walls,
balustrades and paving have separate treatment. Ten coating regions preserve
ambient; 17 new regions and the balcony extension admit reception through the
unchanged runtime gates.

Six original `Resistance/R01WarCry.Noe` developer-camera views cover the major
variants with PBR Off / PBR On–GI Off / GI On at 09:00 and 17:30, verified CPU
toggles/restoration, advancing-simulation translation/turns and representative
0/8/22/52/92-metre retreats. Grouped control sheets,
[CPU](evidence/material-modernization/batch-40/CPU-controls.jpg) and
[movement/LOD](evidence/material-modernization/batch-40/movement-LOD-controls.jpg)
retain the evidence. The obstructed `02` rear baseline, balcony-floor scout and
terrain-blocked 92 m arcade retreat are not appearance acceptance. This is not
physical traversal, Steam acceptance or certification of every LOD/placement.

New **native GI reception** is observed: the `03` arcade front changes from
0 to 1,977 facade-family pixels and 18 to 6,285 mineral-family pixels; the
monastery side changes from 0 to 68,533 facade pixels. Verified forced-CPU
controls at the arcade front change from 0 to **1,099 captured CPU facade**
pixels and 22 to **5,149 CPU mineral** pixels. The balcony and passage views do
not establish new reception on either route. These are family/frame counts
including scenery, not identity masks or percentages.

Independent probes hit new reflector identities **796, 798, 802, 803, 812, 821,
822, 824 and 829**: coatings, balustrades, blank walls and paving. Other admitted
identities were not independently hit; reflector participation does not prove
native transformation or GI reception.

RTX 4060 Ti / driver 617.14, native 1080p/TAA100%, auto PBR, one-bounce GI,
RT sun/cloud shadows; AO/water/volumetrics off. The bounded Batch 39 follow-up
repeats its retained baseline (`145bcb4`) and accepted (`9ea17b9`) runtimes:
warm 10 seconds, GI Off/On/On/Off, settle four/sample six; means of paired
interval medians, whole-scene milliseconds. No builds/tests/censuses run during timing.

| Original view | GPU GI Off, before → after | GPU GI On | Trace + filter + temporal | CPU recording GI On |
| --- | ---: | ---: | ---: | ---: |
| `panelak` | 2.255 → 2.222 | 2.445 → 2.564 | .292 → .331 | 4.556 → 5.814 |
| `panelak3` tower | 4.226 → 5.217 | 4.976 → 6.192 | .692 → .991 | 11.805 → 14.715 |

GI-on lit draws increase **434 → 708 / 1,109 → 1,426**. Short-block lighting
binds stay 176 and texture binds 272; tower lighting binds rise 417 → 470 and
texture binds 597 → 598. Added material/draw ranges therefore explain submission
work beyond texture binding and GI tracing. Tower world raster time also rises
3.192 → 3.962 ms; its full-frame increase is not solely GI cost. Existing ranges
merge consecutive equal qualifications while preserving original order and
lighting boundaries. No local defect or safe isolated fix was established.
These are scene costs, not isolated material budgets or speedup claims.

Vulkan/tools and GL33 builds pass. Focused Vulkan tests: **99 cases / 462,912
assertions**; GL33: **98 / 462,872**. Road/surface ordering passes 17 / 52 in each
build, Vulkan policy 690 checks and inventory/census 19 Python tests. Both fresh
726-model censuses reproduce all historical prerequisites and preserve geometry,
UVs, normals, flags and lighting. Synchronization validation records **zero
errors/warnings**, including the final runtime.
Live GL33 preserves three world ROIs exactly across 15 unsupported PBR commands.
All **270 original input hashes / 67 settings, fixture and input hashes** match;
private DLSS stays clean and remote-free, with SDKs and snapshots preserved.

Inventory remains **726 models / 1,711 authored / 1,708 runtime LODs**. Global
prerequisites become **28,431 highest-detail / 55,701 alternatives**. The ledger
remains **380 complete / 104 partial / 31 pending / 211 retained-existing**, with
**135 models / 384 distinct pending keys**. Four per-model paving-source entries
close; no model assessment is forced closed. Raw intervals, hashes, channels,
probes and preservation checks are in the [qualification receipt](evidence/material-modernization/batch-40/qualification.json).
Stop after Batch 40.
