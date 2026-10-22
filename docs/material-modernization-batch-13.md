# Original chimney brick regions

Base: `9f8abf23d30ca52af66603bd33f0e93a224d3390`.
This follows the brick-only candidates identified in batch 12, not an expansion
of its whole-image roof declarations. The full inventory goal remains incomplete.

## Source and original-face proof

The original `o/hous/orlhot_kom.pac` is a 256x128 opaque photograph containing
brick/mortar, a left frame/panel and a right black aperture. It is **not** a
whole-source material. Original source hashes and decoded RGBA fingerprint
`13663165639993867772` identify the reviewed image; opacity alone is not evidence
of physical material or an unobstructed surface.

Thirteen exact-model regions, identities **544-556**, admit only original
triangles entirely inside **U .22...79 / V 0..1**. Every selected polygon fan
remains complete. The first border-adjacent brick faces, left frame/photo margin
and right aperture stay untyped. Nothing is cut, wrapped into another atlas
tile, repainted or supplied with a generated material map.

The [source audit](evidence/material-modernization/batch-13/source-audit.json)
records all 13 models / 50 authored visual levels, exact binary resolutions,
geometry and ordered texture-binding fingerprints, original normals, hints,
material indices, sections and UV bounds. All selected normals have Y=0:
these are vertical chimney walls, not horizontal caps. Selected highest-detail
areas range from .882 to 3.472 square metres per distinct model. There are
64 additional highest-detail prerequisites and 248 across alternative levels;
neither is a visible-coverage percentage or the reason to accept this batch.
The source is used by 127 original Nogova placements; placements are not
multiplied into those triangle counts.

Existing masonry family 6, roughness .90, dielectric F0 .04 and metallic 0
are unmeasured existing-path assumptions, not recovered optical measurements.
Only `triPBR auto` consumes these regional rows. Enhanced-only and v3 whole-source
tables remain unchanged. Original opacity/render, per-triangle geometry,
authored lighting, static-instance and captured ambient/direct-light gates
remain authoritative. Existing region 529 retains its reflection/PBR-only
ambient-preserving contract. No shader, ray/filter budget or gate is changed.

## Actual gameplay appearance and limits

Direct developer-harness runs inspect original `R01WarCry.Noe` placements using
copied profiles, `--nosound` and grounded standing cameras. This is not Steam-route
or mission-completion acceptance. Full-size, unamplified captures preserve
original color detail and the surrounding untreated facade/window/door imagery.
The result is modest chimney detail, not broad facade relighting.

| Standing view | Before | Candidate |
| --- | --- | --- |
| Hotel front | [before](evidence/material-modernization/batch-13/modernization-b13-baseline2-noe-1080-brick-hotel-gi.png) | [after](evidence/material-modernization/batch-13/modernization-b13-final-noe-1080-brick-hotel-gi.png) |
| School | [before](evidence/material-modernization/batch-13/modernization-b13-baseline2-noe-1080-brick-school-gi.png) | [after](evidence/material-modernization/batch-13/modernization-b13-final-noe-1080-brick-school-gi.png) |
| Town houses | [before](evidence/material-modernization/batch-13/modernization-b13-baseline2-noe-1080-brick-house-gi.png) | [after](evidence/material-modernization/batch-13/modernization-b13-final-noe-1080-brick-house-gi.png) |
| Hillside cottage | [before](evidence/material-modernization/batch-13/modernization-b13-baseline2-noe-1080-brick-cottage-gi.png) | [after](evidence/material-modernization/batch-13/modernization-b13-final-noe-1080-brick-cottage-gi.png) |

The town-house native masonry sample changes from zero to 660 pixels and the
cottage from zero to 86 at the half-resolution diagnostic footprint. The school
sample changes from 549 to 569; its totals also include existing masonry, so
569 is not a new-source count. Hotel-front totals are unchanged at 9,035:
that view is explicitly **not** proof of new native chimney reception. Neither
these samples nor route filenames certify every source or every frame.

Overcast, dusk, moving/settled TAA and FSR Quality, two-bounce, grounded retreats
and hardware-T&L/forced-CPU/restored routes are exercised. The existing MSAA
spatial-only single-sample grain is not clean MSAA visual acceptance. Cropped
or obstructed scouting views and the earlier idle-held scouting run are excluded
from acceptance and timing totals.

