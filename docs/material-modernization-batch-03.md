# Batch 03: fence foundations, clay tiles and blank plaster

Base: `40fcc8702b1e0bcb70df1698f25b0078020404a4`. Eleven exact original
sources join the existing enhanced-only rough-dielectric table (r=.90, F0=.04,
metallic=0). They do not alter v3's 23 entries, transport, defaults or ray budgets.

| Original source (`data/`) | Physical surface | Identity / receiver family |
| --- | --- | --- |
| `podezdivka1.pac` | Separate rough concrete foundation | 141 / concrete |
| `podezdivka2.pac` | Separate brick/mortar foundation | 142 / stone |
| `2_sloup_beton.pac` | Concrete fence pillar | 143 / concrete |
| `vrsek_zidky.pac` | Worn masonry/concrete wall cap, separate from wire | 144 / concrete |
| `tasky_p.pac` | Dedicated clay roof tiles | 145 / roof |
| `tasky_hnus.pac`, `tasky_hnus90.pac` | Dedicated weathered roof tiles | 146–147 / roof |
| `blok_a_back.pac` | Blank plaster wall/gable, baked shrub/shadow color retained | 148 / facade |
| `blok_a_rantl_top.pac` | Plaster/masonry roof edge | 149 / facade |
| `komin_vetsi_side_01.pac` | Plastered chimney side | 150 / facade |
| `ujezd_up.pac` | Blank weathered plaster patch | 151 / facade |

These are bounded existing rough-material approximations, not measured optical
coefficients. Original pixels, geometry, UVs, normals and lighting were inspected;
no artwork, maps or original assets were generated or edited. Shared-source review
covers **24 models / 106 visual LODs**. Only source identity changes: all geometry,
flags, material-lighting hints, UV bounds and rejection buckets agree with the
pre-batch reports after replaying identities 141–151 as unknown. Section multisets,
not allocation-dependent section ordinals, are compared. Two additional shared
roof users take their pre-change baseline from the immutable original inventory.
The [source audit](evidence/material-modernization/batch-03/source-audit.json)
records each model and decision.

Seven reviewed sources retain fallback with concrete reasons: painted/rusted gate
posts and frames (`sloupek_vrata`, `zlenej_blivajz`, `plot_rust_noise`), mixed
paint/oxide/steel obstacle `jezek_texture`, and unidentified dark fills `blck_sum`,
`univ_black2`, `cihldum_sum`. Color/opacity alone does not establish a physical
material. Thirty-one other viewed sources remain explicitly pending: mixed
window/door atlases require exact triangle/UV-region proof, metal/corrugated
sources need defensible optical evidence, and dedicated timber candidates await
the related wood batch. Authored lighting and all original fallback gates remain.

## Gameplay appearance and receiver limits

The direct developer harness runs original Everon and Nogova missions with copied
profiles and `--nosound`. Standing cameras view original WRP placements, not an
artificial mesh fixture. This does not certify Steam launch or mission completion.
The following full-size captures are unamplified:

| Scene | Before | After |
| --- | --- | --- |
| Nogova fence foundation | [before](evidence/material-modernization/batch-03/modernization-b03-baseline-noe-1080-foundation.png) | [after](evidence/material-modernization/batch-03/modernization-b03-new-noe-1080-foundation.png) |
| Everon blank barracks wall, close local view | [before](evidence/material-modernization/batch-03/modernization-b03-baseline-eden-1080-barracks.png) | [after](evidence/material-modernization/batch-03/modernization-b03-new-eden-1080-barracks.png) |
| Farm roof, normal compound distance | [before](evidence/material-modernization/batch-03/modernization-b03-baseline-eden-1080-farmroof.png) | [after](evidence/material-modernization/batch-03/modernization-b03-new-eden-1080-farmroof.png) |
| Weathered tiles | [before](evidence/material-modernization/batch-03/modernization-b03-baseline-eden-1080-tiles.png) | [after](evidence/material-modernization/batch-03/modernization-b03-new-eden-1080-tiles.png) |
| Town plaster | [before](evidence/material-modernization/batch-03/modernization-b03-baseline-eden-1080-cityplaster.png) | [after](evidence/material-modernization/batch-03/modernization-b03-new-eden-1080-cityplaster.png) |

Large blank walls gain local rough-material shading and less uniformly filled
ambient response; original grime and baked shadows remain. Foundation and roof
changes are more localized, not a wholesale relighting of mixed facades. Windows,
doors, gutter/downpipe imagery, corrugated hatches and fence wire remain legacy.
Cloud, vegetation and runtime LOD variation prevent bit-exact cross-run comparison.

The close wall sample has 346,262 native facade receiver pixels in the half-
resolution diagnostic footprint. The farm view has 4,093 native roof receiver
pixels. These family counters alone cannot distinguish every shared original
source. Bounded stochastic probes independently identify new plaster/chimney/
edge identities 148/150/149 (29/39/2 unique sampled grid-phase paths) and foundation/
pillar identities 141/142/143 (8/3/2). They demonstrate actual qualified hits, not
world-wide transport statistics or a rendering-coverage percentage.

