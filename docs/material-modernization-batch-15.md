# Village bell masonry and ambient-preserving clay tiles

Base: `087912ecbd9a2c8c0865bfa3d9aabc47c04845a2`.
This batch reviews shared village-building and masonry sources across
**12 models / 51 authored visual levels**. It does not complete the original-world
goal or claim broad facade relighting.

## Admitted material paths

`data/zvonicka_side.pac` is the original weathered mineral body of the roadside
bell station: pedestal, column, frame and cap. Its bell is separately textured
with the original alpha/cutout `data/zvonicka_bell.paa`, which stays legacy.
Original pixels, geometry/UVs and a standing world view support the mineral
body, not a guessed metal bell or a whole building atlas. Whole-source identity
**189**, masonry family 6, roughness .90 uses the existing enhanced/auto paths.
The 128x256 decoded RGBA fingerprint is `13867833788182690161`.

The independently reviewed `data/tasky_tmavsi90.pac` contains overlapping clay
tiles. It is not a filename alias or bit-identical copy of the supported tile
source. Its 256x256 RGBA fingerprint is `13546172877813697002`.
The admitted policy uses **three exact-model regions 557-559**, not a whole-source
entry: `data3d/dum_mesto.p3d`, `data3d/dum_mesto_in.p3d` and
`data3d/mesto_dum.p3d`. They require the original pixels, model and **14 exact
geometry/ordered-binding identities**, including exact binary resolutions.
The U -4..4 / V -.003..2.375 bounds retain the original repeated/clamped tile
coordinates and complete original faces. No polygon is split or repaired.

These tile regions are **PBR/reflector-only, `reception=false`**, through the
existing ambient-preserving region contract. They retain original ambient in
both native and CPU routes. Only auto consumes them; v3 and enhanced-only tile
behavior remain unchanged. The bell source is available in enhanced and auto.
Existing region 529's ambient-preserving contract is unchanged.

Coefficients are existing unmeasured rough-dielectric assumptions: roughness .90,
F0 .04 and metallic 0. No artwork, material map, shader, ray/filter budget,
normal, authored-lighting hint or safety gate changes. Original opacity/render,
geometry, static-instance and captured-lighting gates remain independent.

## Receiver trial rejected by appearance, then constrained by existing gates

An unshipped whole-source roof receiver trial, identity190, darkened the distant
CPU-lit roof relative to the preserved baseline. Compare the
[baseline CPU view](evidence/material-modernization/batch-15/modernization-b15-baseline2-abel-cpu-village-bell-CPU.png),
[withdrawn receiver trial](evidence/material-modernization/batch-15/modernization-b15-final-abel-1080-village-bell-CPU.png)
and [admitted ambient-preserving view](evidence/material-modernization/batch-15/modernization-b15-accepted-abel-1080-village-bell-CPU.png).
The trial's native reception was not sufficient to override this failure.
It is not part of the admitted policy.

A subsequent family-zero whole-source registration failed the existing
enhanced-catalog invariant `receiverFamily>=2`. That test was **not changed**,
skipped or converted to an exception. The invalid row was removed. The existing
exact-model region path instead supplies stronger image/geometry/binding proof
and explicitly retains ambient, as designed. No source190 whole-image identity
is shipped. The rejected policy/test receipt and exact receiver-pilot executable
identity are retained in the evidence.

## Actual gameplay and limits

Original Malden `01TakeTheCar` gameplay uses the existing direct developer
harness, copied profiles and `--nosound`. Cameras are grounded standing views,
not a new mesh preview or an aerial roof inspection. This is not Steam-route
or mission-completion acceptance. Gable-only and foliage-obstructed scouts and
their incomplete timing run are excluded; a fresh baseline2 uses the verified
clear view. Runtime detail selection differs across some runs, so captures are
not a bit-exact cross-run optical comparison. All original 51 source geometry/
binding identities remain unchanged; same-runtime controls separate responses.

| Standing view | Before | Admitted policy |
| --- | --- | --- |
| Bell station / village | [before](evidence/material-modernization/batch-15/modernization-b15-baseline2-abel-1080-village-bell-gi.png) | [after](evidence/material-modernization/batch-15/modernization-b15-accepted-abel-1080-village-bell-gi.png) |
| City-house roof | [before](evidence/material-modernization/batch-15/modernization-b15-baseline2-abel-1080-village-roof-gi.png) | [after](evidence/material-modernization/batch-15/modernization-b15-accepted-abel-1080-village-roof-gi.png) |

Native bell reception remains **zero** in these samples. Forced CPU reception
is independently demonstrated at approximately 2,500 masonry pixels in the
half-resolution diagnostic footprint. Two bounded reflected-hit observations
identify new source189. No rejecting capture gate is inferred or relaxed.
The admitted roof regions claim **no new reception**. Bounded probes observe no
new tile-region hit in the sampled phases; that is neither observed all-world
transport nor proof of zero transport elsewhere. Existing nearby roofs also
contribute to family counters, which are not source-specific percentages.

