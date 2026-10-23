# Batch 35 — Original churches and chapels

From `origin/main` at `f25fe08`: four models, 19 authored visual LODs,
49 original placements, all 29 pending texture keys assessed. Twenty-four
exact-model regions across 17 keys use existing mechanisms. No whole-source
promotion, renderer infrastructure, texture, geometry, UV, normal or gate changes.

## Source decisions

Original pixels, triangle fans, UVs, normals, lighting/material indices and
samplers were inspected across all 19 levels, reusing chapel, masonry and roof
evidence from Batches 05, 12 and 21. Accepted weathering and baked shadows remain
artwork. Roughness .90 / F0 .04 / metallic zero are conservative approximations,
not measured coefficients. The [source audit](evidence/material-modernization/batch-35/source-audit.json)
records every key, per-LOD qualification, fingerprint and retained boundary.

| Original model | LODs / placements | Accepted regions | Added highest-detail / alternative prerequisites | Remaining source-unknown alternatives |
| --- | --- | --- | --- | --- |
| `data3d/kostel.p3d` | 5 / 15 | 641, 645, 651, 652, 660 | 152 / 376 | 853 |
| `data3d/kostel3.p3d` | 5 / 5 | 661–664, 666–671 | 59 / 73 | 1,088 |
| `data3d/kostelik.p3d` | 6 / 15 | 672–677 | 80 / 174 | 556 |
| `o/misc/kapl.p3d` | 3 / 14 | 678–680 | 35 / 67 | 129 |

Blank plaster, intact mineral trim, shrine base/side and bounded tower jambs
qualify. Dark folded tower roof 660 and plain shrine rear 678 are reflector-only,
preserving ambient. Receiver-enabled prerequisites total 207 / 435; reflector-only
119 / 255. These are prerequisites, not simultaneous triangles or visible coverage.
Previously covered tiled roofs are not new reception; no new timber is adopted.
The prior `k1_stara_prkna` lighting/exclusion contract remains.

Sixteen regions were actually built and rejected: 642–644, 646–650, 653–659 and
665. Portal wings, windowed facade strips, bell/door jambs and the repeated-window
upper band produced conspicuous diagonal/triangular brightness blocks already
with PBR On / GI Off. Separate built `reception=false` controls still failed;
forced CPU controls reproduced the failure. See the [receiver trial](evidence/material-modernization/batch-35/rejected-church-front-PBR-on-GI-off.png),
[reflector-only trial](evidence/material-modernization/batch-35/rejected-reflector-only-church-front-PBR-on-GI-off.png)
and [upper-band failure](evidence/material-modernization/batch-35/rejected-upper-plaster-band-PBR-on-GI-off.png).
No speculative geometry or shading repair is attempted.

Specific remaining boundaries: `k2_back_back` and `k2_back_back_back` clean
plaster boxes contain no complete ordinary fans; broad faces also span roof,
brick and ground/grass photos. `k2_vez_bot_front` and `kaple_main_front` safe
strips split original fans or select none; broad faces cross door/glazing or
portal lattice. `zsu_det_insidehole` is the dark bell recess with strongly
repeated negative UVs, not an exposed mineral wall. Clock, shutters, windows,
doors, devotional niche, flowers, painted columns, ornaments and mixed artwork
stay original. All 946 render-behavior and 476 authored-lighting scoped triangles
retain their gates. The unused invalid normal remains untouched.

## Actual gameplay participation

Final bytes were tested in original Nogova `R01WarCry.Noe`, Malden
`01TakeTheCar.ABEL` and Everon `05HeavyMetal.Eden`: nine sampled original
placements, copied profiles, no sound, grounded developer cameras. Controls
compare native PBR Off, auto/GI Off and auto/GI On at 09:00 and 17:30,
advancing-simulation movement/turns, retreats and independently verified forced
CPU toggles/restoration. This is not physical traversal or mission completion,
nor proof of all 49 placements or every runtime LOD transition.

- **Kostel:** coherent broad blank-wall response; Nogova side records native
  facade 0 / captured CPU 52,049 versus zero baseline. Forced CPU records 51,959;
  Everon normal-route CPU 52,238. Rejected window-facade blocks are absent.
  See [CPU comparison](evidence/material-modernization/batch-35/noe-kostel-left-CPU-comparison.jpg).
