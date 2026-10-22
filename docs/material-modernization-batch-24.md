# Original wooden piers: exposed timber, retained waterline and ceiling contracts

Base: `cfcb205ef12d3d8dba41864af50ef847e438e9d6`.
Three original models / 14 authored visual levels / 12 world placements are
assessed: the covered pier, open pier and end piece. Five source-unknown images
are physically reviewed among six decoded references. This is not a claim of
six optical approvals, twelve visible improvements or full pier rendering coverage.

## Original pixels and complete faces

Enhanced-only identities **202** `data/oldwood.pac` and **203** `data/dosky.pac`
reuse the existing .85 rough wood / family 4 approximation. It is an unmeasured
rough-dielectric assumption, F0 .04 and metallic 0, not a recovered material map.
The former is a continuous aged plank photograph, the latter overlapping wood
shingles. Original texture color, UV repetition/clamping and authored lighting
remain. Four near roof triangles occupy 121.34 square metres on sloped roof
planes; they are not a ground floor. Near deck meshes occupy 84.08 square metres
on each long pier and 35.45 on the end piece, not necessarily exposed dry area.

The original `data/molo_drev_side.pac` photograph mixes an upper dry timber row
with green/dark lower waterline treatment. It remains untyped as a whole source.
Exact region **569** on `data3d/molo_drevo.p3d` requires original 64x256 pixels,
RGBA fingerprint `3594089815499870373`, exact model/LOD/geometry/bindings and
original ClampV. U[0,43], V[0,.25] contains 192 / 96 / 0 / 0 / 0 ordinary
triangles by authored level. Observed repeated U reaches 42.5805; shortening it
to 20 splits two original fans and is rejected. The admitted bounds split zero
original fans. Near selected mesh area is 10.42 square metres, local Y[2.4285,3.3375],
above the original deck. The same dry row selects no complete ordinary faces
on the open/end models. No UV/sampler/normal/geometry repair or generated mask.

Retained cases have concrete reasons:

- Lower wet/green side pixels do not inherit uniform dry-wood optics. No wetness,
  algae, coating or cleanup map is guessed.
- `data/prkynka.pac` is the dark roof underside. Its 16 near and 16 next-level
  triangles carry authored lighting; only 12 coarse alternatives lose those
  prerequisites. They do not justify manufacturing independent ambient or a
  far-only adoption. This is a ceiling, not a new ground-floor source.
- `data/prkynka_l.pac` is bright timber on the end piece. Its 172 near ordinary
  triangles occupy 13.94 square metres. Surveyed original end placements are
  submerged or show thin near-waterline tops, while other shared model users
  are authored-gated. A dedicated identity is not justified by hidden counts.
- Original authored, invalid-geometry, opacity, static-instance, captured ambient
  and sunlight/water gates remain independent of physical source proof.

## Gameplay and water-state control

Original Malden `01TakeTheCar` and Nogova `R01WarCry` are inspected through the
existing direct developer harness with copied profiles and --nosound. Shoreline
standing views show the covered roof and exposed Nogova deck. A separate
developer camera is 1.8m above the covered pier's original live deck, ASL 1.68144.
It is a wading-level diagnostic, not physically walked or boat-route acceptance.
Actual live object positions differ from the rounded WRP diagnostic; neither
original placement nor asset is repaired to improve a screenshot.

Initial unnormalized waterline captures expose a deck with missing ocean, even
in the same-runtime native and CPU PBR-off controls. Original executable controls
exercise AA/weather, distance and CPU route without reproducing that state.
Existing `triSetSimTime 0` restores the ocean on the unchanged candidate executable.
The exact original time/water-state cause is not inferred. Initial eight runs
are excluded from admission and paired cost claims, with identities/captures
retained. All admitted pairs are rerun with explicit simulation clock 0 before
each review action. No renderer, water, shader or material-rule fix is made.

At the admitted state, the main deck remains submerged. Only visible upper
beams and the roof are claimed there; deck reception is demonstrated separately
on the exposed Nogova pier. Original green lower posts and dark ceiling retain
their treatments. Family receiver counters are frame samples, not per-source
pixel masks, all-placement reception or world coverage percentages. Reflected
eligible hits must be reported separately from those reception counters.

Same-runtime GI-off, full CPU/GI-on, CPU/PBR-off, CPU/GI-off and sky-only controls,
overcast/dusk, TAA/FSR Quality/MSAA4 settled captures, two bounces and bounded
original-feature water on/off controls are retained. Water reports active capture
and composite, but supports raster-only coastal terrain, not invented model-bottom
transmission. Spatial-only MSAA grain and settled movement are not clean MSAA
equivalence or in-motion acceptance. Shore retreats do not certify every authored
LOD or submerged end placement; the waterline camera is not terrain-retreated.

## Cost, census and preservation

