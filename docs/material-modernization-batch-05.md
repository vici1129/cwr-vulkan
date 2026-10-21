# Batch 05: dedicated building finishes and exact chapel masonry

Base: `b7943574e1a6211d126c4fb7665662d5573ebb6e`. Fourteen dedicated original
sources join the existing enhanced/automatic table. Three exact chapel regions
join the existing auto-only original-triangle path. Defaults, v3, catalog entries,
GL33 and transport algorithms are unchanged.

## Source review and boundaries

All **54 original images** in this family review were decoded and individually
inspected. Native positions, normals, UVs, flags and lighting hints were reviewed
for **55 models / 231 visual LODs**, including every placed user of each proposed
source. The [source audit](evidence/material-modernization/batch-05/source-audit.json)
records original hashes, image dimensions, section qualifications and exact
geometry/ordered-binding fingerprints. Full original vertex/UV buffers and decoded
images remain in ignored local build evidence, not published as replacement assets.

| Identity | Exact original texture | Existing rough dielectric approximation |
| --- | --- | --- |
| 152 | `data/futra.pac` | Painted/aged wooden frames and panels, .85 / wood |
| 153 | `data/kostel_t_tramy_podel.pac` | Dark timber boards, .85 / wood |
| 154 | `o/hous/drevo02.pac` | Weathered boards/timbers, .85 / wood |
| 155 | `o/hous/drevo_modre.pac` | Blue painted boards, .85 / wood |
| 156 | `o/hous/strecha02.pac` | Rough nonmetal roof covering, .90 / roof |
| 157 | `o/hous/strecha_tasky.pac` | Rough nonmetal roof covering, .90 / roof |
| 158 | `o/hous/tasky_tmavsi.pac` | Roof tiles, .90 / roof; distinct from `data/` |
| 159 | `o/hous/dumpod_beton.pac` | Cracked concrete, .90 / concrete |
| 160 | `o/hous/dumpod_podez.pac` | Stone/mortar foundation, .90 / stone |
| 161 | `o/hous/zalchat_beton.pac` | Weathered concrete, .90 / concrete |
| 162 | `o/hous/zalchat_strop.pac` | Wooden ceiling/panels and trim, .85 / wood |
| 163 | `o/hous/stenain01.pac` | Blank plaster, .90 / facade |
| 164 | `o/hous/panel04.pac` | Rough concrete/plaster patch, .90 / concrete |
| 165 | `o/hous/panel_inter02.pac` | Wall covering/painted wall, .90 / facade |

These retain original color, dirt and photographic shadows. F0 .04, metallic 0
and roughness are existing bounded artistic assumptions, **not measured optics**.
No normal/roughness map, artwork, texture or geometry is generated or altered.
Window, door, socket, grille, balcony hardware and gutter atlases inherit nothing
from an adjacent plain source.

The chapel (`data3d/kaple.p3d`) has five original visual LODs, resolutions
.4/.8/1.6/2.4/4.8. Regions 515/516/517 cover only original masonry triangles using
`data/kaple_01_back.pac`, `kaple_01_front.pac` and `kaple_01_side.pac`. They admit
58/18/18/18/10 original triangles respectively across those LODs. The whole mixed
source identity remains zero. Exact model, LOD, vertex/section counts, geometry,
ordered texture bindings and immutable decoded pixels are required together;
every triangle corner must lie in the inspected rectangle. Existing render,
ordinary-vertex, geometry, static-object and captured-lighting gates still apply.
The side's original V overshoot (about 1.00447) is explicitly bounded by its exact
geometry proof; no modulo qualification or sampler change is introduced.

Five separate chapel cross/ornament/cap sources retain their original response:
their small weathered photos cannot independently distinguish stone, wood and
coated/oxidized metal. An adjacent masonry niche is not proof of their substrate.
The unidentified dark fill, ambiguous apartment roof and metal gutter also have
specific inspected fallback reasons. **29 mixed/deferred sources remain pending**,
including wooden barn doors with hinges, apartment windows/vents and wall sockets.
Rejecting a whole atlas does not close a review where original triangles may
still separate safe physical regions.

