# Original tiled roofs and roof/chimney source survey

Base: `95ca6f88934630355fe0d95147a17d99256c6e07`. This batch surveys **31 original
roof/chimney images across 64 world-placed models / 269 authored visual LODs**,
also retained by the ordinary adapter for these models. Names selected the
review queue, not the physical class. All 31 decoded images were individually
inspected alongside native geometry, UVs, normals, render flags, lighting hints
and original polygon-fan boundaries. Other sources on these models are not
thereby reviewed or approved.

## Defensible original material separation

Three dedicated images contain overlapping rounded/curved roof tiles throughout
the original source. They join the existing enhanced PBR table and automatic
material path, with the existing roof family 3, roughness .90, F0 .04 and metallic
zero. These are unmeasured rough-dielectric approximations, not reconstructed
optical maps. Stock v3's 23 entries and identities remain unchanged.

| Original source / identity | Placed model users | Otherwise-qualified whole triangles by source LOD |
| --- | --- | --- |
| `data/kostel_strecha.pac` / 185 | `data3d/kostel.p3d` | 0:60, 1:60, 2:36, 3:26, 4:14 |
| `data/strecha_tasky.pac` / 186 | `data3d/kostelik.p3d` | 0:46, 1:24, 2:20, 3:16, 4:9, 5:4 |
| `o/hous/hasicakstrecha.pac` / 187 | `o/hous/brana02.p3d`, `brana02nodoor.p3d` | Each 0:36, 1:12 |
| Same source / 187 | `o/hous/hasic_zbroj.p3d` | 0:8, 1:8, 2:8, 3:4 |

The five directly affected models have 19 source levels. Their repeating UVs
sample the same physical tile class; no UV clipping, generated masks, geometry
splits or modulo sampling manufacture a region. Original fans have zero partial
classifications. These source sections have finite UVs, valid normals and no
authored-lighting rejection, but every original source, render-behavior,
geometry, lighting and captured-input gate remains authoritative at runtime.
The 186 additional highest-detail prerequisites are not a coverage denominator
or a reason to prefer these roofs over visible gameplay benefit.

Original pixels, weathering, photographed shadows, UVs, geometry and lighting
indices remain unchanged. No artwork, maps, shaders, sampling/filter budgets,
defaults or safety gates are added or changed. No exact-model regional identity
is introduced in this batch. Region 529 remains PBR/reflection-only.

## Unresolved and actionable cases

The [source audit](evidence/material-modernization/batch-12/source-audit.json)
records a concrete reason for every surveyed source, not an opaque filename
allowlist. In particular:

- `komin_top_01`, `komin`, and `komin_roura_top` include photographed black
  apertures. A hole is not a rough solid receiver. `orlhot_kom` also includes a
  frame/panel and black recess beside brickwork; it remains untyped as a whole.
- Seamed/corrugated roof sheets and green weathered panel roofing do not encode
  independent bare-metal/coating properties. Color, rust and opacity do not
  justify guessed optical coefficients. Blue church/spire photographs also
  include uncertain substrate, background or architectural bands.
- `hruzdum_strecha` includes pale perimeter and lower ground/debris. The
  exploratory U .04...96 / V .04...88 tile rectangle contains **no complete
  otherwise-eligible original triangle**. `kdomek_komin` likewise has none in
  U 0..1 / V .18...91. Broad faces are not clipped or promoted across the boundary.
- The narrow `komin_maly_side_01` experiment selects near-level horizontal
  cap/detail faces, not a demonstrated full chimney body. It is not added merely
  to increase qualified counts.
- `orlhot_kom` U .22...79 / V 0..1 does select complete original brick faces in
  13 models with zero partial fans. This is an **actionable pending region
  review**, requiring its own lighting/gameplay proof, not a completed
  implementation or a permanent refusal. The low-resolution `komin_side_af`
  body and rough rectangular `kostel02_strechal` roof units likewise remain
  follow-up candidates rather than implemented coverage.

Other mixed roof photographs, ragged bark/support edges and unreviewed non-roof
atlases retain their specific reasons. A whole-image refusal does not complete
a model where original faces may still safely separate a physical region.

## Gameplay method and limits

Actual original Nogova `R01WarCry` and Malden `01TakeTheCar` gameplay uses the
direct developer harness, copied profiles and `--nosound`. This is not Steam-
route or mission-completion acceptance. Grounded standing cameras inspect the
original church, fire station, gate variants and chapel roof caps; no aerial
roof inspection substitutes for normal gameplay value. Initial obstructed
church/gate scouts and the wall-obstructed `tile-chapel-hill` baseline view are
not appearance acceptance. The clear paired views have separate recorded
coordinates. Supplementary paired views at two original chapel placements show
the shallow main-roof strip from normal distances. They do not substitute an
aerial view or claim an unobstructed broad main-roof inspection.

The church-side view demonstrates captured CPU-lit roof reception, including
when hardware T&L is enabled; it does **not** demonstrate new native-channel
church reception. Route-control names describe requested hardware/CPU states,
not the actual channel used by every model. Native/CPU family counters and
bounded reflected-hit probes are recorded separately. Nearby preexisting tile
sources contribute to town/gate roof-family totals, so those counters are not
reported as source-specific rendering percentages. In particular, the chapel
has **zero native roof reception** in both tested standing views; forced CPU
lighting separately demonstrates reception. The existing input gate is not
relaxed and its exact cause is not inferred from a zero counter.