At 1080p, normal native wood reception changes from zero to **51,880**
(covered roof), **4,126** (upper beams), and **64,599** (exposed Nogova deck).
Full CPU-transform wood examples change from zero to **51,926 / 4,108 / 64,503**.
These are separate route/frame samples at the half-resolution diagnostic footprint.
No new eligible reflected hit on identities 202/203/569 is observed in the 160
bounded phase polls per view; source/reflector prerequisites are not an actual
reflected-hit claim or an inferred exact live rejection cause.

| View / resolution | GI-on GPU frame, before / candidate | Trace + filter + history, before / candidate |
| --- | ---: | ---: |
| Covered roof / 1080p | 1.962 / 2.172 ms | 0.148 / 0.233 ms |
| Upper beams / 1080p | 1.948 / 2.162 ms | 0.155 / 0.189 ms |
| Exposed deck / 1080p | 1.974 / 1.986 ms | 0.144 / 0.208 ms |
| Covered roof / 1440p | 2.690 / 2.791 ms | 0.166 / 0.250 ms |
| Upper beams / 1440p | 2.601 / 2.621 ms | 0.167 / 0.185 ms |
| Exposed deck / 1440p | 2.763 / 2.876 ms | 0.167 / 0.259 ms |

Values average the two GI-on interval medians. All raw Off/On intervals, individual
stages, CPU recording and world costs remain in the
[qualification receipt](evidence/material-modernization/batch-24/qualification.json).
Eight matched-clock runs normally close after **177,014 presented frames**, with
zero Vulkan validation errors/warnings and nine restored hardware-T&L toggle pairs.
An additional untimed dusk PBR control closes after 2,396 frames, zero errors/warnings;
it is separate from the paired total.
Auto/TAA 100%, one bounce, RT sun/cloud shadows, AO/water/volumetrics off during
cost sampling. Separate water-feature controls restore off first. Warm 10 seconds,
then Off/On/On/Off with 4 seconds settle and 6 seconds sampling per interval. Both
1920x1080 and 2560x1440; no build, heavy census or other game during sampling.
Frame and GI-stage costs include scene/route/LOD/history work; negative frame
deltas are not an optimization or isolated material-cost result.

Fresh censuses inspect 726 models / 1,708 retained levels and all 1,711 source levels.
All 726 retained historical reports verify with zero replay differences; the same
three original forest source-only levels remain outside the native baseline.
Exact new source/region replay reproduces the preceding policy prerequisites.
204 highest-detail / 356 alternative-LOD additional eligible triangles are source
diagnostics, not the adoption target, exposed area or rendering coverage.

Focused Vulkan tests pass 89 cases / 113,107 assertions; GL33 passes 88 / 113,067.
Existing 690 Vulkan policy checks and 18 Python inventory/census tests pass. Authored/geometry/alpha rejection,
required original ClampV, wrong pixel hash, changed geometry/bindings/LOD and
mixed wet-row UV negatives remain explicit. Protected qualification functions
and old enhanced-material safety tests are byte-identical to the base.

Fresh GL33 controls report all five PBR commands unsupported and zero changed
world-ROI pixels. All 270 original input and 67 settings/fixture/input hashes remain
unchanged; private DLSS is clean and remote-free, with SDKs and preserved snapshots.
No original asset, artwork/map, settings, GL33 implementation, private DLSS,
shader, transport/filter/ray budget or safety gate changes.

Source assessment advances separately from rendering coverage. After admission,
222 models are source-complete, 121 partial, 172 pending and 211 retain their existing
classifier contracts. The 30-task dispositions remain 11 complete / 16 partial / 3 retained.
The 726-model goal remains incomplete and continues through related families.

The [source audit](evidence/material-modernization/batch-24/source-audit.json)
contains exact source pixels, geometry/UV/lighting and retained reasons. Compare
[covered-roof baseline](evidence/material-modernization/batch-24/baseline-pier-covered-shore-gi.png)
and [candidate](evidence/material-modernization/batch-24/final-pier-covered-shore-gi.png),
[waterline baseline](evidence/material-modernization/batch-24/baseline-pier-covered-waterline-gi.png)
and [candidate](evidence/material-modernization/batch-24/final-pier-covered-waterline-gi.png),
and [Nogova deck baseline](evidence/material-modernization/batch-24/baseline-pier-noe-shore-gi.png)
and [candidate](evidence/material-modernization/batch-24/final-pier-noe-shore-gi.png).
The [dusk PBR-off](evidence/material-modernization/batch-24/control-dusk-PBR-off.png)
and [PBR-on/GI-off](evidence/material-modernization/batch-24/control-dusk-auto-GI-off.png)
control show the existing broad warm grazing-sun sheen; nearby plank detail remains.
This is continuous raster BRDF response, not a GI patch or a wet-finish declaration.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-24/final-runtime-census `
  --output build/material-modernization-20261021/batch-24/final-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 203 `
  --replay-regional-identities @(512..569)
```

Add `--all-source-lods` with a separate `batch-24/final-source-census` cache/output.
Accepted Vulkan executable:
`E024AC18913AC3EFA9AA926CB437D21F7E2F6A6D649992E28C6ED6C2ADC2C434`.