### A candidate rejected by gameplay review

The initial candidate also proposed `dum_podhradi1_1`, `_4` and `_5`. The dusk
control exposed a rectangular wall/roof lighting discontinuity beside unresolved
atlas regions. Pixel reinspection also found dark horizontal trim of unresolved substrate in `_4`,
invalidating its proposed whole-source masonry classification. All three additions
were **withdrawn before commit**. The dedicated plaster and paving sources remain
deferred with concrete appearance/continuity reasons, not declared impossible.

The [candidate dusk result](evidence/material-modernization/batch-05/modernization-b05-dusk-new-dusk-auto-on.png)
is deliberately not an accepted improvement. Compare the
[retained baseline](evidence/material-modernization/batch-05/modernization-b05-dusk-baseline-dusk-auto-on.png)
and [final reduced policy](evidence/material-modernization/batch-05/modernization-b05-final-noe-1080-dusk-auto-on.png).
The wall/roof discontinuity is absent in the final runtime. Its separate concrete
porch floor still has the new direct material response; this is not whole-house GI.
No shader/filter/sample change conceals the rejected result.

## Actual gameplay evidence

Original Nogova `Resistance/R01WarCry.Noe` and Malden `01TakeTheCar.ABEL` gameplay
run directly through the developer harness, copied profiles and `--nosound`.
Cameras stand at queried terrain height around original WRP placements. This
does not certify a Steam launch or mission completion. Full-size, unamplified
morning captures use auto, TAA and one bounce:

| View | Before | Accepted runtime |
| --- | --- | --- |
| Smithy rafters and roof; windowed facade retained | [before](evidence/material-modernization/batch-05/modernization-b05-baseline-noe-1080-smithy.png) | [after](evidence/material-modernization/batch-05/modernization-b05-final-noe-1080-smithy.png) |
| Barn roof; mixed wooden-door atlas retained | [before](evidence/material-modernization/batch-05/modernization-b05-baseline-noe-1080-barnroof.png) | [after](evidence/material-modernization/batch-05/modernization-b05-final-noe-1080-barnroof.png) |
| Chapel masonry; separate ornaments retained | [before](evidence/material-modernization/batch-05/modernization-b05-baseline-abel-1080-chapel.png) | [after](evidence/material-modernization/batch-05/modernization-b05-final-abel-1080-chapel.png) |
| Podhradi foundation/concrete only; main walls deferred | [before](evidence/material-modernization/batch-05/modernization-b05-baseline-noe-1080-podhradi.png) | [after](evidence/material-modernization/batch-05/modernization-b05-final-noe-1080-podhradi.png) |

The additional [front niche view](evidence/material-modernization/batch-05/modernization-b05-final-abel-1080-chapelfront.png)
shows the preserved cross and photographic details. The ordinary apartment-front
view records **zero new concrete receiver pixels**: its dominant window-bearing
facade remains unsupported. Dedicated-source prerequisite counts do not establish
visible apartment facade GI or explain the absence of a sampled receiver.

Same-runtime v3, auto/GI-off, sky-only and GI-on controls separate material from
reflected-surface response. Overcast/dusk, moving/settled TAA and FSR Quality,
original lower-LOD retreats and two-bounce operation are checked. MSAA4 continues
to execute with the existing spatial-only GI grain; it is not a clean visual
acceptance result. Original photographic shadows, geometry LOD changes and texture
resolution remain. Native/CPU/native-restored captures and source-specific bounded
grid/phase ray probes are retained in the receipt; no world-wide hit distribution
or rendering-coverage percentage is inferred from them.

The final native frame samples record 35,677 roof-family pixels at the barn and
7,089 wood / 6,518 roof at the smithy. Podhradi has 6,069 stone / 5,155 concrete
pixels on native, 6,094 / 5,145 on CPU, and 6,069 / 5,155 after native restoration;
**zero facade receivers** confirms that its withdrawn walls are not adopted.
Chapel stone reception is 31,624 native, 31,621 CPU and 31,624 native-restored.
The bounded unique grid/phase probes record nine eligible identity-160 foundation
paths and chapel identities 515/517/516 on 31/4/3 paths. Those are actual sampled
qualified ray endpoints, not nearby-object names. No absent source hit is claimed.

