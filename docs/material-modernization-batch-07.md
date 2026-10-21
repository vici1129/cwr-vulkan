# Batch 07: correct LOD provenance and retain canopy contracts

Base: `29361318ba8aa4a22fb039dcad792e7827140040`. This is a diagnostic correction,
not a material-coverage increase. No new material identity, receiver, shader,
transport budget, default or gameplay LOD policy is introduced.

## Authored levels are not retained runtime levels

The original inventory contains **726 world-placed models / 1,711 authored visual
LODs**. The ordinary gameplay-adapter census retains **1,708 visual LODs** under
the inspection application's configuration. Three original resolution-40 levels
are removed by the existing `notl=1` / hardware-lighting optimization:

- `o/tree/les_nw_jehl_t1.p3d`
- `o/tree/les_nw_jehl_t2.p3d`
- `o/tree/les_nw_jehl_trojuhelnik.p3d`

The old parser joined native results to declared source indices without checking
their resolutions. After optimization compacted the indices, it attached collision
LOD vertex/normal metadata to those three removed visual slots. They had no visual
qualification sections. This invalidates the earlier description of 1,711 levels
as gameplay-retained inspections, not the original source inventory or the
locally verified material batches. No accepted material region used these slots.

The parser now checks current native resolutions and fails closed on unexpected
retained-visual reindexing; it never copies nonvisual metadata into visual slots.
It does not guess by resolution: four placed models have duplicate resolutions.
All 726 historical raw reports first reproduce their immutable inventory records
exactly. Historical correction is explicitly separate from current parsing.
One legacy report, `data3d/les trojuhelnik pruchozi.p3d`, also printed four malformed
resolution floats; these are exposed as diagnostics, not accepted from a current
tool. Its declared identities and complete prerequisites match fresh inspection.
The original inventory and historical batch receipts remain unchanged.

`PoseidonTools model inspect --runtime-loader --all-source-lods` now offers an
explicit **diagnostic source-only** mode. It scans the original adapter output
before LOD removal; ordinary game callers retain the previous optimization path.
It changes no opacity, authored-lighting, geometry or material gate. The new
[source census](evidence/material-modernization/source-census.json) inspects all
1,711 authored visual levels. The separate
[ordinary-adapter census](evidence/material-modernization/runtime-census.json)
records the 1,708 retained levels and three explicit exclusions. Live optimization
and LOD selection can additionally depend on game settings and instances; neither
count is a visible rendering-coverage percentage.

The source-only extra levels contain 10 / 7 / 7 triangles respectively, all rejected
on their original cutout/render contract. They have no retained historical native
qualification baseline, which is explicitly recorded rather than certified as a
successful comparison. Every one of the 1,708 retained levels has identical full
section metadata, including UV bounds and regional reception-defer flags, between
the two current modes and against the previous accepted ordinary adapter.
Historical prerequisite comparison is zero-difference after explicit replay of
the already-approved post-inventory identities, on that retained scope.

## Canopy pilots assessed without opaque promotion

Backlog 28 (`o/tree/smrk_maly.p3d`) and 29 (`o/tree/dd_bush02d.p3d`) have been
reinspected across all three original visual LODs each. The five original canopy
images were individually viewed: spruce needle/branch cards, whole-tree impostors,
and two bush cutouts. Their alpha silhouettes and photographed shading are part
of the original appearance, not missing opaque material maps.

The bush uses intersecting, double-sided cutout cards: all vertices have authored
material 202 and land hints. Its 98 alternative-LOD triangles reject on render
behavior. The small spruce has 488 render rejections and 52 authored-lighting
rejections across its alternative LODs; even its separately textured trunk vertices
are authored material 202. Neither model has an ordinary source-unknown candidate.
Native geometry, original face/UV placement, section flags and lighting metadata
were checked without changing them. Branch/leaf pixels are not a solid wood
surface; opaque single-surface dielectric transport cannot supply missing thin-leaf
transmission or independent ambient. These tasks are **assessed: retain existing
contract**, not implemented PBR/GI reception and not proof of complete foliage
rendering coverage. A future thin/cutout vegetation contract is a separate issue;
existing gates are not relaxed to manufacture coverage.

