# Original-world PBR / RTGI reconnaissance

Base: public `e38ca72`, verified against origin/main. Eleven distinct locations
in five original missions span Malden, Everon, Kolgujev and Nogova. This is a
purposive sample, **not a global world-coverage percentage**.

The major gap is ordinary building envelopes, not absent terrain receivers.
Seven selected views have terrain-only GI reception; only the agricultural
compound records facade-family receivers. Rocks, original stone barriers and
qualified barns/roofs provide positive controls. Large town facades can remain
entirely legacy-lit even when the scene has PBR submissions and many GI rays.
No material was promoted and no renderer/shader was changed for this survey.

## Locations and actual counters

Existing muted mission/camera tools, copied profiles, 1920x1080 world/output,
TAA, 1400 m visibility, enhanced PBR and **one bounce** were used. RT sun/cloud
shadows were on; AO, volumetrics and water optics were off to simplify inspection.
Views are ground + 1.8 m, not aerial showcase cameras. Morning is 09:00;
overcast is controlled weather at noon; dusk is 17:30. Each location has ordinary
PBR v3 / enhanced GI Off / enhanced GI On captures and 24 existing sparse-probe
readbacks. These are original missions rendered through the diagnostic harness,
not mission-completion or normal Steam-launch qualification.

Counts below are completed-frame **half-resolution eligible receiver samples**
on a 960x540 grid, after the 45 m bound. They are not full-resolution object
pixels, a material inventory or a percentage of beneficial lighting. Object
receivers exclude terrain but include rocks, fences and poles, not just buildings.
`N/C` means native / captured CPU-lit. PBR values are submission-count deltas
divided by existing frame-count deltas; they measure work submitted, not area.
Qualified/rejected hits are a separate stochastic first-hit statistic.

| Location / light | Terrain receivers | Object receivers N/C | Qualified / rejected hits | PBR submissions N/C per frame | What the normal view shows |
| --- | ---: | ---: | ---: | ---: | --- |
| Nogova, Petrovice apartments / morning | 113,067 | 0 / 0 | 2 / 19,954 | 0 / 0 | `panelak2` dominates; concrete/window facade stays legacy. |
| Nogova, Lipany townhouses / overcast | 134,099 | 0 / 0 | 15 / 20,098 | 1 / 1 | `dum_patrovy02` and neighboring townhouses; no building reception. |
| Nogova, Mokra Vrata ruins / dusk | 211,667 | 0 / 0 | 23 / 7,071 | 0 / 0 | `ruiny_obvod_3`, cottages and trees; ground responds, masonry is unclassified. |
| Nogova, factory NW of Petrovice / morning | 209,353 | 0 / 0 | 13 / 20,543 | 0 / 0 | Large `tovarna1` envelope, industrial glazing and concrete yard remain legacy as objects. |
| Kolgujev, SE military base / morning | 89,313 | 0 / 0 | 1 / 5,130 | 0 / 20 | `budova2` row and surrounding barracks; PBR submissions do not establish visible receiver coverage. |
| Kolgujev, northern ruined roadside village / dusk | 109,207 | 0 / 0 | 14 / 17,630 | 3 / 0 | `dum_zboreny` family plus many alpha-tested surfaces. |
| Kolgujev, conifer hillside / overcast | 372,243 | 0 / 0 | 2 / 2,769 | 9 / 0 | Broad terrain reception; no inferred foliage diffuse transport. |
| Malden, Le Port residential street / morning | 122,218 | 2,600 / 49,997 | 32,211 / 9,292 | 2 / 12 | Native roof plus CPU-lit catalog stone wall, not the large house facades. |
| Malden, SW rocky coast / morning | 32,458 | 149,291 / 0 | 44,944 / 8,466 | 4 / 0 | Existing sandstone-rock PBR/GI is clearly participating. |
| Everon, Saint Pierre town edge / morning | 33,920 | 7,661 / 0 | 235 / 10,260 | 2 / 0 | Existing roof participation; dominant `dum_mesto2/3` facades remain legacy. |
| Everon, east-central farm / morning | 216,212 | 130,077 / 0 | 40,725 / 2,639 | 22 / 0 | 58,443 roof, 70,438 facade and 1,196 wood samples: qualified `stodola` and shed materials. |

No aggregate coverage ratio is calculated. Scene framing, sky, distance,
occlusion, LOD and material eligibility all affect these numbers. A large
receiver count does not measure the magnitude of a visible improvement. In
particular, terrain can receive calibrated visible sky while almost all
surface hits are rejected; that is not evidence of extensive building bounce.
The farm's normal GI Off/On change is comparatively subtle despite its much
larger qualified object footprint.

