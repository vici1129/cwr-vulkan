# Ruin stone regions and related plaster assessment

Base: `b7b7ea1999355b37e0944ff0af14c5c8eedde522`. The survey covers **six original
world-placed models / 19 authored visual LODs**, also retained by the ordinary
adapter. Four original atlas images were individually inspected with native
geometry, UVs, normal/lighting hints and original polygon-fan boundaries. No
whole-source identity is added. Four exact-model stone regions use existing PBR/reflection
and captured-light GI paths in `auto`; stock v3 and `enhanced` remain unchanged.

## Original material evidence

`o/hous/zborenina1.pac` contains rough stone/mortar, a cracked panel, an arch and
uncertain upper lintel/frame detail. The lower stone rows and the corner model's
left stone tiles are independently supported. The upper lintel detail does not
inherit stone merely because adjacent masonry is opaque. `zborenina1a.pac`
includes a photographed debris/ground strip. Its safe upper masonry rectangle
contains **no complete otherwise-eligible original triangle** in these users;
expanding it across that boundary is not an approved material separation.

`o/hous/domek_ruina1.pac` mixes upper rough plastered masonry, cornices, blocked
niches, lower windows and a timber door. Only original faces wholly in the upper
plastered row below the cornice were considered in a pilot, then withdrawn.
Windows, door/frames, lower-row
photography and cornices remain untyped. `domek_ruina2.pac` includes diagonal
architectural bands, dark recesses, a chimney/soot photograph, a blank support
patch and rubble. No independently justified region is added from that image.

The same colors, photographed weathering and shadows, normals, authored light
indices and UVs are retained. Existing .90-roughness dielectric approximations
use F0 .04 and metallic zero; they are not measured/reconstructed optical data.
No image, map, mask, mesh, sampler, source hint, shader, filter, default or
transport budget is generated or changed.

| Exact model under `o/hous/` | Region / existing family | Source-LOD qualified whole triangles |
| --- | --- | --- |
| `ruiny_3_dvere.p3d` | 540 / stone 6 | 0:44, 1:30, 2:20 |
| `ruiny_3_prasklina.p3d` | 541 / stone 6 | 0:44, 1:32, 2:22 |
| `ruiny_3_roh.p3d` | 542 / stone 6 | 0:35, 1:18, 2:10 |
| `ruiny_obvod_3.p3d` | 543 / stone 6 | 0:125, 1:60, 2:46 |
| `domek_kovarna.p3d` | No new region | Existing qualified wood/roof retained; mixed plaster remains pending |
| `domek_ruina.p3d` | No new region | Existing qualified wood/roof retained; mixed plaster remains pending |

Stone regions 540/541/543 use U -2..2 and V .45..1, where the entire repeated
horizontal source row remains stone. Region 542 instead uses U 0...5 / V 0..1;
negative U is not wrapped into another tile. No mixed triangle is clipped, split or sampled modulo the source to
manufacture an identity.

Broader exploratory rectangles partly classified original polygon fans. They
were rejected; the four retained candidates above have **zero partly classified original
faces** across all selected levels. Their nearest dumped UV corner is .001575
from a boundary, well outside six-digit dump rounding. All model, source,
dimension, immutable RGBA, geometry and ordered texture-binding fingerprints
must agree. Authored neighbors do not reject an unrelated ordinary triangle,
but every authored triangle and every existing material/capture gate remains
authoritative. This is not full-model or full-wall approval.

Twelve additional source-LOD geometry identities support the four regions.
Three far levels use original float bits rather than rounded text:

| Model | Exact original resolution | Float bits | Rejected rounded value |
| --- | ---: | ---: | ---: |
| `ruiny_3_dvere` | 6.903059482574463 | 1088218589 | 6.90306 |
| `ruiny_3_prasklina` | 6.867042541503906 | 1088143056 | 6.86704 |
| `ruiny_3_roh` | 7.313592433929443 | 1089079539 | 7.31359 |

There is no tolerance or broader LOD match. The exact-float diagnostics added
in batch 09 supply these identities without a tool/loader behavior change.

The [source audit](evidence/material-modernization/batch-11/source-audit.json)
records native qualification and whole-face proof. Original decoded PNGs and
full vertex/UV/normal dumps remain ignored locally; only metadata/fingerprints
and actual gameplay captures are public. The additional highest-detail
prerequisite count is 248 across distinct models, not placements or summed
alternative LODs. It is not a rendering-coverage denominator or adoption target.

## Withdrawn plaster pilot

