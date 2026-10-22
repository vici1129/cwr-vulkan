# Original bridge concrete

Base: `3a49531235dd3859bbd6c079172c138d295aa730`. This batch completes the
source assessment of `o/hous/most_stred30.p3d`: four authored visual LODs,
three previously unreviewed images and ten original Nogova placements.
The 1,776 source-unknown alternative-LOD triangles are fully accounted for;
400 gain existing concrete prerequisites and 1,376 retain assessed fallback.
The classifier still calls the latter untyped; assessment does not remove gates.

## Original sources and geometry

Enhanced-only identity **206**, `o/misc/most02.pac`, is the continuous original
poured-concrete panel photograph on supports and underside. Auto-only exact
region **570** selects concrete curb/parapet faces in `o/misc/most01.pac`,
UV U[-1,9], V[0,.24], original ClampV, exact 512x512 pixels, model, geometry
and texture bindings. All selected original polygon fans remain whole.
Both use existing concrete family 7, roughness .90, F0 .04 and metallic zero:
unmeasured rough-dielectric approximations, with original photographic shading.

| Authored resolution | Original `most01` / `most02` / opaque road triangles | New concrete prerequisites | Retained alpha-overlay triangles |
| --- | ---: | ---: | ---: |
| 1 | 784 / 98 / 2 | 16 + 98 | 0 |
| 2.5 | 372 / 98 / 2 | 16 + 98 | 2 |
| 5 | 228 / 70 / 2 | 16 + 70 | 2 |
| 9 | 48 / 70 / 2 | 16 + 70 | 2 |

All four LODs have ordinary lighting, valid normals/UVs and no authored-material
or land-fitting hints. Repeated concrete UVs remain in their inspected material;
geometry/binding fingerprints match. No mesh, UV, normal or sampler repair.
The near support/curb meshes are 1,304.22 / 105.17 square metres, including
hidden/buried/submerged portions, not exposed coverage or recovered waterline optics.

The remaining sources retain specific reasons:

- `most01.pac` combines concrete, cracked paving, vegetation, blue coated/oxidized
  rail, lamp, drain and hardware. Only its upper concrete row is admitted.
  Its other 1,368 alternative triangles retain their mixed/uncertain finish.
- `o/misc/sil_new.pac` is opaque, unlike the road PAA. Each LOD uses a full-width
  quad, two triangles spanning asphalt, white markings and shaded/green shoulders,
  with V[-2,5.998]. No whole asphalt-only face can be separated. All eight
  remain original; alpha/terrain fitting is not their blocker.
- `o/road/sil_new.paa` remains the separately reviewed fractional-alpha overlay:
  flags `0x24100`, two triangles in each lower LOD. Opacity/render behavior rejects
  it independently of the bridge's solid geometry. Ordering, depth and lighting stay intact.

## Original Nogova execution

Original `R01WarCry.Noe` uses the existing developer harness, copied profiles,
--nosound and synchronization validation. All ten original transforms are read
without moving objects. Four views span both locations, 1.8m above original decks
or bank terrain; frozen actions match simulation clock zero. This is selected
developer-camera evidence, not Steam launch, physical traversal or mission completion.

| View | Native concrete sample, before / after | Forced-CPU concrete sample, before / after |
| --- | ---: | ---: |
| Main deck | 0 / 18,850 | 0 / 4 |
| Main bank/support | 0 / 133,267 | 0 / 129,688 |
| Eastern deck | 0 / 19,446 | 0 / 15 |
| Eastern bank/support | 0 / 39,115 | 0 / 20,459 |

These are half-resolution family/frame samples, not identity-specific masks or
coverage percentages. Native reception returns after CPU toggles. Separate
CPU/GI-on controls record 0 / 17 on the decks; broad CPU curb reception is unproved.
Actual native eligible reflected hits identify **570** at the main deck,
**206** under both bridges, and both identities in the eastern deck view.
Bounded probes are execution examples, not hit distributions or all-world coverage.

Same-runtime native PBR-off, PBR-on/GI-off, sky-only and GI-on controls preserve
the original panel/curb seams and leave blue rails, hardware and roadway under
their original material treatment. Forced-CPU controls remain separate: their
terrain, water, foliage and LOD differences are not whole-scene equivalence.
Compare the main bank's [PBR-off](evidence/material-modernization/batch-27/final-bridge-bank-native-PBR-off.png),
[GI-off](evidence/material-modernization/batch-27/final-bridge-bank-native-auto-GI-off.png)
and [GI-on](evidence/material-modernization/batch-27/final-bridge-bank-native-auto-GI-on.png),
and the [native deck](evidence/material-modernization/batch-27/final-bridge-deck-native-auto-GI-on.png).

