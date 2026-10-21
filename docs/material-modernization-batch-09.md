# Batch 09: sandstone, foundation stone and memorial stone regions

Base: `659e940f128511bb9ba9f8520d9b90a2e3e43a18`. This batch assesses **18 placed
models / 53 authored visual LODs**, all also retained by the ordinary adapter under
inspection defaults. Seven dedicated original sources and six exact-model stone
regions use existing material paths. No replacement artwork, material map, mesh,
UV, authored-lighting hint, default, shader, transport budget or old material gate
is changed. The 23-entry v3 table and private DLSS remain untouched.

## Source decisions

| Original source | Identity | Existing approximation |
| --- | ---: | --- |
| `data/piskovec.pac` | 178 | Natural sandstone, .85 roughness / rock family 2 |
| `data/skala_piskovec.pac` | 179 | Rough castle foundation stone, .90 / stone family 6 |
| `data/skala_piskovec3.pac` | 180 | Natural cliff stone, .85 / rock family 2 |
| `data/bozi_muka_sokl.pac` | 181 | Separately modeled wayside-shrine stone base, .90 / stone family 6 |
| `data/hrobecek2_side.pac`, `side2.pac`, `side3.pac` | 182-184 | Separate memorial stone side surfaces, .90 / stone family 6 |

The last row's three names all have the `data/hrobecek2_` prefix. All ten original
survey images were individually viewed, including the three mixed/ambiguous
sources below. Source names do not establish physical identity: gameplay shows
179 on rocky castle foundations, not the main facades or arches. The selected
images support rough stone dielectric approximations, not measured optical data.
F0 .04 and metallic 0 reuse the existing paths. Photographed color, stains,
weathering, highlights, shadows and the original normals remain.

All shared models and alternative LODs were checked through original native
geometry, original face/UV placement, section flags, normals and authored-lighting
metadata. Repeating coordinates are supported for the dedicated images because
their entire source is the same inspected physical class, not because wrapping
or opacity proves a material. All render/authored/geometry rejection buckets
remain unchanged. Neighboring castle openings, facades, interiors, floors and
ground images do not inherit a foundation-stone response and remain unreviewed
source questions in those three partial model assessments.

The small `data/cross_opak.pac` crop supplies insufficient independent evidence
to choose a stone/wood/metal substrate for the shrine's dark cross. It remains
untyped, as do its cutout figure and original invalid-geometry cases. The mixed
`data/hrobecek2.pac` front photograph includes sculpted foreground, substantial
photo canvas and ground/vegetation; it is not a whole-source stone approval.

## Exact original grave-stone tile regions

`o/hous/nahrobky01.pac` mixes stone, inscriptions/plaques, carved/cross photographs,
soil/grass and canvas. Its whole-source identity is **still zero**. Only the
inspected lower-right stone tiles, bounded by original U/V .503-.998, are admitted
for these exact models:

| Model under `o/hous/` | Region | Qualified source LODs / whole triangles |
| --- | ---: | --- |
| `hrob1.p3d` | 534 | .5: 20 |
| `hrob2.p3d` | 535 | .5: 22 |
| `nahrobek1.p3d` | 536 | .5: 8; next LOD retains fallback |
| `nahrobek2.p3d` | 537 | .5: 26; 1.5: 8 |
| `nahrobek4.p3d` | 538 | .5: 8; 1.5: 2 |
| `nahrobek5.p3d` | 539 | .5: 20; exact fractional next LOD: 20 |

These use .90 roughness / stone family 6. Original ClampU and ClampV are required;
no sampler or coordinate is repaired. Model name, exact source resolution,
vertex/section counts, original geometry and ordered texture-binding fingerprints,
512x512 dimensions and immutable RGBA fingerprint must agree. All corners of the
whole original triangle must stay inside the inspected convex rectangle. Every
selected original polygon fan is complete: **zero original faces are partly
classified**. The nearest dumped UV lies .000057 from a registry boundary, well
outside the dump's rounding uncertainty. Neither plaque/soil-crossing triangles
nor clipped/generated CPU triangles gain a material.

Nine source-LOD geometry identities support these six regions. `nahrobek3` has
no triangle in this inspected tile rectangle and gains no region. Other physical
parts and front photographs remain pending; these are not complete grave/model
rendering approvals. The [source audit](evidence/material-modernization/batch-09/source-audit.json)
retains exact rules, original SHA256 provenance, all 53 source geometry identities,
native qualifications and whole-face review. Original decoded images and full
vertex/UV/normal dumps remain in ignored local evidence, not public assets.