| 1080p frame sample | Hardware T&L enabled: native/CPU roof pixels | Forced CPU | Hardware T&L restored |
| --- | ---: | ---: | ---: |
| Church side | 0/19,143 | 0/19,139 | 0/19,154 |
| Fire-station town view | 6,222/0 | 0/8,811 | 6,221/0 |
| Chapel front/caps | 0/0 | 0/547 | 0/0 |
| Second chapel placement, shallow main roof | 0/0 | 0/2,288 | 0/0 |

These are observed family totals, not source-specific percentages. The game log
verifies each Off/On hardware-T&L toggle, including restoration, rather than
assuming that successful keyboard delivery proved the route. Bounded unique
grid/phase probes observe one eligible reflected hit on new source 187 and two
on 186. They do not observe a new-source 185 hit in the sampled phases; that is
not a claim of zero all-world transport. Source 185 reception is independently
demonstrated above. No universal native adoption or complete roof coverage is
claimed.

Clear TAA views retain the original tile detail, facade photographs, windows,
doors and authored lighting. Overcast, dusk, moving/settled TAA and FSR Quality,
two-bounce, hardware-T&L/forced-CPU/restored routes and grounded retreat views
are checked through the existing paths. MSAA4 executes, but its existing spatial-
only single-sample GI remains grainy; this is not clean MSAA visual acceptance.

## Measured GPU cost

TAA at 100% scale, one bounce, RT sun/cloud shadows, AO/water/volumetrics off.
Each timing scene warms for ten seconds, then uses GI Off/On/On/Off with four
seconds settling and six seconds sampling per interval. Probes are off; no
build, heavy census or other game runs during timing. Values below are averages
of the two GI-on interval medians; the receipt preserves every interval and
sample array. World/output resolution is verified live.

| Scene / output | GI-on GPU frame before / final | GI trace+filter+history before / final |
| --- | ---: | ---: |
| Church side 1920x1080 | 2.273 / 2.318 ms | .367 / .411 ms |
| Church side 2560x1440 | 3.386 / 3.415 ms | .542 / .602 ms |
| Fire-station town 1920x1080 | 5.289 / 4.983 ms | 1.041 / 1.015 ms |
| Fire-station town 2560x1440 | 5.145 / 5.072 ms | .992 / 1.001 ms |
| Chapel front 1920x1080 | 5.809 / 6.091 ms | .669 / .702 ms |
| Chapel front 2560x1440 | 6.001 / 6.054 ms | .627 / .644 ms |

The church samples add about .044/.060 ms to measured GI stages. These are
bounded scene costs, including existing terrain/materials, not an isolated tile
benchmark or a universal budget. The town's lower full-frame figures are not
claimed as an optimization: CPU recording and GI-off frame costs also vary.
Likewise the chapel's .283 ms 1080p frame increase is mostly present already
with GI off (.314 ms); it is not an isolated new-GI cost. Native chapel reception
remains gated even though reflected source participation changes. No sample or
filter budget is increased to obtain these results.

The nine Vulkan baseline/candidate/supplementary runs present **139,557 frames**,
synchronization validation on, zero errors and zero warnings, and normal process
exit. The tested Vulkan executable SHA256 is
`99BD79AAB9D78C7258FE0E0572DE325B8BCBDF3C9738B61741C9F0C59CD30C3A`.
It was built from the stated base plus this batch's source changes before the
documentation commit; source HEAD alone does not identify the loaded bytes.

## Tests and reproducible census

Public Vulkan game/tools/tests and GL33 game/tests build. Focused filter
`[vulkan-shape],[Shaders],[enhanced-material],[ray-surface],[render-flags]` passes
**80 cases / 66,611 assertions** on Vulkan and **79 / 66,571** on GL33. Python
inventory/census tests pass 18 and driver-free Vulkan policy guards pass 690.
The intentional policy teardown error is not a gameplay validation finding.
Tests pin all three identities, unchanged v3, namespace/extension boundaries,
the other 28 whole-source refusals and original material/capture gates.

Fresh censuses cover **726 models / 1,708 ordinary-adapter retained levels**
and, separately, **726 / 1,711 authored levels**. All previous complete
prerequisites reproduce after replaying **only new identities 185-187**. All
original section/UV/normal/hint metadata and all 269 surveyed geometry/ordered
binding fingerprints remain unchanged. The three forest source-only levels
still have no retained historical native comparison. Counts do not certify
live reception or complete optical review.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-12/final-runtime-census `
  --output build/material-modernization-20261021/batch-12/final-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 187 `
  --replay-regional-identities @(512..543)
```

Add `--all-source-lods` with a separate cache/output under
`batch-12/final-source-census` for the authored-source diagnostic. Original
decoded images and full native vertex/UV/normal buffers remain ignored locally;
only metadata/fingerprints and real gameplay captures are published.

Live GL33 Off/On/Enhanced/Auto/Off controls remain unsupported for Vulkan-only
features and have **zero changed world-ROI pixels**. All **270 original input
hashes** and **67 settings/fixture/input hashes** match their retained manifests.
Private DLSS is clean and remote-free, with both SDKs and preserved build/runtime
snapshots present. Full measurements, artifact/policy hashes, counters, probes
and preservation are in the [qualification receipt](evidence/material-modernization/batch-12/qualification.json).

The ledger records **60 complete source reviews, 122 partial, 333 pending and
211 retained existing contracts**. Twenty-seven previously pending models now
have partial roof/chimney review; all 64 surveyed models still have unresolved
or unreviewed source boundaries. The 30 pilot tasks retain 11 complete reviews,
16 partial and three retained contracts. The full inventory goal is incomplete;
these states are not rendering-coverage percentages.