The related bark survey spans 37 models / 114 authored levels, of which 111 are
retained by the ordinary adapter. Nineteen original images were viewed. Twelve
dedicated bark sources remain candidates for the next material batch, pending
their shared-geometry/UV and gameplay acceptance. Bent-birch photo/background and
cut-end-on-canvas sources are not whole-source approvals. No candidate was added
while repairing this provenance defect. Original pixels and per-vertex geometry
dumps remain in ignored local evidence, not public assets.

## Gameplay regression, tests and preservation

Original `Resistance/R01WarCry.Noe` gameplay is checked through the direct developer
harness with copied profiles and `--nosound`, not Steam-route or mission-completion
acceptance. A terrain-grounded foliage hillside and the previously improved smithy
are compared with the previous accepted executable. The hillside is a regression
view, not proof of a particular bark/forest model's reception. No new visual
improvement or universal GPU-cost result is claimed for this diagnostic batch.
The [receipt](evidence/material-modernization/batch-07/qualification.json) contains
runtime hashes, full states, validation shutdowns and paired GPU intervals.

The serial baseline/final runs present **10,684 frames**, synchronization validation
on, zero errors and warnings, with verified shutdown. Unamplified regression views:
[smithy before](evidence/material-modernization/batch-07/baseline2-smithy-gi.png) /
[after](evidence/material-modernization/batch-07/final-smithy-gi.png),
[foliage hillside before](evidence/material-modernization/batch-07/baseline2-pine-gi.png) /
[after](evidence/material-modernization/batch-07/final-pine-gi.png).
The accepted Vulkan executable SHA256 is
`3632095DCF3ED6B944F33814C3DEECFD1AD2221A930187C0D83C99D8927EC905`.

RTX 4060 Ti, 1920x1080 smithy view, ten-second warmup then Off/On/On/Off with
four-second settling and six-second samples: paired GI-on interval medians average
5.390 / 5.566 ms GPU frame, .729 / .748 ms GI trace, and 1.152 / 1.188 ms combined
trace/filter/history. CPU command medians average 14.938 / 14.166 ms. The +.177 ms
frame and +.036 ms GI-scope differences coexist with raster and Off-frame variation;
no rendering policy changed, so these short runs do not isolate a diagnostic-mode
runtime cost or prove zero overhead. No build or other game ran during timing.
An earlier exploratory run's timing is excluded entirely because harness actions
overlapped an interval; the published baseline/final tests are serial replacements.

Public Vulkan game/tools/tests and GL33 game/tests build. Focused Vulkan tests pass
76 cases / 58,357 assertions; GL33 75 / 58,317; Python inventory/census 18; driver-free
Vulkan policy checks 690. The deliberate driver-free teardown error is not a
gameplay validation finding. New tests cover removed-slot/collision confusion,
unexpected reindexing, explicit source-only scope, malformed historical resolution
diagnostics and source LOD geometry/UV preservation.

All 270 original input hashes and 67 retained settings/fixture/input hashes match.
GL33 live Off/On/Enhanced/Auto/Off controls remain unsupported no-ops with zero
changed original-world ROI pixels. Private DLSS remains clean and remote-free, with SDKs and preserved
snapshots present. No original asset or retained setting is edited.

## Reproduction and progress

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-07/runtime-census `
  --output build/material-modernization-20261021/batch-07/runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 165 `
  --replay-regional-identities 512 513 514 515 516 517 518 519 520 521 522 523 524 525 526 527 528 529 530 531 532 533
```

For the separate authored-source census, add `--all-source-lods` and use a separate
cache/output under `batch-07/source-census`. Missing historical reports, unresolved
current index/resolution mismatches and prerequisite differences fail the check;
they are not silently ignored. Earlier commands describe historical snapshots.

The ledger remains **32 complete source reviews, 75 partial, 408 awaiting source
review and 211 retained prerequisite dispositions**. Backlog 28/29 now carry concrete
source-reviewed retain reasons; their pixels are not claimed as newly covered.
The overall goal is incomplete. Shared bark and unresolved mixed-atlas work continue.