Native advancing-simulation overcast/dusk controls reapply the exact view for
each PBR/GI comparison; selected small camera movements retain aligned panels.
Bank-side underside darkening persists with sky-only and RT sunlight shadows
off; reflected GI adds fill. No new source-face cut is observed in these views;
the exact occluder is not traced. Shadows are restored. Settled TAA/FSR/MSAA4
and two bounces execute; spatial-MSAA grain and older frozen post-AA camera
limits are not fixed or broadly recertified by this material batch.

## Measured workload

| Main-bridge view | GI-on GPU frame, before / after | GI trace + filter + history, before / after |
| --- | ---: | ---: |
| Deck / 1080p | 5.292 / 5.281 ms | .316 / .424 ms |
| Bank / 1080p | 5.323 / 5.357 ms | .522 / .916 ms |
| Bank / 1440p | 5.406 / 5.278 ms | .453 / .845 ms |
| Separate forced-CPU bank / 1080p | 39.605 / 44.221 ms | 2.638 / 6.616 ms |

RTX 4060 Ti; auto/TAA100%, one bounce, RT sun/cloud shadows, AO/water/volumetrics
off. Warm10, then Off/On/On/Off with settle4/sample6; values average paired
interval medians. The added GI work is about .11 ms on deck and .39 ms under the
bridge. Whole-frame variation and native CPU recording around 12.7-13.0 ms
prevent an isolated material-cost or FPS-improvement claim. These are selected
main-bridge workloads, not all ten placements. No build, census or other game
runs during sampling; raw Off/On and individual stages remain in the receipt.
The forced-CPU comparison adds about 4.62 ms GPU frame / 3.98 ms GI-stage work;
CPU recording is 214.82 / 211.14 ms. Its adaptive intervals use warm20/settle8,
at least 45 seconds and three fresh frame/GI-stage timestamps, bounded at 120
seconds. It is one diagnostic support view, separate from native gameplay costs.

## Validation and reproduction

Public Vulkan, tools and GL33 builds pass. Focused tests pass **92 cases /
115,638 assertions** on Vulkan and **91 / 115,598** on GL33. Road ordering/ZBias
passes **17 / 52** per build; 690 policy checks and 18 inventory/census tests pass.
Bridge regressions pin exact source/geometry/pixels, ClampV and mixed-row bounds;
all existing lighting, opacity, static-source and captured-light gates remain.

Seven Vulkan runs close normally after **69,707 frames**, with zero validation
errors/warnings and 14 restored hardware-T&L pairs. This includes CPU diagnostic
controls, not 69,707 exclusively native frames. Live Nogova GL33 controls report
all five PBR commands unsupported and **zero changed world-ROI pixels**. All
**270 original input hashes** and **67 settings/fixture/input hashes** match;
private DLSS is clean and remote-free, with SDKs and preserved snapshots present.

Fresh censuses inspect 726 models / 1,708 retained / 1,711 authored LODs. Each
verifies all 726 historical reports with zero replay differences. Only bridge
prerequisites change; the immutable inventory and three source-only forest levels remain.

```powershell
dist/vulkan-local/PoseidonTools.exe model inspect `
  build/original-asset-inventory-20261021/library/o/hous/most_stred30.p3d `
  --runtime-loader --all-source-lods --textures --sections --lighting --qualify --uv-triangles `
  --texroot build/original-asset-inventory-20261021/library `
  --material-config game-local/Remastered/BIN/CONFIG.BIN

python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-27/reproduce-runtime `
  --output build/material-modernization-20261021/batch-27/reproduce-runtime/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 206 `
  --replay-regional-identities @(512..570)
```

Add `--all-source-lods` with a separate source cache/output for the authored census.
The accepted scoped Vulkan executable is
`4820D5BC5D011047B5282A4E21F91BCE53FF0749D2042FDC6CA7625E3DD8E4FE`.
Original v3, qualification functions, existing regional entries, safety tests,
shaders, budgets, original assets and private DLSS remain unchanged.
Source assessment totals become **228 complete / 119 partial / 168 pending**,
plus **211 retained existing contracts**. This batch stops at this bridge family.

The [source audit](evidence/material-modernization/batch-27/source-audit.json)
retains original identities, all-LOD UV/lighting, geometry and exclusions; the
[qualification receipt](evidence/material-modernization/batch-27/qualification.json)
retains loaded hashes, controls, timing intervals, counters and preservation checks.
