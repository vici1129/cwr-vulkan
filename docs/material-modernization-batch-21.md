# Original timber: hunting stand and authored-floor limits

Base: `994fb48a744d5ec79b5552642e6984bdf957d3e2`.
The shared timber survey inspects **16 models / 78 authored visual levels**.
Eleven original images are physically viewed; 221 decoded references are not
221 optical approvals. Actual new rendering paths are limited to the original
hunting stand, `o/hous/posed.p3d`, at **eight Nogova placements**. Other surveyed
buildings retain their unrelated pending source cases.

## Admitted paths and original-source boundaries

Four exact original keys use the existing enhanced-only rough-wood approximation
(.85 roughness, family 4), not measured reflectance or new material maps:

| Identity | Original image | Separately modeled timber |
| --- | --- | --- |
| 194 | `o/hous/posedbok.pac` | Plank wall |
| 195 | `o/hous/posedbokin.pac` | Plank wall and timber frame |
| 196 | `o/hous/posedfloor.pac` | Decking/underside |
| 197 | `o/hous/posed01a.pac` | Posts |

The original timber roof underside `o/hous/prkno_str.pac` uses exact region
**561**, .85 / family 4, **reception=false**. Original pixels (256x256,
RGBA fingerprint `11743484817861242001`), model, four geometry fingerprints and
ordered texture bindings are required. Ten complete original triangles at each
of four source levels fit U/V [-.005, 1.005]. Tiny original edge overshoot samples
the same all-timber image; no UV repair, clipping, class-boundary extrapolation
or new clamp exception is used. The existing PBR/reflection path remains separate
from ambient reception; the underside keeps its original ambient lighting.

Original photographs, seams and shaded pixels remain untouched. The stand's
mixed bark/peg atlases (`posed6`, `posed8`), multiple cut-end photos on dark
backing (`letokruhy`) and cracked/peeling top roofing (`posed_strecha`) retain
concrete optical limits. Inspected band-only UV bounds select nothing, cut an
original face fan or isolate at most .283 square metres of uncertain material.
No background/peg mask or guessed bark/felt/metal finish is generated.

All eleven world-model users of `data/podlaha_prkna_h.pac` are inspected across
54 authored levels. **510 triangles retain authored lighting**; the only two
ordinary triangles total .0417 square metres of vertical watchtower platform
edge. No ordinary visible interior floor is recovered. No global registration
is added to count this nearly invisible edge, and no authored hint or normal
is changed. This resolves the watchtower's specific batch-20 followup.

The old church-board atlas `data/k1_stara_prkna.pac` retains its existing
whole-source exclusion. Original dark hardware-like/inset marks remain mixed
with worn boards. Central inspected bounds select only one small complete
diagonal church face (.172 square metres); broader V=.9 bounds cross the lower
dark-marked strip. Other users yield no ordinary faces in those bounds, while
588 authored alternatives remain gated. No demonstrated normal-view benefit
justifies adding the tiny narrow patch. Other church/facade sources remain
pending. Geometry and a board-like filename do not override the source contract.

Original provenance, decoded identities, all geometry/binding identities,
27 UV trials and per-source decisions are in the
[source receipt](evidence/material-modernization/batch-21/source-audit.json).
Raw decoded originals and full UV/position buffers stay ignored locally.

## Rejected proposals and gameplay appearance

A first six-source build failed the unchanged regression
`EnhancedPBRIdentity(k1_stara_prkna)==0`. That proposal is withdrawn **before
candidate gameplay**; no existing test or safety gate is removed. The reduced
five-source build passes tests but fails appearance: GI-on produces a distinct
circular bright patch under the roof in both native and CPU views. It disappears
with GI off. See [rejected GI-on](evidence/material-modernization/batch-21/withdrawn-roof-receiver-gi.png)
and [same-runtime GI-off](evidence/material-modernization/batch-21/withdrawn-roof-receiver-enhanced-off.png).
Zero validation findings do not turn that visual failure into acceptance.

Removing roof reception through the existing exact-region contract removes
the patch in the [admitted native view](evidence/material-modernization/batch-21/accepted-wood-stand-open-gi.png)
and [CPU view](evidence/material-modernization/batch-21/accepted-wood-stand-open-CPU.png).
The [baseline](evidence/material-modernization/batch-21/baseline-wood-stand-open-gi.png)
remains available. The visible gain is localized, less uniformly filled shaded
timber, not complete stand/world relighting. Native and CPU reception are
separate observations; the underside is deliberately not a receiver.
Admitted route samples record wood-family footprints of 10,417 native and 13,177
CPU pixels in separate observations (half-resolution diagnostic footprint,
not source-specific or world coverage). Bounded probes observe all four new
whole identities and roof region 561 as eligible hit materials; this is not a
transport distribution or proof that the roof receives ambient replacement.

