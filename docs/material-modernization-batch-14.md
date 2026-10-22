# Roadside concrete and sign/post source assessment

Base: `d599b9b7f26e1e0d0625df57998d693a97c47c9f`.
This batch assesses recurring original roadside signs and marker/barrier models.
Only the dedicated concrete source gains rendering support; assessment of a
sign is not a new sign-material adoption or a coverage percentage.

## Original source and geometry

The [source audit](evidence/material-modernization/batch-14/source-audit.json)
inspects **131 models / 177 authored visual levels** using the native adapter.
Ten source-unknown identities receive original-pixel and geometry/UV review.
Ninety-three referenced images are decoded locally; the remaining gated images
are **not** claimed as 93 optical material approvals. Their original alpha/blend,
lighting and geometry fallback contracts are retained and independently checked.

`data/patnik_new.pac`, original SHA256
`8fba30e6fdc85ad1686757edc5cac76a61bfdd1afdefb889d2aa92c8b05db731`,
is a dedicated 128x256 opaque weathered/aggregate concrete source. The decoded
RGBA fingerprint is `16035785947823801141`. The original image includes the
concrete cap and mossy/weathered sides, not hardware, apertures or a reflector.
Existing concrete family 7, roughness .90, dielectric F0 .04 and metallic 0
are reused as unmeasured assumptions, not recovered optical measurements.

Whole-source identity **188** applies to `data3d/patnik.p3d` and
`data3d/patniky.p3d`: six and five complete original quad faces respectively.
Both have one authored visual level, valid normals, ordinary lighting and
original U/V clamping. The original cap U overshoot to 1.00755 remains clamped;
no geometry, UV, normal, lighting hint or pixel is modified. The 22 additional
highest-detail prerequisites are a diagnostic result, not the adoption target.
These two models have 116 original placements across Everon, Kolgujev and Nogova.

The barrier's separately textured rail keeps existing identity 86 and its
authored signpost lobe hint. That source remains PBR/reflector-only; it does not
inherit concrete reception. Stock v3 and regional identities 512-556 are
unchanged. Existing source, render/opacity, geometry, authored-lighting,
static-instance and captured ambient/direct-light gates remain authoritative.
No shader, material map, artwork, ray/filter budget or safety gate changes.

## Explicit retained cases

Nine other source-unknown identities keep their original response. The audit
records each exact source and its original-face/UV extents, rather than treating
opaque pixels or a similar filename as material proof.

- `data/rez_trubka.pac`: dark rust/oxide/metal pipe photograph; the original
  faces do not independently separate coating, oxide and exposed conductor.
- `data/sloupek_bily_orezly.pac` and `data/sloup_bily_orez_v.pac`: mixed worn
  paint/substrate and photographic shading. They are not aliases of the
  separately supported underscored post source. No source-proved whole-face
  coating boundary is promoted.
- Hospital and fuel-sign back images contain sheet, mounting rails and fasteners
  spanning the same faces; no guessed uniform dielectric or metal response.
- Hospital, fuel, dead-end and gravel-warning artwork does not establish an
  independent coating/enamel/reflective-layer contract. No retroreflective,
  conductor or coating reconstruction is invented.

These are completed source assessments with concrete unresolved optical
reasons, not implemented rendering coverage. Degenerate/invalid neighboring
sign geometry, transparent sign cards and authored lighting retain their
original gates. No repair or gate relaxation is used to qualify them.

## Actual gameplay appearance

Original Nogova `R01WarCry.Noe` gameplay is launched directly through the existing
developer harness, using copied profiles and `--nosound`. Grounded standing
cameras inspect original parking-edge blocks and their row of rails. This is
not Steam-route or mission-completion acceptance. Initial fence/tree-obstructed
scouts are excluded; clear paired views are used for stills and timing.

| Clear standing view | Before | Candidate |
| --- | --- | --- |
| Near barrier | [before](evidence/material-modernization/batch-14/modernization-b14-baseline-noe-1080-marker-barrier-gi.png) | [after](evidence/material-modernization/batch-14/modernization-b14-final-noe-1080-marker-barrier-gi.png) |
| Marker row | [before](evidence/material-modernization/batch-14/modernization-b14-baseline-noe-1080-marker-row-gi.png) | [after](evidence/material-modernization/batch-14/modernization-b14-final-noe-1080-marker-row-gi.png) |

Original aggregate, weathering and cap detail remain. Native concrete samples
change from zero to **5,041 / 4,455 pixels** at the half-resolution diagnostic
footprint. These are frame samples, not world percentages. The forced-CPU row
sample has 4,457 concrete pixels; the near CPU total of 10,330 also includes
existing neighboring concrete and is not a new-source pixel count.

