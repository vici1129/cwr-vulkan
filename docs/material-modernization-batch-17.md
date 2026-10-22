# Exposed concrete barriers and military structures

Base: `78664ff6c2191d1e9c4ac9ce840ab6d2b5e2dfbc`.
This family audit inspects **10 models / 37 authored visual levels**. Three
independently reviewed original source images use existing enhanced/automatic
PBR and GI paths; this is not whole-building or complete world rendering coverage.

## Physical source decisions

| Identity | Original source | Source/geometry interpretation | Decoded RGBA fingerprint |
| --- | --- | --- | --- |
| 190 | `data/beton_universal.pac` | Weathered mineral concrete on exposed pyramid-barrier sides | 256x256, `6959856631353983829` |
| 191 | `data/beton_preklad_opak.pac` | Rough concrete structure faces on garage/ammunition-store/lighthouse-base models | 256x64, `8379358289529718884` |
| 192 | `data/panel_maskac_opak.pac` | Weathered painted concrete on thick firing-position panels | 256x128, `3981822726900047474` |

Coefficients remain bounded, unmeasured rough-dielectric assumptions: concrete
family 7, roughness .90, F0 .04, metallic 0. Original photos, geometry, UVs,
normals, lighting hints, settings and all existing material paths are preserved.
No artwork, normal/roughness maps, shader or ray/filter budget is generated or changed.

The first image also occurs on **six downward-facing hospital underside
triangles**, not a visible walkway. The face normals corrected an initial
walkway hypothesis; this is explicitly **not a claimed hospital improvement**.
The barrier's separate `data/beton_top.pac` is on its buried underside; its
other use is rejected by original authored lighting. It is not registered to
increase invisible qualified counts.

Garage faces contain original authored-lighting triangles alongside ordinary
triangles, including within some original polygon fans. Whole-source physical
identity is constant across these faces; no new UV boundary or triangulation is
introduced. Existing per-triangle authored-lighting rejection remains intact.
An ordinary neighboring triangle does not inherit that rejection, as required by
the existing classifier and tests. Source, render/opacity, geometry, static
instance and captured ambient/direct gates remain independent and unchanged.

The dark `beton_preklad_opakd` sibling is not an alias or neutral-lighting proof.
It remains legacy, as do garage doors, vehicle/crate atlases, optical lamp-cap/
cavity images, metal supports and the ambiguous brown pole sleeve. The perforated
`beton_cesta` photograph does not turn its hole marks into uniform concrete or
metal. Source-specific reasons are in the
[source audit](evidence/material-modernization/batch-17/source-audit.json).
Ten source-unknown identities were physically inspected; **94 decoded references
are not 94 material approvals**. Other mixed images remain pending.

## Actual gameplay and limits

Original Malden `01TakeTheCar` runs through the existing direct developer harness
with copied profiles, `--nosound` and synchronization validation. Cameras are
terrain-grounded standing views. This is not Steam-route or mission-completion
acceptance. Preliminary Malden/Everon scouts are not candidate acceptance runs.

The three paired views target exposed barriers, the garage entrance and firing
panels. The first GI-on native diagnostic footprints show new concrete-family
reception of **6,900** barrier pixels and **28,743** panel pixels, versus zero in
the corresponding baseline samples. Forced CPU footprints are **6,899** and
**28,773**; native restoration remains active. The garage demonstrates **32,580
CPU pixels but zero native pixels** in this view. No rejecting capture gate is
inferred or weakened; no broad native garage adoption is claimed. These are
half-resolution family/frame diagnostics, not all-world or source-specific
rendering percentages.

| Standing view | Before native | After native | After forced CPU |
| --- | --- | --- | --- |
| Pyramid barriers | [before](evidence/material-modernization/batch-17/modernization-b17-baseline-abel-1080-concrete-blocks-gi.png) | [after](evidence/material-modernization/batch-17/modernization-b17-final-abel-1080-concrete-blocks-gi.png) | [CPU](evidence/material-modernization/batch-17/modernization-b17-final-abel-1080-concrete-blocks-CPU.png) |
| Garage entrance | [before](evidence/material-modernization/batch-17/modernization-b17-baseline-abel-1080-concrete-garage-gi.png) | [after](evidence/material-modernization/batch-17/modernization-b17-final-abel-1080-concrete-garage-gi.png) | [CPU](evidence/material-modernization/batch-17/modernization-b17-final-abel-1080-concrete-garage-CPU.png) |
| Firing panels | [before](evidence/material-modernization/batch-17/modernization-b17-baseline-abel-1080-concrete-panel-gi.png) | [after](evidence/material-modernization/batch-17/modernization-b17-final-abel-1080-concrete-panel-gi.png) | [CPU](evidence/material-modernization/batch-17/modernization-b17-final-abel-1080-concrete-panel-CPU.png) |