The supplementary [hotel east view](evidence/material-modernization/batch-13/modernization-b13-final-noe-supplement-brick-hotel-east-gi.png)
has a clear chimney but does not demonstrate new native reception either.
Forced-CPU masonry samples increase from 9,035 to 9,129 in the paired front
control and from 4,255 to 4,392 on the east. Those are family totals, not exact
chimney pixel counts. Both the [preserved baseline CPU view](evidence/material-modernization/batch-13/modernization-b13-baseline2-noe-supplement-brick-hotel-CPU.png)
and [candidate CPU view](evidence/material-modernization/batch-13/modernization-b13-final-noe-supplement-brick-hotel-CPU.png)
show an existing black roof; this is **not** clean whole-scene CPU acceptance.
The exact native capture rejection is not inferred and no gate is relaxed.

Bounded grid/phase probes observe no new chimney-region reflected hit. This
does not mean zero transport across the world; it also does not demonstrate
new-source reflected transport in these samples. The existing reflector path
and immutable source prerequisites remain distinct from actual reception and
observed transport. All sampled paths and limits are retained in the receipt.

## Measured GPU cost and preservation

RTX 4060 Ti; TAA at 100% scale, one bounce, RT sun/cloud shadows;
AO/water/volumetrics off. Each scene warms ten seconds, then uses GI Off/On/On/Off
with four-second settling and six-second sampling intervals. Probes are off;
no build, heavy census or other game runs during timing. The table averages
the two GI-on interval medians; individual intervals and samples are preserved.

| Scene / output | GI-on GPU frame before / final | GI trace+filter+history before / final |
| --- | ---: | ---: |
| Hotel front 1920x1080 | 5.953 / 5.985 ms | .901 / .899 ms |
| Hotel front 2560x1440 | 6.121 / 6.139 ms | .961 / 1.070 ms |
| Cottage 1920x1080 | 4.818 / 4.859 ms | 1.009 / 1.018 ms |
| Cottage 2560x1440 | 4.789 / 4.856 ms | .954 / .976 ms |

These are bounded whole-scene costs, including existing materials and terrain,
not an isolated chimney benchmark or universal budget. The 1440p hotel trace
has one .852 ms interval and one .657 ms interval; both remain in the reported
average rather than being discarded. Small negative deltas are not performance
improvement claims. The unchanged hotel native sample is not promoted to visible
reception merely because reflector prerequisites or timing change.

Six Vulkan measured/supplementary runs present **91,518 frames**, synchronization
validation on, zero errors/warnings and normal process exit. The tested Vulkan
executable SHA256 is
`016FE603267A027F9CFF248851DD81FB20841063F471CDAB4C4E637E71E74F2A`.
It was built from the stated base plus this batch's source changes before the
documentation commit. Policy and executable hashes identify these chimney
rules separately from historical unshipped plaster pilots that used IDs544/545.

Live GL33 Off/On/Enhanced/Auto/Off controls have zero changed world-ROI pixels.
All **270 original input hashes** and **67 settings/fixture/input hashes** match.
Private DLSS remains clean and remote-free; SDKs and preserved snapshots remain
present. Full artifact hashes, measurements, counters, probes and preservation
are in the [qualification receipt](evidence/material-modernization/batch-13/qualification.json).

## Tests and reproducible census

Public Vulkan game/tools/tests and GL33 game/tests build. Focused filter
`[vulkan-shape],[Shaders],[enhanced-material],[ray-surface],[render-flags]` passes
**81 cases / 87,953 assertions** on Vulkan and **80 / 87,913** on GL33. Python
inventory/census tests pass 18 and driver-free Vulkan policy guards pass 690.
The intentional policy-test teardown error is not a gameplay validation finding.

Fresh censuses cover **726 models / 1,708 ordinary-adapter retained levels**
and separately **726 / 1,711 authored levels**. Previous complete prerequisites
reproduce after replaying **only new regions 544-556**. Original section/UV/
normal/hint metadata and all 50 surveyed geometry/binding identities remain
unchanged. All 726 immutable historical reports verify; the same three forest
source-only levels remain outside a retained historical native comparison.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-13/final-runtime-census `
  --output build/material-modernization-20261021/batch-13/final-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 187 `
  --replay-regional-identities @(512..556)
```

Add `--all-source-lods` with separate cache/output under
`batch-13/final-source-census` for the authored-source diagnostic. Original decoded
images and full native vertex/UV/normal buffers remain ignored locally; only
metadata/fingerprints and real gameplay captures are published.

All 13 models were already partial source reviews in batch 12 and remain partial:
other sources and mixed-atlas regions are unresolved. The ledger therefore stays
at **60 complete source reviews, 122 partial, 333 pending and 211 retained existing
contracts**. The 30 pilot tasks retain 11 complete reviews, 16 partial and three
retained contracts. Those are assessment states, not rendering percentages.