The distant CPU roof keeps its baseline tile appearance after ambient retention.
The nearer CPU roof also has an existing dark face in the baseline; this is
**not** clean whole-scene CPU acceptance. Original facade photographs, windows,
bell cutouts and authored lighting remain unchanged. Auto/enhanced material
controls, overcast, dusk, moving/settled TAA and FSR Quality, two-bounce,
native/forced-CPU/restored and grounded retreat views are exercised. MSAA retains
its existing spatial-only single-sample GI grain, not clean MSAA acceptance.

## GPU cost and validation

RTX 4060 Ti; TAA100%, one bounce, RT sun/cloud shadows;
AO/water/volumetrics off. Each scene warms ten seconds, then uses GI
Off/On/On/Off with four-second settling and six-second sampling intervals.
Probes are off; no build, heavy census or other game runs during timing.
The table averages the paired GI-on interval medians; every interval remains
in the receipt. These are scene measurements, not an isolated material benchmark.

| Scene / output | GI-on GPU frame before / admitted | GI trace+filter+history before / admitted |
| --- | ---: | ---: |
| Bell/village 1920x1080 | 4.871 / 4.913 ms | .570 / .572 ms |
| Bell/village 2560x1440 | 5.196 / 5.127 ms | .548 / .557 ms |
| Roof-side street 1920x1080 | 4.249 / 4.101 ms | .519 / .498 ms |
| Roof-side street 2560x1440 | 4.351 / 4.375 ms | .509 / .511 ms |

Negative deltas are not optimization claims: CPU recording, GI-off costs and
original runtime detail selection also vary. Most measured GI work is existing
terrain/materials; the admitted tile regions do not receive GI. No filter/sample
change, discarded outlier or universal performance budget is implied.

Five admitted baseline/candidate/supplementary runs present **80,369 frames**,
synchronization validation on, zero errors/warnings and normal process exit.
The withdrawn receiver pilot's two candidate runs present 29,843 frames with
zero validation findings, but **fail visual adoption** and are counted separately.
The earlier 85,607-frame diagnostic run is excluded from both acceptance totals.
The admitted executable SHA256 is
`5E383D5E22C90B4BAD2BE9514222C063F3D45639D8354ED5460AFE2A590DB501`.
It was built from the stated base plus this batch's data/tests before the
documentation commit; source HEAD alone does not identify loaded bytes.

Public Vulkan game/tools/tests and GL33 game/tests build. Focused filter
`[vulkan-shape],[Shaders],[enhanced-material],[ray-surface],[render-flags]` passes
**83 cases / 94,434 assertions** on Vulkan and **82 / 94,394** on GL33. Python
inventory/census tests pass 18; driver-free Vulkan policy guards pass 690.
The intentional policy teardown error is not a gameplay validation finding.
The unchanged receiver-family invariant and explicit native/CPU ambient-retention
tests pass. Live GL33 Off/On/Enhanced/Auto/Off controls have zero changed world-ROI
pixels. All **270 original input hashes** and **67 settings/fixture/input hashes**
match. Private DLSS is clean and remote-free; SDKs and preserved snapshots remain.

## Source assessment and reproducible census

The [source audit](evidence/material-modernization/batch-15/source-audit.json)
records 31 physically reviewed source-unknown identities and a supported-tile
comparison. The 110 locally decoded referenced images are not 110 optical
approvals; eight additional city-house source images remain unreviewed.
Mixed windows/doors/cornices, timber/substrate uncertainty, gutter metal,
chimney apertures and shaded/photo patches retain concrete source-specific
reasons. The blank-plaster U -20..20 / V 0...92 trial admits zero complete
original faces in `deutshe_mini`; no broad plaster coverage is added.

Broad hole-free rectangles for `o/divk/zed_diry.pac` split original polygon fans
and were not admitted. A narrower `hrdb01` U0..2 / V0...50 candidate preserves
whole fans across three levels, but most of its 13.665 square metres are
horizontal (only .709 square metres near-vertical). It remains actionable
pending a standing-view/value and receiver-seam check, not implemented wall
coverage. No new masonry region was added by this batch.

Fresh censuses inspect **726 / 1,708 ordinary-adapter retained levels** and
separately **726 / 1,711 authored levels**. Previous complete prerequisites
reproduce after replaying **only source189 and regions557-559**. Original
sections/UVs/normals/hints and all 51 geometry/binding identities are unchanged.
The same three forest source-only levels remain outside a retained historical
native comparison; all 726 immutable historical reports verify. The 193 new
highest-detail prerequisites include reflector-only regions and are not a
receiver count, percentage or adoption target.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-15/accepted-runtime-census `
  --output build/material-modernization-20261021/batch-15/accepted-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 189 `
  --replay-regional-identities @(512..559)
```

Add `--all-source-lods` with separate cache/output under
`batch-15/accepted-source-census`. Original decoded images and full native
buffers remain ignored locally; only metadata/fingerprints and actual gameplay
captures are published. Full intervals, controls, rejected trials, artifact/
policy hashes and preservation are in the [qualification receipt](evidence/material-modernization/batch-15/qualification.json).

The ledger records **192 complete source assessments, 125 partial, 198 pending
and 211 retained existing contracts**. Only the bell model gains complete source
assessment; the other 11 surveyed models remain partial. The 30 pilot tasks
retain 11 complete reviews, 16 partial and three retained contracts. These are
assessment dispositions, not rendering percentages; the 726-model goal continues.
