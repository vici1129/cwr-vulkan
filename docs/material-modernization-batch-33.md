# Original ruined buildings

Base: `origin/main` at `b1187b7`. All **four source assessments close**:
17 authored visual LODs, 22 original placements and 21 distinct pending keys.
This is not a claim of full-model PBR or GI coverage. The
[source audit](evidence/material-modernization/batch-33/source-audit.json) gives
every pending key its own source, geometry, UV, lighting and retained-boundary
disposition, grouping shared photographs without changing their other users.

**25 exact-model regions, 591–615**, use the existing original-triangle path.
They pin original pixels, 17 geometry/binding fingerprints, complete original
faces and inspected sampler behavior. No whole-source alias is added. Wood uses
roughness .85; mineral/plaster/coarse finish uses .90, with F0 .04 and metallic
zero. These are conservative unmeasured dielectrics. Original shading, wear,
filled recesses and photographed green base color remain artwork; no recovered
albedo, normal maps, roughness maps, masks or separate fastener optics are claimed.

| Model | Regions / accepted source regions | Demonstrated live participation |
| --- | --- | --- |
| `data3d/dum_zboreny.p3d` | 591–600: broken mineral/brick wall edges, rubble floor, weathered exterior/interior plaster and pale ceiling finish; plain between-window finish exists only at an alternative LOD | Native plaster/mineral reception and eligible reflected hits |
| `data3d/dum_zboreny_lidice.p3d` | 601–604: low ruined masonry, reverse wall, rubble and broken wall thickness | Native mineral reception and eligible reflected hits |
| `data3d/dum_zboreny_total.p3d` | 605–611: taller ruined masonry, rubble, coarse mineral/beam-band finish, timber ceiling and beam ends | Native mineral/timber reception and eligible reflected hits |
| `data3d/dumruina.p3d` | 612–615: plain chimney sides/ends and narrow mineral opening returns | CPU chimney reception; native reception and new eligible reflected hits remain unproved. Returns are source prerequisites, not demonstrated broad-facade coverage |

The [ruined wall](evidence/material-modernization/batch-33/cain-dum_zboreny-right-comparison.jpg),
[total ruin](evidence/material-modernization/batch-33/cain-dum_zboreny_total-front-comparison.jpg),
[Lidice](evidence/material-modernization/batch-33/cain-dum_zboreny_lidice-front-comparison.jpg)
and [ceiling](evidence/material-modernization/batch-33/cain-dum_zboreny_total-interior-comparison.jpg)
controls retain coherent material boundaries. The ceiling darkens substantially
under GI, across its complete faces; no new diagonal or circular receiver patch
was observed. Frame samples show facade **0 → 138,038** at the main ruin, stone
**0 → 82,104** at the total ruin and **0 → 30,682** at Lidice. The upward interior
view records **193,065 native wood** receiver pixels. These half-resolution family
counts are view evidence, not identity masks or coverage percentages. Bounded
probes independently identify new eligible hits, including 593 (floor), 596
(wall), 601/605 (ruined masonry), 609 (ceiling) and 610 (beam ends).

`dumruina` retains zero new native facade reception in the tested Kolgujev,
Malden and Everon exterior headings, and in close chimney/recess trials. The
separate close CPU chimney control ([before](evidence/material-modernization/batch-33/baseline-cpu-cain-dumruina-chimney-CPU-auto-GI-on.png),
[after](evidence/material-modernization/batch-33/candidate-detail-cain-dumruina-chimney-CPU-auto-GI-on.png))
records **0 → 12,057 facade receiver pixels**;
the **126,974 roof** pixels there belong to existing identity 66. The recess view
does not demonstrate new reception on either route. No new eligible reflected
hit is observed for this model in bounded probes. Original ambient stays intact
where existing captured-input/static-instance gates do not admit participation;
no renderer or gate workaround is introduced.