Terrain-grounded cameras inspect original `Resistance/R01WarCry.Noe` gameplay
through the existing direct developer harness, copied profiles, `--nosound`
and synchronization validation. This is not Steam-route or mission-completion
acceptance. Initial wooded-stand scouts are obscured; a different original open
placement is used for paired evidence. The church view is an unchanged-source
control, not a claimed church improvement. Five HWTL Off/On pairs are verified
across baseline/admitted 1080p runs, including the extra admitted CPU controls.

Morning, [overcast](evidence/material-modernization/batch-21/accepted-wood-stand-open-overcast-on.png),
[dusk](evidence/material-modernization/batch-21/accepted-wood-stand-open-dusk-on.png),
grounded 0/8/22/52/92-metre retreats, enhanced/auto, one/two bounce, TAA and FSR
Quality controls are exercised. Captures after bounded camera movement are
settled views, not in-motion screenshots. Existing spatial-only MSAA GI grain
is visible on the new timber receivers and is **not clean MSAA acceptance**;
no shader, sample budget, filtering or acceptance gate is changed to hide it.
Existing foliage/water/LOD and CPU/native differences remain separate limits.

## GPU cost, source census and preservation

RTX 4060 Ti; TAA100%, RT sun/cloud shadows, one bounce, AO/water/volumetrics off.
Each scene warms ten seconds, then runs GI Off/On/On/Off with four-second settling
and six-second sampling at 1920x1080 and 2560x1440. No build, heavy census or other
game runs during timing. Full paired intervals, GI-off costs, CPU-recording
medians, receiver footprints and bounded reflected-hit samples are in the
[qualification receipt](evidence/material-modernization/batch-21/qualification.json).
Scene deltas do not isolate material cost, establish an optimization or prove
a universal performance budget; the unchanged church is a variation control.

| Scene / resolution | GI-on GPU frame, before / admitted | Trace + filter + history, before / admitted |
| --- | ---: | ---: |
| Stand / 1080p | 2.490 / 2.564 ms | .297 / .365 ms |
| Stand / 1440p | 4.006 / 4.124 ms | .473 / .572 ms |
| Unchanged church / 1080p | 2.627 / 2.696 ms | .471 / .550 ms |
| Unchanged church / 1440p | 3.744 / 3.655 ms | .500 / .504 ms |

The stand's measured GI-stage delta is approximately .067 / .099 ms in these views.
CPU command recording is about 5.1-5.3 ms with GI on. The unchanged church also
varies, including a negative GPU-frame delta; none of this is an optimization
or an isolated universal material budget.

Fresh censuses inspect **726 / 1,708 ordinary-adapter retained levels** and
**726 / 1,711 authored levels**. All 726 historical reports verify. Replaying
only whole identities 194-197 and region 561 reproduces the prior complete
prerequisites with zero differences. All 78 family geometry/binding identities,
UVs, normals, flags and authored hints remain unchanged. Three forest source-only
levels still lack retained historical native comparison. The immutable original
inventory is not rewritten. The 136 new highest-detail prerequisites / 412 across
alternative levels are not a rendering percentage, adoption target or receiver
count. The failed five-source roof trial has the same counts but fails appearance.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-21/accepted-runtime-census `
  --output build/material-modernization-20261021/batch-21/accepted-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 197 `
  --replay-regional-identities @(512..561)
```

Add `--all-source-lods` with separate cache/output under
`batch-21/accepted-source-census`.

Focused tests pass **86 cases / 98,486 assertions** on Vulkan and
**85 / 98,446** on GL33, plus 690 policy guards and 18 inventory/census tests.
The whole-source church-board exclusion and receiver-family invariant are
unchanged. The rejected six-source build and rejected roof-receiver preflight
are recorded separately, not blended into admitted evidence.

The admitted executable is
`2E35843532E3151FD6E54C6B3162D91184EA910589BD084498076B26B4C21E1F`.
Source HEAD alone does not identify loaded bytes. Identity 194 now names the
stand's plank image; it does not re-admit the rejected batch-18 castle source.
The original stone exclusion remains pinned. Preservation and fresh live GL33
controls are recorded with the admitted run, not inferred from source inspection.
The four paired Vulkan runs close normally after **115,381 presented frames**,
synchronization validation on, **zero errors and warnings**. The visually rejected
roof pilot's 25,565 frames and the 3,640-frame initial scout are separate and
excluded from admitted timings. Fresh live GL33 Off/On/Enhanced/Auto/Off controls
change zero world-ROI pixels. All 270 original input hashes and 67 settings/
fixture/input hashes match. Private DLSS stays clean and remote-free, with both
SDKs and preserved build/runtime snapshots present.

The ledger records **212 complete source assessments, 124 partial, 179 pending
and 211 retained existing contracts**. Stand and watchtower reviews close;
fourteen other surveyed models are not silently promoted to complete source
coverage. The full original-world goal continues.