- **Kostel3:** Nogova tower records 0 / 7,092; forced CPU 256. Everon left
  demonstrates no new facade reception. Existing cemetery stone and roofs are
  not target coverage. Retained clock/jamb controls do not reproduce band 665's
  failure; [tower motion](evidence/material-modernization/batch-35/noe-kostel3-tower-motion.jpg).
- **Kostelik:** Malden front CPU 1,795; close rear-wing 5,455, native facade
  zero. Everon forced CPU 1,266. Some near candidates remain sampler-gated;
  farther source qualifications do not prove visible adoption. Original portal
  and photographed wall shadows remain; [close CPU](evidence/material-modernization/batch-35/abel-kostelik-rear-wing-CPU-comparison.jpg).
- **Shrine:** isolated baseline stone reception is zero. Final native front /
  back / left / right records 8,562 / 7,584 / 15,222 / 14,979; CPU front 8,572.
  This is base/side 679/680, not reflector-only rear 678. The rear triangular
  pattern exists with PBR Off and is not a new defect;
  [native comparison](evidence/material-modernization/batch-35/noe-kapl-open-back-comparison.jpg).

Normal-route bounded probes independently identify eligible new reflector
regions 678/679 on Nogova, 676 on Malden and 673 on Everon (17/27/4/5 unique
probe keys). No new eligible kostel/kostel3 hit was observed; this does not prove
zero transport. Counters are half-resolution family samples, not model masks.
Captured CPU-lit geometry still uses GPU GI tracing, not CPU ray tracing.
Surrounding terrain, old roofs, bark and cemetery stone are not credited.
Four retained motion sheets show no new gross boundary failure; obstructed
shrine/foliage scouts and an exploratory enhanced-mode retreat are excluded.

## GPU cost and validation

RTX 4060 Ti / driver 617.14, native 1920×1080 TAA100%, automatic PBR,
one-bounce GI, RT sunlight/cloud shadows; AO/water/volumetrics off. Warm 10 s,
GI Off/On/On/Off, settle 4 s, sample 6 s; means of paired interval medians.
No builds/tests/censuses ran during timing. Whole-scene ms, baseline → final:

| Original view | GPU frame GI Off | GPU frame GI On | Trace + filter + temporal |
| --- | --- | --- | --- |
| Kostel side | 2.099 → 2.161 | 2.331 → 2.385 | .418 → .510 |
| Kostel3 front | 4.700 → 4.895 | 5.200 → 5.321 | .844 → .853 |
| Kostelik front | 3.710 → 3.772 | 4.098 → 4.126 | .398 → .402 |
| Clear shrine front | 4.421 → 4.538 | 4.941 → 4.986 | .909 → .958 |

These include scenery and ordinary variation, not isolated material costs or
FPS gains. Forced-CPU route performance was not separately timed. All intervals,
run states and hashes are in the [qualification receipt](evidence/material-modernization/batch-35/qualification.json).

Vulkan/tools and GL33 builds pass. Focused Vulkan: 96 cases / 251,583 assertions;
GL33: 95 / 251,543. Road/surface ordering: 17 / 52 each; Vulkan policy: 690
checks; Python inventory/census: 19 tests. All Vulkan runs, including rejected
trials, total 321,750 presented frames with synchronization validation and zero
errors/warnings. Six live GL33 views return `UNSUPPORTED` for all five PBR
commands with zero changed pixels.

Fresh 726-model censuses reproduce historical prerequisites with no regression:
1,711 source levels / 1,708 retained runtime levels. Geometry, UVs, normals,
lighting, section flags, 270 original input hashes and 67 settings-fixture hashes
are preserved. Private repository/SDKs and snapshots remain untouched.

All four source assessments close, **not full-model PBR coverage**. New
prerequisites are 326 / 690; 2,626 of the original 3,316 source-unknown
alternatives retain explicit original boundaries. Global totals: **22,304 /
44,246** qualified prerequisites; **364 complete / 117 partial / 34 pending /
211 retained-existing** model dispositions. Backlog: **151 models / 408 distinct
pending keys**. Original 726-model inventory is unchanged. Stop after Batch 35.