Narrow V≤.88 masonry trials split 9/14 original total-ruin outer/inner fans and
13/4 Lidice fans. Broader complete-face rough candidates were built and tested
successfully instead. No admitted candidate was withdrawn for an appearance seam.
For `dumruina`, a broad gable box would include 132.33 m² of near geometry but
split four fans and cross roof/background/arch artwork. Ordinary broad wall fans
cross glass, doors or the diagonal pipe; mineral bands isolate no complete broad
wall faces. Its 16 remaining `domek1_fr` triangles are horizontal chimney caps
using a window-photo fragment as opening/trim artwork, without a separate mineral
cap/flue boundary. These concrete exclusions retain **524 alternative-LOD source
triangles**. The separate **132 render-behavior, 788 authored-lighting and four
degenerate triangles** retain their original gates. Details and UV bounds for
every key are in the audit, rather than a generic mixed-texture rejection.

Original `03Bomberman.CAIN`, `01TakeTheCar.ABEL` and `05HeavyMetal.Eden` missions
run through the existing direct developer harness, copied profiles and disabled
sound. Four headings per model/placement, three corrected ruin interiors and two
close detail views compare native PBR Off, PBR On/GI Off and PBR On/GI On at
09:00/17:30. **On means `triPBR 'auto'`**, which enables existing regional rules.
Advancing-simulation translation/turning and 0/8/22/52/92 m retreats check motion
and LOD behavior. Original photographic/LOD artifacts remain; no new material
trail or silhouette change was observed. These are developer-camera gameplay
observations, not physical traversal, Steam acceptance or mission completion.
An obstructed initial interior scout and an initially unframed ceiling view are
excluded; original controls were rerun at the corrected candidate cameras.

RTX 4060 Ti / 617.14, native 1080p, auto/TAA100%, one bounce, RT sun/cloud shadows;
AO/water/volumetrics off. Warm10, GI Off/On/On/Off, settle4/sample6, means of paired
interval medians. No compilation/census/tests during timing. Values are matched
scene workloads, including ordinary variation and other visible materials, not
isolated material costs or FPS gains. The Everon front workload includes foliage;
its clearer rear view supplies an additional appearance control.

| Native GI-on scene | Original → candidate GPU frame | GI trace/filter/history total |
| --- | ---: | ---: |
| Kolgujev main ruin | 3.136 → 3.403 ms | .487 → .945 ms |
| Kolgujev total ruin | 3.240 → 3.362 ms | .323 → .483 ms |
| Kolgujev Lidice | 6.676 → 6.580 ms | .527 → .657 ms |
| Kolgujev `dumruina` | 3.817 → 4.070 ms | .622 → .637 ms |
| Malden `dumruina` | 4.517 → 4.596 ms | .749 → .756 ms |
| Everon `dumruina` | 2.367 → 2.381 ms | .279 → .290 ms |

The [qualification receipt](evidence/material-modernization/batch-33/qualification.json)
retains actual states, camera/route controls, individual intervals, GI-off costs,
hit examples, hashes and limits. Public Vulkan/tools and GL33 builds pass.
Focused suites pass **95 cases / 169,092 assertions Vulkan**, **94 / 169,052 GL33**;
road ordering **17 / 52** each, **690** policy checks and **19** inventory/census
tests. Vulkan validation: **223,466 presented frames, zero errors
or warnings**. GL33 preserves four model views across five PBR commands with
zero changed world-ROI pixels. All 270 original-input and 67 settings/fixture
hashes match; private DLSS, SDKs and preserved snapshots remain intact.

Fresh censuses cover **726 models / 1,708 retained / 1,711 authored visual LODs**,
with zero historical replay differences. Baseline prerequisites use the committed
`b1187b7` source census; fresh UV/geometry inspection and native comparison runs
provide independent current evidence. Extend Batch 32's existing regional replay
through **615**, retaining its exact texture/identity-86 alias replay and existing
enhanced interval 130–207. Qualification functions, prior entries, shaders,
transparency, authored lighting, geometry, original assets and GL33 are unchanged.

New prerequisites: **1,117 highest-detail / 2,455 alternative-LOD triangles**;
totals **20,226 / 38,631**. These include source-qualified `dumruina` regions with
the live limits above, not visible coverage. Assessment totals become **356
complete / 119 partial / 40 pending / 211 retained existing contracts**. The
backlog is **159 models**, with **452 distinct pending assessment keys** elsewhere.
All 21 keys in these four models have explicit final source dispositions.
Stop after Batch 33.