An initial six-region executable also tested U -2..3 / V .07...49 on
`domek_ruina1.pac` for the smithy and ruined house. Native inspection admitted
74 / 52 highest-detail triangles respectively, but their modeled surface areas
were only about 4.81 / 3.54 square metres. Inspecting their original coordinates
showed mainly recess/detail surfaces, not the broad facade. The broad wall faces
cross the cornice or mixed-photo boundary and cannot inherit the narrow approval.

The normal-distance front/back gameplay views and explicit native/CPU controls
showed **zero new facade-family receiver pixels**. The bounded pilot probes did
observe one reflected hit on the smithy candidate, not zero transport everywhere;
they did not establish meaningful visible plaster reception or broad-wall proof.
Neither the exact input gate responsible for absent reception nor an all-instance
reception claim is inferred from those counters. Both plaster candidates were
removed. No capture, color, geometry or lighting gate was relaxed to activate
them, and no new plaster rendering coverage is claimed. Their source questions
remain concretely partial rather than increasing the qualification tally by 126.

## Actual gameplay and limits

Original Nogova `R01WarCry` gameplay uses the direct developer harness, copied
profiles and `--nosound`. Grounded standing cameras inspect original WRP ruin
placements in the desert settlement and northern village, plus related house
and smithy front/back preservation views. This is not normal Steam-route or
mission-completion acceptance. Initial scout views that landed inside a nearby
barn were not used as facade acceptance; the paired views use a verified clear
placement instead.

The initial northern `ruin-crack` camera is foliage-occluded. Its counters and
images are retained as diagnostics, **not clear crack appearance acceptance**.
A supplementary pair at original desert placement 2638.5,5068.2 uses a standing
camera at 2638.5,5056.2 facing positive world Z. The first edge-on scout at that location is
also not substituted for the final clear paired view. Renderer/material policy
and the tested executable hashes are unchanged for this supplementary check.

The clear final view records 19,452 native stone-family receiver pixels; its
native/CPU/restored controls record 19,444 / 19,297 / 19,500. Its bounded probes
observe 541 on six reflected paths (plus neighboring 540/543 once each). The
before/after pair was visually inspected with no new triangle seam observed.
The two supplementary runs present **36,639 frames** with synchronization
validation enabled, zero errors and warnings, and normal exits. They are
appearance checks, not extra timing intervals; the earlier performance table
is unchanged. Original-input/settings hashes and private-repository preservation
were rechecked after these runs.

Stock v3, auto/GI-off, sky-only and GI-on views separate material and transport
effects. Morning, overcast/dusk, moving/settled TAA and FSR Quality, two-bounce
operation, explicit native/CPU restoration and lower-LOD retreats are independent
controls. MSAA4 executes and validates, but its existing spatial-only GI grain is
not certified as a clean visual result. No filter, extra samples, source pixels
or acceptance gate is changed to hide original photographic shading, foliage,
LOD transitions or unresolved neighbors.

The [qualification receipt](evidence/material-modernization/batch-11/qualification.json)
keeps final accepted runs separate from the withdrawn six-region pilot. Family
receiver counters are frame samples, not source-specific pixel percentages.
Bounded source-identity probes are not all-world coverage or transport statistics.

The original final-build morning cameras record 30,996 stone-family receiver pixels at the desert
door group, 712 in the foliage-occluded northern crack diagnostic, 12,980 at the corner view and 20,148 at the
enclosure. The explicit door native/CPU/restored control records 30,911 / 18,178 /
30,996; the enclosure records 20,186 / 20,183 / 20,148. These routes are separately
observed and are not claimed to have identical capture or clipping footprints.
The visible stone change is modest rough/ambient response on proved faces, not
full ruin relighting; neighboring debris, upper lintels and old photographic
contrast remain. No new triangle seam was observed in the inspected morning,
overcast and dusk views.

The bounded final desert-view probes independently observe region identities
540 / 541 / 542 on 11 / 16 / 27 paths; the enclosure observes 543 on 12. The
corner sample observes 541 / 542 on 6 / 4 paths, while the crack sample observes
one 543 hit on a neighboring enclosure. These are reflected-source observations,
not source-specific receiver counts or proof that every eligible face was hit.

| Original standing view | Before | Final stone-only build |
| --- | --- | --- |
| Desert door group | [before](evidence/material-modernization/batch-11/modernization-b11-baseline-noe-1080-ruin-door-gi.png) | [after](evidence/material-modernization/batch-11/modernization-b11-final-noe-1080-ruin-door-gi.png) |
| Stone crack, clear desert placement | [before](evidence/material-modernization/batch-11/modernization-b11-crack-clear-baseline-gi.png) | [after](evidence/material-modernization/batch-11/modernization-b11-crack-clear-final-gi.png) |
| Stone corner | [before](evidence/material-modernization/batch-11/modernization-b11-baseline-noe-1080-ruin-corner-gi.png) | [after](evidence/material-modernization/batch-11/modernization-b11-final-noe-1080-ruin-corner-gi.png) |
| Enclosure | [before](evidence/material-modernization/batch-11/modernization-b11-baseline-noe-1080-ruin-enclosure-gi.png) | [after](evidence/material-modernization/batch-11/modernization-b11-final-noe-1080-ruin-enclosure-gi.png) |

