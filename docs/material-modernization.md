# Original-world material modernization

This is ongoing work against the [726-model / 1,711-visual-LOD inventory](original-asset-inventory.md).
The [per-model ledger](evidence/material-modernization/progress.json) retains all
726 paths, original placement counts, reviewed and pending texture identities,
and concrete prerequisite blockers. It is not a visible-coverage percentage.
The original inventory remains an immutable baseline.

After batch 38, 377 models have complete source assessment, 107 have partial source
review, and 31 await source review. Complete assessments include explicitly
retained optical cases, not only new material adoptions. Another 211 have no
source-unknown triangles under the existing native classifier: their existing coverage and opacity,
authored-lighting or geometry fallback contracts are retained. This last group
is a prerequisite disposition, not a claim that all of its pixels are PBR or GI.
Backlog tasks 1–4, 6–8, 14–16, 19–20 and 24–27 have source review complete; the remaining tasks continue.
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
Batch 10 [reviews fitted roads and shared bridge overlays](material-modernization-batch-10.md)
across 75 models / 106 levels. Seventeen original fractional-alpha sources retain
their blend, surface/depth and terrain-fitting contracts; no road rendering
coverage is added. The bridge's other sources are subsequently assessed in batch 27.
Batch 11 [adds exact original ruin-stone regions and assesses related plaster](material-modernization-batch-11.md)
across six models / 19 authored levels. Four whole-face stone regions use the
existing paths. Two plaster candidates were withdrawn after normal-view checks
showed no new facade reception; broad wall/cornice/window boundaries remain unresolved.
Batch 12 [adds dedicated original roof tiles and surveys related roof/chimney sources](material-modernization-batch-12.md)
across 64 models / 269 authored levels. Three dedicated sources use existing
paths, with explicit native-versus-CPU reception limits. Mixed holes, sheet/coating
ambiguity and photo borders remain unchanged. Identified brick-only chimney
faces are followed up in batch 13; the whole mixed chimney atlas stays untyped.
Batch 13 [adds exact original chimney brick regions](material-modernization-batch-13.md)
across 13 models / 50 authored levels. Whole original vertical brick faces use
existing paths; frame/photo borders, apertures and border-adjacent faces remain
untyped. This is modest roof detail, not broad facade relighting or completion
of those already-partial model reviews. Live route limits remain explicit.
Batch 14 [assesses recurring roadside signs/posts and adds dedicated concrete blocks](material-modernization-batch-14.md)
across 131 models / 177 authored levels. Only the two concrete marker/barrier
models gain a source identity. Rusted pipes, mixed paint/substrate, mounting
hardware and unsupported sign optics retain source-specific reasons. The 131
assessment dispositions are not 131 new rendering adoptions or coverage percentages.
Batch 15 [adds original bell masonry and ambient-preserving clay-tile regions](material-modernization-batch-15.md)
while reviewing 12 models / 51 authored levels. The bell's native reception stays
gated while CPU reception is demonstrated. A roof receiver trial failed CPU
appearance and was withdrawn; exact tile regions retain ambient through existing
PBR/reflector-only paths. Receiver-family safety tests were not relaxed. Mixed
facades remained pending; an identified castle masonry candidate was deferred
to a standing-view assessment, not counted as completed coverage.
Batch 16 [completes the castle wall source assessment with documented fallback](material-modernization-batch-16.md).
Broad stone/slit boundaries split original polygon fans; the narrow safe patch
is chiefly elevated broken wall caps with no demonstrated ordinary-view benefit.
No new material identity or rendering adoption is counted. Original recesses,
dark support treatment and authored-lighting gates remain intact.
Batch 17 [adds exposed concrete barriers, structure faces and firing panels](material-modernization-batch-17.md)
after inspecting 10 models / 37 authored levels. Barriers and panels demonstrate
native/CPU reception; the garage demonstrates CPU reception and reflected hits,
but zero native reception in its sampled view. The hospital's downward-facing
underside is not a visible walkway or claimed improvement. Original authored
triangles, dark siblings and ambiguous pole/optical surfaces retain their gates.
Batch 18 [assesses the remaining castle masonry family](material-modernization-batch-18.md)
across nine models / 33 authored levels. Only gate/corner stone and exact slit-free
faces are adopted; native reception remains zero in sampled views, with separate
CPU/reflection evidence. A second stone source is withdrawn because same-runtime
CPU controls expose a hard raster-lighting boundary even with GI off. Original
lighting is retained without shader or gate changes. Tiny rubble and mixed
roots/recess atlases receive explicit fallback reasons, not coverage claims.
Batch 19 [completes crooked-birch and apple source assessments](material-modernization-batch-19.md)
across two models / six levels / 1,037 placements. Mixed trunk/background and dark
utility sources retain specific limits; inspected bark-only bounds contain no
complete original faces. Existing blended tree cards and the mixed-birch
regression exclusion stay intact. No new rendering identity or coverage is counted;
the material policy and current census remain unchanged from batch 18.
Batch 20 [assesses guard booths, pallets and watchtower sources](material-modernization-batch-20.md)
across six models / 20 levels. Booth/pallet assessments close with specific source
limits; paint-only bounds split original faces or isolate tiny fragments. The
booth's ordinary quad is its roof, not its authored interior floor. Watchtower
metal/coating photographs retain their original treatment; shared painted boards
remain an actionable followup. No new material or rendering coverage is counted.
Batch 21 [adds bounded original hunting-stand timber paths and resolves the shared-floor followup](material-modernization-batch-21.md)
while surveying 16 models / 78 levels. New paths apply to eight stand placements;
four original timber identities gain existing paths, with native/CPU reception
demonstrated at the stand, while an exact roof
underside region retains original ambient after a circular receiver-patch failure.
The old church-board whole-source exclusion stays intact. All 510 shared-floor
authored triangles remain gated; unrelated building facades remain pending.
Stand/watchtower source reviews close, not 16 new rendering adoptions.
Batch 22 [adds limited CPU-route fountain mineral coverage and documents native/ray limits](material-modernization-batch-22.md)
across two models / seven levels / 16 placements. Separate classic mineral
sources and an exact lower Nogova stone row use existing paths. New reception
is demonstrated on the CPU-transform route only; native Nogova stone counts
were already present in surrounding scenery, and no new eligible reflected hit
is observed in bounded probes. Hidden slab planes are not an exposed plaza.
Mixed spout/vegetation/water/ornament photos and original capture gates remain.
Batch 23 [adds exact Helfenburk mineral rows and broken stone edges](material-modernization-batch-23.md)
across five models / 20 levels / six Everon placements. Normal-route castle
reception and bounded eligible hits are demonstrated on the reduced build.
One outer battlement region is withdrawn for a same-runtime diagonal raster
boundary, despite passing source tests and producing an eligible hit. The
detached wall's native reception remains unproved; CPU reception and reflected
hits are separate. Mixed aperture/composite/ground/finish sources stay original.
Batch 24 [adds original pier decks, wood shingles and exact dry upper beams](material-modernization-batch-24.md)
across three models / 14 levels / 12 placements. Native and CPU reception are
demonstrated on visible roof/beam surfaces and an exposed Nogova deck. No new
eligible reflected hit is observed in the bounded probes. Initial unmatched
waterline states are excluded; explicit matching simulation clocks retain the
original ocean and submerged deck. Wet/green posts, authored ceiling and mostly
submerged end-board sources retain concrete limits. Original assets and water
rendering are unchanged.
Batch 25 [adds scoped lighthouse stone participation](material-modernization-batch-25.md)
across three models / 10 levels / 17 placements. CPU reception and normal-route
eligible reflected hits are demonstrated; native reception remains zero in the
sampled views. Crown, balcony, mixed door, optical and distant stair sources
retain concrete reasons. Fresh fixed-view and advancing-simulation controls
align, but a frozen post-AA view mismatch remains unresolved and is excluded
from matched appearance acceptance. No renderer or gate workaround is made.
Batch 26 [adds scoped original quay paving participation](material-modernization-batch-26.md)
across two models / seven levels / four placements. CPU paving reception and a
normal-route main-upper reflected hit are demonstrated, not new native paving
reception. The southern native stone footprint was already present in surrounding
scenery. Mixed door/cap, green vertical seawall, corroded shed and utility sources
retain concrete reasons. A dusk band is isolated to existing RT sunlight shadows
and moves with sun time; shadows are restored without any renderer/gate workaround.
Separate forced-CPU GPU cost rises materially and is reported, not recommended.
Batch 27 [adds original bridge support and bounded curb concrete](material-modernization-batch-27.md)
after inspecting `o/hous/most_stred30.p3d`, all four authored visual levels and
its three remaining source images. The 1,776 previously source-unknown triangles
are fully assessed: 400 gain existing concrete prerequisites, while 1,376 mixed
atlas/road triangles retain specific fallback reasons. Six alpha road-overlay
triangles keep their original contract. Native reception and eligible reflected
hits are demonstrated in original Nogova views; CPU reception is reported
separately. No lighting gate, geometry, shader or GL33 behavior is changed.
Batch 28 [assesses common classic brick-house facades](material-modernization-batch-28.md)
across three variants / 15 authored levels / 75 Malden placements. Broad original
wall faces cross photographed windows, doors or pipes; conservative brick bands
select no complete ordinary wall triangles, while the safe side-source region
selects only small chimney faces. No rendering change qualifies. Original
rendering and gates remain; these models stay partial with concrete regional
limits recorded. No new native reception or coverage is counted.
Batch 29 [reviews the complete Nogova multistory-building family](material-modernization-batch-29.md):
10 models / 38 authored levels / 84 original placements, with explicit dispositions
for all 19 pending texture keys. One dedicated roof source and 11 exact mineral
regions add 214 highest-detail / 592 alternative-LOD prerequisites across eight
models. Retained mixed facades, utility fills, gutter optics and captured-lighting
limits are not counted as new PBR coverage. All ten models remain partial; original
artwork, geometry, UVs, lighting, transparency and qualification gates are preserved.
Batch 30 [completes all original direction-sign source assessments](material-modernization-batch-30.md):
113 models / 113 authored levels / 119 placements, grouped into four post-face
arrangements. Mixed worn-paint/substrate photos and utility caps retain explicit
original treatment; transparent artwork stays gated. Original hashes establish
two byte-identical differently named post-source pairs, correcting earlier
non-alias wording without extending the inherited signpost exception. All 113
reviews close with zero new rendering coverage or qualified triangles. Global
pending assessment keys after Batch 30 number 471; its family assessment is complete.
Batch 31 [completes the original fence/gate source reviews](material-modernization-batch-31.md):
11 models / 36 authored levels / 326 placements, grouped into green gates,
wire-fence posts and corrugated panels. All five pending keys retain explicit
original rendering: mixed coating/substrate, black utility caps and photographed
sheet optics do not establish a safe new material. Transparency, damaged openings,
authored lighting and invalid-normal exclusions remain. All 11 reviews close with
zero new rendering coverage or qualified triangles. Global pending assessment keys
number 468; none remains pending within this batch's 11 models.
Batch 32 [builds and tests the matching signpost and corrugated-panel candidates](material-modernization-batch-32.md).
The shaded signpost source reuses identity 86; exact-model fence regions 589/590
use rough dielectric PBR/reflector-only treatment. Original daylight/dusk, Off/On,
motion and native reflector checks qualify both without adding GI receivers.
The 424 highest-detail / 442 alternative-LOD prerequisite additions do not count
as new source assessments. All assessment states and the 163-model backlog remain.
Batch 33 [assesses the four original ruined-building models](material-modernization-batch-33.md):
17 visual LODs / 22 placements / 21 pending source keys. Exact-model regions
591–615 add mineral, plaster and timber prerequisites, with native reception and
eligible reflector hits demonstrated on the three Kolgujev ruin variants.
`dumruina` chimney reception is demonstrated separately on the CPU route; its
native reception and new eligible reflector hits remain unproved in the tested
Kolgujev, Malden and Everon views. Broad glazed/door/pipe faces and photographed
chimney-cap openings retain explicit original-face exclusions. All four source
assessments close without claiming full-model rendering coverage. Totals become
20,226 highest-detail / 38,631 alternative-LOD prerequisites, with 159 models in
the assessment backlog and 452 distinct pending assessment keys elsewhere.
Batch 34 [assesses the four original Nogova industrial buildings](material-modernization-batch-34.md):
19 visual LODs / 22 placements / all 16 pending keys. Twenty exact-model rough
dielectric regions add concrete, plaster and ambient-preserving coated-surface
prerequisites. Native station plaster reception, CPU factory lower-wall reception,
and independent native factory/transmitter reflector hits are demonstrated.
Factory native reception, near hangar participation and broad floor/roof GI remain
unproved or gated. Built gold-apron and bright window-strip failures are withdrawn;
original authored lighting, land, alpha and road/surface gates remain unchanged.
All four source assessments close without implying full-model rendering coverage.
Totals become 21,978 highest-detail / 43,556 alternative-LOD prerequisites,
with 155 models in the assessment backlog and 437 distinct pending keys elsewhere.
Batch 35 [assesses the four original churches and chapels](material-modernization-batch-35.md):
19 visual LODs / 49 placements / all 29 pending keys. Twenty-four exact-model
regions add rough plaster, mineral and ambient-preserving roof prerequisites.
The isolated shrine demonstrates new native base/side reception; church-wall
reception is demonstrated mainly on captured CPU-lit geometry. Independent new
native reflector hits identify shrine and chapel regions, not the two larger
churches in the bounded probes. Sixteen built raster failures are withdrawn,
including their separately tested reflector-only variants. Optical artwork,
whole-face atlas boundaries, authored lighting and sampler gates remain original.
All four source assessments close without claiming full-model PBR coverage.
Totals become 22,304 highest-detail / 44,246 alternative-LOD prerequisites,
with 151 models in the assessment backlog and 408 distinct pending keys elsewhere.
Batch 36 [assesses the original observation towers and scaffolding](material-modernization-batch-36.md):
four models / 17 visual LODs / 76 placements / all seven pending texture keys.
Eight exact-model rough coating regions add 1,862 highest-detail / 3,350
alternative-LOD PBR/reflector prerequisites while preserving original ambient.
No new native or captured CPU GI receiver is enabled. Transparent decks,
ladders, rungs and basket cards retain their original render-behavior gates.
All four source assessments close without implying full-model modernization.
Totals become 24,166 highest-detail / 47,596 alternative-LOD prerequisites,
with 147 models in the assessment backlog and 402 distinct pending keys elsewhere.
The shared `hlaska_sum` source remains pending on its other three model users.
Batch 37 [assesses the original Nogova barns and farm buildings](material-modernization-batch-37.md):
five models / 19 visual LODs / 176 placements / all four pending texture keys.
Seventeen exact-model timber, blank-plaster and mineral regions add 206
highest-detail / 452 alternative-LOD prerequisites. Seven built window-plaster
candidates are withdrawn after raster wedges persist with GI off and with
ambient-preserving treatment; two upper bounds narrow to complete blank walls.
Region 529 retains ambient. New captured CPU-lit reception is demonstrated;
new native reception remains unobserved, with independent reflector hits recorded.
All five source assessments close with concrete retained geometry and rendering
dispositions. Totals become 24,372 / 48,048 prerequisites, with 142 models /
400 distinct pending keys in the assessment backlog. This is not full-model
modernization or a visible-coverage percentage.
Batch 38 [assesses the original Nogova village houses](material-modernization-batch-38.md):
four models / 14 visual LODs / 104 placements / all seven pending texture keys.
Twenty-four new exact-model regions and four bounded foundation extensions add
414 highest-detail / 914 alternative-LOD prerequisites. Broad and narrowed
plaster candidates are actually built: 17 raster brightness failures and four
native-GI continuity failures are withdrawn. Mineral roofs/steps, foundations,
trim, chimney plaster and timber hatches retain useful adoption. Closed variants
demonstrate native reception; open variants demonstrate captured CPU-lit
reception, with independent reflector hits distinguished. All four assessments
close with 3,465 retained source-unknown alternatives explicitly disposed.
Totals become 24,786 / 48,962 prerequisites, leaving 138 models / 394 distinct
pending keys. Assessment completion remains separate from rendering coverage.
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
Backlog 30 likewise retains the fitted-road contract after explicit source review.
All 30 pilot tasks now have a disposition, but 16 remain partial source reviews;
the wider 726-model assessment and remaining defensible implementations continue.
Use the [batch-29 inspection command](material-modernization-batch-29.md#validation-and-reproduction)
with the [batch-33 census replay additions](material-modernization-batch-33.md)
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
