# Batch 39 — Original Nogova apartment blocks

From `origin/main` at `145bcb4`: all ten pending texture keys on three models,
13 visual LODs and 36 original placements are assessed. **29 exact-model
regions** add 922 highest-detail / 2,398 alternative-LOD prerequisites. The
remaining 10,940 source-unknown alternatives have explicit original-face or
tested rendering dispositions. Assessment completion is not full-model PBR
coverage.

| Original model under `o/hous/` | LODs / placements | Added highest-detail / alternative prerequisites |
| --- | --- | ---: |
| `panelak.p3d` | 5 / 26 | 230 / 616 |
| `panelak2.p3d` | 4 / 7 | 300 / 708 |
| `panelak3.p3d` | 4 / 3 | 392 / 1,074 |

The [source audit](evidence/material-modernization/batch-39/source-audit.json)
reuses Batch 12's roof pixels and original geometry evidence. Roughness .90,
F0 .04 and metallic zero are conservative artistic approximations, not measured
optical coefficients. No atlas is promoted wholesale. Original geometry,
UVs, normals, opacity, lighting, sampling and transparency gates remain.

| Pending source | Accepted / rejected / retained-original disposition |
| --- | --- |
| `data/blck_sum.pac` | Retain black detail/occlusion fill; constant UV 0,0 and black pixels establish no physical class. 2,744 unknown alternatives remain. |
| `panel01.pac` | Three substantial blank-panel candidates built and rejected after CPU-lit GI-off brightness patches; lower window-side boxes split original fans. Windows and all 4,770 unknown alternatives remain original. |
| `panel02.pac` | Nine concrete structural-row / grille-free fragment regions accepted (128.12 m² across the three near models). Main walls repeat through the ventilation grille; lower-strip candidates cut original fans. 1,237 unknown alternatives remain. |
| `panel03.pac` | Three complete rough mineral bands accepted as PBR/reflectors with ambient preserved (348.20 m²). Receiver trial fails dusk GI continuity. Upper hooks, lower entry/glazing and partial border fans remain original; 1,813 unknown alternatives remain. |
| `panel05.pac` | Three rough coating roof-edge rows accepted with ambient preserved (29.05 m² total near area), plus three tiny plaster edges (.249 m²). Broad ordinary face spans the glass blocks and stays original. 126 unknown alternatives remain. |
| `panel_dvere01.pac` | Four painted stile regions accepted as ambient-preserving rough PBR/reflectors, 2.05 m² near area. Glass, handles, other frames and authored-lit panels retain their gates; 12 unknown alternatives remain. |
| `panel_inter01.pac` | Two complete lower-wallpaper candidates built and rejected after CPU-lit GI-off triangular patches. Socket/trim and authored walls stay original; 24 unknown alternatives remain. |
| `panel_lod.pac` | Two regions accept complete concrete roof-edge triangles on the shorter variants, only 1.279 m² per far model. Larger end-wall quads repeat through window artwork. Tower strip cuts an original fan; window/entry photographs stay original. 210 unknown alternatives remain. |
| `panel_strecha.pac` | Three exact-model rough coating regions built and accepted, 534.40 m² near roof area. Original ambient retained; no assumed bare-metal substrate or new receiver. Zero unknown alternatives remain for this source. |
| `panel_zabradli.pac` | Two tiny plaster-region details accepted, only .002 m² total near area. Metal strip and authored balcony panels retain their gates; four unknown alternatives remain. This is not broad balcony coverage. |

Unqualified full fans are enumerated with UV/position bounds, area, flags and
LOD. For example, the large foundation fans span U .0012–2.0035 /
V .1271–.9939 and 63.74 m² through repeated grille artwork. No rectangle can
remove that grille from those original faces. The tiny upper-row V overshoots
sample the same concrete bottom border; larger repeats through mixed rows
remain original. No clipping, wrapping only triangle corners, generated masks
or normal/geometry repairs manufacture material separation.

All 31 initial candidates and three later roof-edge coating candidates were
built and rendered. Three broad `panel01`
regions and two wallpaper regions produce captured CPU-lit brightness patches
or wedges with **PBR On / GI Off** and are withdrawn from raster and reflector
participation ([facade](evidence/material-modernization/batch-39/trial-panelak-front-CPU-auto-GI-off.png),
[wallpaper](evidence/material-modernization/batch-39/trial-panelak-interior-CPU-auto-GI-off.png)).
The separate 26-region [receiver trial](evidence/material-modernization/batch-39/receiver-trial-panelak3-front-dusk-PBR-on-GI-on.png)
exposes a broad dusk color break on the tower's `panel03` band with GI On. Rebuilding those same
three mineral regions with `reception=false` preserves ambient while retaining
PBR/reflectors ([final](evidence/material-modernization/batch-39/final-panelak3-front-dusk-PBR-on-GI-on.png)).
This failure is directly observed on the tower; the two shorter
variants adopt the same conservative contract. Roof and painted-frame receiver
variants were not tested or represented as failures. Thirteen accepted regions
preserve ambient; 16 admit reception subject to unchanged runtime gates.

