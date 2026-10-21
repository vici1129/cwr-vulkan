# Fitted-road assessment: original blend and terrain contracts

Base: `1d048c7ae83a8f7752ba0d11de7904eb7ca3142c`. This batch reviews the
remaining fitted-road backlog task and all placed users of its related classic
and Nogova source images: **74 road models / 102 authored visual LODs**, plus
four LODs of `o/hous/most_stred30.p3d`, whose three lower levels share a road
overlay. All **75 models / 106 levels** are also retained by the ordinary adapter.
They account for 17,956 original WRP placements, including ten bridge placements;
this is reuse, not a screen-area or coverage measurement.

The defensible result is **retaining the existing contract, not adding road PBR
or GI coverage**. No renderer, shader, source identity, regional rule, material
parameter, default, asset or transport budget changes. The code addition is a
regression test for the independently required surface/alpha/terrain gates.
This closes an assessment question without manufacturing a rendering increase.

## Original pixels and physical boundaries

All 17 decoded original images were individually viewed. The [source audit](evidence/material-modernization/batch-10/source-audit.json)
records immutable archive/loose provenance, dimensions, alpha-value histograms,
shared native sections, UV bounds, normal/lighting metadata and original geometry
fingerprints. Original images and full vertex/UV/normal dumps remain local and
ignored; no replacement artwork or renderer-consumed map is produced.

| Sources | Observed original content | Fractional / fully opaque texels |
| --- | --- | --- |
| `data/asfaltka.paa` / `_kr.paa` | Mottled asphalt and feathered shoulders / terminating overlay | 2,048 / 14,208; 8,968 / 3,838 |
| `data/cesta.paa` / `_kr.paa` | Dirt wheel tracks and photographed ground shading / termination | 8,192 / 0; 7,212 / 0 |
| `data/silnice.paa` / `_kr.paa` | Grey roadway, shoulder shading / termination | 1,408 / 14,848; 10,155 / 5,661 |
| `o/road/asf_new.paa` / `asf_konec.paa` | Rough asphalt, blended edges / termination | 13,312 / 49,664; 32,683 / 25,746 |
| `o/road/ces_hned.paa` / `ces_hned_konec.paa` | Dirt, wheel-track relief/shading and blended ground | 80,642 / 0; 64,702 / 0 |
| `o/road/sil_new.paa` | Asphalt, painted white edge markings and shoulders | 4,352 / 61,184 |
| `o/road/sil_konec.paa` / `sil_konecupl.paa` | Asphalt/painted-edge end and junction treatments | 42,036 / 13,632; 37,770 / 27,724 |
| `o/road/sil_kos.paa` / `sil_kos_k.paa` | Cobbled paving and border/end treatments | 4,154 / 60,614; 6,437 / 16,839 |
| `o/road/sil_kos_cur.paa` / `sil_kos_cur2.paa` | Cobbled corner/junction, border and transparent support | 7,516 / 112,929; 4,570 / 20,204 |

Fractional alpha means values 1-254; zero-alpha texels are separately retained in
the audit. The dirt sources have **no alpha-255 texels**. Opacity is not a physical
material label: asphalt/paint, kerbs, shoulders and photographic track shading
also cannot be promoted wholesale from a name or an opaque-looking center.

Original quads/fans commonly span the full road width, mixing road centers with
the feathered support. Junction/end treatments and repeating longitudinal UVs
also remain. Source UV bounds include V -2 through 6.00763; no coordinate is
wrapped, clamped, clipped or repaired to invent a safe region. No opaque-core
rectangle is approved: even hypothetical physical source proof would not make
an alpha-blended fitted overlay eligible for the existing opaque-object path.

## Independent blockers, including exceptions

Every road-source section records `opaque=0`, source identity zero and
render-behavior rejection: **462 alternative-LOD road triangles**, with zero
eligible or source-unknown road triangles. First-failure buckets do not erase
independent defects. One hundred shared-model levels also carry original
`ClipLandOn` hints on all vertices; the ordinary material classifier rejects
those independently of source identity and alpha.

Two near cobble levels, `o/road/kos10 50.p3d` and `kos10 75.p3d` at resolution
.5, have no source land hints. They are **not** ordinary opaque exceptions:
their original alpha section flags still reject them. Their resolution-4
levels do carry land hints. At runtime `RoadType` additionally applies
`OnSurface | NoZWrite` and land-on hints to road shapes without solid geometry;
missing source hints are not permission to bypass road routing.

