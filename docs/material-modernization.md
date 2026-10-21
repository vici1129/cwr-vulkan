# Original-world material modernization

This is ongoing work against the [726-model / 1,711-visual-LOD inventory](original-asset-inventory.md).
The [per-model ledger](evidence/material-modernization/progress.json) retains all
726 paths, original placement counts, reviewed and pending texture identities,
and concrete prerequisite blockers. It is not a visible-coverage percentage.
The original inventory remains an immutable baseline.

After batch 09, 60 models have complete source review, 90 have partial source
review, and 365 await source review. Another 211 have no source-unknown triangles
under the existing native classifier: their existing coverage and opacity,
authored-lighting or geometry fallback contracts are retained. This last group
is a prerequisite disposition, not a claim that all of its pixels are PBR or GI.
Backlog tasks 1–4, 6–8 and 24–27 have source review complete; the remaining tasks continue.
Batch 02 covers [brick pillars, bark and stacked logs](material-modernization-batch-02.md);
batch 03 covers [fence foundations, roof tiles and blank plaster](material-modernization-batch-03.md).
Batch 04 adds [original-triangle UV regions for farm masonry](material-modernization-batch-04.md),
while retaining the main windowed facades. Its visible change is deliberately
limited to soffits and occasional exposed foundations, not full building relighting.
Batch 05 adds [dedicated timber, roofs/plain finishes and exact chapel masonry](material-modernization-batch-05.md).
Three proposed Podhradi-specific sources were withdrawn after the dusk appearance
check; their mixed trim and partial-wall lighting boundaries remain pending.
The main Podhradi and apartment facades are not claimed as completed coverage.
Batch 06 adds [multiple physical regions in original barn atlases](material-modernization-batch-06.md)
across ten already-partial model reviews. One new board region participates in
PBR/reflection but retains its ambient reference after failed receiver trials;
the evidence explicitly distinguishes reflector prerequisites from reception.