Original `Resistance/R01WarCry.Noe` placements use copied profiles, muted direct
developer launch, 1920×1080/TAA100%, PBR Off / auto-GI Off / auto-GI On at 09:00
and 17:30, verified CPU toggles/restoration, advancing-simulation camera
translation/turns and 0/8/22/52/92-metre retreats. Separate far matrices test
PBR/GI independently. Interior and elevated roof controls are supplementary;
this is not physical traversal, Steam-route or mission-completion acceptance.
The [motion](evidence/material-modernization/batch-39/motion-controls.jpg),
[CPU](evidence/material-modernization/batch-39/CPU-controls.jpg) and
[distance](evidence/material-modernization/batch-39/distance-controls.jpg)
sheets retain these limits. Tower front retreats encounter trees/neighbors;
the [left 22-metre view](evidence/material-modernization/batch-39/final-panelak3-left-retreat-22.png)
exposes full height with partial foliage obstruction. Both 220-metre front and
reverse scouts are terrain/shoreline-obstructed, not appearance acceptance.
Small far roof-edge identities lack independent ordinary-view appearance
proof. Not every runtime LOD or all 36 placements is certified.

No new **native reception** is observed in the three representative front
controls. Normal hardware-T&L operation contains **captured CPU-lit reception**
on the tower: concrete-family pixels change from 0/0 native/CPU to 0/8,749.
The shorter variants remain 0/0 on that normal route. Verified forced-CPU
controls change from zero concrete reception to **8,796 / 10,598 / 8,902** pixels
for `panelak` / `panelak2` / `panelak3`. Family totals include scenery and are
not per-model masks or coverage percentages. Independent bounded reflector
probes hit new identities **759, 762, 771 and 774**. Other admitted identities
are not independently hit; reflector endpoints do not prove reception or
native vertex transformation.

The bounded Batch 38 check uses its unchanged accepted executable at the same
yellow-house view: two log-verified hardware-T&L Off/On cycles, with samples
before/after captures after .1/1/3/8-second waits. All 33 CPU facade samples
stay between 5,298 and 5,329. The 5,303→312 collapse does not reproduce;
hardware state/capture timing do not explain the earlier isolated result in
this check. No renderer bug is established or fix made.

RTX 4060 Ti / driver 617.14; auto PBR, one-bounce GI, RT sun/cloud shadows,
AO/water/volumetrics off. Warm 10 seconds; GI Off/On/On/Off, settle four seconds,
sample six seconds. Means of paired interval medians, whole-scene milliseconds:

| Original front view | GPU frame GI Off, before → after | GPU frame GI On | Trace + filter + temporal |
| --- | ---: | ---: | ---: |
| `panelak` | 2.267 → 2.304 | 2.565 → 2.720 | .307 → .353 |
| `panelak2` | 2.339 → 2.350 | 2.256 → 2.872 | .253 → .359 |
| `panelak3` | 4.309 → 5.405 | 4.975 → 6.281 | .708 → .986 |

GI stages increase .045/.106/.278 ms. GI-on CPU command medians also rise
4.603→5.879, 4.931→6.156 and 11.750→14.836 ms. The tower's GI-off frame already
increases 1.095 ms; its full-frame increase is not solely GI cost. These are
bounded scene costs, not isolated material budgets or speedup claims. No
builds/tests/censuses run during timing; forced-CPU performance is not timed.

Vulkan/tools and GL33 builds pass. Focused Vulkan: **98 cases / 401,250
assertions**; GL33: **97 / 401,210**. Road/surface ordering: 17 / 52 each;
Vulkan policy: 690 checks; Python inventory/census: 19 tests. Both fresh
726-model censuses retain 1,711 authored / 1,708 runtime LODs and reproduce
historical prerequisites while preserving all old qualifications, geometry,
UVs, normals, flags and lighting metadata. Synchronization validation has
**zero errors/warnings over 306,851 presented frames** across six runs,
including trials/scouting/idle; the final runtime presents **77,133 frames**.
Live GL33 has zero changed world-ROI pixels across three views / 15 unsupported
PBR commands. All 270 original input hashes and 67 settings/fixture/input
hashes match. The private repository stays clean and remote-free; SDKs and
preserved snapshots remain present. Raw
intervals, runtime hashes, channels and probes are in the
[qualification receipt](evidence/material-modernization/batch-39/qualification.json).

The ledger becomes **380 complete / 104 partial / 31 pending / 211
retained-existing**, leaving **135 models / 385 distinct pending keys**.
Global prerequisites become **25,708 highest-detail / 51,360 alternatives**.
The original 726-model inventory is unchanged. Stop after Batch 39.