Native reception is not universal. The weathered-tile view has new PBR draws but
**zero native roof receiver pixels**. An exact-window native/CPU/native-restored
control records 14,515 CPU roof pixels, then zero again after restoration:
[native](evidence/material-modernization/batch-03/modernization-b03-extra-eden-1080-tiles-native.png),
[CPU](evidence/material-modernization/batch-03/modernization-b03-extra-eden-1080-tiles-CPU.png),
[restored](evidence/material-modernization/batch-03/modernization-b03-extra-eden-1080-tiles-native-restored.png).
Existing live captured-lighting/alpha-route gates remain authoritative; no missing
ambient is invented for parity. Reflector eligibility is not receiver eligibility.

Same-runtime v3, enhanced/GI-off, sky-only and GI-on controls, overcast and dusk,
moving/settled TAA and FSR Quality, lower-LOD retreats and two-bounce operation
were inspected. The demonstrated temporal modes retain original detail. MSAA4
executes with zero validation errors but retains existing spatial-only GI grain;
it is not claimed as a clean visual result or hidden by changing filters/rays.

## Cost and regressions

RTX 4060 Ti; one bounce, TAA at 100% world resolution, RT sun/cloud shadows on;
AO/water optics/volumetrics off. Each runtime warms for ten seconds, followed by
Off/On/On/Off intervals with four-second settling and six-second samples. Values
average the paired interval medians. Probes are off, with no build/other game
during timing. Raw intervals remain in the receipt.

| Scene | GI-on GPU frame, before / after | GI trace, before / after | Trace+filter+history, before / after |
| --- | ---: | ---: | ---: |
| Close blank wall, 1080p | 3.579 / 3.643 ms | .182 / .648 ms | .305 / .962 ms |
| Close blank wall, 1440p | 4.288 / 5.581 ms | .196 / .995 ms | .334 / 1.532 ms |
| Nogova foundation, 1080p | 9.481 / 9.511 ms | 1.165 / 1.223 ms | 1.683 / 1.741 ms |
| Nogova foundation, 1440p | 9.595 / 9.626 ms | 1.152 / 1.220 ms | 1.780 / 1.855 ms |

The close large receiver adds about .66 / 1.20 ms to the measured GI scopes and
.065 / 1.293 ms to total GPU frame time. This is a local wall/stress view, not a
town budget. CPU recording is around 8.3 ms there and 23 ms at the foundation;
raster/clock variation makes the small total-frame differences inconclusive.
No speedup or universal scaling/budget claim is made. Defaults and sampling are
unchanged; the extra cost is exposed, not normalized away.

Nine Vulkan runs presented **105,157 frames**, synchronization validation enabled,
zero errors and warnings, and verified shutdown. Public Vulkan game/tools/tests
and GL33 game/tests builds succeed. Focused Vulkan tests pass 69 cases / 49,630
assertions; GL33 68 / 49,590; driver-free policy checks 690; inventory and census
Python tests 13. The intentional driver-free teardown injection is not a gameplay
validation finding. Live GL33 Off/On/Enhanced/Off control has zero changed pixels.
All 270 inventory input hashes and 67 retained settings/fixture/input hashes match.
Private DLSS remains clean, remote-free, with SDKs and snapshots present. Runtime
hashes, source-specific probes, receiver controls and measurements are in the
[qualification receipt](evidence/material-modernization/batch-03/qualification.json).

## Loader correction and next regional work

The earlier CLI comment incorrectly equated `LODShape::LoadOptimized` with normal
gameplay loading. Ordinary ODOL gameplay uses `ModelCache` and `ShapeAdapter`.
The read-only `model inspect --runtime-loader` option now exposes that exact route;
the historical default is retained for reproducible old inventories. UV triangle
dumps include the original runtime source fingerprint; legacy zero fingerprints
are explicitly marked invalid. Exact texture identity normalization matches the
runtime (case/slash normalization only, no basename guessing).

The [runtime census](evidence/material-modernization/runtime-census.json) repeats
all 726 / 1,711 through the adapter. Replaying enhanced identities 130–151 as
unknown reproduces every LOD, normal/vertex-lighting hint and complete section
prerequisite multiset of the original policy, with **zero differences**. Only
section ordering changes. The original inventory remains an immutable baseline.
This is prerequisite assessment, not completed optical review of all 726 models.

Reproduce without changing assets:

This command replays the batch-03 tool/policy snapshot. For the current executable,
use the reproduction command linked from [the main progress page](material-modernization.md),
which explicitly replays subsequent enhanced and regional identities.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --cache build/material-modernization-20261021/runtime-census `
  --output build/material-modernization-20261021/runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 151
```

Two existing ordinary triangles on `statek_kulna` sample a wall-only lower strip
of the otherwise window-bearing `budova2_s` atlas. This is a concrete regional
lead, not a whole-image approval. Runtime section numbers differ from the legacy
report, so future qualification must use original geometry fingerprint, exact
texture pixels and UV-region proof, with every existing lighting/render/geometry
gate intact. That implementation and live validation remain pending.

The ledger now has 31 complete and 31 partial source reviews, 453 awaiting source
review, and 211 retained prerequisite dispositions. There is no 100% rendering
coverage claim, and the overall goal remains active.