## Runtime census, tests and preservation

The fresh [runtime census](evidence/material-modernization/runtime-census.json)
checks all 726 models / 1,711 visual LODs. Complete section prerequisite multisets,
normal/lighting hints, UV bounds, geometry and ordered bindings agree with the
pre-batch reports after replaying only new identities. Replaying all post-inventory
identities reproduces the immutable original inventory with **zero differences**:

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --cache build/material-modernization-20261021/batch-05/runtime-census-final `
  --output build/material-modernization-20261021/batch-05/runtime-census-final/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 165 `
  --replay-regional-identities 512 513 514 515 516 517
```

This command describes the batch-05 policy snapshot. With the current executable,
use the reproduction command linked from [the main progress page](material-modernization.md).

The highest-detail prerequisite increment is 2,055 across distinct models, without
placement or alternative-LOD multiplication. It is an audit count, not an adoption
objective. The 55-model source review includes withdrawn and unresolved candidates,
not 55 fully modernized buildings.

Public Vulkan game/tools/tests and GL33 game/tests build successfully. Focused
Vulkan tests pass 70 cases / 50,408 assertions; GL33 69 / 50,368; Python inventory/
census 15; driver-free Vulkan policy checks 690. Wrong source/model/LOD, pixel and
binding substitutions, mixed UVs, authored lighting, render state and missing
captured-lighting tests still reject. Live GL33 Off/On/Enhanced/Auto/Off remain
unsupported no-ops, with zero changed original-world ROI pixels.

All 270 original inventory input hashes and 67 retained settings/fixture/input
hashes match. Private DLSS is clean and remote-free; SDKs and preserved snapshots
remain present. Original assets, settings, GL33, ray budgets and existing rendering
functionality are preserved. The [qualification receipt](evidence/material-modernization/batch-05/qualification.json)
distinguishes final runtime/cost acceptance from the rejected exploratory policy.

## GPU cost and progress

RTX 4060 Ti; auto, one bounce, TAA, 100% world resolution, RT sun/cloud shadows;
AO/water optics/volumetrics off. Each view warms ten seconds, then runs Off/On/On/
Off with four-second settling and six-second samples. Values average paired
interval medians; probes are off, with no concurrent build or other game.

| Standing view | GI-on GPU frame, before / final | GI trace, before / final | Trace+filter+history, before / final |
| --- | ---: | ---: | ---: |
| Podhradi, 1920×1080 | 2.578 / 2.642 ms | .293 / .327 ms | .457 / .498 ms |
| Podhradi, 2560×1440 | 3.651 / 3.739 ms | .381 / .438 ms | .648 / .725 ms |
| Chapel, 1920×1080 | 4.745 / 4.881 ms | .529 / .723 ms | 1.008 / 1.249 ms |
| Chapel, 2560×1440 | 4.533 / 4.766 ms | .461 / .644 ms | .924 / 1.150 ms |

Measured GI scopes add about .041/.077 ms at Podhradi and .241/.226 ms at the
chapel. Total GPU-frame deltas are +.064/.088 and +.136/.233 ms. CPU command
recording is around 6.0–6.1 / 11.5–12.0 ms. Raster, Off-frame and clock variation
prevent treating the total-frame changes as isolated GI cost or a universal
budget. The rejected 17-source runtime is not used for these final cost claims.

The eight paired baseline/final runs present **116,243 frames**, synchronization
validation on, zero errors and warnings, all with verified shutdown. Separate
candidate/dusk diagnostic runs remain explicitly exploratory. The accepted Vulkan
executable SHA256 is
`03F265B0BF8087D12FC1D2D88D9C73BB72951F1D03DE49A16A434301DA72B278`.

The ledger now records **32 complete source reviews, 75 partial, 408 awaiting
source review and 211 retained prerequisite dispositions**. Task 24 (chapel) is
source-review complete with its five concrete optical-ambiguity fallbacks, not
100% rendering coverage. Related building/door/trim atlas separation continues;
the deferred Podhradi cases must resolve appearance continuity before adoption.
