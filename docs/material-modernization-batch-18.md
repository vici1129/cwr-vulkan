# Castle gate masonry and slit-free original faces

Base: `9f0d934968751133637d996d2ad793e3b4598a23`.
This family assessment covers **nine models / 33 authored visual levels**.
Only one whole source and one exact-model region are adopted. A second whole-
source trial is withdrawn for an actual CPU raster-lighting discontinuity.

## Admitted paths and retained originals

Whole source **193**, `o/divk/zed.pac`, is independently inspected continuous
original stone/mortar on gates and corners. Its 512x512 RGBA fingerprint is
`1855102579002673049`. It uses existing enhanced/automatic masonry paths.

Automatic-only region **560** selects the lower slit-free mineral/plaster part
of `o/divk/zed_lv03.pac` on **only `o/divk/zedlv.p3d`**. It requires the original
512x512 pixels (`1822267094740399805`), model, ordered texture bindings and three
exact geometry identities at binary resolutions 1.5/3/6.5. U0..1 / V.17..1
selects **14/8/8 original prerequisites**, preserving complete polygon fans.
The upper photographed arrow slits and all other model/source uses remain legacy.
The same bounds select no complete faces on `zedvn`; it does not inherit the rule.

Both use existing unmeasured rough-dielectric assumptions: masonry family 6,
roughness .90, F0 .04, metallic 0. No maps or artwork are generated. No original
geometry, UVs, normals, hints, source assets, settings, shaders or budgets change.
All source/render/opacity/geometry/static/captured-lighting gates stay independent.
Original authored-lighting triangles and invalid corner normals remain rejected.
Existing v3 and regions512-559, including ambient-preserving regions, are unchanged.

Root-bearing `r_sm_km` remains legacy. Its tested safe bounds either cut original
fans or select only .659 square metres of mostly tiny rubble/caps despite 117
triangles. Recess/puncture atlases `zed_lv05`, `zed_pr01`, `zed_pr04` and
`zed_pr05` also retain concrete fan-boundary, zero-face or tiny-fragment reasons.
They do not gain broad stone coverage. The [source audit](evidence/material-modernization/batch-18/source-audit.json)
records every trial. Eight source-unknown images were physically inspected;
**47 decoded references are not 47 optical approvals**. Decoded originals and
full UV/position buffers remain ignored locally.

## Whole-source trial rejected by actual appearance

The unshipped `o/divk/hrad_zedop.pac` trial194 is physically mineral, but creates
a hard horizontal lighting boundary across the original CPU-lit continuous wall.
Same-runtime controls isolate the failure: it persists with **PBR auto / GI off**
and disappears with **PBR off**. It is not merely a GI receiver issue.

The whole source is **withdrawn entirely**. No ambient-only assumption, new ray-
only path, shader change, family-zero exception or gate relaxation is used.
Original response is retained. The reduced policy's same-runtime CPU controls
restore the continuous wall. The trial executable/policies, control modes,
screenshots and fixed-view ROI differences remain in the qualification receipt;
these are not calibrated reflectance measurements or whole-scene acceptance.

Compare the trial's same-runtime
[PBR-off wall](evidence/material-modernization/batch-18/modernization-b18-pilot-cpu-controls-ruined-core-CPU-PBR-off.png)
and [PBR-auto / GI-off wall](evidence/material-modernization/batch-18/modernization-b18-pilot-cpu-controls-ruined-core-CPU-auto-GI-off.png)
with the reduced policy's
[PBR-auto / GI-off wall](evidence/material-modernization/batch-18/modernization-b18-accepted-noe-1080-ruined-core-CPU-auto-GI-off.png).

The trial's full 1080p run plus diagnostic controls present 42,840 frames with
zero validation findings, but **fail visual adoption**. They are separate from
the admitted baseline/candidate totals. No whole194 identity is shipped.

## Actual gameplay and limits

Original Nogova `R01WarCry` runs through the existing direct developer harness,
copied profiles and `--nosound`, with synchronization validation. Three terrain-
grounded standing views cover the gate, interior and plaster/slit wall. This is
not Steam-route or mission-completion acceptance. Ivy and foreground vegetation
remain original, and runtime detail/mission state can differ across runs; images
are not bit-exact cross-run comparisons.

Native masonry reception remains **zero** in these samples; no exact rejecting
capture gate is inferred or weakened. The reduced policy's regular forced-CPU
family footprints are **6,792 / 8,313 / 1,198 pixels** at gate/interior/plaster
views, versus zero in baseline samples. These half-resolution family/frame
diagnostics are not individual-source or world rendering percentages.