Batch 07 [corrects LOD provenance and documents the canopy pilots](material-modernization-batch-07.md).
Batch 08 [assesses dedicated original bark across 35 shared models](material-modernization-batch-08.md).
Twelve source identities gain the existing PBR/GI-reflector paths; native auto
tree reception remains gated and CPU reception is independently demonstrated.
Mixed bent-birch/cut-end sources and canopy cards do not inherit the bark response.
Batch 09 [assesses sandstone, castle foundations and memorial stone](material-modernization-batch-09.md)
across 18 models / 53 authored levels. Seven dedicated sources and six exact-model
stone-tile regions gain existing paths; mixed plaques, soil, canvas, crosses and
unreviewed castle facades retain their specific unresolved boundaries.
The [ordinary-adapter census](evidence/material-modernization/runtime-census.json)
rechecks 726 models / **1,708 retained visual LODs** under the inspection configuration.
The separate [source-only census](evidence/material-modernization/source-census.json)
inspects all **1,711 authored visual LODs**. Three forest levels marked `notl=1` are
removed by the original hardware-lighting policy; the old parser mislabeled their
slots with later collision metadata. The immutable original inventory is unchanged.
Every retained level reproduces the previous complete prerequisites, including UVs;
the three source-only extra levels are explicitly outside that historical native
comparison. Section ordering is not an asset identity. Neither census certifies
live instance lighting, GI reception or optical review of every original image.
Backlog 28/29 retain their alpha-card and authored-lighting contracts after original
pixel/geometry review; this is not newly implemented foliage coverage.
Use the [batch-09 reproduction command](material-modernization-batch-09.md#tests-census-and-progress)
for the current executable; earlier batch commands describe their policy snapshots.

## Batch 01: Nogova rough stone and boundary masonry

Base: `67b2eb2f98b85b30dc3b8a81a65cdfe053701f5e`.
Four exact original identities join the existing enhanced-only material table:

| Original texture | Existing class | Roughness / family |
| --- | --- | --- |
| `o/skal/kamen02.pac` | Rough stone | .85 / rock |
| `o/skal/kamen04.pac` | Rough stone | .85 / rock |
| `o/skal/kamen05.pac` | Rough stone | .85 / rock |
| `o/hous/zidka01.pac` | Worn plaster, stone/mortar and masonry cap | .90 / stone |

Decoded original pixels were inspected locally. These images have no glass or
hardware regions requiring a different physical response. Cracks, dirt, baked
shading and the original color image are preserved; coefficients are existing
unmeasured rough-dielectric assumptions, not recovered optical measurements.
No texture, normal map, roughness map or replacement artwork was generated.

The rock images are each shared by five rock variants. The wall image also
appears on rubble, gate masonry and separately sectioned boundary walls attached
to houses. All **27 placed models / 85 visual LODs** using these images were
reinspected with the native loader, triangle qualifier and original config.
Original flags, normals, lighting indices, finite UV bounds and material source
identities are retained in the [source audit](evidence/material-modernization/batch-01/source-audit.json).
Repeating rock UVs and clamped wall atlas coordinates are diagnostic evidence,
not permission to infer a new material from UV extent. Seven shared models
retain other unknown sections; they are explicitly partial reviews.

The shared table supplies raster and ray identities 130–133 in `enhanced` and
`auto` modes. The 23-entry v3 table is unchanged. Existing per-triangle geometry,
authored-lighting, opacity, static-object and captured-ambient/direct-light gates
remain authoritative. Gate hardware, windows and unrelated textures inherit
nothing from adjacent masonry. GL33, defaults, presets, transport, ray budgets,
second bounce and original assets are unchanged. The highest-detail source
audit adds 3,230 prerequisite-qualified triangles across distinct models, without
placement multiplication or summing alternative LODs; this is an audit result,
not the adoption target or a rendering-coverage denominator.

## Actual gameplay and cost

Original Resistance `R01WarCry.Noe` gameplay was launched directly through the
existing developer harness, with copied profiles and `--nosound`. Standing
cameras view original WRP placements: the Lipany wall/rock enclosure and three
rock-family locations. This does not certify a normal Steam launch or mission
completion. Full-size, unamplified captures are retained alongside the receipt.

| Normal-distance scene | Before, enhanced GI | After, enhanced GI |
| --- | --- | --- |
| Wall enclosure | [before](evidence/material-modernization/batch-01/modernization-baseline-noe-1080-wall.png) | [after](evidence/material-modernization/batch-01/modernization-new-noe-1080-wall.png) |
| Mossy stone | [before](evidence/material-modernization/batch-01/modernization-baseline-noe-1080-rock1.png) | [after](evidence/material-modernization/batch-01/modernization-new-noe-1080-rock1.png) |
| Dark stone | [before](evidence/material-modernization/batch-01/modernization-baseline-noe-1080-rock2.png) | [after](evidence/material-modernization/batch-01/modernization-new-noe-1080-rock2.png) |
| Coastal stone | [before](evidence/material-modernization/batch-01/modernization-baseline-noe-1080-rock3.png) | [after](evidence/material-modernization/batch-01/modernization-new-noe-1080-rock3.png) |

The change is localized ambient replacement and rough material response.
Shaded stone and masonry become less uniformly filled; original detail remains.
New-source receiver counts are nonzero: one sample records 44,077 masonry pixels
and rock-family views record 35,827 / 145,982 / 16,779 rock pixels at the half-
resolution diagnostic footprint. These are frame samples, not world percentages.
Qualified reflected hits execute the existing visibility queries.

Same-runtime v3, enhanced/GI-off, sky-only and GI-on captures separate material
response from reflected surfaces. Selected SDR regions change GI-on versus
enhanced-off by -13.93% / -44.17% / +2.39% for the three stones and -21.75% for
the wall. GI-on versus sky-only changes only +1.63% / +1.78% / +6.57% and +1.80%:
most visible change is sky/ambient response, not a large sunlight bounce.
These are screenshot luminance comparisons, not HDR energy measurements.
Cloud/TAA variation and runtime LOD selection prevent bit-exact cross-run image
comparison; one coastal silhouette differs between runs. No geometry is edited.

RTX 4060 Ti; TAA, 100% world resolution, RT sun/cloud shadows, one bounce;
AO/volumetrics/water optics off. Each runtime warms for ten seconds, then uses
Off/On/On/Off, four-second settling and six-second sample intervals. The table
averages the paired interval medians. No build or other game runs during timing.

| Wall scene | GI-on GPU frame, before / after | GI trace, before / after | Trace+filter+history, before / after |
| --- | ---: | ---: | ---: |
| 1920×1080 | 4.181 / 4.223 ms | .515 / .729 ms | .718 / .968 ms |
| 2560×1440 | 5.967 / 6.444 ms | .706 / 1.088 ms | 1.061 / 1.512 ms |

The added GI work is approximately .25 / .45 ms in these views. GPU frame cost
rises .042 / .477 ms. CPU command recording remains around 9.6–9.7 ms; the 1080p
full-frame delta is therefore not an estimate of isolated GI cost. These two
measurements do not establish a universal budget or performance improvement.

Morning, overcast and dusk were visually inspected, along with moving/settled
TAA and FSR Quality cameras and two-bounce operation. MSAA4 executes and validates,
but its existing world-stable single-sample/spatial-only GI is visibly grainy on
the new large receivers. It is **not** a clean visual acceptance result for MSAA;
no filtering, extra samples or acceptance gate was changed to hide this limit.
The temporal modes provide the demonstrated normal-gameplay result.

The four Vulkan runs presented **67,400 frames**, synchronization validation on,
zero errors and warnings. Public Vulkan and GL33 builds pass. Focused Vulkan
material/geometry tests pass 54 cases / 14,596 assertions, shader tests 15 /
34,760, GL33 focused tests 68 / 49,316, inventory/parser tests eight, and driver-
free Vulkan policy checks 690. The intentional policy-test teardown error is
not a gameplay validation finding. The live GL33 Off/On/Enhanced/Off control has
zero changed pixels in its original world ROI.

All **270 inventory input hashes** and **67 settings/fixture/input hashes** match
their retained manifests. Private DLSS remains clean and remote-free, with both
SDKs and preserved build/runtime snapshots present. Full measurements, runtime
SHA256s, counters, probes, original-input preservation and sampling limits are
in the [qualification receipt](evidence/material-modernization/batch-01/qualification.json).
Task-local commands and raw captures are retained in
`build/material-modernization-20261021/` and `build/shadow-live/modernization-*`.

Work continues through farm/building UV regions and the remaining related
families. Metal, apertures, mixed photographic supports and window/door atlases
retain their specific unresolved boundaries in the ledger. A whole-atlas rejection
does not close a model review when existing triangles may safely separate a wall.