Bounded phase/grid probes independently observe four eligible reflected hits
for source190, ten for source191 and twelve for source192, with repeated keys
collapsed. Thus the garage source has observed reflection participation despite
zero native reception in its sampled view. These observations are not a
statistical transport distribution or all-world coverage measure.

Original garage authored-lit faces, doors and dark sources retain their response;
CPU comparison does not reproduce the rejected roof-darkening behavior from
batch 15. Overcast/dusk, moving/settled TAA and FSR Quality, two-bounce,
enhanced/auto, native/forced-CPU/restored and grounded 0/8/22/52/92-metre retreat
controls are exercised. Existing MSAA spatial-only GI grain is not clean MSAA
acceptance. Original terrain/water appearance is not clean whole-scene acceptance.
Mission state and original runtime detail selection can differ across runs;
captures are not bit-exact cross-run comparisons.

## GPU cost, validation and preservation

RTX 4060 Ti, TAA100%, one bounce, RT sun/cloud shadows and AO/water/volumetrics
off. Each scene warms ten seconds, then samples Off/On/On/Off with four-second
settling and six-second intervals. Probes are off, with no builds, heavy census
or other games during timing. The table averages paired GI-on interval medians;
all samples, GI-off intervals and CPU-recording medians remain in the receipt.

| Scene / output | GPU frame before / after | Trace+filter+history before / after |
| --- | ---: | ---: |
| Barriers 1920x1080 | 3.971 / 3.965 ms | .524 / .556 ms |
| Barriers 2560x1440 | 4.116 / 4.022 ms | .472 / .494 ms |
| Garage 1920x1080 | 2.309 / 2.452 ms | .346 / .382 ms |
| Garage 2560x1440 | 3.369 / 3.398 ms | .523 / .552 ms |
| Firing panels 1920x1080 | 6.064 / 6.206 ms | 1.149 / 1.320 ms |
| Firing panels 2560x1440 | 6.239 / 6.334 ms | 1.200 / 1.362 ms |

The new participation has a measured cost: panel-view GI stages rise about
.16-.17 ms. These are scene measurements, not an isolated source benchmark or
universal budget. CPU recording and GI-off costs also vary; negative frame
deltas are not optimization claims. No samples, safety gates or ray/filter
budgets are changed to obtain these results.

Four paired Vulkan runs close normally after **104,835 presented frames**,
synchronization validation on, **zero errors and warnings**. The candidate
executable SHA256 is
`AC7A6CD1746F2DD70F6153DDF1E56D397DDF2693BAC217FC8546E5E6FB9338DC`,
built from the stated base plus this batch's data/tests before the documentation
commit. Source HEAD alone does not identify loaded bytes. Baseline snapshots
remain local and hash-pinned.

Live GL33 Off/On/Enhanced/Auto/Off controls have zero changed world-ROI pixels.
All **270 original input hashes** and **67 settings/fixture/input hashes** match.
Private DLSS remains clean and remote-free; SDKs and preserved snapshots remain.
Full intervals, route controls, actual process identities and preservation are
in the [qualification receipt](evidence/material-modernization/batch-17/qualification.json).

## Source census and tests

Fresh ordinary-adapter and all-source censuses cover **726 / 1,708 retained** and
**726 / 1,711 authored visual levels** respectively. All 726 original historical
reports verify. Complete prior prerequisites reproduce after replaying **only
sources190-192**, with zero differences; all 37 surveyed geometry/ordered-binding
identities, normals, UVs, flags and authored hints are unchanged. The same three
forest source-only levels have no retained historical native comparison.

There are 331 additional highest-detail prerequisites / 915 across alternatives.
These include the hidden hospital underside, are not receiver counts and are not
an improvement target or rendering percentage. No original inventory is rewritten.

Focused filter `[vulkan-shape],[Shaders],[enhanced-material],[ray-surface],[render-flags]`
passes **84 cases / 94,556 assertions** on Vulkan and **83 / 94,516** on GL33.
Driver-free policy guards pass 690; inventory/census Python tests pass 18.
The intentional driver-free teardown error is not a gameplay validation finding.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-17/final-runtime-census `
  --output build/material-modernization-20261021/batch-17/final-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 192 `
  --replay-regional-identities @(512..559)
```

Add `--all-source-lods` with a separate cache/output under
`batch-17/final-source-census`. Original decoded artwork and full native buffers
remain ignored locally.

The ledger records **198 complete source assessments, 127 partial, 190 pending
and 211 retained existing contracts**. Barrier, lamp and both firing-position
models gain complete assessment, including explicit retained optical cases;
four other previously pending models gain partial review. Remaining garage/
vehicle/crate, lighthouse and facade images are not silently closed. The original
726-model goal continues; these are assessment dispositions, not rendering percentages.