The additional highest-detail prerequisite count is 1,128 across distinct models,
without placement multiplication or adding mutually exclusive LOD alternatives.
It is not a rendering-coverage denominator or an adoption objective.

## Exact LOD provenance without tolerance

One original `nahrobek5` LOD is **1.31150221824646**, float bits **1067966286**.
Default tool text rounded it to `1.3115`, which does not satisfy exact matching.
`--uv-triangles` now appends `resolutionBits` to its geometry diagnostic, without
changing existing float formatting or native loader behavior. The registry uses
the round-tripping `1.31150222f`. The unit test explicitly rejects `1.3115f` and
accepts the exact value; no epsilon, broader LOD match or safety-gate relaxation
was introduced. Existing geometry/binding fingerprints are unchanged.

## Actual gameplay and limits

Original Cain `03Bomberman`, Abel `01TakeTheCar`, Eden `05HeavyMetal` and Noe
`R01WarCry` gameplay use the direct developer harness, copied profiles and
`--nosound`. Cameras stand at terrain height near original WRP placements. This
is not normal Steam-route or mission-completion acceptance.

Before/after, v3, auto/GI-off, sky-only and GI-on views separate effects. Morning,
overcast/dusk, moving/settled TAA and FSR Quality, two-bounce operation, CPU/native
restoration and original lower-LOD retreats are checked separately. Original
photo shading, foliage silhouettes and LOD transitions remain; no filtering or
acceptance gate is changed to conceal them. MSAA4 executes and validates but its
existing spatial-only GI grain is not certified as a clean visual result.

The cliff sample records 140,228 native rock-family receiver pixels; the explicit
native/CPU/restored control records 140,365 / 140,434 / 140,429 in their respective
routes. The sandstone view records 58,243 native rock-family pixels. These are
frame/family samples, not per-source pixel percentages. Unique grid/phase probes
independently hit new identities 180 on 64 paths and 178 on 93 paths.

The memorial side view records 16,984 native stone-family pixels; its nearby
catalog stone wall remains separately reported as old catalog reception. Probes
hit side identities 182/183/184 on 18/1/3 paths. Castle foundation stone records
42,971 native stone-family pixels and 18 probe paths on identity 179. These results
do not claim that the main castle facades are modernized. The shrine base has
11 reflected paths on identity 181, zero new native stone receivers in that view,
and 3,550 CPU stone-family receiver pixels when independently captured. Its cross,
cutouts and existing native alpha/capture gate remain intact.

The [qualification receipt](evidence/material-modernization/batch-09/qualification.json)
contains full states, executable hashes, all timing intervals, probes, validation
shutdowns and preservation checks. Bounded probes are not all-world transport
statistics and absent identities are not claimed as observed hits.

The grave view records 2,481 native / 2,480 CPU / 2,481 restored stone-family
receiver pixels. Its enhanced-only control has zero stone receivers; automatic
mode restores 2,494 in that later frame. Daylight, overcast and dusk show a subtle,
coherent response on the exposed stone rather than a full monument relighting.
The bounded grid/phase probe sample hits old identities 133 and 173, **no new
grave-region identity**; it does not prove reflected hits on regions 534-539.
The independent exact original-face qualification and observed reception are
reported separately. No new triangle seam was observed in these controls.

| Normal-distance view | Before | After |
| --- | --- | --- |
| Cain cliff | [before](evidence/material-modernization/batch-09/modernization-b09-baseline-cain-1080-cliff-gi.png) | [after](evidence/material-modernization/batch-09/modernization-b09-new-cain-1080-cliff-gi.png) |
| Malden sandstone | [before](evidence/material-modernization/batch-09/modernization-b09-baseline-abel-1080-sandstone-gi.png) | [after](evidence/material-modernization/batch-09/modernization-b09-new-abel-1080-sandstone-gi.png) |
| Everon castle foundation | [before](evidence/material-modernization/batch-09/modernization-b09-baseline-eden-1080-castle-gi.png) | [after](evidence/material-modernization/batch-09/modernization-b09-new-eden-1080-castle-gi.png) |
| Nogova grave | [before](evidence/material-modernization/batch-09/modernization-b09-baseline-noe-1080-graves-gi.png) | [after](evidence/material-modernization/batch-09/modernization-b09-new-noe-1080-graves-gi.png) |