The bridge is the other explicit exception. Its solid geometry avoids that
road-fitting treatment. Only its three lower-level `o/road/sil_new.paa` overlays
are reviewed here, and they remain alpha-rejected without land hints. The
separate `o/misc/most01.pac`, `most02.pac` and `sil_new.pac` sources are **not**
reviewed or approved; its 1,776 source-unknown alternative-LOD triangles remain
pending. The bridge is a partial source review, not a completed bridge assessment.

The existing implementation explains why an opaque substitution is unsafe:

- `RoadType` establishes terrain fitting, no depth writes and surface routing.
- `Object::Draw` uses the terrain surface-split path for all-land-on shapes.
- The scene orders road overlays before decals and projected shadows; visible
  asphalt blend sections do not behave like standalone opaque depth owners.
- The material classifier independently rejects alpha, on-surface routing,
  no-depth-write, depth bias and fitted-land vertex hints. Captured ambient and
  direct lighting remain mandatory for reception.

These paths are unchanged. A future road solution would need a verified
fitted-overlay lighting/compositing contract: terrain-conformed geometry and
normals, alpha/underlying-ground ownership, correct depth/shadow/decal ordering,
and valid ambient/direct-light capture. That is not supplied by the existing
opaque PBR/GI paths or by the original color image alone. It is documented as an
unresolved capability, not implemented by removing the gates.

## Gameplay verification scope

Original Malden `01TakeTheCar` and Nogova `R01WarCry` gameplay run directly through
the developer harness with copied profiles and `--nosound`. Terrain-grounded
standing cameras inspect original asphalt, dirt, roadway, cobbled street and
junction placements. This is neither normal Steam-route nor mission-completion
acceptance. GI-off/on, stock v3 and auto separate existing surroundings from
road-source participation. Overcast/dusk, moving/settled TAA and FSR, MSAA4,
two-bounce operation, native/CPU restoration and distance retreats are independent
controls, not proof of a newly implemented road response.

The classic views report zero non-terrain receiver families in the sampled
frames; native terrain samples are 78,668 / 132,557 / 68,202 for asphalt / dirt /
roadway. The asphalt route control separately records 78,729 native terrain,
78,771 CPU terrain and 78,688 restored native terrain pixels. Those counters are
**not road receiver counts**. Underlying ground, existing surrounding materials,
sun/shadow/weather and alpha composition can change visible road-area pixels
without giving the road itself a qualified material or separate GI receiver.

Nogova asphalt and dirt views likewise record terrain reception, not road
identities. The cobble-edge view faces an existing qualified stone wall and roof:
46,432 stone / 10,182 roof / 88,196 terrain pixels in one native sample. The
CPU/restored controls continue those existing paths independently; they are not
new cobble reception. The separate junction view looks along the full street.
Original photograph contrast, paving repetition, shadow boundaries, LOD behavior
and visible tile-edge discontinuities remain; this is not a blanket clean-image
certification. No authored feature or acceptance gate was changed to hide them.

Full states, timing samples, validation shutdowns, preservation checks and
sampling limits are retained in the [qualification receipt](evidence/material-modernization/batch-10/qualification.json).

| Original-world view | Auto / GI off | Auto / GI on |
| --- | --- | --- |
| Classic asphalt | [off](evidence/material-modernization/batch-10/modernization-b10-roads-abel-1080-classic-asphalt-enhanced-off.png) | [on](evidence/material-modernization/batch-10/modernization-b10-roads-abel-1080-classic-asphalt-gi.png) |
| Classic dirt track | [off](evidence/material-modernization/batch-10/modernization-b10-roads-abel-1080-classic-dirt-enhanced-off.png) | [on](evidence/material-modernization/batch-10/modernization-b10-roads-abel-1080-classic-dirt-gi.png) |
| Classic roadway | [off](evidence/material-modernization/batch-10/modernization-b10-roads-abel-1080-classic-road-enhanced-off.png) | [on](evidence/material-modernization/batch-10/modernization-b10-roads-abel-1080-classic-road-gi.png) |
| Nogova asphalt | [off](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-nogova-asphalt-enhanced-off.png) | [on](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-nogova-asphalt-gi.png) |
| Nogova dirt track | [off](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-nogova-dirt-enhanced-off.png) | [on](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-nogova-dirt-gi.png) |
| Nogova marked road | [off](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-nogova-road-enhanced-off.png) | [on](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-nogova-road-gi.png) |
| Cobble edge / existing wall | [off](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-cobbles-enhanced-off.png) | [on](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-cobbles-gi.png) |
| Cobble junction / full street | [off](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-cobble-junction-enhanced-off.png) | [on](evidence/material-modernization/batch-10/modernization-b10-roads-noe-1080-cobble-junction-gi.png) |

