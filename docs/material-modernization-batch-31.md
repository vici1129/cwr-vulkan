# Original fences and gates

Base: `41d1dc80293160a9e2bf95a50ad92d02ea5eb2b4`. All **11 models / 36 visual
LODs / 326 original placements / five pending texture keys** have complete source
assessments. **Newly qualified materials: none. New rendering coverage: zero
models, identities, qualified triangles or GI receivers.** All five keys retain
original rendering with explicit reasons; completion is source assessment.

| Shared arrangement | Resolved original model paths | LODs / placements |
| --- | --- | ---: |
| Green posts, braces and separate gate artwork | `data3d/plot_green_` + `brank_o`, `branka`, `vrat_l`, `vrat_o`, `vrat_r`, `vrata` (`.p3d`) | 24 / 101 |
| Flaking-painted cylindrical posts, black caps and cutout wire | `o/hous/dd_pletivo.p3d`, `dd_pletivo_dira.p3d`, `dd_pletivo_sl.p3d` | 6 / 88 |
| Corrugated panels and narrow support strips | `o/misc/plot_vlnplech1.p3d`, `plot_vlnplech2.p3d` | 6 / 137 |

The [source audit](evidence/material-modernization/batch-31/source-audit.json)
records original provenance and pixel fingerprints, all shared users, every LOD's
sections/lighting/UV gates, original triangles and diagnostic region bounds.
All 12 used textures were decoded and inspected, including seven separate
transparent gate/leaf/wire sources. No global shared identity is adopted.

| Assessed and retained source | Original-pixel and geometry decision |
| --- | --- |
| `data/sloupek_vrata.pac` | 32x256 worn green post photograph with baked shading and coating/substrate variation; 370 untyped triangles. Full-height faces cross the source; central bounds isolate no complete triangle. Retains Batch 03's optical contract. |
| `data/zlenej_blivajz.pac` | 8x64 dark green hardware texture; 91 untyped triangles. Narrow brace faces repeat V approximately -1.076 to 6.020. Unit-square bounds select no complete triangle; green color does not establish uniform coating optics. |
| `data/blck_sum.pac` | 32x32 opaque black utility fill; 60 untyped post-cap triangles. Tiny caps and black pixels do not establish paint/metal albedo or response. |
| `o/misc/dd_rust_tile03.pac` | 128x256 peeling green coating and pale exposed/weathered substrate; 92 untyped triangles. Full-height cylindrical sides cross the mixture and repeat U; upper-strip/interior bounds isolate no complete ordinary triangle. Lower panel LODs retain 24 invalid-normal exclusions. |
| `o/hous/vlnityplot02.pac` | 512x512 photographed weathered sheet, seams, fasteners and baked corrugation shading; 40 untyped triangles. Lower-half bounds isolate all 12 broad panel triangles per model across three LODs, but retain mixed sheet/coating/weathering optics. Upper bounds select only eight small support triangles per model; a central patch selects none. No mineral, conductor or coating reconstruction is justified. |

All **653 source-unknown triangles** now have assessed retained-original reasons.
Another **707 render-behavior**, **96 authored-lighting** and **24 geometry**
exclusions remain. Corrugated supports retain original material 207 lighting;
untextured simplified faces and alpha gate/wire sections retain their flags.
No geometry, UVs, normals, samplers, lighting, opacity/order or safety gates change.
Shared `blck_sum` and `vlnityplot02` users outside the batch remain independently
scoped; these two keys can therefore remain pending elsewhere.

Original Nogova `R01WarCry.Noe` runs native Windows Vulkan through the existing
muted developer harness and copied profile. All **281 Nogova placements**, covering
all 11 models, match original horizontal dumps within .051m. The other 45 placements
are inventory scope, not live checked; differing height conventions prevent a
full-transform equality claim. Grounded front/back views check the
[open pedestrian gate](evidence/material-modernization/batch-31/fence31-plot_green_brank_o-17417-front-native-auto-GI-on.png),
[closed double gate](evidence/material-modernization/batch-31/fence31-plot_green_vrata-16744-front-native-auto-GI-on.png),
[damaged wire opening](evidence/material-modernization/batch-31/fence31-dd_pletivo_dira-4055-front-native-auto-GI-on.png)
and both corrugated variants. These are camera observations, not physical traversal
or mission completion. PBR-off and auto/GI Off/On controls retain the source
boundaries; corrugated [daylight](evidence/material-modernization/batch-31/fence31-plot_vlnplech1-159618-front-native-auto-GI-on.png)
and [dusk](evidence/material-modernization/batch-31/fence31-plot_vlnplech1-159618-front-dusk-on.png)
also retain the original photographic surface. Surrounding terrain/foundations
may receive GI; their counters are not new fence reception. No implementation
cost delta or material before/after GPU benchmark is claimed for unchanged bytes.

The [qualification receipt](evidence/material-modernization/batch-31/qualification.json)
records **17,060 presented Vulkan frames, zero validation errors/warnings**, and
GL33's five unsupported PBR controls with zero changed world-ROI pixels. Existing
focused suites pass **93 cases / 127,492 assertions Vulkan**, **92 / 127,452 GL33**;
road ordering passes **17 / 52** per build, plus **690 policy checks** and **18
inventory/census tests**. Hash-verified accepted Batch 29 binaries are reused;
no rebuild or global census rerun is claimed. Fresh 11-model / 36-LOD inspection
matches the current census exactly. All 270 original-input and 67 settings/fixture
hashes match; private DLSS, SDKs and preserved snapshots remain intact.

Ledger: **352 complete / 119 partial / 44 pending / 211 retained existing
contracts**. Global pending assessment keys: **468**; family pending keys: **zero**.
Global qualified prerequisites remain **18,685 highest-detail / 35,734 across
alternative LODs**, with zero added. These are not visible-coverage percentages.
Stop after Batch 31.