## Measured GPU cost

RTX 4060 Ti; TAA at 100% world resolution with RT sun/cloud shadows, one bounce,
AO/water optics/volumetrics off. Each runtime warms ten seconds, then runs
Off/On/On/Off with four-second settling and six-second sampling. Values average
the paired interval medians. Probes, builds and other games are absent during
timing. Both 1440p pairs report actual 2560x1440 world/output dimensions.

| View / output | GI-on GPU frame before / after | Trace before / after | Trace+filter+history before / after |
| --- | ---: | ---: | ---: |
| Cliff 1920x1080 | 2.507 / 2.721 ms | .171 / .297 ms | .424 / .642 ms |
| Cliff 2560x1440 | 3.988 / 4.488 ms | .243 / .458 ms | .684 / 1.073 ms |
| Sandstone 1920x1080 | 2.014 / 2.065 ms | .149 / .237 ms | .312 / .437 ms |
| Castle foundation 1920x1080 | 4.048 / 4.079 ms | .264 / .320 ms | .544 / .650 ms |
| Grave 1920x1080 | 4.694 / 4.855 ms | .875 / .933 ms | 1.326 / 1.397 ms |
| Grave 2560x1440 | 4.882 / 5.052 ms | .904 / .968 ms | 1.365 / 1.440 ms |

The large cliff receiver adds about .218 / .390 ms to measured GI stages at
1080p / 1440p. The small grave change is not free either. Full GPU-frame deltas
are not isolated transport measurements: CPU recording, surrounding draws and
interval variation remain in the receipt. In particular, the sandstone baseline
Off intervals vary 1.808-2.009 ms; the castle CPU difference is not a claimed
optimization. These samples establish view-specific cost, not a universal budget.

The 12 Vulkan runs present **192,593 frames** with synchronization validation on,
zero errors and zero warnings. All exit normally and no game process remains.
The accepted candidate executable SHA256 is
`78396415024D5A9CDAF2264A919EC3A6BD25A0A6E41D4F7A09ABD0E3AA614CC6`.
The final source comment corrects the foundation-stone wording only; no executable
logic differs from the tested candidate. Live GL33 Off/On/Enhanced/Auto/Off
controls report unsupported Vulkan features and zero changed world-ROI pixels.
All **270 original input hashes** and **67 settings/fixture/input hashes** match;
private DLSS is clean and remote-free, with its SDKs and snapshots preserved.

## Tests, census and progress

Public Vulkan game/tools/tests and GL33 game/tests build. Focused Vulkan tests pass
77 cases / 62,484 assertions, GL33 76 / 62,444, Python inventory/census 18 and
driver-free Vulkan policy checks 690. The intentional driver-free teardown error
is not a gameplay validation finding. Tests cover unchanged v3 behavior, new source
families, exact fractional LOD matching, immutable model/pixel/binding proof,
missing clamp flags, mixed plaque/soil triangles and every existing rejection gate.
The final focused filter is
`[vulkan-shape],[Shaders],[enhanced-material],[ray-surface],[render-flags]`;
the separate `[vulkan-aa]` controls also pass.

Fresh censuses retain 726 models, 1,708 ordinary-adapter retained visual levels
and all 1,711 authored source levels. Complete prerequisites match the immutable
original inventory after explicit post-inventory identity replay; the three
source-only forest levels remain outside retained historical native comparison.
All original normals, hints, section state and UV metadata reproduce the previous
accepted source policy after replaying only this batch's additions.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-09/runtime-census `
  --output build/material-modernization-20261021/batch-09/runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 184 `
  --replay-regional-identities 512 513 514 515 516 517 518 519 520 521 522 523 524 525 526 527 528 529 530 531 532 533 534 535 536 537 538 539
```

For the separate source census, add `--all-source-lods` and use a separate cache/output
under `batch-09/source-census`. Neither census certifies live reception or optical
review of every original image. The ledger now records **60 complete source
reviews, 90 partial reviews, 365 pending and 211 retained existing contracts**.
Six natural-rock models finish source review; the other twelve remain partial
for the concrete mixed/unreviewed surfaces described above. The full 726-model
goal remains incomplete; these dispositions are not rendering percentages.