### Existing-renderer GPU workload

RTX 4060 Ti, TAA at 100% scale, RT sun/cloud shadows, one bounce;
AO/volumetrics/water optics off. Warm ten seconds, then Off/On/On/Off with
four-second settling and six-second samples. Values average paired interval
medians; no build or other game runs during timing and probes are off.

| Scene | GPU frame GI off / on | GI trace / filter / history | CPU recording GI on |
| --- | ---: | ---: | ---: |
| Classic asphalt / village 1920x1080 | 5.953 / 6.546 ms | .375 / .163 / .143 ms | 17.122 ms |
| Cobble edge / wall 1920x1080 | 3.512 / 3.849 ms | .437 / .086 / .069 ms | 9.104 ms |
| Cobble edge / wall 2560x1440 | 4.023 / 5.469 ms | .609 / .163 / .093 ms | 9.393 ms |

These measure the **existing** renderer's work in road views, including terrain
and already supported surrounding wall/roof materials. They are not before/after
performance deltas or the cost of new road coverage: no road material/rendering
implementation was added. In the village, paired GI-on frame medians vary
6.292-6.801 ms and Off medians 5.860-6.045 ms. Whole-frame/CPU variation and
non-additive per-stage samples are retained rather than presented as an isolated
road transport budget. Both 1080p and 1440p world/output sizes are verified live.

All three Vulkan runs exit normally after **47,718 presented frames**, with
synchronization validation enabled, zero errors and zero warnings. The tested
Vulkan SHA256 is
`BF0730CEC3DFD54F8AD5EDA35F415DCEADC5A590AC3B84B5F3BE248BF3A41313`.
This public rebuild identifies batch-09 HEAD; the only new tracked code is the
road regression test. No game process remains after acceptance.

## Tests and reproduction

Public Vulkan and GL33 builds pass. Focused tests use
`[vulkan-shape],[Shaders],[enhanced-material],[ray-surface],[render-flags]`:
Vulkan **78 cases / 62,541 assertions**, GL33 **77 / 62,501**. The new test pins all
17 source identities as untyped, four observed alpha flag combinations,
hypothetical-known-source rejection and independent land/surface/depth gates.
Existing `[surface],[ZBias]` tests pass **17 cases / 52 assertions** on each build,
including road-before-decal ordering. Python inventory/census tests pass 18;
driver-free Vulkan policy guards pass 690. Their intentional teardown error is
not a gameplay validation finding.

Inspect a source model with the existing CLI, substituting any audited path:

```powershell
dist/vulkan-local/PoseidonTools.exe model inspect `
  "build/original-asset-inventory-20261021/library/o/road/kos10 50.p3d" `
  --runtime-loader --all-source-lods --textures --sections --lighting --qualify --uv-triangles `
  --texroot build/original-asset-inventory-20261021/library `
  --material-config game-local/Remastered/BIN/CONFIG.BIN
```

All 75 fresh native reports match both accepted source and ordinary-adapter
censuses without any identity replay. The tool SHA256 is unchanged from batch 09;
no global material policy changes. The immutable 726-model / 1,711-authored-level
inventory and 1,708-retained-level distinction remain. This batch does not add
eligible triangles or claim full rendering coverage.

Live GL33 Off/On/Enhanced/Auto/Off controls report unsupported Vulkan features
and **zero changed pixels** in the original-world ROI. All **270 original input
hashes** and **67 settings/fixture/input hashes** match their retained manifests.
Private DLSS remains clean and remote-free, with both SDKs and preserved
build/runtime snapshots present. Original assets, profiles and settings remain.

Backlog task 30 is now `assessed-retain-existing-contract`, alongside the two
canopy tasks. The 30 pilot tasks have 11 complete source reviews, 16 partial
reviews and three retained contracts—not 30 completed implementations. The
726-model ledger records **60 complete source reviews, 91 partial, 364 pending
and 211 retained existing contracts**. The 74 roads retain their disposition
with stronger original-source evidence; the shared bridge becomes partial and
its three other source images remain pending. The full goal remains incomplete.