### Measured GPU cost

RTX 4060 Ti, TAA at 100% world resolution, RT sun/cloud shadows, one bounce;
AO/volumetrics/water optics off. Each executable warms ten seconds, then uses
Off/On/On/Off with four-second settling and six-second samples. The table averages
paired interval medians. No build, other game or probe runs during timing. Both
requested world/output resolutions are verified live.

| Scene / output | GI-on GPU frame before / final | GI trace before / final | Trace+filter+history before / final |
| --- | ---: | ---: | ---: |
| Enclosure 1920x1080 | 9.106 / 9.049 ms | .908 / 1.026 ms | 1.507 / 1.739 ms |
| Enclosure 2560x1440 | 9.144 / 9.307 ms | .869 / 1.001 ms | 1.514 / 1.700 ms |
| House preservation view 1920x1080 | 7.880 / 7.833 ms | .857 / .900 ms | 1.438 / 1.478 ms |
| House preservation view 2560x1440 | 8.036 / 8.128 ms | .843 / 1.045 ms | 1.555 / 1.838 ms |

These are bounded scene workloads, including terrain, surrounding existing
materials and nearby newly qualified stone—not isolated plaster cost or a
universal budget. The 1440p enclosure sample adds about .186 ms to the measured
GI stages and .163 ms to the full GPU frame. The 1080p final enclosure history
interval medians differ .489 / .273 ms, and its Off frame medians 5.583 / 5.176 ms
differ materially from baseline 7.377 / 7.608 ms. CPU recording remains roughly
22-25 ms in GI-on views. Consequently, lower full-frame figures are **not**
claimed as an optimization, and the 1080p stage difference is not a precise
isolated cost estimate. All intervals and sample arrays remain in the receipt.

The four final-acceptance/baseline runs present **80,650 frames**, synchronization
validation on, zero errors and zero warnings. The withdrawn pilot separately
presents **34,048 frames**, also validation-clean; it is not counted as accepted
six-region coverage. Every process exits normally. The final Vulkan SHA256 is
`A1E32467B6F36D257FDA10B61FDA4DAA7E31F3EA4A4974A75FAA5D179DD2714E`.

## Tests, census and progress

Public Vulkan game/tools/tests and GL33 game/tests build. The final focused filter
`[vulkan-shape],[Shaders],[enhanced-material],[ray-surface],[render-flags]` passes
**79 cases / 66,490 assertions** on Vulkan and **78 / 66,450** on GL33. Python
inventory/census tests pass 18; driver-free Vulkan policy guards pass 690. Their
intentional teardown error is not a gameplay validation finding. Tests pin whole-
image fallback, all four stone boundaries, authored-lighting rejection and exact
fractional LODs; the rejected rounded resolutions cannot gain an approval.

Fresh 726-model censuses retain the immutable **1,711 authored / 1,708 ordinary-
adapter retained** scope distinction. All previous complete prerequisites,
normal/hint state, section flags and UV metadata reproduce the accepted policy
after replaying only new regions 540-543. All 19 shared source geometry/binding
fingerprints remain unchanged. Three source-only forest levels still have no
retained historical native qualification and are not claimed as compared.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-11/final-runtime-census `
  --output build/material-modernization-20261021/batch-11/final-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 184 `
  --replay-regional-identities @(512..543)
```

For the authored-source census, add `--all-source-lods` and use a separate cache
and output under `batch-11/final-source-census`. Neither census certifies live
reception or optical review of every original image.

Live GL33 Off/On/Enhanced/Auto/Off controls report unsupported Vulkan features
and **zero changed world-ROI pixels**. All **270 original input hashes** and
**67 settings/fixture/input hashes** match their retained manifests. Private
DLSS remains clean and remote-free, with both SDKs and preserved build/runtime
snapshots present; original assets, profiles and settings remain unchanged.

All six models remain partial reviews for the concrete mixed-source boundaries
above. The ledger now records **60 complete source reviews, 95 partial, 360
pending and 211 retained existing contracts**. The 30 pilot tasks still have
11 complete source reviews, 16 partial and three retained contracts. The full
726-model goal remains incomplete; none of these states is a rendering percentage.