Overcast, dusk, moving/settled TAA and FSR Quality, two-bounce operation,
hardware-T&L/forced-CPU/restored routes and grounded retreats are exercised.
MSAA executes and validates but retains visibly grainy spatial-only single-
sample GI; it is **not** a clean MSAA visual acceptance result. No filtering or
sample increase hides this limitation. Unmodified full-size captures and
bounded probe/counter details are retained in the qualification receipt.
The bounded grid/phase observations contain **9 / 31 eligible reflected hits**
on source188 in the near/row views. These observations demonstrate sampled
reflector execution, not an all-world transport distribution or coverage measure.

## GPU cost and preservation

RTX 4060 Ti, TAA at 100% scale, one bounce, RT sun/cloud shadows;
AO/water/volumetrics off. Each scene warms ten seconds, then measures GI
Off/On/On/Off with four-second settling and six-second sampling intervals.
Probes are off; no build, heavy census or other game runs during timing.
Values below average the paired GI-on interval medians, not all-world cost.

| Scene / output | GI-on GPU frame before / final | GI trace+filter+history before / final |
| --- | ---: | ---: |
| Near barrier 1920x1080 | 7.011 / 7.120 ms | 1.523 / 1.728 ms |
| Near barrier 2560x1440 | 7.435 / 6.858 ms | 1.806 / 1.611 ms |
| Marker row 1920x1080 | 8.139 / 8.011 ms | 2.734 / 2.644 ms |
| Marker row 2560x1440 | 8.873 / 8.674 ms | 2.825 / 2.825 ms |

The 1080p near-barrier candidate trace intervals are .914 and 1.218 ms; both
remain in the average. The 1080p row has substantial interval variation in
both baseline and candidate, and its GI-on full-frame samples are lower than
GI-off. These are variable scene measurements, **not** an isolated additive
GI benchmark or evidence that adding GI improves performance. CPU recording
and GI-off costs also vary. No sample/filter budget changes, outlier removal
or universal optimization claim are used to present the results.

Four Vulkan baseline/candidate runs present **37,153 frames**, synchronization
validation on, zero errors/warnings and normal process exit. The tested Vulkan
executable SHA256 is
`58DC17327D8C5B037CAE21DF349A93DD94B0100ECE082EF9D559534CE0A9EE21`.
It was built from the stated base plus this batch's source changes before the
documentation commit; source HEAD alone does not identify loaded bytes.

Live GL33 Off/On/Enhanced/Auto/Off controls have zero changed world-ROI pixels.
All **270 original input hashes** and **67 settings/fixture/input hashes** match.
Private DLSS remains clean and remote-free, with SDKs and preserved snapshots
present. Full artifact/policy hashes, intervals, counters, probes and preservation
are in the [qualification receipt](evidence/material-modernization/batch-14/qualification.json).

## Tests and reproducible census

Public Vulkan game/tools/tests and GL33 game/tests build. Focused filter
`[vulkan-shape],[Shaders],[enhanced-material],[ray-surface],[render-flags]` passes
**82 cases / 87,992 assertions** on Vulkan and **81 / 87,952** on GL33. Python
inventory/census tests pass 18; driver-free Vulkan policy guards pass 690.
The intentional policy-test teardown error is not a gameplay validation finding.
The tests pin source188, refusal of the nine uncertain sources, unchanged v3
and rail hints, namespace/extension boundaries and independent rejection gates.

Fresh censuses cover **726 / 1,708 ordinary-adapter retained levels** and,
separately, **726 / 1,711 authored levels**. Previous complete prerequisites
reproduce after replaying **only new identity188**. All original section/UV/
normal/hint metadata and all 177 surveyed geometry/ordered-binding identities
remain unchanged. The same three forest source-only levels remain outside a
retained historical native comparison; all 726 immutable historical reports
verify. The original inventory is unchanged.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-14/final-runtime-census `
  --output build/material-modernization-20261021/batch-14/final-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 188 `
  --replay-regional-identities @(512..556)
```

Add `--all-source-lods` with separate cache/output under
`batch-14/final-source-census` for the authored-source diagnostic. Original
decoded images and full native vertex buffers remain ignored locally;
only metadata/fingerprints and actual gameplay captures are published.

The ledger now has **191 complete source assessments, 122 partial, 202 pending
and 211 retained existing contracts**. The 131 roadside models each have a
disposition for every source-unknown identity, including explicit retained
reasons. This does not mean 131 new PBR/GI adoptions, complete optical review of
every gated image, or 191 fully modernized models. The 30 pilot tasks retain
11 complete reviews, 16 partial and three retained contracts. The full
726-model goal remains incomplete.