| Standing CPU view | Before | Reduced policy |
| --- | --- | --- |
| Gate | [before](evidence/material-modernization/batch-18/modernization-b18-baseline-noe-1080-ruined-gate-CPU.png) | [after](evidence/material-modernization/batch-18/modernization-b18-accepted-noe-1080-ruined-gate-CPU.png) |
| Interior | [before](evidence/material-modernization/batch-18/modernization-b18-baseline-noe-1080-ruined-core-CPU.png) | [after](evidence/material-modernization/batch-18/modernization-b18-accepted-noe-1080-ruined-core-CPU.png) |
| Plaster/slit wall | [before](evidence/material-modernization/batch-18/modernization-b18-baseline-noe-1080-ruined-plaster-CPU.png) | [after](evidence/material-modernization/batch-18/modernization-b18-accepted-noe-1080-ruined-plaster-CPU.png) |

Bounded grid/phase probes record four reflected source193 hits at the gate and
one source193 plus three region560 hits in the interior. The plaster probe
observes no eligible new identity in its sampled phases. This is bounded
reflection evidence, not a statistical transport distribution or proof of
all-world transport. No withdrawn-source participation is counted as admitted.

Overcast/dusk, moving/settled TAA and FSR Quality, two-bounce, enhanced/automatic,
native/CPU/restored and grounded 0/8/22/52/92-metre retreat controls are exercised.
Existing CPU/native differences and MSAA spatial-only GI grain are not clean
whole-scene/MSAA acceptance. No broad native castle relighting is claimed.

## GPU cost, validation and preservation

RTX 4060 Ti, TAA100%, one bounce, RT sun/cloud shadows and AO/water/volumetrics
off. Each scene warms ten seconds, then uses GI Off/On/On/Off with four-second
settling and six-second intervals. Probes are off; no build, heavy census or other
game runs during timing. The table averages paired GI-on interval medians. All
samples, GI-off costs and CPU-recording medians remain in the receipt.

| Scene / output | GPU frame before / reduced policy | Trace+filter+history before / reduced policy |
| --- | ---: | ---: |
| Gate 1920x1080 | 3.654 / 3.729 ms | .446 / .453 ms |
| Gate 2560x1440 | 4.289 / 4.299 ms | .524 / .558 ms |
| Interior 1920x1080 | 4.379 / 4.514 ms | .872 / .916 ms |
| Interior 2560x1440 | 4.590 / 4.563 ms | .959 / .992 ms |
| Plaster/slit wall 1920x1080 | 2.296 / 2.323 ms | .331 / .358 ms |
| Plaster/slit wall 2560x1440 | 3.766 / 3.809 ms | .629 / .681 ms |

These are scene costs, not isolated material benchmarks or a universal budget.
CPU recording and GI-off costs also vary; negative frame deltas are not
optimization claims. Native reception remains zero, while reflection and CPU
paths add real work. No timing samples, filters or safety gates are changed.

Four admitted baseline/candidate runs close normally after **163,263 presented
frames**, synchronization validation on, **zero errors and warnings**. The
withdrawn trial's 42,840 frames are excluded from this total. The admitted
Vulkan executable SHA256 is
`6F0A14974528E8485EF3A3509398CD78F29F0782D1ADF20A880C5834AB3DBC65`,
built from the stated base plus reduced batch data/tests before the documentation
commit. Source HEAD alone does not identify loaded bytes. Baseline and rejected
trial executable snapshots remain hash-pinned locally.

Fresh live GL33 Off/On/Enhanced/Auto/Off controls have zero changed world-ROI
pixels. All **270 original input hashes** and **67 settings/fixture/input hashes**
match. Private DLSS remains clean and remote-free; SDKs and preserved snapshots
remain present. Full modes, intervals, artifacts and preservation are in the
[qualification receipt](evidence/material-modernization/batch-18/qualification.json).

## Source census and tests

Fresh censuses inspect **726 / 1,708 ordinary-adapter retained levels** and
**726 / 1,711 authored levels**. All 726 historical reports verify. Replaying
**only source193 and region560** reproduces all prior prerequisites with zero
differences. All 33 surveyed geometry/ordered-binding identities, UVs, normals,
flags and authored hints are unchanged. The same three forest source-only levels
have no retained historical native comparison.

The 441 new highest-detail prerequisites / 1,116 across alternative levels are
not receiver counts, an adoption target or a rendering percentage. The original
inventory stays immutable. Focused tests pass **85 cases / 96,317 assertions** on
Vulkan and **84 / 96,277** on GL33; 690 policy guards and 18 Python inventory/
census tests pass. The intentional driver-free teardown error is not a gameplay
validation finding. Tests pin the withdrawn source's fallback and protect the
reserved source-identity namespace without changing an existing invariant.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-18/accepted-runtime-census `
  --output build/material-modernization-20261021/batch-18/accepted-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 193 `
  --replay-regional-identities @(512..560)
```

Add `--all-source-lods` with separate cache/output under
`batch-18/accepted-source-census`.

The ledger records **206 complete source assessments, 126 partial, 183 pending
and 211 retained existing contracts**. Eight models gain complete source
assessment, including explicit rejected/retained cases; existing `zed_03`
contract is retained. These are not eight new rendering adoptions or rendering
percentages. The original 726-model goal continues.