Six representative **unamplified, full-size GI On** screenshots:
[apartments](evidence/rtgi-world-survey/petrovice-gi-on.png),
[industrial factory](evidence/rtgi-world-survey/factory-gi-on.png),
[military base](evidence/rtgi-world-survey/base-gi-on.png),
[Le Port street](evidence/rtgi-world-survey/leport-gi-on.png),
[rocky countryside](evidence/rtgi-world-survey/rocks-gi-on.png),
[farm](evidence/rtgi-world-survey/farm-gi-on.png).
All eleven camera coordinates, three-mode local capture paths, per-family N/C
breakdowns, runtime hashes and probe examples are in
[the survey receipt](evidence/rtgi-world-survey/survey.json). Raw captures remain
under `build/shadow-live/world-survey-*`; no difference amplification was used.

## What actually blocks the missing surfaces

The original PBO models, decoded textures, native geometry normals, per-vertex
material indices and render flags were inspected. Twelve selected models were
examined, not a complete world/asset inventory. Texture names below are exact
keys; a similar basename under `o\hous` does not inherit a `data\` classification.

- **Classification is genuinely missing.** The exact 23-entry v3 table and two
  enhanced-only keys do not cover most residential, military or `o\hous` walls.
  Petrovice's sparse sample has 29 opaque static rejections with availability
  mask 39: position, geometric/shading normal, UV and accepted vertex normals
  are present, but no qualified diffuse color/identity is uploaded. This is not
  proof that those models lack normals. Nearest world-object positions alone
  are not used to assign an exact material to an arbitrary probe hit.
- **A texture section is not necessarily one lighting material.** Native
  `dum_mesto2` LOD0 has 2,505 vertices and 2,505 usable normals, but 1,657 vertices
  carry nonzero material indices. Its `dum2_m_meziwin.pac` section contains
  76 ordinary and 76 index-201 vertices; `dum2_patro_stena.pac` has 194 index-201
  vertices. Some of these are authored interior/recess/shadow ranges, not safe
  exterior candidates merely because the texture looks like plaster.
  `panelak2`'s large `panel01.pac` section has 980 ordinary, 16 index-205 and
  35 index-207 vertices, all with valid normals. Original
  [material definitions](../engine/Poseidon/Graphics/Rendering/Draw/SpecLods.hpp)
  and [lighting](../engine/Poseidon/Graphics/Rendering/Lighting/TransLight.cpp)
  define 201 as shadow-only, and 205/207 as shadow-only with 0.75/0.50 ambient
  scaling. These are not ordinary white diffuse materials or missing maps.
- **Current conservative gates are coarser on the ray side.**
  [Raster CPU capture](../engine/PoseidonVK/EngineVK_Software.cpp) already splits
  triangles by captured lighting slots and leaves unsupported ranges legacy;
  diffuse coefficients below .99 are not adopted. The
  [static ray collector](../engine/PoseidonVK/EngineVK_RTAO.cpp) requires every
  vertex in a stock section to have `ClipUserMask == 0`. One authored-shadow
  range can therefore prevent an otherwise ordinary part of the section from
  reflecting. Clearing that gate wholesale would destroy authored lighting.
- **Opaque alpha does not mean a homogeneous wall material.** Original
  `ujezd.pac`, `domek1_side.pac`, `o\hous\panel01.pac`, `panel03.pac`,
  `dum_patrovy02.pac` and `dum_patrovy02a.pac` artwork mixes masonry with baked
  windows, doors or hardware. The inspected PACs decode fully opaque. A rough plaster BRDF for every
  texel would incorrectly promote the painted glass/hardware as well.
  Conversely, `ujezd_up.pac`, `dum2_m_meziwin.pac`, `panel02.pac` and `panel04.pac`
  contain useful separable plaster/concrete regions. Native `panelak2` has
  600 and 50 ordinary-material vertices on the last two, with valid normals;
  `panelak` has 480 and 50. This is safer source data, not permission to promote
  the complete mixed facade automatically.
- **The barracks are not missing normals or trapped in CPU lighting.**
  `budova1` and `budova2` LOD0 have respectively 512/512 and 442/442 usable
  vertex normals, no nonzero material-index vertices and ordinary clamped/
  repeating flags. Their `blok_a_back.pac` is an opaque plaster gable;
  `blok_a_side.pac` and `blok_a_side_02.pac` bake windows/doors into the wall.
  The corrugated `blok_a_strecha.pac` cannot safely acquire guessed metallicity.
- **Intentional fallbacks remain important.** Alpha-tested rejection is not
  synonymous with foliage: Kolgujev's ruined-village probe has 36 alpha-tested
  rejections out of 42, including static-class hits. Original building holes
  and cutouts must remain real occluders/holes. Factory
  `tovarna1_stenaext.paa` is ARGB4444 with 27.8% zero-alpha and 17.7% partial-alpha
  texels; multiple factory sections use `IsAlpha | IsAlphaOrdered`, not ordinary
  opaque walls. Other factory solid sections are unclassified. Forest foliage,
  blended surfaces and water are not candidates for blanket opaque adoption.
  Six sampled forest terrain rejections have color/albedo but no accepted
  interpolated vertex normal; this identifies the existing normal-validation
  fallback, not a demonstrated absence of original source normals.

CPU lighting by itself is not the shared failure: Le Port's stone wall receives
GI through the captured CPU path, whereas ordinary native town meshes still
lack classified masonry. Native/CPU submission counts are never used as a
surrogate for visible material area.

## Three next implementation targets, ranked

This is a priority judgment from the sample and inspected data, not measured
future visual gains or a performance guarantee. Complete the qualified opaque
envelope of an archetype, retaining its authored glass/interior/metal fallbacks,
rather than adding another unrelated list of texture names.

1. **Classic town/village houses: `dum_mesto3`, then `dum_mesto2`.**
   These two focal models have 47 placements in the existing Malden/Everon
   receipts (29 / 18), and dominate the two newly visited residential streets.
   Begin with `dum_mesto3`: 471 valid normals, ordinary material zero throughout,
   existing supported `tasky_tmavsi` roof, and original plaster `ujezd_up` plus
   mixed `ujezd` / `domek1_fr` / `domek1_side` facades. The next useful work is
   **model/section/UV-qualified masonry participation shared by raster and hits**,
   not whole-atlas promotion. Only original geometry/UV regions that unambiguously
   exclude windows/hardware may be adopted; unresolved regions stay legacy.
   Reuse that result on `dum_mesto2` with triangle/range-level lighting
   eligibility matching existing CPU capture; do not turn index-201 interiors
   into ordinary sunlit reflectors. Potential benefit: shaded street walls and
   house/terrain exchange, rather than roofs alone. Effort **medium-high**;
   atlas isolation and authored-lighting fidelity are the acceptance risks.

2. **Nogova apartment envelopes: `o\hous\panelak` / `panelak2`.**
   Targeted queries find 26 and seven placements, respectively. The Petrovice
   view demonstrates a large, near-camera gap that the current small stock
   catalog misses completely. Start with verified plain concrete `panel02` and
   `panel04`, then complete demonstrably separable masonry portions of
   `panel01/03`, keeping glazing, balcony rails and shadow-only interiors
   distinct. Reuse range-level qualification from target 1; preserve 205/207
   ambient scaling wherever such ranges are eventually supported. There is no
   need to invent normal maps: the inspected main wall sections already have
   valid normals. Effort **medium-high** for the envelope, **low** for the initial
   plain-panel proof. Large projected wall area makes this the strongest regional
   visual candidate, but also the greatest new-receiver/ray-cost risk.

3. **Military plaster barracks: `budova1` / `budova2` (`blok_a_*`).**
   Existing Malden/Everon receipts contain 20 placements of these two models,
   with further repeated rows verified in the sampled Kolgujev base. This is
   the lowest-risk material-contract pilot: valid normals, ordinary lighting,
   opaque plaster and simple geometry. Treat the envelope as one task: blank
   gable, plaster-only side regions, original window/door exclusions, with the
   uncertain corrugated roof remaining legacy. Effort **low-medium**, provided
   original faces/UVs isolate those regions; otherwise scope must stop rather
   than generate replacement maps or guess reflectance. Expected benefit is
   localized ground/barracks shade and colored plaster reflection, not a
   promise to improve all military equipment or metal structures.

The factory is a lower priority despite its imposing screenshot: the targeted
query finds only one `tovarna1` placement, and its mixed/translucent/ordered-alpha
contract is substantially harder. Foliage is a separate transmission/two-sided
material problem; it should not be pursued just to raise hit eligibility.

For all three targets, adding reflector eligibility uses the existing primary
ray but can add sunlight/skylight visibility queries and texture data. Adding
receivers also increases the number of active half-resolution primary rays,
even with unchanged samples per eligible pixel. Per-triangle metadata/cache
cost and town-view GPU timings need measurement during implementation. This
reconnaissance does not extrapolate a millisecond cost from submission counts
or claim the previous Everon budget will survive broad facade adoption.

## Diagnostic change, checks and preservation

The sole code change adds optional read-only native lighting output to the
existing inspector:

```text
PoseidonTools model inspect <original-extracted.p3d> --lighting --sections
```

It reports the gameplay ODOL loader's vertex/normal counts, unusable normals,
land/light hints and per-section material-index histograms. Section order is
runtime-local; compare texture + flags + LOD, not section ordinal. These facts
are explicitly **not PBR/GI eligibility**: normal validity here checks finite,
non-negligible length, not the ray's facing/hemisphere tests. Default inspection is unchanged;
the extra native inspection reports MLOD as unsupported instead of substituting
preview/generated normals. Both public builds and manual ODOL/MLOD tool checks
passed. Existing focused model/material/shader/receiver checks pass: Vulkan
70 cases / 49,224 assertions; GL33 69 / 49,184. Five Vulkan mission runs presented
97,395 frames with synchronization validation and zero errors/warnings.

All 59 original configuration hashes and seven original asset hashes match the
fresh preservation snapshot. Private DLSS remains clean, remote-free and
untouched, with its SDKs present. Temporary build-helper target selection was
restored. PBR/RTGI remain developer-only/default Off; one bounce stays default,
the optional second bounce and all rendering functionality are unchanged.
No scanner framework, material editor, new artwork or large test matrix was added.
