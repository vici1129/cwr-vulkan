# Vulkan backend development

## Optional contact-hardening CSM (2026-10-12)

Built on `28139f7`; the existing contact-shadow, RT, volumetric and MSAA fixes
remain in place. Default **Off**, runtime-only and Vulkan-only:

```sqf
triSoftShadows "on"
triSoftShadows "off"
triSoftShadows "status"
```

No hardware RT or `CWR_VK_RTAO=1` requirement. No new menu, saved setting,
dependency, shadow map, fullscreen pass, history or denoiser. Status reports
the switch, current CSM/source availability, sampling budget, radius cap and
the explicitly unchanged hard-RT behavior. Toggling resets ordinary world-AA
history but does not recreate render targets. Enabling this does not enable
CSM, RT or `triContactShadows`; these remain independent controls.

### Surface filtering and lighting

`shadow_sample.glsl` retains the existing cascade selection, coverage
fallthrough, quadratic cascade bias, 15% transition bands and far fade. Off
executes the original nine nearest-depth comparisons and their original
arithmetic. On adds a bounded directional-light PCSS-style branch:

- Search 13 depths: the original 3×3 footprint plus four wider light-space
  probes. The wider search targets 60 cm, limited to 1–8 shadow texels. No
  blockers returns lit; small estimated penumbrae reuse the original 3×3 result.
- Recover UV/metre and depth/metre from the lengths of the orthographic
  light-matrix rows. Average positive receiver-to-blocker depth separation is
  converted to metres, then multiplied by `tan(sun radius) = 0.007` (about
  0.4 degrees). This is a modest gameplay-tuned sun size, not a calibrated
  physical area-light solution. Cascade depth padding does not set softness.
- Clamp the resulting filter radius to 1–8 texels. Smoothly transition from
  the contact filter over the first two texels, avoiding a kernel switch at
  the threshold. Larger penumbrae use eight fixed disk positions, each with
  a four-depth gather and bilinear comparison weights: at most 21 texture
  operations / 45 fetched depth values per sampled cascade. Cascade blending can
  sample two cascades. The initial sparse search could miss narrow casters;
  retaining the complete 3×3 neighborhood fixed that during tuning.
- Derive receiver-plane gradients before divergent cascade selection. Correct
  each comparison's receiver depth while retaining the original base bias;
  bound correction to max(2×bias, 5 cm in light-depth units). Narrow kernels
  near map edges and clamp footprints inside the map so the white sampler
  border cannot become a spurious penumbra light source.

The pattern is fixed in light space with the existing snapped cascade maps;
there is no frame-varying noise. World AA sees the result normally, including
jittered TAA/FSR input and MSAA. Resolution is the actual CSM texel size, not
the screen/output resolution.

Soft surface shadows use the existing separated-direct-sun lighting path,
including CPU-lit terrain, without requiring a sunlight MRT. Ambient, local
lights and emissive terms are not passed through the soft filter. This matters
visually: the original non-RT legacy CSM path attenuates the whole surface;
the sunlight-only branch can have **lighter shadow interiors**, especially
on ambient-heavy terrain/foliage. The comparisons therefore show both the
penumbra change and this existing lighting-path distinction, not just a blur.
Off restores the legacy path exactly; RT-enabled lighting keeps its existing
separation. Water, blended/unsupported sunlight receivers and volumetric
scattering keep their original shadow sampling. Night/zero-source/full-overcast
use the original path, with no additional soft filtering or sun separation.

### Hybrid RT and contact shadows

The raw RT visibility/reduction and its off-screen caster coverage are
unchanged. Existing conservative `min(CSM, RT, contact)` composition remains;
there is no multiplication of independent darkening factors. A hard RT result
can dominate the softer CSM edge. **RT silhouettes have not become physically
soft**: this milestone adds neither multiple area-light rays nor RT denoising.
Disable RT sunlight to inspect the CSM penumbrae independently. Short-range
screen-space contact shadows remain separate and can reinforce feet/tracks
without widening their own rays or changing their sampling budget.

### Gameplay evidence and checks

Muted, isolated developer launches used stock GOG 3.05 missions
`02Infantry.Abel` (Malden) and `05HeavyMetal.Eden` (Everon), not a Steam-route
or mission-completion qualification. Final tested executable SHA-256:
`99edc54f9e95e9e57ca08d524fb8f94ee2f4bb1ff2a27a21adc3b131eb1f94d7`.
Subsequent source changes to the tested implementation are comments only.

Frozen Off → On → Off views cover soldiers, jeep wheels, tank tracks, building
edges, fences, poles, bushes, forest floor and terrain. Tested noon, 06:00,
18:00, full overcast and midnight; CSM and RT sunlight independently Off/On;
AA Off, TAA, FSR Quality, MSAA 2/4/8; separate contact shadows; SSAO/RTAO,
atmosphere/volumetric fog and water reflections. The Malden water run reported
96,994 shaded reflection hits in the sampled completed frame, not merely an
enabled toggle. Actual walking inputs moved the player on both islands;
camera sweeps and distance sweeps crossed the leading approximately 20/50 m
omni-cascade transition regions. Also checked CSM resolutions 1024/2048/4096
and odd 1281×721 resize/minimize/restore.

Native-resolution crops, with no brightness enhancement; RT sunlight and
screen-space contact shadows are Off in these pairs:

| View | Original CSM | Soft CSM |
|---|---|---|
| Malden dawn building edge | ![Building Off](evidence/soft-shadows/malden-building-off.png) | ![Building On](evidence/soft-shadows/malden-building-on.png) |
| Everon dawn forest floor | ![Forest Off](evidence/soft-shadows/everon-forest-off.png) | ![Forest On](evidence/soft-shadows/everon-forest-on.png) |
| Everon noon track contact | ![Tracks Off](evidence/soft-shadows/everon-contact-off.png) | ![Tracks On](evidence/soft-shadows/everon-contact-on.png) |

Raw local captures, action receipts, hashes, status responses and logs are
under the original checkout's `build/shadow-live/soft-malden-final` and
`soft-everon-final`. Final validation/synchronization-validation shutdowns:

- Malden: 23,372 submitted / 23,371 presented; **0 errors, 0 warnings**.
- Everon: 60,160 submitted / 60,158 presented; **0 errors, 0 warnings**.
- Fixed 1800×550 scene crops returned pixel-identically after disabling the
  effect in the Malden village/dawn/noon and Everon forest/dawn comparisons.
  Night, full overcast and CSM-Off views (RT independently Off/On) also had zero
  Off/On changed pixels in those checked regions. This excludes changing UI
  and does not claim arbitrary full-frame temporal-AA bitwise equality.
- Cross-executable check against the original `28139f7` build: a fixed
  730×80 terrain region was pixel-identical with this feature Off, both with
  CSM enabled and disabled. The larger cross-launch view was not identical
  (moving scene/UI elements); this is a scoped check, not a full-frame claim.

The stale system Epic overlay manifest still emits a loader message, separate
from the zero live validation counters. Vulkan+FSR and Vulkan-disabled GL33
builds passed. Focused results: **425** driver-free policy/lifetime checks,
**949 assertions / 67 cases** for shader/shadow-math/Vulkan-shape tests, and
**909 assertions / 66 cases** in the GL33-only equivalent. The policy suite's
deliberately injected teardown validation error is expected. New tests cover
default/invalid/independent controls, sun-source gating and the actual shared
GLSL penumbra scalar math (contact floor, cap, monotonicity and cascade units).
The GL33-only executable also returned `UNSUPPORTED: soft shadows require
Vulkan` for all three runtime commands.

### Performance

RTX 4060 Ti, driver 617.14; validation disabled for timings. Fixed Malden
village camera, CSM 2048, AA Off, 100% render scale, VSync Off, RT/contact/AO/
volumetrics Off. No RT opt-in environment variable was present; status showed
zero BLAS/TLAS/instances. Two Off/On cycles per condition, each with four
seconds settling and eight seconds measurement (four existing two-second
GPU profiling windows). Each table cell is the median of eight windows.

| Actual world/output extent | Sun | Original CSM, ms | Soft CSM, ms | Observed whole-frame delta |
|---|---|---:|---:|---:|
| 1920×1080 | Noon | 1.592 | 1.659 | +0.067 ms / 4.2% |
| 1920×1080 | Dawn | 1.609 | 1.692 | +0.082 ms / 5.1% |
| 2560×1440 | Noon | 1.848 | 1.925 | +0.077 ms / 4.1% |
| 2560×1440 | Dawn | 1.867 | 1.956 | +0.090 ms / 4.8% |

These are **whole-frame GPU timestamps, not isolated PCSS cost**. Automatic
GPU clocks increased with the added load: 1080p noon 1155→1365 MHz, dawn
1230→1500 MHz; 1440p noon 1680–1695→1995 MHz, dawn 1725–1785→2130–2145 MHz.
Memory clocks were stable within each resolution (5001 / 9201 MHz reported).
Consequently the observed delta understates added work at equal clocks; no
fixed-clock cost or cross-GPU performance claim is made. The unchanged CSM
depth pass also became faster with higher clocks. All sampled windows reported
zero allocations/frame. There is no separate soft-shadow pass or new image
memory. The 1440p window's actual render/output extent was verified by status
on a 1080p desktop; this is not a native-1440p display-quality qualification.

Raw profiling windows and clock readings are retained locally in
`build/shadow-live/soft-perf1080` and `soft-perf1440` in the original checkout,
as `soft-timing.json` and `soft-timing-dawn.json`.

### Limitations

This is a small-budget shadow-map approximation. The average blocker can mix
several caster depths; the bounded search can miss thin distant blockers and
the eight-texel cap deliberately limits very long low-sun penumbrae. Existing
CSM caster coverage, rasterization bias, resolution and foliage alpha/LOD
limitations remain. The contact floor and transition bands preserve detail
but cannot make different cascade resolutions identical. Camera sweeps did
not reveal new gross seams or frame-noise crawling in the checked views;
subpixel foliage/fences can still alias with AA Off, and existing temporal AA
has its own motion limitations. Ambient-heavy scenes can show a larger change
in shadow intensity than penumbra width, as explained above. Hard RT edges
remain hard. These checks do not establish artifact-free rendering for every
mod, grazing angle, GPU or extreme shadow tuning.

## Optional screen-space contact shadows (2026-10-12)

Built on `8b25bd1`, including the volumetric/MSAA follow-up below. Default **Off**:

```sqf
triContactShadows "on"
triContactShadows "off"
triContactShadows "status"
```

Vulkan only; **no `CWR_VK_RTAO=1` or hardware ray tracing requirement**. GL33
returns `UNSUPPORTED` without changing rendering. There is no menu, persistent
setting, new SDK, scene database, denoiser, or separate temporal history. Status
reports requested/active state, whether the last world drew, cumulative draws,
sunlight source, actual world extent and fixed ray parameters. Changes take
effect at the next frame's ordinary resource recreation and reset world AA
history. Independent MRT blending is required.

### Implementation and tuning

`VulkanContext_Contact.cpp` draws one full-resolution triangle using existing
world depth and the separated-sunlight MRT. `contact_shadows.frag` reconstructs
camera-space positions from the actual depth texture extent, pixel centers,
projection and current projection jitter. The engine's current sunlight vector
is transformed through the camera rotation; no guessed screen-space direction
or output-resolution assumption is used. The existing atmosphere frame carries
these inputs even with atmosphere/volumetric effects disabled.

There are 16 deterministic uniform samples over a 1.2 m ray. The origin moves
2 cm along a depth-derived normal and 1 cm toward the sun. A bounded 16–20 cm
view-depth slab, a near-hit ramp, receiver-plane height confidence (fade from
20 to 50 cm), and diminishing confidence along the ray suppress self-hits and
detached silhouette bands. Contact fades at grazing receivers, within 3.5% of
screen edges, and between 25 and 40 m from the camera. Off-screen samples stop
the ray. These are gameplay-tuned heuristics, not geometric ray intersections:
an initial thinner slab missed feet; longer/nonuniform steps produced separated
pole marks, and raised tank hulls needed the receiver-plane confidence guard.

Opaque/cutout raster depth includes animated soldiers without adding them to an
RT scene. Receiver capture excludes water, blended surfaces and unsupported
legacy lighting paths. The shared `sunlight_depth.glsl` retains the existing RT
MSAA coherence guard: reject mixed background/depth/light samples instead of
subtracting a full sample's sunlight from resolved foreground color.

Only the extracted direct-sunlight contribution is subtracted. Contact-only
capture preserves the original legacy CSM scene result, including its existing
ambient/fog behavior, wherever no additional occlusion is found. Visibility is
`min(CSM, RT, contact)`, not multiplied darkening: when RT sunlight runs, the
contact triangle takes the componentwise maximum of its sunlight reduction and
the existing raw RT reduction, replacing the RT sunlight composition triangle.
RT still supplies hidden/off-screen casters and longer shadows. Without RT, the
same pass runs independently, before AO, reflections, volumetrics and world AA;
it does not darken local lights, emissive/ambient terms, scattering, sky or UI.

No fullscreen contact draw or sunlight capture runs when the engine's current
sun source is below 0.001 or sun color is negligible (including tested full
overcast/night). While enabled, the shared sunlight attachment remains allocated
and cleared even then. Off creates no contact pipelines/descriptors, requests no
additional sunlight target and submits no contact draw. Enabling it without an
existing sunlight MRT adds an 8-byte-per-sample world attachment: about 47.5 MiB
at 1080p or 84.4 MiB at 1440p for three single-sample swapchain images, multiplied
by MSAA sample count. No contact result image/history is allocated.

### Gameplay evidence and regression checks

Local muted developer launches used stock GOG 3.05 content, isolated profiles,
Malden `02Infantry.Abel` and Everon `05HeavyMetal.Eden`. This is gameplay/render
qualification, not a Steam launch or mission-completion claim. Frozen cameras
were compared Off → On → Off; physical input also moved soldiers approximately
10 m on Malden and 16 m on Everon. Checked feet, jeep wheels, tank tracks,
buildings, walls/fences, poles, bushes and forest floor at noon, 06:00, full
overcast and midnight; CSM/RT sunlight independently Off/On; AA Off, TAA, FSR
Quality and MSAA 2/4/8. Volumetric fog, atmosphere, SSAO/RTAO and RT water
reflections were exercised, plus 1281×721 resize and minimize/restore.

Native-resolution crops below are from the final shader, identical frozen
Off/On cameras with CSM and RT sunlight Off so the added short contact is visible.
No brightness adjustment or enhancement was applied.

| View | Off | On |
|---|---|---|
| Malden boots | ![Boots Off](evidence/contact-shadows/malden-feet-off.png) | ![Boots On](evidence/contact-shadows/malden-feet-on.png) |
| Everon tracks | ![Tracks Off](evidence/contact-shadows/everon-tracks-off.png) | ![Tracks On](evidence/contact-shadows/everon-tracks-on.png) |

Final executable SHA-256:
`36bcb2609eb622b6572d09b60923ffb4c98891218fa8595b0bfea6c4218bb78a`.
Local raw captures, command receipts, launch hashes and logs are under
`build/shadow-live/` in the original `cwr-vulkan` checkout:

- `contact-malden-final`: broad Malden matrix during tuning, 32,409 submitted /
  32,408 presented, zero Vulkan validation errors/warnings. The final receiver
  confidence adjustment was subsequently rechecked on Malden in
  `contact-perf1080` (feet, village, vegetation, no RT opt-in).
- `contact-everon-v2`: final shader, complete Everon AA/weather and tank/CSM/RT
  pairs, forest fog and camera motion; 20,655 submitted/presented, zero errors
  or warnings. Frozen tank-region Off-return differences were zero with CSM
  Off, CSM On and RT On; full-overcast/night Off/On differences were also zero
  in that region and status reported no contact draw.
- `contact-combined1440`: final shader, all AA modes with CSM, RT sunlight,
  RTAO and volumetrics enabled, walking, forest fog and odd resize/minimize;
  34,478 submitted / 34,477 presented, zero errors/warnings.
- `contact-baseline` versus `contact-perf1440`: pre-change `8b25bd1` executable
  versus new executable with contact Off at 1281×721, CSM Off and On. The fixed
  1040×240 terrain crop was pixel-identical in both cases. Whole frames were
  **not** identical (NPC/cloud/UI changes and a few building pixels); this is
  not a claim of full-frame cross-process bitwise equivalence.
- `contact-gl33`: Vulkan-disabled build; all three commands returned unsupported.

The system's stale Epic overlay manifest emitted a loader message; the Vulkan
validation and synchronization-validation counters above remained zero. The
policy test's deliberately injected teardown validation error is an expected
driver-free test, not a live validation failure.

Vulkan+FSR and GL33-only builds passed. Focused Vulkan policy/lifetime suite:
417 checks; shader/shadow-math/Vulkan-shape suite: 931 assertions in 66 cases;
GL33-only equivalent: 891 assertions in 65 cases. Added coverage includes
default/deferred/invalid controls without a ray scene, shared MSAA sunlight
policy, and projection/jitter round trips at native, scaled and odd extents.

### Measured GPU cost

RTX 4060 Ti, driver 617.14, VSync Off, AA Off, 100% scale, RT opt-in absent,
validation disabled for timing. Fixed Malden village view
`[7116,73,6085,-1,-0.1,-0.2]`, view distance 900, noon/clear/fog zero. Each state
had 3 s warm-up and 8 s measurement; entries are medians of four 2 s profiling
windows. Contact timestamps bracket the march/composition and are read after
the existing frame fence (no new wait). Raw data: `contact-perf1080` and
`contact-perf1440/contact-timing.json` (each run has its own JSON).

| Actual world extent | CSM | Contact pass ms | GPU frame Off → On ms |
|---|---|---:|---:|
| 1920×1080 | Off | 0.512 | 1.201 → 1.680 |
| 1920×1080 | On | 0.405 | 1.576 → 1.716 |
| 2560×1440 | Off | 0.583 | 1.669 → 1.959 |
| 2560×1440 | On | 0.451 | 1.730 → 1.914 |

All measured steady-state windows reported zero allocations/frame. These are
not locked-clock benchmarks: sampled graphics clocks rose from 795–1110 to
1275–1665 MHz at 1080p, and 1245–1755 to 2010–2580 MHz at 1440p; memory clocks
also changed. Consequently frame subtraction understates the isolated pass
cost and the resolutions are not directly scaling-comparable. The contact
timer excludes sunlight-MRT capture overhead; whole-frame timing includes it.
1440p used a verified 2560×1440 world/output extent in an oversized window on
the 1080p desktop, not a full-monitor 1440p visual inspection. MSAA, other GPUs
and denser scenes can cost more; the table is not a universal FPS promise.

### Remaining limitations

Only the frontmost screen depth is known. Hidden/off-screen casters cannot cast
contact shadows, and edge/distance fades deliberately lose them. Thin fences,
foliage and subpixel geometry can alias or miss steps with AA Off; TAA/FSR use
only the existing world history. Mixed MSAA pixels and grazing/raised surfaces
are intentionally conservative and may lose detail. The final tuning removed
the observed detached tank-front band and avoided broad dark halos in the
tested views, but does not establish artifact-free motion for arbitrary assets,
camera angles or mods. This is short contact reinforcement, not a replacement
for CSM/RT or an attempt to synthesize full-length soldier/building shadows.

## Focused MSAA and volumetric CSM follow-up (2026-10-12)

Built on `64a7c1c`. Four narrowly scoped corrections:

- RT sunlight's post-resolve subtraction now rejects ambiguous MSAA pixels.
  All samples must have valid, coherent view depth and sunlight records; the
  depth tolerance is max(2cm, 0.2% of depth), and sunlight channels tolerate 2%
  with a 0.0001 floor. This permits small slopes but falls back to ordinary CSM
  at discontinuities instead of subtracting one sample's full sunlight from an
  averaged color. This is conservative rejection, not per-sample ray tracing;
  steep slopes/material boundaries can lose the optional correction.
- RT water requires every MSAA sample to carry water. The shared `waterAt`
  guard runs in tracing and final composition, including filter neighbors, so
  filtering cannot refill rejected water/foreground pixels. Mixed pixels retain
  ordinary water/foreground rendering. Full-water pixels retain their blend,
  excluding rejected neighbors; the single-sample path is unchanged. Both
  guards cover MSAA 2x, 4x and 8x; RTAO is unchanged.
- Volume-only CSM demand and terrain submission share the scattering draw's
  source threshold (0.001) and valid-fog-range predicate. Current-frame source
  data already includes weather, sun elevation and night vision. Independently
  enabled surface CSM still requests its maps and retains its existing night
  gate; a zero volumetric source does not disable surface shadows.
- The terrain patch is now an immutable cached mesh, uploaded only when its
  landscape/grid/cell/radius key changes. Vertices are relative to a stable grid
  origin; each cascade incorporates origin-minus-camera translation into its
  existing matrix. No per-frame combined-caster copy, terrain translation loop,
  or terrain upload remains. Existing frame-held mesh ownership protects old
  patches during replacement. The same bounded, double-sided geometry is drawn
  separately, adding one draw per cascade. Maximum geometry is still 270,000
  vertices / 3,240,000 bytes at 600m and 8m cells; cache-boundary rebuilds and
  their allocations remain. This does not claim a measured end-to-end CPU/FPS
  improvement or eliminate ordinary object caster collection.

Verification: Vulkan/FSR and GL33-only builds pass; all twelve AO/sun/water shader
variants compile. Driver-free policy/lifetime tests pass **412 checks**. Focused
`[Shaders],[ShadowMath],[vulkan-shape]` tests pass **893 assertions / 66 cases**
with Vulkan and **889 / 65** in GL33-only. Tests compile the shipped scalar
coverage predicates, cover partial coverage at 2x/4x/8x, pin final water-filter
rejection, test the common source threshold and verify cached terrain matrix
translation. The broader `[Graphics]` run has **13 PAA/DDS failures and one
stock-data skip**, reproduced with the unchanged pre-fix `35f7658` build; these
are not counted as passes or changed in this fix.

Live evidence is under `build/shadow-live/surgical-fixes-v1` (Malden, 1080p) and
`surgical-everon-v1` (Everon, 2560x1440 attachments in an oversized desktop
window). Malden exercised foggy dawn, noon water/pier reflections, overcast,
night, cache-cell-crossing movement, volume Off/On, MSAA 2x/4x/8x, Off/FXAA/SMAA/
TAA/FSR Quality, odd resize and minimize/restore with all three RT effects.
Overcast reported zero sunlight/scattering and no CSM with surface CSM Off;
turning surface CSM On retained ready maps without enabling scattering.
Malden closed after **17,452 submissions / 17,451 presentations, zero Vulkan
validation errors or warnings**. Everon exercised MSAA 8x, AA Off and TAA with
all RT effects, dawn forest scattering and camera translation/rotation; it
closed after **15,614 submissions/presentations, zero errors or warnings**.
Its frozen-camera volume Off/On/Off sequence restored the compared 1600x850
world-image region exactly (zero differing pixels). Pixel-level MSAA correctness
is established
by the source/math regression checks; these captures are gameplay smoke checks,
not an exhaustive before/after silhouette comparison. Earlier GPU benchmark
tables below describe the original implementation, not this optimization.

## First CSM volumetric sunlight and fog (2026-10-12)

Built on public `35f7658`, default **Off**, Vulkan only. Process-local commands:

```sqf
triVolumetricFog "on"
triVolumetricFog "off"
triVolumetricFog "status"
```

No ray-tracing device, startup opt-in, SDK, persistent setting or menu is required.
GL33 returns unsupported. Status separates the request, allocated resources and
last scattering draw, and reports completed recording count, sunlight weight,
fog interval, CSM reach and actual half-resolution dimensions. Changes use the
existing idle/recreation boundary and invalidate world temporal history.

### Rendering and integration

`EngineVK::UpdateAtmosphere` supplies the camera basis, direction toward the sun,
eye-adapted engine diffuse sunlight, fog color and original fog interval. Source
strength follows `NightEffect`, `SkyThrough` squared and a smooth low-sun elevation
fade; night vision disables it. No fog or mission visibility values are written.
The homogeneous scattering density is `1 / (fogEnd - fogStart)`. The legacy
clear-distance offset remains a visibility control, not an empty sphere of air:
scattering can occur between the viewer and nearby trees. Analytic Beer-Lambert
segment weights and a Henyey-Greenstein phase function (`g=0.55`) approximate
single scattering. Added luminance is bounded to 30% of the original fog-color
luminance, scaling RGB together to retain the sun hue. The existing fog color,
transmittance and lighting calculation remain in their original shaders; there
is no second fog blend or additional attenuation of scene color.

One FP16 target per swapchain image is half the **world input** dimensions, rounded
up. Each pixel marches 48 spatially jittered samples, bounded by the nearest depth
in its 2x2 footprint and 400 metres. MSAA variants take the nearest covered sample.
Every sample uses current camera-relative CSM matrices, sphere/frustum tier
selection, four depth comparisons and cascade transition bands. Unknown coverage
contributes no sunlight rather than assuming open sky. The volumetric comparison
uses a small air-sample bias, independently of the surface receiver's darkness
and larger bias. Spatial noise breaks coherent march bands without a separate
volumetric history, denoiser or full-resolution fallback march.

A four-tap joint bilateral upsample uses stored view depth and the full-resolution
world depth. Unmatched silhouette samples contribute zero instead of background
light leaking over foreground geometry. Additive RGB blending preserves scene
alpha and writes the existing resolved FP16 world color in its LOAD pass. This
runs after world/AO/RT composition and before TAA or FSR, spatial AA, bloom and
tone mapping. TAA/FSR therefore reconstruct the completed scattering with their
existing history rejection. Cockpit, weapon, HUD, map and UI remain after this
boundary. There is no change to the RT scene, ray-query shaders or AA algorithms.

`NeedsSunShadowDepth` defaults to the existing surface-CSM selection. Vulkan can
also request depth for volumetrics while preserving that surface selection and
projected shadows. The existing caster collection provides buildings, objects,
cutout foliage and other supported casters. It previously contained no terrain:
while volumetrics is active, Vulkan appends a two-sided landscape patch sampled
from `SurfaceY`. Its radius is `min(CSM far + 100m, 600m)` and cell size is at least
8m (or the coarser current terrain grid). The follow-up above caches a grid-local
mesh until the camera crosses a cell or the landscape/grid/radius changes, and
applies camera translation through each cascade matrix instead of the vertex
stream. No second shadow-map system is added.

When a volumetric draw runs, the existing screen-space sun-shaft draws are
suppressed. Their requested setting survives; status reports
`suppressed=volumetric-fog`. With volumetrics Off, their original behavior resumes.
Off allocates no volumetric images, submits no volume draws and requests no extra
CSM or terrain work. Resources use the existing idle swapchain teardown and
frame-fenced uniform allocator. Profiling adds two queries to the existing frame
query pool, read after its fence, with no synchronous GPU wait.

### Gameplay and regression evidence

Tests used isolated stock GOG 3.05 missions and profiles, muted direct developer
launches, RTX 4060 Ti / driver 617.14. These are renderer qualification runs, not
Steam launch qualification or completed missions. Local screenshots, executable
hashes, evaluator actions and logs are under `build/shadow-live/volume-*` in the
original checkout; no stock mission or asset was edited.

- Malden `02Infantry.Abel`: frozen Off/On village, hillside and forest comparisons;
  dawn, noon, sunset, overcast and night; camera translation/rotation; Off, FXAA,
  SMAA, TAA, MSAA 2/4/8 and FSR Native/Quality/Balanced/Performance. FSR dispatches
  and actual reduced input dimensions were verified. Combined RTAO, RT sunlight,
  water-reflection, atmosphere, bloom and sun-shaft selections were exercised,
  including a shoreline view. Resize to 1280x720 and minimize/restore passed.
  `volume-malden-v3` closed after 14,818 submitted / 14,817 presented frames with
  zero core/synchronization validation errors or warnings; one presentation was
  interrupted by recreation.
- Everon `05HeavyMetal.Eden`: the forest at camera `[6750,86,5360]`, direction
  `[1,.03,0]`, July 5 at 06:00, fog `.65`, shows distinct warm shafts through the
  trunks. Foreground trunks remain dark and occlude the light. Off/On/Off with AA
  Off restored the 1600x900 world-image comparison region exactly (zero differing
  pixels); HUD/window chrome were excluded. Translation/rotation sequences with
  AA Off and TAA showed no obvious long trails in the inspected frames. Assisted
  placement followed by physical forward input moved the player about 25m through
  the real forest with live simulation and visible scattering. Noon, overcast,
  night, sunset and dawn were also compared. `volume-everon-v3` closed after 29,536
  frames with zero validation findings, without ray-tracing startup opt-in.
- Night reported no CSM/scattering and its tested Malden Off/On image region was
  identical. Full overcast reported zero source weight and no scattering draws.
  Temporally reconstructed comparison images are not generally pixel-identical
  across toggle/reset boundaries; only the specific stable comparisons above are
  claimed as exact matches.
- The final binary's `volume-final-validation` run covered a 2560x1440 drawable,
  FSR Quality with RT effects/bloom, resize/minimize/restore to 1281x721, SMAA at
  150% (1922x1082 world / 961x541 volume), MSAA 4x and returning to volume Off.
  It closed after 23,942 submitted / 23,941 presented frames with zero validation
  findings. A stable AA-Off comparison of both shaft selections versus volume
  alone was identical in the 1600x850 world-image region excluding the HUD;
  status confirmed screen-space shafts were suppressed. The GL33-only build
  also rendered stock infantry, returned unsupported for all three volume
  commands and closed normally (`volume-gl33`).
- Vulkan and GL33-only RelWithDebInfo builds passed. Focused
  `[vulkan-shape],[ShadowMath],[Conventions],[ShutdownOrder]` suites passed 76 cases /
  831 assertions with Vulkan and 75 / 827 with GL33 only. The driver-free Vulkan
  policy/lifetime suite passed 394 checks, including default-Off, deferred
  enablement and invalid-command state preservation. Its deliberate mock teardown
  error is separate from live validation.

### Actual GPU cost

Validation-disabled, VSync off, AA Off, scale 100, fixed Malden forest camera
`[6800,79,5500,1,.05,0]`, July 5 at 06:00, fog `.6`, view/object/shadow ranges
900/600/250m. Each result is the median of four existing two-second profiling
windows, after three seconds of warmup. The new interval includes both marching
and depth-aware composition, not just the half-resolution draw.

| Output | Half target | Volume GPU ms (surface CSM Off / On) | CSM depth GPU ms with volume On |
| --- | --- | ---: | ---: |
| 1920x1080 | 960x540 | 0.813 / 0.779 | 0.505 / 0.490 |
| 2560x1440 | 1280x720 | 1.391 / 1.393 | 0.489 / 0.488 |

CSM depth is additional work when surface CSM was Off. When surface CSM was
already On, its maps are shared, with the terrain patch added. Whole-command-
buffer GPU measurements for context:

| Surface CSM | Volumetrics | 1080p frame GPU ms | 1440p frame GPU ms |
| --- | --- | ---: | ---: |
| Off | Off | 1.489 | 1.581 |
| Off | On | 1.937 | 2.930 |
| On | Off | 1.655 (repeat 1.637) | 1.841 (repeat 1.843) |
| On | On | 1.970 | 3.090 |

Automatic GPU clocks changed: 1080p volume-On sampled 2700–2820MHz graphics /
9201MHz memory, versus 915–1605 / 5001MHz with volume Off; 1440p sampled 2850 /
9201MHz On and 1665–2205 / 9201MHz Off. These are actual observed workloads,
not clock-normalized kernel benchmarks or end-to-end FPS deltas. In particular,
subtracting whole-frame times understates the added work when clocks rise.
All measured windows reported zero new steady-state transient allocations.
CPU caster collection/terrain translation and acquire/present pacing are not
included in these GPU intervals. The 1440p drawable was an oversized window on
the 1080p desktop; actual 2560x1440 attachments were confirmed in status, but
this does not claim a full-monitor 1440p visual inspection. Raw data is retained
in `volume-perf1080/volume-timing.json` and `volume-perf1440/volume-timing.json`.

### Scope and limitations

This is a bounded sunlight source term, not a participating-media simulation or
replacement fog model. Clear weather gives weaker haze; dense fog gives stronger
shafts without changing how far geometry is visible. The cap deliberately keeps
the legacy atmosphere dominant. CSM caster LOD, alpha cutoff, coverage and depth
precision limit the result; remote occluders outside the existing shadow range
are not reconstructed. The tested profile's CSM reach was 250m even though the
march cap is 400m. The terrain patch is coarse and bounded, and its addition can
also improve terrain occlusion for existing surface CSM when both are enabled.

Thin foliage can shimmer with AA Off. Half-resolution depth rejection may lose
some light on subpixel geometry. Transparent surfaces use the opaque depth behind
them; there is no separate fog-depth or transparency volume. TAA/FSR uses surface
motion, not dedicated participating-media motion, so arbitrary fast motion and
all disocclusions are not guaranteed artifact-free. This milestone adds no clouds,
local-light volumes, multiple scattering, GI, RT requirement or new AA mode.

## First hardware water reflections (2026-10-12)

Built on public **eebe9bcb**, default **Off**. Startup still uses
`CWR_VK_RTAO=1`; reflections do not require RTAO or RT sunlight to be enabled.

```sqf
triRTReflections "on"
triRTReflections "off"
triRTReflections "status"
```

This is a Vulkan `rayQueryEXT` closest-hit implementation, not SSR. It uses the
existing capability checks, spatial collection, mesh buffers, alpha data, BLAS
cache and two frame-slot TLAS structures. All three effects share that scene.
Independent MRT blending is required. Missing capabilities/startup opt-in return
the reason without changing ordinary rendering. Optional resource failures
disable reflections, retain ordinary water and report the failure; retry requires
a restart. GL33 returns the Vulkan-required reply. No private NGX code is included.

### Real water surfaces and textured hits

Landscape water uses `SpecularTexture` with the legacy **Detail** shader family,
not the explicit Water family. A no-op-by-default engine draw marker identifies
native landscape water without changing that shader selection. Software water
uses the existing `CaptureTemporalTerrain(..., water)` boundary. Off retains the
original diffuse animation, secondary-texture sampling, lighting and composition.

With reflections selected, an FP16 world attachment records only rasterized
water's linear view depth, world-normal X/Z and fog visibility. Normals combine
the actual water triangle with the existing specular/bump texture and slow wave
UV translation; software shoreline UVs already contain their water animation.
Foreground surfaces invalidate this record; projected shadows retain it. Water
position reconstruction removes projection jitter and uses the real surface
depth, camera transform and projection, not a guessed screen-wide water plane.

One ray per qualifying water pixel travels at most **70 m** from a **4 cm**
surface offset (2 cm ray minimum). Receivers fade from 60 to **80 m**. The shared
160 m scene neighborhood covers that reach, independently of camera visibility.
Closest committed intersections provide instance/geometry/primitive IDs,
barycentric UVs and triangle positions. Real decoded RGBA textures are reused
through the existing alpha-data buffers, with point/bilinear sampling and wrap
flags; no arbitrary object colors or screen-color hit substitution is used.

- Rigid, undamaged visual-LOD0 buildings, piers, walls and eligible static objects
  reuse their existing mesh buffers/transforms. Real alpha tests preserve window,
  fence and static vegetation gaps. Animated/deforming/proxy/hidden objects,
  dynamic textures, terrain-fitted objects and genuine blended effects remain
  outside this shared scene.
- Nearby terrain reuses the cached, actual-height 16x16-cell patches introduced
  by RT sunlight. Reflection-enabled patches group triangles by their actual
  landscape texture and add world-grid UVs with existing terrain UV jitter.
  Texturing activation can create a new cached mesh/material subset once; stable
  frames do not rebuild it. No whole-island reconstruction or second AS system.
- Hit lighting is deliberately basic: decoded base color, geometric normal,
  engine sun/ambient colors, eye accommodation, night response and distance fog.
  It is **not** a full material renderer: local lights, emissive overrides,
  material specular, detail textures and secondary shadow rays are not evaluated.
  Clamped terrain transition textures and raster terrain LOD can differ from the
  bounded reflected height/UV representation.

Misses or untextured hits use a gradient derived from the existing weather sky
texture's horizon/zenith colors plus the engine sun-sky lobe and night response.
This is not a reflected cloud/stars/moon cubemap. Excluded geometry is absent from
the TLAS, so a ray may instead reach supported geometry behind it or the sky.
Far/underwater/unsupported receivers retain ordinary water.

Fresnel weight ranges from 0.025 to 0.425 before distance fading, retaining the
original water texture rather than replacing it with a perfect mirror. A small
3x3 depth-aware water-only filter blends into linear HDR world color before
TAA/FSR, bloom and tone mapping. Original direct sunlight/CSM and AO selections,
fog/atmosphere, shafts, overlays and HDR headroom remain in their established
pipeline. Reflection hit fog uses the engine fog color rather than a second
volumetric evaluation. Water is marked reactive for TAA/FSR; reflected geometry
does not pretend to have valid stationary-surface motion vectors. This avoids
long reflection history trails but leaves some fine-edge shimmer/aliasing.

`status` separates **completed water rays**, intersections and **successfully
texture-shaded hits**, with terrain and alpha-tested shaded-hit counts. Shared
live/cached BLAS, TLAS and instance/triangle counts follow. Execution booleans
are lifetime evidence, not capability initialization; last samples may precede
an Off toggle. A dispatch with no visible qualifying water has zero water rays.
`CWR_VK_PROFILE=1` reports separate shared BLAS/TLAS, fused trace+hit-shading,
filter/composition and complete GPU-frame intervals. Trace and hit shading are
one compute shader, not two separately timed passes.

### Coastal qualification and costs

RTX 4060 Ti 8 GB / driver 617.14, isolated stock GOG Malden Infantry gameplay,
900 m visibility. Useful shoreline views were `[3500,8,3230,1,-.1,0]` and
`[3520,8,3240,1,-.5,.2]` (engine X/up/Z, direction). Original water Off/On captures
show recognizable house walls/roofs, pier decking/posts, trees and beach terrain
in the water. Tilting down retains house/tree reflections while the source
geometry leaves the camera view. Actual foliage holes remain open. Miss-heavy
open-water views use the weather-colored sky. These are sampled observations,
not a promise of complete scene coverage or flicker-free edges.

The close view used **18 BLAS, two TLAS, 80 instances, 11,080 triangles**, including
nine terrain patches and 56 extended alpha subsets. A representative 1080p TAA
sample traced **1,606,710 rays**, with **235,969 textured hits** including
**161,118 terrain hits and 18,933 alpha-tested hits**. Reflection-only operation
also recorded real hits with both other RT selections Off. Native overlays were
unaffected; dry inland views executed zero water rays.

Noon, dawn, overcast/night, animated water with a 12-step moving camera,
inland/coast scene changes, TAA, FSR Quality/Native, MSAA2/4, SMAA, RTAO and RT
sunlight together, atmosphere/bloom/shafts, resizing to 1280x720 and
minimize/restore were exercised. Night did not retain daylight-colored
reflections. The main run closed normally after **25,902 submitted frames with
zero core/synchronization validation errors or warnings**. HDR requested the
existing SDR fallback: FP16 composition was exercised, **physical HDR10 output
was unavailable**. These are developer launches, not Steam-route qualification.

The final 2560x1440 validation run closed after **25,342 submitted/presented
frames with zero core/synchronization errors or warnings**. It repeated the AA
and combined-RT checks, then independently disabled RTAO/RT sunlight while
retaining reflections and exercised SSAO. A TAA sample recorded 2,857,659 water
rays and 419,614 textured hits (286,668 terrain, 33,818 alpha-tested). A downward
off-screen-source view retained recognizable roof/tree reflections.

An additional Everon Heavy Metal coastal run, built **without FSR**, exercised
reflection-only TAA followed by all three RT effects and MSAA4/overcast. At
`[5100,16,3780,0,-.35,1]` it recorded 847,257 water rays and 114,842 shaded hits,
predominantly the actual steep terrain bank (114,688); that bank legitimately
blocks most village reflections. It closed after **16,452 frames, zero core/sync
findings**. The no-opt-in Vulkan 1.0 menu smoke retained zero AS/rays and closed
after 803 frames with zero findings. GL33-only menu startup returned the
Vulkan-required reply and closed normally.

Performance uses the close view above, noon/clear, VSync off, validation off,
atmosphere/bloom/shafts off and CSM on. Each measurement warms three seconds and
collects eight seconds; Off/On baselines repeat in reverse order. Automatic GPU
clocks differ between modes, so subtracting whole-frame times is not a reliable
incremental reflection cost. Complete GPU frame means the submitted workload,
not CPU scene collection/recording or presentation latency.

| Output | Mode | Trace + hit shading ms | Filter + blend ms | Complete GPU frame ms |
| --- | --- | --- | --- | --- |
| 1920x1080 | Original water, AA/RT Off | n/a | n/a | 1.812 |
| 1920x1080 | Reflections only, AA Off | 0.536 | 0.277 | 2.039 |
| 1920x1080 | All RT + TAA | 0.363 | 0.142 | 2.226 |
| 1920x1080 | All RT + FSR Native | 0.358 | 0.137 | 2.694 |
| 1920x1080 | All RT + FSR Quality | 0.189 | 0.069 | 2.056 |
| 2560x1440 | Original water, AA/RT Off | n/a | n/a | 1.866 |
| 2560x1440 | Reflections only, AA Off | 0.660 | 0.311 | 2.148 |
| 2560x1440 | All RT + TAA | 0.637 | 0.301 | 3.628 |
| 2560x1440 | All RT + FSR Native | 0.614 | 0.307 | 4.798 |
| 2560x1440 | All RT + FSR Quality | 0.295 | 0.085 | 2.650 |

Initial 1080p shared construction of 18 BLAS/one TLAS measured **0.568/0.251 ms**;
the second frame slot builds its own TLAS. Warm stationary frames reuse both
levels (empty timestamp intervals approximately 0.0001 ms, not hidden rebuilds).
The 1440p initial build measured **0.364/0.164 ms**, with **0.172 ms** for the
other slot's TLAS. Different automatic clock states explain why AS time is not
resolution-monotonic. No AS builds occurred in the warm fixed-camera windows.

Focused tests: Vulkan **991 assertions / 75 cases**, GL33-only **987 / 74**,
driver-free Vulkan policy/lifetime **390 checks**. The CPU-only header test also
passes with PCH disabled and a rejecting Vulkan-header sentinel. The deliberate
policy teardown-error fixture is not an actual GPU validation finding.
Vulkan builds with/without FSR and the GL33-only build pass; reflection compute,
filter, MSAA variants and water MRT SPIR-V pass SDK validation.

## First hardware ray-query sunlight shadows (2026-10-12)

Built on public RTAO commit **c87d79b**, retaining the subsequent CPU-only
transform-header fix **555f6d6**. Default **Off**, independently of RTAO/SSAO.
Reuse the existing startup capability opt-in `CWR_VK_RTAO=1`, then:

```sqf
triRTShadows "on"
triRTShadows "off"
triRTShadows "status"
```

No new menu, persisted setting, SDK, NGX code or ray-tracing pipeline. The same
Vulkan 1.2 ray-query/AS capability checks and initialization serve both effects;
sunlight additionally requires independent attachment blending. Missing opt-in
or capabilities returns `UNSUPPORTED` with the reason without changing AO/CSM.
GL33 returns `UNSUPPORTED: RT shadows require Vulkan`. Ordinary non-opt-in Vulkan
keeps its existing device path. Public changes contain no private NVIDIA files.

### One scene, actual casters, bounded coverage

The existing RTAO collector, immutable mesh buffers, world transforms, alpha
data, BLAS cache and two frame-slot TLAS allocations are shared. There is no
second AS system or scene database. RT sunlight expands the existing spatial
bucket search to **160 m**, independent of screen visibility. Unchanged mesh
subsets reuse BLAS; unchanged instance lists reuse TLAS. RTAO-only retains its
80 m neighborhood. Additional sunlight-only geometry has a separate instance
mask, so AO does not suddenly treat terrain or foliage cards as new occluders.

- Buildings, rigid walls, signs, posts and eligible static object sections use
  their existing visual LOD0 vertices and indices. Building cutouts keep real
  windows/doorway alpha holes. Object animation, damage, proxies, surface fitting,
  hidden objects and genuinely alpha-blended effects remain excluded as casters.
- Static alpha-tested foliage/fence sections reuse the same UV/decoded texture
  alpha candidate test already used for building windows. Explicit cutout
  sections are eligible even when their texture contains intermediate alpha;
  they are **tested**, not promoted to opaque rectangles. Base-mip point/bilinear
  alpha and sampler wrapping/cutoff are preserved. Dynamic textures, wind/deforming
  meshes and animated objects are not represented by invented rigid geometry.
- Terrain uses bounded **16x16-cell patches** sampled from the landscape's actual
  subdivided `SurfaceY` height lattice and matching cell diagonal. Immutable
  native-layout buffers/BLAS are cached while the sampled heights are unchanged.
  Moving outside a patch neighborhood prunes that CPU cache; landscape/grid
  changes invalidate it. No whole-island heightfield is built. Collection reserves
  room for terrain (800 object/subset instances, 1,024 total instance safeguard).
  Rendering's distant/coarse terrain LOD can still differ from this height lattice.

`status` reports shared live/cached BLAS, TLAS and instance/triangle counts,
terrain patches, extended alpha instances, completed GPU ray/hit counts and
terrain/alpha hit counters. `rayQueriesExecuted` means **completed GPU rays**, not
extension initialization. It is a lifetime flag; last-completed samples may
precede an Off toggle. `lastWorldDispatch` distinguishes dispatch from capability
availability; a nighttime dispatch can finish with zero actual rays. The
`foliageInstances` field counts additional alpha-tested subsets, including fences,
not a botanical census. Off stops sunlight work but reusable AS allocations can
remain cached. AO and sunlight have separate execution counters and selections.

### Sunlight-only hybrid, not whole-frame darkening

An opt-in FP16 world attachment records only direct sunlight and existing CSM
visibility. Native material lighting separates directional diffuse, forced
diffuse and sun specular from ambient, emissive and engine-selected local lights.
Software terrain retains its actual CPU-calculated colorized sun contribution in
a small clipping-interpolated sidecar. Unsupported software lighting has no such
record and stays on the old shadow path. GL33 never requests this sidecar.

The record accounts for the existing legacy-to-linear lighting conversion,
night-eye response and fog transmittance. The already-added fog/atmosphere color
is not multiplied. The completed-world pass subtracts **only lost sunlight** in
linear HDR, preserves alpha, and precedes AO, TAA/FSR reconstruction, bloom and
tone mapping. Translucent/effect draws conservatively invalidate the record;
native overlays, sky, water and unclassified receivers do not receive the new
post-lighting correction. No clamp to SDR is introduced.

One deterministic hardware visibility ray is sent toward the engine's actual
sun direction for each qualifying world-resolution pixel within 50 m. The depth
reconstruction removes temporal projection jitter and estimates a surface normal
from nearer depth derivatives. The origin uses a 2.5 cm normal offset plus 1.5 cm
toward the sun, a 1 cm minimum distance and a **100 m** ray length. Coverage fades
from 40 to 50 m. Night/daylight weighting follows the engine; fully night scenes
execute zero rays. Full-resolution hard shadows require no stochastic denoiser
or extra spatial blur; existing TAA/FSR motion reconstruction stabilizes edges.
MSAA selects the sunlight sample matching the nearest depth sample.

The combined visibility is **min(CSM, RT)**, not a product. Thus two shadow
systems do not independently multiply/darken the sunlight. CSM retains distant
and unsupported animated-caster coverage, and RT can add missing nearby detail.
This conservative first hybrid **cannot remove an already over-broad CSM shadow**
or promise fully ray-traced silhouettes. With RT enabled CSM is also applied to
the separated sunlight for supported receivers, so ambient remains brighter
inside some old CSM shadows. This is intentional, not loss of the sun shadow.
At very low angles the existing CSM direction clamp can disagree with the actual
sun direction used by rays; retaining both can leave a broad/doubled silhouette.

### RTX 4060 Ti observations

Windows, RTX 4060 Ti 8 GB, driver 617.14, isolated stock GOG data/profile and muted
developer launches. Malden Infantry/Houdan comparisons used noon/clear weather,
900 m visibility and camera `[7116,73,6085,-1,-0.1,-0.2]` (engine X/up/Z).
The useful scene contained **49 BLAS, two TLAS, 262 instances, 28,955 instanced
triangles**, including **nine terrain patches and 160 extended alpha subsets**.
The native 1080p view traced about **1.190 million sunlight rays/frame**, with
**186,265 hits**, including about **114,245 alpha-subset hits**. Sloped terrain
views recorded actual terrain intersections (for example 1,982 hits near
`[6800,129.035,6100]`), rather than merely reporting allocated terrain BLAS.
RTAO retained approximately 36,273 hits in its original village comparison.

CSM-only versus hybrid captures show sharp sign silhouettes, tighter building
contacts and detailed leaf/fence gaps instead of solid-card shadows. Ground views
looking away from the house retain shadows from off-screen geometry. The fixed
ray is free of stochastic noise; hard silhouette aliasing without AA and residual
coarse CSM edges remain visible. No gross acne, detached contact band or broad
opaque foliage-card shadow was observed in sampled views. This is not a claim of
complete caster coverage or flicker-free rendering in all content.

The comparisons include CSM disabled with RT enabled; this is **local RT plus any
remaining legacy projected-shadow fallback**, not a falsely labelled fully RT
renderer. No reliable shadow coverage was disabled in the production hybrid.
Noon, low sunlight, overcast/night commands, a 20-step moving-camera sequence,
eight terrain neighborhoods, actual infantry movement and Jeep driver-camera
movement were exercised. Night Off/hybrid captures matched with zero traced
rays. TAA, FSR Native/Quality, MSAA2/4, SMAA, RTAO and SSAO, atmosphere, shafts,
bloom, resize to 1280x720/1600x900 and minimize/restore were exercised. HDR10
requested the existing SDR fallback: internal FP16 lighting was tested, but
**physical HDR output was not available**.

The main validation session closed normally after **30,634 submitted frames**,
with **zero core/synchronization validation errors and warnings**. Earlier
qualification closed after 16,699 frames with the same zero findings. Preexisting
loader messages about missing external overlay manifests are not counted as
engine validation success/failure. The final Everon Heavy Metal run traversed
four neighborhoods, including the forest at X/Z `[6300,5500]`: **18 BLAS, 127
instances, 64,760 triangles, nine terrain patches and 76 alpha subsets** in that
view. About 501,173 hits from 1.271 million rays included 486,737 alpha-subset
hits; the forest floor showed resolved foliage gaps/contact shadows. It repeated
MSAA4 and FSR Quality with RTAO plus actual overcast skies, then closed after
**6,455 frames with zero core/sync findings**. These are sampled visual checks,
not a fixed-clock forest benchmark.

Vulkan-without-FSR executed both effects with TAA and closed after **1,599 frames**
with zero validation findings. The ordinary no-opt-in Vulkan 1.0 run rejected RT
enablement, kept zero AS/rays, and closed after **3,277 frames**, also zero
findings. GL33-only menu startup returned the Vulkan-required reply and closed
normally. All smoke sessions use the isolated developer profile, not Steam-route
qualification.

### GPU measurements

Validation disabled, fixed Houdan view above, VSync off, AO/AA off for the first
three modes, atmosphere/shafts/bloom disabled. Each window warms three seconds,
then collects eight seconds; baseline modes repeat in reverse order. GPU clocks
are automatic, so whole-frame differences are not fixed-clock incremental cost.
Ray/composition and shared AS build costs are directly timestamped, not inferred
from FPS. Complete-frame values cover the entire submitted GPU command workload,
not CPU collection/recording, presentation or display pacing.

| Output | Mode | Sun ray ms | Sun composition ms | Complete GPU frame ms |
| --- | --- | --- | --- | --- |
| 1920x1080 | CSM only | n/a | n/a | 1.677 |
| 1920x1080 | Local RT + legacy fallback | 0.464 | 0.083 | 1.833 |
| 1920x1080 | Hybrid | 0.352 | 0.060 | 1.869 |
| 1920x1080 | Hybrid + RTAO + TAA | 0.295 | 0.042 | 2.091 |
| 1920x1080 | Hybrid + RTAO + FSR Native | 0.293 | 0.038 | 2.533 |
| 1920x1080 | Hybrid + RTAO + FSR Quality (1280x720 input) | 0.154 | 0.025 | 1.969 |
| 2560x1440 | CSM only | n/a | n/a | 1.729 |
| 2560x1440 | Local RT + legacy fallback | 0.515 | 0.085 | 1.917 |
| 2560x1440 | Hybrid | 0.496 | 0.075 | 2.103 |
| 2560x1440 | Hybrid + RTAO + TAA | 0.497 | 0.074 | 3.077 |
| 2560x1440 | Hybrid + RTAO + FSR Native | 0.498 | 0.073 | 4.302 |
| 2560x1440 | Hybrid + RTAO + FSR Quality (1706x960 input) | 0.238 | 0.030 | 2.543 |

The 1080p cold build of 49 BLAS cost **0.545 ms GPU**; initial TLAS builds cost
**0.215 / 0.231 ms** for the two frame slots. Frozen scenes executed no further
build commands (empty intervals below 0.001 ms). Changed membership/transforms
rebuild the shared slot TLAS; there is no deforming geometry refit. Performance
evidence is kept separately from validation runs.
The 1440p cold BLAS batch cost **0.513 ms**, with **0.157 / 0.165 ms** initial
TLAS builds. Later frozen scenes again had zero builds. Observed graphics clocks
ranged from 1,020 to 2,850 MHz at 1080p and 1,620 to 2,850 MHz at 1440p; this
explains why resolution and whole-frame timings do not scale proportionally.
The "composition" column is a fullscreen sunlight subtraction, not a hidden
multi-ray filter. RTAO costs are additional in the AO-enabled rows and included
in their complete frame values.

### Remaining limits and evidence

The fixed 50 m receiver / 100 m ray reach cannot replace island-scale CSM. Dense
areas can hit the instance safeguard, with CSM still providing fallback. Proxy
doors, destructed geometry, infantry/vehicles and animated vegetation are not RT
casters. They retain existing raster shadows; moving cameras do not imply those
objects have BLAS. Alpha is sampled at the decoded base mip rather than a full
ray-differential material system. Depth normals and centimetre offsets can miss
thin contacts, and actual terrain samples can disagree with coarse raster LOD.
Transparent foregrounds deliberately suppress the correction instead of
shadowing the surface behind them through an unreliable depth match.

Local raw images, actions, launch hashes, validation logs and timestamp samples
are under ignored `build/shadow-live/rtshadow-*` in the developer checkout. No
proprietary game data, captures or external SDK binaries are committed.
The `rtshadow-perf1080` and `rtshadow-perf1440` directories contain
`rtshadow-perf.json` with individual samples and clock readings. Their measured
executable SHA256 values respectively are
`54E37B03BA93D34DAEBAE6DD70773F428FD7495F7A2F98B153C9A481B91E317F` and
`F34407E433613BEDC584FCCFC2B580183F736871181FD3E163EBEB0A7E0ED8B3`.
Between those builds only formatting, status wording, preserving the shared
scene on AO-disable, and additional assertions changed; GPU shaders are identical.

Vulkan+FSR, Vulkan without FSR and GL33-only RelWithDebInfo builds pass. The focused filter
`[vulkan-shape],[vulkan],[vulkan-aa],[ShadowMath],[render-flags],[render-descriptor]`
passes **989 assertions / 75 cases** with Vulkan and **985 / 74** without Vulkan.
Policy/lifetime checks pass **382 guards**, including sunlight default-Off,
independence, rejected unsupported enablement and no false ray-execution status.
The isolated no-PCH GL33 compiler check rejects a deliberately forced Vulkan
include, then successfully compiles the CPU shape tests with that same rejecting
header sentinel in place. All four AO/sun single/MSAA ray-query SPIR-V variants
validate for Vulkan 1.2. No unsupported physical GPU was available; rejection
tests do not constitute hardware qualification on non-RTX devices.

## First hardware ray-query ambient occlusion (2026-10-11)

The following records the original AO milestone. Its AO geometry mask remains
unchanged; the sunlight section above describes the later shared-scene extension.

Based on public `main` **2bd25ff**. RTAO defaults **Off**. Opt into the Vulkan
device capabilities at process startup with `CWR_VK_RTAO=1`, then use:

```sqf
triRTAO "on"
triRTAO "off"
triRTAO "status"
```

There is no new menu or persistent setting. The environment variable enables
capabilities, not AO. Without it the ordinary Vulkan 1.0 device path remains.
GL33 returns `UNSUPPORTED: RTAO requires Vulkan`. A missing opt-in, extension,
feature, queue capability, AS limit or entry point returns a reason; failed
enablement leaves SSAO and ordinary rendering intact. RTAO can be built and run
without FSR. With FSR enabled, its Vulkan 1.2 feature chain is merged without
duplicating buffer-device-address feature structures.

### Actual geometry and tracing

- Uses `VK_KHR_ray_query`, `VK_KHR_acceleration_structure` and their deferred
  host operations dependency, Vulkan 1.2 buffer device addresses, `shaderInt64`
  for building material addresses, and a compute-capable graphics queue. See the
  [Khronos ray-query requirements](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_ray_query.html)
  and [AS requirements](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_acceleration_structure.html).
  No ray-tracing pipeline, SBT, NVIDIA SDK, NGX or vendor-specific dispatch is used.
- Queries the existing landscape object buckets within 80 m of the camera,
  including off-screen objects. Bounds intersecting that neighborhood are eligible.
  Collection is independent of raster visibility and the CSM enable switch. It
  does not enumerate or build the entire island, or create another world database.
- Static, undamaged, non-deforming visual LOD0 geometry supplies the BLAS.
  Native immutable `VertexBufferVK` position/UV/index allocations are reused;
  eligible off-screen shapes can create the same native buffer through the
  existing shape API. Model-to-world transforms come directly from the objects.
  BLAS cache keys include allocation identity and the actual included material
  ranges, including distinct subsets of a shared mesh. Shared ownership prevents
  address reuse and premature destruction. Unused cache entries age out after
  120 active AO world frames; referenced frame-slot structures retain them safely.
- Each of the two frame slots retains its own TLAS, instance upload and scratch
  resources. An identical instance list reuses its TLAS; changed membership,
  transforms or BLAS references rebuild that slot after its fence. This milestone
  has no deforming BLAS updates and no per-frame rebuild of unchanged meshes.
  Collection caps at 1,024 instances; each BLAS is limited to 1,024 section
  geometries and one million triangles. These are safeguards, not coverage targets.
- Opaque sections cast normally. Stock building walls often use cutout textures
  for windows/doors: those triangles produce non-opaque ray-query candidates,
  interpolate their **existing** mesh UVs and test the decoded texture's real alpha
  before confirming an intersection. Base-mip alpha, clamp/repeat, point/bilinear
  sampling and the existing cutoff are used. Cached alpha bytes are owned by
  the existing texture allocation. Transparent holes are not filled with walls.
  This limited building-only hit test is not a replacement material system.
- Foliage cards, blended/dynamic/animated textures, hidden/proxy placeholders,
  animated or damaged objects, surface-fitted meshes, roads/surface decals,
  water and transparent effects are excluded as occluders. Opaque tree trunks,
  posts and similar rigid sections can participate. There are no invented hulls.
  **Terrain receives AO but is not in the TLAS.** Its software-transformed,
  clipped render vertices are transient; this milestone does not reconstruct an
  island heightfield or claim terrain-to-wall occlusion. Proxy doors and damaged
  buildings also need later coverage work.

The half-resolution compute pass reconstructs view positions from world depth,
removes temporal projection jitter, estimates a normal from the nearer depth
derivative on each axis and transforms into the same world space as the TLAS.
Four cosine-weighted hemisphere rays query actual triangles over a 2 m radius,
with a 3.5 cm surface offset. Distance-weighted occlusion fades out by 60 m radial
camera distance, inside the collection margin. Integer world-cell hashing keeps
the sample rotation independent of frame number and avoids large-island-coordinate
precision loss in floating-point random hashes. A 5x5 depth-aware half-resolution
filter upsamples/modulates the linear HDR world color, preserving alpha.

RTAO **replaces the SSAO pass** while enabled. The SSAO shader, settings and path
are unchanged, and disabling RTAO restores the previous SSAO preference. Empty
coverage leaves world lighting untouched; it does not substitute hidden SSAO.
The pass runs at the existing completed-world boundary before TAA/FSR, AA
composition, bloom and tone mapping. Existing world atmosphere/fog/CSM rendering
and native-resolution cockpit, weapon, HUD and UI boundaries remain in place.
This is post-lighting modulation, not separately computed ambient irradiance:
already-composited world transparency/fog can inherit a small modulation from
the opaque depth beneath them. It does not separate direct and indirect light.

### Status and lifetime evidence

`status` reports live BLAS (including entries retained by TLAS frame slots), cache
entries, actual TLAS allocations, latest scene instance/instanced-triangle counts,
completed passes/builds, GPU-written ray/hit counts, and whether the latest world
submitted the AO dispatch. `rayQueriesExecuted` is a lifetime completed-ray
indicator; `lastCompletedRays`/`Hits` refer to the last retired pass and may precede
an empty scene or an Off toggle. They are read only after the frame fence, so
initialization alone cannot report tracing. `lastWorldDispatch=no` distinguishes
a skipped world pass. Off stops AO work but keeps existing cached allocations for
reuse. Device-idle shutdown destroys TLAS before its referenced BLAS/buffers;
resize recreates descriptors/images/filter pipelines without throwing away BLAS.

Set `CWR_VK_PROFILE=1` for separate GPU timestamps around BLAS building, TLAS
building, ray dispatch and filtering. Full-frame GPU timestamps remain the
existing complete command-buffer workload measurement. Status labels unavailable
timing explicitly; counts do not require profiling. CPU recording, acquisition,
presentation and display pacing are not included in these GPU costs.

### RTX 4060 Ti gameplay qualification

Windows, RTX 4060 Ti 8 GB, driver 617.14, isolated stock GOG data/profile, muted
direct developer launches (not Steam-route qualification). The stock Malden
Infantry mission was relocated to Houdan. Comparisons used fixed noon/clear
weather and camera `[7116,73,6085,-1,-0.1,-0.2]` in raw engine X/up/Z order.
The useful traced set was **18 cached BLAS, 67 instances and 12,818 instanced
triangles**, with two TLAS allocations for the frame slots. It included opaque
sections and cutout walls of `hruzdum`, `dum_rasovna`, `cihlovej_dum_mini`,
`deutshe_mini`, opaque shed geometry, rigid fence/post/sign sections and selected
opaque trunks. The native 1080p view executed about **1.987 million rays/frame**
and recorded about **36,273 hits/frame**; initialization had zero rays and zero AS.

Frozen Off/SSAO/RTAO captures show a ground contact band beside the house,
window/door recess shading and post contacts. SSAO adds screen-depth edge/contact
contrast, including foliage silhouettes absent from the TLAS. RTAO follows real
nearby solid surfaces and preserves alpha openings; it is not uniformly stronger.
The camera `[7111,72.5,6082,1,-0.7,0]` looks away from the building. RTAO still
shades the ground at the right side from the off-screen wall, where SSAO lacks
that wall's screen-depth samples. Four-ray grain is visible on close ground;
TAA reduces it. Bias, half-resolution normals/filtering and fixed visual LOD0 can
miss very thin contacts, soften edges or disagree with a distant raster LOD.
No broad solid foliage-card shadows or gross trails were observed in the sampled
motion sequence, but this is not a claim of flicker-free rendering in all content.

A 20-view moving-camera sequence, actual player movement/mouse look, map opening
and return, empty-area/coast/village transitions, and resize to 1280x720 with
minimize/restore were exercised. Empty coverage submitted no AO dispatch; returning
to the village rebuilt evicted geometry. TAA, FSR Native/Quality/Performance,
MSAA2/4, SMAA, CSM, atmosphere, bloom and fog were exercised together with RTAO.
The main qualification run closed after **29,781 submitted frames with zero
core/synchronization validation errors or warnings**. The final executable rerun
closed after **23,728 submitted frames, also zero errors and warnings**, repeating
the AO comparisons, AA mode changes, off-screen view, scene transitions and resize.
Separate no-FSR ray-query
execution closed after 4,346 frames, also zero findings. The ordinary no-opt-in
Vulkan run logged API **1.0**, rejected RTAO enablement and closed after 4,493 frames
with zero findings. GL33-only build/menu startup and its unsupported reply passed.
HDR10 negotiation returned the existing SDR fallback because the OS/surface did
not expose HDR10. Internal linear FP16 lighting, bloom and SDR tone mapping were
verified; **physical HDR output was not verified**.

Vulkan+FSR, Vulkan without FSR and GL33-only RelWithDebInfo builds pass. Focused
Vulkan rendering/shape/shadow tests pass **845 assertions / 71 cases**, including
ray-instance transform and depth-reconstruction agreement with raster geometry;
the GL33-focused subset passes **467 assertions / 48 cases**. Standalone Vulkan
policy/lifetime checks pass **376 guards**, including default-Off, unsupported
enablement preserving SSAO, invalid controls and no false ray-execution status.
Ray-query SPIR-V validates for Vulkan 1.2. No unsupported physical GPU was
available; missing-opt-in/GL33 rejection and feature checks are the fallback
evidence, not a claim of broad hardware qualification.

### Measured GPU workload

Validation disabled, frozen Houdan view, 1920x1080 output, 900 m visibility,
existing projected shadows enabled, CSM/atmosphere/shafts/bloom disabled, VSync
off. Each sample warms for 3 seconds then collects 8 seconds of timestamps.
The first three modes were repeated in reverse order; table values average those
two windows. GPU clocks were automatic (roughly 750 MHz Off, 885-900 MHz SSAO,
1,215-1,230 MHz RTAO, 1,530 MHz TAA/RTAO and 1,635 MHz FSR/RTAO).

| World mode | BLAS/TLAS steady-state builds | Ray query ms | Filter ms | SSAO ms | Complete GPU frame ms |
| --- | --- | --- | --- | --- | --- |
| Off | none | n/a | n/a | n/a | 1.385 |
| SSAO | none | n/a | n/a | 0.303 | 1.481 |
| RTAO, AA Off | none (cached) | 0.343 | 0.374 | skipped | 1.642 |
| RTAO + TAA | none (cached) | 0.276 | 0.299 | skipped | 1.740 |
| RTAO + FSR Quality (1280x720 input) | none (cached) | 0.131 | 0.131 | skipped | 1.688 |

The initial 18 BLAS builds cost **0.552 ms GPU**; the initial TLAS build cost
**0.160 ms GPU**. The second frame slot also builds a TLAS once. Unchanged scenes
execute no build commands (empty timestamp intervals were below 0.001 ms).
Changed instances use a small TLAS rebuild, not a deformation update/refit.
Automatic clock changes mean differences between whole-frame rows are not an
isolated fixed-clock AO overhead or an FPS improvement. Ray/filter times are
measured directly, not inferred from FPS. The cached path and cold build are
reported separately; CPU collection/driver recording costs are not quantified
by these GPU timestamps.

Local evidence is under ignored `build/shadow-live/rtao-{first,census,census2,
alpha,quality,perf-final,nofsr,default,gl33,final}` in the original developer
checkout. `rtao-perf-final/rtao-perf.json` retains samples and clock readings;
its measured executable SHA256 is
`172C91A742BF8FE74A058639DA74320B3E5410B15923456C6FD47916E1E65948`.
The final qualification executable is
`7BDD2592E666688427DA4BC3DC6582835B0C729FC1D575846BE5A1C293B39E18`;
subsequent changes from the measurement build concern capability/lifetime/cache
guards and formatting, not the measured ray/filter shaders. The original dirty
checkout and private NGX files were preserved; this public change was developed
in an isolated checkout from `2bd25ff`. Only original GPL-compatible engine and
shader changes are distributed. No new vendor code or stock game assets are added.

## Opt-in atmospheric perspective and sunlight shafts (2026-10-11)

Based on public `main` **e340399**. Both effects default **Off**, independently,
with process-local developer controls only:

```sqf
triAtmosphere "on"
triAtmosphere "off"
triAtmosphere "status"
triSunShafts "on"
triSunShafts "off"
triSunShafts "status"
```

Other arguments are rejected without changing state. Shaft status distinguishes
requested enablement from allocated resources; `visibility` is the CPU terrain
visibility, not a GPU readback of the visible sun aperture. Active resources do
not mean shafts are contributing at night or with a covered/off-screen sun.
GL33 returns `UNSUPPORTED` and its renderer is unchanged. No NGX/private files,
stock assets, weather scripts or graphics menus are changed.

### Existing atmosphere and the analytical change

Inspection covered `Scene::ResetFog`, `FogExponential`/`Fog8`, sky texture color
lookup, `MainLightChanged`, `LightSun`, `SkyThrough`, distance culling, native
shape fog and software-transformed terrain, and the flare visibility path.
The stock sky texture and eye-adjusted sky lighting still establish the fog
color. Mission/weather fog ranges, far clipping, tactical visibility, LODs and
both native and software fog **transmittance are unchanged**.

Atmosphere changes the in-scattered RGB in that same fog mix. A broad cool
air component and a restrained forward component derived from the actual sun
color/direction replace part of its uniform color. The existing distance fog
factor supplies distance dependence. Camera altitude and upward ray height
reduce the contribution; daylight, sky transmission and short weather-limited
visibility fade it away. Full fog returns to exactly the original fog color,
keeping the horizon/culling boundary matched to the sky. There is no second fog
pass or added extinction. Native objects and software terrain share this color
calculation; alpha-fog particles keep their existing alpha fade. NVG disables
the enhancement. Clear/sunset views gain color separation without making hills
disappear earlier; dense fog and night retain the game's original response.

### Screen-space shafts and composition

The existing sun direction and camera projection locate the source. The existing
flare terrain/horizon intersection supplies terrain visibility. Object coverage
uses sampled world depth: the coarse CPU occlusion sphere treats tree bounds
as solid and cannot represent leaf holes. Flare rendering itself is unchanged.

Two quarter-width/quarter-height FP16 targets hold an area-filtered sky-opening
mask and its 48-sample radial integration. A filtered sun aperture shuts down
scattering behind covered discs. Five-tap spatial reconstruction plus current
world-depth coverage composites the result into a separate FP16 world image.
MSAA depth coverage is averaged over its samples. Sun elevation, weather,
visibility and an 8%-of-screen border fade bound the contribution. A short
120 ms scalar terrain-visibility fade avoids a hard terrain transition; there
is **no image history, stochastic sampling, volumetric grid or extra geometry**.

The conservative receiver rule only adds scattering to visible sky openings.
Solid walls, terrain and foreground objects receive no beam overlay. This
produces modest soft sunlight around foliage/building silhouettes, not dramatic
shafts across dark ground or inside rooms. It deliberately avoids the common
screen-space artifact of painting rays over unrelated foreground geometry.
Fully covered sun apertures contribute nothing even if the CPU terrain test
says visible. Thin leaves/edges and partial sun coverage remain approximations.
Clouds and transparent smoke without depth writes have no separate occlusion
mask; cloud suppression uses the existing weather transmission. Off-screen
sources contribute nothing. Arbitrary mods and unusual cropped world views are
not qualified.

Atmosphere is part of world shading before AA. Shafts execute **after** world
AA/TAA/FSR reconstruction and SSAO, **before** bloom and SDR/HDR10 tone mapping.
They never feed TAA/FSR history or reactive masks. Native TAA's current jittered
lattice and FSR's unjittered reconstruction use their respective depth alignment.
Cockpit/weapon geometry, HUD, maps and menus remain downstream at native
resolution. Water, CSM and projected shadows retain their existing paths.

Resources follow acquired-image ownership and the existing idle swapchain/AA
recreation, including partial-creation cleanup. Off allocates no shaft targets.
On uses two quarter-resolution targets and one world reconstruction-sized color
target per acquired image (about 17.8 MiB at native 1080p or 31.6 MiB at 1440p).
No new steady-frame allocation or wait is introduced. The existing profiler
reports `Vulkan SunShafts: gpu_ms=...` for all three passes, using fence-retired
timestamps. Fog's extra arithmetic is in the existing world shader, so its cost
is assessed with paired whole-frame GPU intervals.

### Verification and limits

Windows RelWithDebInfo Vulkan+FSR and Vulkan-disabled builds pass. The driver-free
policy/lifetime tests pass **370 checks**, including defaults, independent
toggles, invalid arguments and requested/active semantics. Their intentional
mock teardown error is expected. Focused shape/fog/layout, shadow, conventions
and AA tests pass **841 assertions in 70 cases in each build**. The two new
std140 vectors have explicit size/offset assertions. Previously known unrelated
PAA/DDS failures remain outside this milestone; no all-green full-suite claim.
The Vulkan-disabled executable also started its stock menu, returned
`UNSUPPORTED` for both controls and status queries, and closed normally.

Stock Malden Infantry/Ambush and Ground Attack assets were used with isolated
profiles. Assisted placement, pinned comparison cameras and time/weather changes
provided matched SDR views. Sunset hills show warmer forward haze and cooler
off-axis distances; clear views retain hill detail and the stock horizon.
Canopy Off/On captures show a restrained warm lift through/around openings,
with dark tree and building silhouettes preserved. Overcast/heavy-fog/night
comparisons check suppression rather than forced universal haze.

Live infantry movement, mouse look, native weapon/HUD, NVG, map/pause and
resize/minimize/restore were exercised. All nine AA choices (Off, FXAA, SMAA,
MSAA 2/4/8, TAA, FSR Native/Quality) ran with atmosphere, shafts, bloom, SSAO
and CSM. A stock Cobra was entered with developer assistance and flown using
bounded controls to about 43 m altitude and 71 km/h during the recorded check,
with cockpit and external views. These are rendering checks, not mission wins.
The initial two infantry runs and successful flight run exited normally with
14,516, 35,218 and 6,332 submissions and **zero core/synchronization validation
errors or warnings**. The infantry recreation run had one out-of-date presentation.
An incorrect mission-path launch and an unsupported diagnostic script expression
were corrected and are not counted as successful gameplay runs. An initial
below-terrain coast camera and a misplaced scripted cockpit comparison are not
valid image evidence.

The final formatted build repeated all nine AA modes in the town/canopy scene,
compared shafts Off/On in the northern Malden forest, and checked a correctly
placed coastal camera with visible water and distant land. Atmosphere also ran
with CSM and SSAO disabled. This run closed normally with **30,741 submissions
and presentations, zero validation errors and zero warnings**. The final Windows
Vulkan+FSR executable SHA-256 is
`39c78bba06c4ce40502e00b10a07bb97ca53d5f43698e10a79868c3270c3be5c`.

### GPU measurements

RTX 4060 Ti, Windows driver 617.14, validation disabled, SDR, TAA at native
resolution with bloom, SSAO and CSM enabled. A pinned Malden town/canopy view at
17:00, clear weather, no mission fog and 1,500 m view distance was used for both
resolutions. Eight configurations in mirrored order tested neither effect,
atmosphere only, shafts only and both, twice each. Each segment had 4 s warm-up
and 10 s measurement, using fence-retired GPU timestamps and 2 s report intervals.
The table gives medians across the two segments' interval averages, in ms:

| Output | Neither: frame | Atmosphere: frame | Shafts: frame | Both: frame | Shafts passes | Both vs neither |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1920 x 1080 | 2.022 | 2.010 | 2.048 | 2.113 | 0.210 | +0.091 |
| 2560 x 1440 | 2.178 | 2.186 | 2.358 | 2.373 | 0.265 | +0.195 |

Atmosphere-only differences (-0.011 / +0.008 ms) are within measurement noise,
not evidence of a speedup or zero cost. Shafts timings cover mask, radial and
composite passes, not the entire toggle's resource/AA-path changes. GPU clocks
were not locked: at 1080p the baseline used roughly 1,485-1,590 MHz core /
5,001 MHz memory; shafts increased these to roughly 1,605-1,740 / 9,201 MHz.
Thus whole-frame differences are observed dynamic-clock results, not additive
fixed-clock penalties or a portable FPS guarantee. Both timing runs closed
normally (18,219 / 22,576 submissions).

### G14 HDR signal check

The G14/RADV + Gamescope/Proton signal-path check ran the final shader code with
TAA, FSR Native/Quality, bloom, SSAO and CSM, toggled SDR/HDR and exited normally
(2,813 submissions). The application negotiated format 64 / color space
1000104008, submitted HDR metadata, and HDMI-A-1 reported BT2020_RGB and PQ
EOTF 2. This was not a validation-layer run or a native Linux/Steam qualification.
**Physical TV brightness, color and appearance remain unverified.** No remote
capture is presented as physical display evidence.

Ignored local receipts, captures and timing data are in the original game
workspace's `build/shadow-live/atmosphere-*`; build/test and G14 receipts are in
the isolated public worktree's `build/`. Private work in the original checkout
was preserved.

## Opt-in HDR bloom (2026-10-11)

Based on public main `dd773be`. Bloom defaults **Off**, is session-scoped and
Vulkan-only. GL33 remains the default renderer. No private NGX files, new
lens flares, exposure adaptation or graphics-menu changes are included.

```sqf
triBloom "on"
triBloom "off"
triBloom "status"
triBloom "threshold 1.25"
triBloom "strength 0.15"
```

Threshold (1..32 linear reference-white units) and strength (0..0.3) are
optional developer tuning. Defaults are 1.25 and 0.15. A 25% soft knee begins
at 0.9375 and selects peak RGB, retaining saturated orange/red sources without
changing their hue. This is deliberately restrained: a reference-white texel
only contributes a tiny knee value, not its entire color. There is no global
brightness multiplier or invented emission. Status distinguishes requested
from active resources and reports level count, extent and composition domain.

### Implementation and source-light corrections

The completed linear FP16 world feeds four reduced-resolution levels, starting
at half the native drawable size, down to 1/16. Extraction and normalized
13-tap downsampling share the first pass; subsequent levels downsample the
extracted energy. Three normalized tent upsampling passes mix neighboring
octaves (65% coarse contribution). Separate down/up targets avoid feedback.
The seven passes process about 0.66 native-frame pixels in total; there is no
full-resolution Gaussian blur. No upper RGB clamp or LDR intermediate is used.
The existing final world composition adds one bloom sample, scaled by strength,
**before** the unchanged SDR/HDR tone mapper. Alpha, scene depth and stencil
are not blurred. Gamma, reference white, BT.2020/PQ and metadata are unchanged.

MSAA resolve, FXAA, SMAA, TAA and FSR finish before extraction. FXAA/SMAA at
100% use an intermediate only when bloom is enabled; Off retains their fused
AA/composition path. Supersampling still performs the existing area resolve.
FSR's HDR flag, unit pre-exposure/null exposure and FP16 reconstruction contract
are unchanged; bloom reads reconstructed output, never feeds its history or
reactive input back into FSR/TAA. SSAO, CSM, projected shadows, sky/fog and water
remain upstream. Native cockpit/weapon geometry, instruments, HUD, menus and
text remain downstream and do not seed bloom. First-person weapon-only flashes
therefore do not bloom; their world lighting can still contribute.

Inspection found a remaining float-to-packed-color loss in emissive billboard
submission. `DrawLightDecal` retains computed RGB for Vulkan while keeping the
original packed alpha/fog and GL33 draw. Existing primary light-source sprites
(`Flare0`, including headlights/fire lights) previously drew after world tone
mapping and were treated like UI. Vulkan now draws these sources before world
AA/composition, preserving their calculated RGB. Their authored texture, size,
visibility and alpha remain unchanged. Secondary lens-flare ghosts keep the
old downstream path; no new flare effect is added. Native muzzle materials,
sun lighting, explosion/local-light equations and headlight attenuation already
retain floating-point sums and were not boosted. Packed particle colors and
intensity baked into legacy textures still limit available energy.
The source-color/order corrections apply to Vulkan even with bloom Off; Off
disables the pyramid, not those HDR correctness fixes.

Resources use the existing acquired-image ownership and idle swapchain/AA
recreation. Off allocates no bloom images or descriptors. On/off, AA/output
changes and resize recreate/destroy all seven images per swapchain image, pool,
framebuffers and pipeline together. No temporal bloom history is retained.
The existing GPU profiler also reports `Vulkan Bloom: pyramid_gpu_ms=...` using
two additional timestamps read after the normal frame fence. This timing covers
the pyramid, not the extra sample in the existing final composition.

### Verification and cost

Windows RelWithDebInfo Vulkan+FSR and Vulkan-disabled GL33 builds succeeded.
The GL33-only executable also opened/closed its menu normally and returned
`UNSUPPORTED` for `triBloom "on"`.
Driver-free policy/lifetime/command tests pass 359 checks, including bloom
argument validation, requested/active semantics and seven closed reduced-size
passes. Focused shape/shadow/convention tests pass 420 assertions in 31 cases
in both builds. A broader `[Graphics]` run is **not all-green**: both builds have
13 failures in untouched PAA/DDS tests (12 PAA writes use `/tmp`, plus a DDS
cache-size assertion); this is not claimed as a passing full suite.

Stock Infantry daylight buildings/foliage/sky/sun and stock Shadow Killer night
were exercised with isolated profiles. Frozen SDR Off/On captures show mild,
localized light-source softening, not a cinematic halo: the tested headlight
difference was only one 8-bit code value around the lamp cores; sun differences
were also small. Fire tuning comparisons retained orange color and unchanged
text/background outside the source. Ordinary surfaces remain recognizable.
Real M16/HK shots consumed ammunition, but brief muzzle captures were not
reliable enough to claim a matched flash improvement. Thrown hand grenades
were observed exploding with FSR Quality; a frozen Off/On explosion comparison
also showed only a small (one code value) change around the bright particles.
Coastal water/fog with FSR Native, NVG, cockpit instruments and pause-menu text
were checked. These are practical staged stock-game checks, not an exhaustive
mission playthrough or temporal-image-quality certification.
The legacy headlight cone geometry remains visibly coarse with either setting;
bloom does not repair or replace it. Settings were not lowered further to make
the daytime sky or white buildings glow. Legacy sources near reference white
can produce little or no visible bloom; this is preferable to inventing energy.

All nine AA selections (Off, FXAA, SMAA, MSAA 2/4/8, TAA, FSR Native/Quality),
150% SMAA, SSAO/CSM, bloom toggles and resize/minimize/restore completed under
Windows synchronization validation with zero errors/warnings. Captures and
profiler receipts are local ignored evidence in `build/shadow-live/bloom-*`.
The final effects session completed 54,351 frames and a normal close with zero
validation errors/warnings. Two earlier sessions ended on invalid test-script
commands (`triSetSSAO` and `moveOut`), not Vulkan failures; both are retained in
the evidence rather than counted as successful normal-close tests.

RTX 4060 Ti, frozen Infantry street, gamma 1 / legacy brightness 1.6, TAA at
100%, SSAO/CSM enabled, VSync off, validation off: four alternating Off/On
windows per resolution, five seconds settling then six two-second reports per
window. 1440p used an oversized drawable on the 1080p desktop.

| Drawable | Off whole GPU frame | On whole GPU frame | Raw On minus Off | Bloom pyramid |
|---|---:|---:|---:|---:|
| 1920x1080 | 1.718 ms | 1.705 ms | -0.013 ms (clock noise) | 0.128 ms |
| 2560x1440 | 1.906 ms | 1.936 ms | +0.030 ms (clock-dependent) | 0.151 ms |

These are not fixed-clock incremental whole-frame costs: enabling bloom raised
GPU clocks (1080p roughly 1.5 to 1.6 GHz, memory also changed; 1440p roughly
2.35 to 2.67 GHz). The negative number is **not a speedup**. The timestamped
pyramid is the meaningful directly measured added work; the composition sample
and extra FXAA/SMAA intermediate are outside that isolated measurement. No GPU
power/clock settings were changed. Off has no pyramid work.

G14/Nobara + Gamescope/Proton 11 + attached 4K TV: bloom with TAA, FSR Native
and Quality, SSAO/CSM and SDR/HDR toggling completed. The application negotiated
10-bit HDR10/PQ BT.2020 (`format=64`, `colorSpace=1000104008`) and submitted HDR
metadata. Independently, the active HDMI DRM connector reported a good link,
`max bpc=10`, BT2020_RGB and PQ EOTF 2 metadata; KWin reported
3840x2160/120 Hz, 10-bit HDR.
This verifies the software/connector signal path, not panel light output.
**Physical TV image, brightness and blooming-zone behavior remain unverified.**
That run did not enable Vulkan validation; validation claims above are Windows.

## Linear HDR scene and optional HDR10 output (2026-10-11)

Based on public main `4691b9a`. Vulkan remains opt-in (`--render vk`), and
GL33 remains the default and retains its original lighting/transfer path.
This is a legacy-lighting HDR extension, not a PBR conversion, automatic
exposure system, bloom pass or graphics-menu change. No NGX integration or
private files are included.

### Color and lighting contract

World color, AA intermediates/history and native composition use sampled,
blendable `R16G16B16A16_SFLOAT`, independent of the swapchain format. Vulkan
checks FP16 sampling, linear filtering and attachment/blend support. A device
without that support reports an error with a GL33 fallback recommendation.
Even Off/100% now has a completed-world composition pass. Depth/stencil,
alpha test, alpha-to-coverage, blend factors and native-overlay ordering are
unchanged; RGB blending and filtering now operate on linear values.

Diffuse assets use sRGB image views, decoding **before** filtering. Raw UNORM
views of the same storage remain available for water normals/detail data and
AA lookup tables. Alpha is not transfer-encoded. Existing authored mip chains
are retained; they have not been regenerated as linear-light mipmaps.

The upper clamps on native vertex diffuse/emission/local-light sums and
specular were removed. A Vulkan-only floating-point sidecar also captures
software lighting before packed ARGB destroys its over-range RGB, including
sky, clouds, sun/light sprites and colorized terrain. Clipping interpolates
this sidecar; the original packed alpha/fog stream and GL33 stay intact.
Specular and water contributions, fog, transparent/additive blending and
reconstruction feed the floating-point scene instead of an 8-bit ceiling.
This preserves real existing lighting headroom rather than scaling an LDR
image to pretend it is HDR.

The legacy light/material equations still produce empirical, display-referred
coefficients. Their 0..1 range is decoded as sRGB; excess above legacy white
extends linearly as light energy. Specular is converted separately and added
in linear light. This keeps ordinary materials recognizable without applying
a power curve to unbounded legacy coefficients. It does **not** make the
old accumulation, attenuation or material model photometric. Intensities
already baked/clipped into textures or supplied only as packed draw colors
cannot be recovered. Existing day/night light-selection rules are retained.

### Tone mapping, gamma and overlays

Fixed unit exposure is used throughout. The neutral RGB-max shoulder is
identity below 65% of output peak, continuously rolls off above that point,
and scales RGB together to preserve hue. SDR peak is 1; HDR peak is configured
nits/reference-white nits. There is no frame-dependent exposure. Shadow and
midtone values below the shoulder are not deliberately crushed or lifted.
The corrected linear filtering/blending and restored light energy still mean
SDR is not pixel-identical to the previous renderer: sunlit surfaces can be
brighter, with softer highlight compression, while legacy textures stay
recognizable.

World tone mapping runs **after** MSAA/FXAA/SMAA/TAA or FSR reconstruction and
before native-resolution overlays. TAA no longer clamps history/output at 1.
FXAA/SMAA use a bounded perceptual proxy for edge detection while filtering
the original linear HDR samples. FSR 3.1.4 explicitly enables
`FFX_FSR3UPSCALER_ENABLE_HIGH_DYNAMIC_RANGE`; input and reconstruction output
are FP16. `preExposure=1` and the SDK's null-exposure default of 1 are used;
automatic exposure and RCAS remain disabled. See AMD's
[input/exposure contract](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/super-resolution-upscaler/).
Existing depth, motion, jitter and reactive/composition masks are retained.

Native cockpit/weapon geometry uses the same tone curve; authored screen-space
HUD, instruments, menus and text are bounded at reference white. Both blend
into the display-linear native target, never into PQ codes. The existing
gamma control runs once in its calibrated perceptual domain at final output;
HDR values above reference white are left alone by gamma. Legacy night-eye
processing also retains its perceptual-domain response. Final SDR uses one
sRGB encode (including the sRGB-attachment fallback); HDR converts linear
BT.709 to BT.2020 and then ST.2084/PQ with absolute nits.

### Native HDR controls and negotiation

Internal HDR is always active in Vulkan; HDR10 **presentation** is separately
opt-in, default off. Startup `CWR_VK_HDR=1` requests it, or use:

```sqf
triHDR "on"
triHDR "off"
triHDR "status"
triHDR "peak 1000"
triHDR "white 203"
```

Default peak/reference white are 1000/203 nits. Peak accepts 100..10000,
white 80..500, with white <= peak. These are display targets, not measured
panel capabilities. Pending values are applied with metadata at a device-idle
swapchain boundary. Commands are session-only; GL33 reports unsupported.

When exposed, `VK_EXT_swapchain_colorspace` permits exact 10-bit
`A2B10G10R10_UNORM_PACK32` or `A2R10G10B10_UNORM_PACK32` paired with
`HDR10_ST2084_EXT`. An unavailable HDR pair, or failed HDR swapchain creation,
falls back to an explicitly supported SDR/sRGB pair. We do not interpret a
random surface format or FP16 buffer as HDR presentation. Optional
`VK_EXT_hdr_metadata` submits BT.2020/D65 primaries, configured peak/MaxCLL
and unknown MaxFALL (zero). See the Vulkan
[color-space](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_swapchain_colorspace.html)
and [metadata](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_hdr_metadata.html)
contracts. Status distinguishes requested mode from actual format/color-space,
metadata availability and fallback reason. It always labels physical display
verification as unverified; the application cannot measure the panel.

### Qualification and remaining limits

Windows RelWithDebInfo Vulkan+FSR and Vulkan-disabled GL33 builds succeeded.
The GL33 executable also passed a main-menu launch/normal-close check and
returned `UNSUPPORTED` for `triHDR "on"`.
Driver-free Vulkan policy/lifetime tests pass 343 checks (including their
intentional teardown-error fixture). The focused shape/shadow/convention/
shutdown tests pass 815 assertions in 75 cases in each build. New coverage
includes HDR format opt-in/fallback, rejecting FP16-as-HDR, luminance argument
validation, status semantics, final HDR/SDR/sRGB-attachment encoding constants
and unclipped software-color interpolation.

Windows RTX 4060 Ti stock-game checks include Infantry daytime streets,
sunlit buildings, foliage/sky, coast, Shadow Killer night and NVG, native
weapon/HUD, camera movement, resize and minimize/restore. All AA selections
(Off, FXAA, SMAA, TAA, MSAA 2/4/8 and the four FSR modes), 150% SMAA,
SSAO and CSM were exercised. Validation-enabled SDR runs completed with
zero errors/warnings, including 27,228 submitted daytime frames and 33,800
night frames. Windows exposes only SDR in this setup: requesting HDR falls
back to SDR without changing internal HDR. Before/after captures used the
public `4691b9a` FSR executable and isolated copies of the same profile.
The existing gamma 1.0 and 1.6 profiles both start at brightness 1.6; an
additional before/after check explicitly set neutral brightness 1.0.
Final-build validation also covered a consumed smoke grenade over coastal
water, fog, jeep cockpit/instruments and pause-menu text with FSR/SSAO/CSM
(17,542 frames, zero validation errors/warnings). Time, position, invulnerability
and temporary stock objects were controlled through the developer harness;
these were representative live-render checks, not full mission playthroughs.
Grenade retries did not produce a usable explosion capture; the headlight
toggle/temporary damaged-jeep check likewise did not establish correct visible
illumination. Explosion/muzzle-flash peaks and headlight quality therefore
remain unqualified rather than being counted as passing visual checks.

### GPU cost

RTX 4060 Ti, Windows, 1920x1080, frozen Infantry street, 900 m view distance,
CSM/SSAO off, gamma 1.0/brightness 1.6, validation off. The existing profiler's
whole-command-buffer timestamps, last four rolling samples per 14-second
mode interval, gave the following arithmetic means:

| Mode | `4691b9a` GPU ms | HDR pipeline, SDR output GPU ms |
| --- | ---: | ---: |
| Off | 0.960 | 1.372 |
| SMAA | 1.322 | 1.478 |
| TAA | 1.407 | 1.474 |
| FSR Quality | 1.684 | 1.674 |

These are **observed run costs, not clock-normalized overhead** or FPS.
Vsync was requested off, the existing profile cap was 240, and GPU clocks
varied substantially (roughly 465..2565 MHz across baseline samples and
690..2565 MHz after). No power/clock settings were changed. In particular,
the near-equal FSR result does not establish zero HDR cost or a speedup.
Off now pays an extra world-to-native composition; FP16 color storage and
bandwidth are twice RGBA8 per pixel. AA/FSR already had world composition.
Per-swap-image targets, temporal histories and selected MSAA/render scale
multiply that storage; this milestone does not optimize their allocation.
G14 HDR TAA+SSAO+CSM samples were about 4.9 ms at 1920x1080 game output
upscaled by Gamescope to the 4K TV, not a native-4K benchmark or isolated
HDR10 encoding-cost measurement.

### G14 HDR signal-path evidence (not physical display qualification)

On the ROG G14/Nobara, the RX 6700S/RADV ran the public Windows binary through
a **direct developer Proton 11 + nested Gamescope Wayland** route, not a
native Linux build or Steam launch qualification. The game negotiated
format 64 / color space 1000104008 (10-bit HDR10/PQ), submitted metadata,
ran FSR Quality and TAA with SSAO/CSM, and switched SDR/HDR live. It exited
normally after 35,959 submitted/presented frames. Linux validation layers
were not installed, so its zero error counter is not validation coverage.

Beyond swapchain creation, KWin reported the attached TCL SMART TV at
3840x2160/120 Hz, HDR/wide gamut enabled. Read-only DRM connector inspection
confirmed HDMI-A-1 link Good, max bpc 10, BT2020_RGB and HDR_OUTPUT_METADATA
EOTF 2 (PQ). This verifies the configured compositor/HDMI HDR signal path,
**not** that the game's image was correct on the physical TV. Remote desktop
captures were black and the user could not inspect the TV. Panel brightness,
highlight separation, color accuracy, HUD luminance and actual visible HDR
gameplay remain unverified. No Auto HDR/inverse-tone-map path was used.
Do not call this physically qualified HDR output. No compositor/driver
settings were changed to chase that missing visual verification.

Local evidence remains under the development checkout's ignored
`build/shadow-live/hdr-*` directories (captures, action logs, executable hash
receipts, shutdown and timing logs). The final Windows Vulkan executable
tested here has SHA-256 `93E3288CE931A1029D6D587D18389C7FB5EBAFC1F3C65C26B429DF5FD4C737B2`.
The G14 run used the earlier milestone build
`4A7EA6E7912D73688F95E46B1F931F16A6601E0505D6DA8C5CE156374C092059`;
the final idle-boundary luminance-setting fix and software-light capture-loop
cleanup were subsequently rebuilt/tested on Windows, not rerun on the G14.

## AMD FSR 3.1.4 temporal upscaling (2026-10-11)

Optional public Vulkan implementation, based on `ecf8f5b`. The actual AMD
FidelityFX SDK **v1.1.4** FSR 3.1.4 upscaler and Vulkan backend are used, with
no sample framework or Frame Generation. The pinned SDK commit is
`c6efa6bf7f2027b3ec94f28578bb5965eabb9e55`. The selected source subset, local
build adaptations and complete MIT notice are documented in
[ThirdParty/FidelityFX](../engine/PoseidonVK/ThirdParty/FidelityFX/README.md).
The combined application retains the repository's GPL-3.0-or-later license
with its existing Section 7 terms. No private NGX files are part of this change.

### Enablement and commands

Build with `CWR_HAS_VULKAN=ON`, `CWR_FSR3=ON`, and
`CWR_FSR_SHADER_COMPILER=<official SDK v1.1.4 FidelityFX_SC executable>`.
Vulkan SDK >= 1.3.250 is required. The compiler and its adjacent dependencies
are external build tools, not distributed runtime dependencies. Missing tools
produce a configure warning and leave ordinary Vulkan available. FSR defaults
OFF at build time. GL33 does not find or link the SDK.

At runtime, start with `CWR_VK_FSR=1` to request the Vulkan 1.2 device path.
Without it, the original API 1.0 device path is retained. Device checks include
compute queue support, independent blending, storage image formats/features,
and FP16/Int16 when the SDK selects half precision. Unsupported requests return
`UNSUPPORTED` and preserve the selected AA mode. Off/100% remains the default
and allocates no FSR context, input textures or reconstruction targets.

```sqf
triAA "fsr-native"
triAA "fsr-quality"
triAA "fsr-balanced"
triAA "fsr-performance"
triAA "status"
```

These are mutually exclusive AA modes. FSR and supersampling conflict in both
selection orders: the command rejects the change without modifying the previous
configuration. Use `triRenderScale 100` before selecting FSR. Status reports
actual world/output dimensions, successful dispatches and history reset counts.
Initialization alone does not increment the dispatch counter. Device-idle
swapchain recreation owns SDK creation/destruction. Creation failures restore
ordinary AA and report the cause; dispatch failure reports its result code,
clears that output frame and schedules Off at the next idle boundary.

Render sizes come directly from `ffxFsr3UpscalerGetRenderResolutionFromQualityMode`,
including AMD's integer truncation. Output always uses the native drawable:

| Mode | Ratio | 1920x1080 input | 2560x1440 input |
| --- | --- | --- | --- |
| Native AA | 1.0 | 1920x1080 | 2560x1440 |
| Quality | 1.5 | 1280x720 | 1706x960 |
| Balanced | 1.7 | 1129x635 | 1505x847 |
| Performance | 2.0 | 960x540 | 1280x720 |

### Input and composition contract

The existing world-effects boundary renders sky, terrain, objects, transparent
effects, water, fog, projected shadows and CSM at internal resolution. SSAO
executes on the world's original sampled depth before FSR. The reconstructed
color and jitter-adjusted sampled depth are composited to native attachments
before cockpit, weapon, HUD, menus and text. Final gamma still executes once.

FSR reuses existing per-instance/deformed-vertex history and its invalidation
for camera cuts, teleports, save/load, NVG changes, bypassed world views and
mode/size changes. AMD's jitter phase-count and offset helpers drive the existing
projection jitter. The positive-height viewport maps positive projection offsets
to right/down geometry displacement. Existing vectors are previous UV minus
current UV including jitter; the input pass subtracts previous-minus-current
jitter in UV units. FSR receives unjittered UV vectors and an input-width/height
motion scale, so the SDK performs its own dilation/reprojection. Invalid motion
stays zero; no rigid motion is invented for unsupported dynamic geometry.

The input pass copies normal nonlinear 0-near/1-far depth to R32F, avoiding
SDK transitions of the engine's combined depth/stencil attachment. Near/far
and vertical FOV derive from the actual world projection. View units are metres,
frame delta is wall-clock milliseconds. The initial `4691b9a` integration used
UNORM LDR; the HDR milestone above supersedes this with linear FP16 and the
SDK HDR flag, retaining unit exposure/pre-exposure and no automatic exposure.
World texture mip
bias follows AMD's `log2(renderWidth/displayWidth)-1`; existing AA and overlays
retain their prior bias. Optional RCAS sharpening is disabled.

Two real R8 masks feed AMD's reactive and transparency/composition inputs.
Existing material flags and motion validity mark water, smoke, alpha-blended
effects and missing/unsupported correspondence as fully reactive with unlocked
history. Valid alpha-tested cutouts use reactive 0.5/composition 0.75; valid opaque
motion uses zero. The shader's alpha test occurs before mask coverage, and
transparent fragments conservatively overwrite validity. This favors less
history over ghost trails; small foliage, animated textures without explicit
flags, sky without motion, and lower-resolution transparent edges remain limits.

Two focused SDK corrections are retained with their provenance: exclude its
unused Frame Generation swapchain callback from the upscaler-only link, and
correct GLSL `rw_luma_history` from `rgba8` to `rgba16f` to match AMD's RGBA16F
allocation. The latter was discovered through real dispatch validation on the
RTX 4060 Ti. Neither substitutes or changes the reference reconstruction algorithm.

### Qualification

Validation uses real stock-game draws on the RTX 4060 Ti, driver 617.14.
Direct developer launches use isolated profiles and `--nosound`; this is not
Steam launch qualification. Local captures, action logs, executable hashes and
validation logs are retained under `build/shadow-live/fsr-*` in the working game
checkout. The desktop is 1920x1080; 2560x1440 tests use an oversized drawable.
Desktop motion capture at 1440p is clipped outside the physical screen and does
not qualify presentation on a physical 1440p monitor.

The GPU profiler reports complete command-buffer workload as `Vulkan GPU:
frame_ms`, and the interval immediately surrounding AMD's dispatch as `Vulkan
FSR: dispatch_gpu_ms`. The latter excludes CWR input conversion and native
composition; those costs remain in the full frame. Neither counter includes
acquire/present pacing or CPU recording. Display-paced FPS is not a performance
measurement. Clock observations must accompany timing comparisons.

#### Observed results

All four modes executed the reference GPU dispatches at 1080p and 1440p, with
the exact input sizes above. The validation-enabled infantry/switching, 1440p,
normal Single Mission save/load, Cobra and Abrams runs completed respectively
36,830 / 23,806 / 26,514 / 8,107 / 29,214 submitted frames, each with **zero
validation errors and warnings** after fixing shaderInt16 enablement and the
SDK luma image-format mismatch.
SSAO and CSM were enabled together during the image-quality runs. These are
stock-game scenes, not a standalone SDK initialization test.

Moving infantry, fences/foliage, tank turret rotation and airborne helicopter
rotors were captured as frame sequences. Native AA looked sharper than the
existing TAA in the sampled village view. SMAA retains spatial detail but has
no temporal reconstruction; sparse fence/foliage coverage still changes with
movement. Quality preserves more small detail than Balanced/Performance; the
lowest input resolution visibly softens distant foliage and small soldiers.
The inspected sequences showed no obvious persistent long trails around the
soldier, turret or rotor. This is a bounded visual assessment, not a claim that
all ghosting or shimmer is eliminated. Thin cutouts and disoccluded edges can
still lose detail or shimmer, especially where conservative masks reject history.
Runs were not deterministic pixel-matched replays, so they do not establish an
objective image-quality ranking. Cockpit/weapon/HUD and pause-menu captures
remain native-sized rather than being reconstructed with the world. Combat
particles, water and fog were also inspected; water coverage was a diagnostic
teleport rather than a complete normal mission traversal.

Both supersampling conflict orders retained the previous configuration.
Teleport, internal/external camera changes, NVG on/off, mode switches and
960x640 resize/minimize/restore recovered and reset history. In a normal Single
Mission (not `--test-mission`), Save followed by movement and Load restored
the saved position, increased the FSR reset counter from 2 to 3 and resumed
successful dispatch on the first rendered post-load frame.

FSR-on Vulkan, FSR-off Vulkan and GL33 game/test builds passed. The Vulkan policy
executable passed 327 checks in both optional-build configurations; focused
shape/temporal/shadow tests passed 812 assertions in 74 cases, including the
GL33 build. Unsupported-device behavior has not been physically qualified on
AMD/Intel or a pre-1.2 Vulkan device. The explicit capability rejection and the
non-FSR runtime/build paths avoid requiring the SDK for ordinary rendering.
Live FSR-runtime-off and FSR-build-off Vulkan smoke runs retained Off when FSR
was requested and closed with zero validation errors/warnings (6,330 and 7,986
submitted frames); MSAA4 and TAA also ran in the latter. The GL33-only executable
rendered normal infantry gameplay and rejected the Vulkan-only AA selector.
The final rebuild repeated all 327 policy checks in both configurations and
the narrower `[vulkan-shape],[temporal],[ShadowMath]` selection (751 assertions,
59 cases). No private integration sources, SDK files or binaries were modified
or included in the public commit; implementation used a separate clean worktree.

#### GPU cost snapshot (validation disabled)

Fixed, paused village view in `02Infantry.Abel`, 900 m view distance, clear
16:00 lighting, SSAO/CSM off, no concurrent build or capture. Each mode ran for
14 seconds; values average the final four two-second timestamp windows. FSR
conversion/composition are included in frame cost but excluded from dispatch.
The executable SHA-256 was
`1c437149402bba5b9310387f5dcdb0f80d99fffe66cfec8a3c54b8dcd6ea28b7`.

| Mode | 1080p frame / FSR ms | Graphics MHz sampled | 1440p frame / FSR ms | Graphics MHz sampled |
| --- | --- | --- | --- | --- |
| Off | 0.931 / -- | 465-2055 | 1.261 / -- | 615-705 |
| SMAA | 1.425 / -- | 705-750 | 1.591 / -- | 1065-1140 |
| TAA | 1.544 / -- | 825-855 | 1.714 / -- | 1320-1350 |
| FSR Native AA | 1.867 / 1.052 | 1650-1725 | 2.133 / 1.274 | 2835 |
| FSR Quality | 1.716 / 0.987 | 1230-1485 | 1.852 / 1.110 | 1965-2355 |
| FSR Balanced | 1.723 / 1.003 | 1155-1215 | 1.873 / 1.127 | 1785-1845 |
| FSR Performance | 1.672 / 0.978 | 1065-1140 | 1.815 / 1.104 | 1650-1725 |

These are **raw costs, not a speedup benchmark**: automatic GPU clocks were
not locked, and the GPU is lightly loaded. Memory clocks were 5001 MHz for
non-FSR and 1080p reduced-resolution modes, 9201 MHz for Native AA and 1440p
FSR; the first 1080p Off sample was still at 9201 MHz. No display-paced FPS
or percentage performance gain is inferred. A controlled-clock, GPU-bound
workload would be needed to quantify speedup. The raw samples and temperature/
power observations are in the local `fsr-timing-1080` and `fsr-timing-1440`
evidence directories. These measurements do not include enabled SSAO/CSM cost.

## Private NVIDIA DLSS experiment (2026-10-11)

**Not a distributable feature.** An uncommitted, local Windows x64 experiment
based on `008bc97` implements direct Vulkan NGX Super Resolution, not Streamline,
an imitation upscaler, or a general upscaling framework. The public repository
contains these findings and the independent native-dynamic-motion rejection fix
only. NVIDIA SDK materials, integration sources and linked executables are not
included in the public change. The public build still has the AA modes below.

### Licensing and packaging gate

The repository's [license](../LICENSE) is GPL-3.0-or-later with additional
Section 7 terms; no NVIDIA linking exception was found. The inspected
[NVIDIA RTX SDK license](https://github.com/NVIDIA/DLSS/blob/374959484e79a640feaba44c93ac8cfb0a03f5b5/LICENSE.txt)
(March 14, 2024) imposes protective redistribution terms in section 2(c) and an
open-source-license restriction in section 4(e). This leaves distribution of
the combined application unresolved; obtaining SDK files from a public NVIDIA
repository does not make them GPL-compatible. This is a release gate, not a
legal determination. Resolve it with the relevant rights holders/legal review
before publishing the integration or a linked build. Private experimentation
does not establish redistribution permission.

Packaging also needs an approved production runtime, the applicable notices,
attribution/branding and release notification, and qualification of the actual
runtime/model selected by the driver. Do not ship the developer DLL. No SDK
download, installation, packaging or redistribution rule was added. Local SDK,
test profiles, captures and executables remain under ignored `build/` paths.

### Implemented locally

The private build option is `CWR_PRIVATE_DLSS=ON`, with `CWR_DLSS_SDK` pointing
to an externally supplied official SDK. It defaults OFF. GL33 does not search
for or link NGX. Runtime use additionally requires `CWR_VK_DLSS=1` and an
absolute `CWR_DLSS_RUNTIME` directory containing `nvngx_dlss.dll` at startup.
Without this opt-in the original Vulkan 1.0 instance/device path is retained.
The opted-in path checks Vulkan 1.1, NVIDIA support, the SDK's queried instance
and device extensions, compute queue support, image formats, independent blend,
storage-image write-without-format and buffer-device-address capabilities.
Failed capability checks retain ordinary rendering and report their reason.

Private developer commands, process-local and not persisted:

```sqf
triAA "dlaa"
triAA "dlss-quality"
triAA "dlss-balanced"
triAA "dlss-performance"
triAA "status"
```

Status distinguishes capability availability from successful inference and
reports actual input/output dimensions, evaluation/reset counts and validation
errors. NGX errors retain the operation and result code. Supersampling and DLSS
are incompatible: both selection orders are rejected without silently altering
the previous settings. Off, FXAA, SMAA, TAA and MSAA remain opt-in alternatives;
there is no stacking, dynamic resolution, Frame Generation, Ray Reconstruction,
Reflex or new menu. A failed feature creation/evaluation schedules an idle-safe
return to Off rather than repeatedly submitting a broken NGX feature.

The same completed-world boundary contains transparency, water, fog, projected
shadows and CSM. SSAO executes before NGX. The reconstructed color and sampled
world depth are composited into native-resolution attachments before cockpit,
weapon, HUD and UI; final gamma still runs once afterward. Existing intro/map/
options bypass behavior is retained, rather than claiming inference in those
views. Resources and NGX features follow the existing idle/swapchain lifecycle.

### Input contract and known quality limits

Checked against NVIDIA's
[Vulkan programming guide](https://github.com/NVIDIA/DLSS/blob/374959484e79a640feaba44c93ac8cfb0a03f5b5/doc/DLSS_Programming_Guide_Release.pdf):

- Existing vectors are previous UV minus current UV, including projection
  jitter. A dedicated RG32F input pass subtracts previous-minus-current jitter
  and multiplies by the actual input dimensions: current-to-previous input
  pixels, upper-left origin/Y downward. `MVLowRes` is enabled; `MVJittered` is
  not. NGX handles dilation, not the engine's TAA dilation.
- The positive-height Vulkan viewport turns positive projection jitter into
  right/down geometry displacement. NGX receives the opposite sampling offset,
  in input pixels. Halton(2,3) uses `ceil(8 * outputArea / inputArea)` phases;
  existing TAA keeps its eight phases. Reduced-resolution texture mip bias is
  `log2(inputWidth/outputWidth)-1`, confined to the DLSS world draws.
- Native depth is 0 near/1 far, not inverted or linearized. The legacy UNORM
  display-referred LDR scene uses no HDR flag, unit exposure, and gamma after
  reconstruction. The private path rejects the unqualified sRGB scene fallback.
- Existing camera/load/resize/mode history invalidation drives NGX reset. Each
  successful inference is counted; an initialized SDK alone is not a pass.
- Software TL retains real previous post-deformation vertices. Dynamic native
  buffers have no such sidecar: they now reject motion and do not store an MVP
  for use as immutable history on the following frame. This small independent
  correctness fix also applies to existing TAA; no new animation system is added.
- Current SDK models do not consume `BiasCurrentColor` as a usable rejection
  mask. The experiment does not pretend otherwise: unsupported/reactive coverage
  rejects the reconstructed output, samples current input spatially, and guards
  the following disocclusion. Remaining color is bounded by the current 3x3
  neighborhood. This does **not** mask NGX's internal history. Sky without motion,
  smoke/water and uncertain deformation/coverage can therefore remain spatially
  filtered, softer or shimmering, especially with low-resolution Performance.

### Actual SDK and driver qualification

Hardware: RTX 4060 Ti, driver 617.14, Windows, native desktop 1920x1080.
Official NVIDIA/DLSS checkout: `374959484e79a640feaba44c93ac8cfb0a03f5b5`;
the tested developer runtime identifies itself as 310.9.1. SDK quality enums
select native DLAA, MaxQuality, Balanced and MaxPerf; optimal dimensions come
from NGX, not hard-coded ratios. The experiment explicitly requests supported
transformer preset K for all four modes.

An existing global NVIDIA App SR override selected a different 310.9.0 runtime
from the driver model cache and forced preset M for Performance despite K being
requested. That combination executed inference but emitted four validation
errors inside SDK-owned `nv.ngx.dlss.resource` clear/layout operations. It is
**not qualified**. A temporary application-only NvAPI profile disabled that SR
override during SDK qualification, leaving global settings unchanged. With the
official 310.9.1 SDK runtime and K, all four modes executed real inference with
zero Vulkan validation errors/warnings in the completed qualification runs.
Initialization logs for other NGX libraries are SDK discovery behavior; the
application creates/evaluates only the SuperSampling feature.

The owned temporary test profile was removed and absence of an application
profile verified; the original global values (override enabled, latest preset)
were unchanged. Future use inherits the user's driver overrides again.
A release must resolve/qualify this driver
interaction, not silently change global NVIDIA settings.

### Gameplay evidence and scope

Isolated stock-game profiles, muted direct developer launches (not Steam
qualification), executable hashes and logs are retained locally under
`build/shadow-live/dlss-*`. Sixty-frame motion captures include moving soldiers,
village houses/foliage/fences and disocclusions in all four new modes; native
1080p SMAA and TAA comparisons are also retained. Separate runs drove the HMMWV
about 59 m and raised the Cobra from ground level to about 55 m with spinning
rotors, including native cockpit views. SSAO/CSM were enabled for these gameplay
checks. Water/fog, smoke, projected shadows, camera changes and resize/minimize/
restore were exercised. The final infantry, car and Cobra runs respectively
closed after 59,408, 6,403 and 3,775 submitted frames with zero validation
errors/warnings.

Inspection found intact native HUD/cockpit and no obvious long soldier/vehicle
trails in the sampled sequences. DLAA softens edges relative to SMAA; TAA and
DLAA both reduce native edge roughness. Performance is visibly softer in foliage
and rotor silhouettes. Thin/reactive coverage remains a quality limitation,
not an asserted temporal-AA success. These sequences are not deterministic
frame-matched replays, so they do not establish a numerical image-quality
ranking or absence of all ghosting/jitter artifacts. Broader content and longer
motion-sequence qualification remain required.

The ordinary single-mission Save/Load path was also tested with DLAA, not just
a scripted teleport. After saving at `[8089.76,5153.36]`, movement reached
`[8090.20,5152.07]`; Load restored the saved position and increased NGX reset
count from 2 to 3. The first post-load inference succeeded, the captured scene
was intact, and `dlss-load-final` closed after 13,902 frames with zero validation
errors/warnings. The earlier save attempt timed out normally during an
interruption; it was not counted as a successful load test.

### GPU workload, not display-paced FPS

Validation-disabled measurements used the frozen stock infantry village,
900 m view distance, projected shadows on, SSAO/CSM off, scale 100, and automatic
GPU clocks. Each cell averages the last four two-second timestamp windows after
a mode switch/warmup. `frame` is GPU command-buffer workload; `resolve` includes
motion conversion, real NGX inference and the output guard, not NGX alone.
These values exclude acquire/present pacing and CPU recording. The process ran
at roughly 160 FPS / 6.2 ms CPU frame cadence in this light scene; that is not
the measured GPU cost and not evidence of a DLSS FPS improvement.

| Mode | 1080p input | 1080p frame / resolve ms | 1440p input | 1440p frame / resolve ms |
| --- | --- | --- | --- | --- |
| Off | 1920x1080 | 1.043 / n/a | 2560x1440 | 1.134 / n/a |
| SMAA | 1920x1080 | 1.297 / n/a | 2560x1440 | 1.525 / n/a |
| TAA | 1920x1080 | 1.531 / 0.350 | 2560x1440 | 1.714 / 0.436 |
| DLAA | 1920x1080 | 1.943 / 1.351 | 2560x1440 | 2.624 / 1.889 |
| DLSS Quality | 1280x720 | 1.907 / 1.397 | 1707x960 | 2.303 / 1.740 |
| DLSS Balanced | 1114x626 | 1.901 / 1.408 | 1485x835 | 2.249 / 1.712 |
| DLSS Performance | 960x540 | 1.896 / 1.416 | 1280x720 | 2.188 / 1.672 |

Outputs are the full requested 1920x1080 and 2560x1440 Vulkan drawables.
The 1440p test used an oversized window on the 1080p desktop, not a physical
1440p monitor; actual attachment dimensions were verified in runtime status.
Window captures were clipped outside the physical desktop: 1440p visual
inspection covers the visible portion, not a full-monitor 1440p presentation.
At 1440p all four NGX modes sampled 2850 MHz graphics / 9201 MHz memory clocks.
At 1080p NGX sampled 1740-2025 MHz / 9201 MHz, while Off/SMAA/TAA used much lower
automatic clocks. These are observed workloads, **not clock-normalized kernel
benchmarks**. In this low-cost world, inference outweighs raster savings;
reduced input dimensions alone do not promise higher FPS. Local evidence:
`dlss-perf-1080/dlss-timing.json`, `dlss-perf-1440/dlss-timing.json` and their
profiler logs under `build/shadow-live/`.

### Regression results

- Private NGX-enabled Vulkan, NGX-disabled Vulkan and GL33 builds completed.
  Each relevant Poseidon test binary passed 74 cases / 812 assertions for
  `[vulkan-shape],[ShadowMath],[Conventions],[ShutdownOrder]`. The private Vulkan
  policy binary passed 319 driver-free checks with NGX both enabled and disabled;
  its intentional teardown-error fixture is not a live validation failure.
- The compiled-OFF Vulkan run retained API 1.0 and exercised Off, FXAA, SMAA,
  TAA, MSAA 2/4/8 and SMAA at 150% (2880x1620 world / 1920x1080 display).
  It closed after 2,145 frames with zero validation errors/warnings.
- The linked private build with an absent runtime reported the exact missing
  directory/DLL requirement, retained Off, accepted TAA, retained API 1.0 and
  closed after 425 frames with zero validation errors/warnings.
- GL33 stock infantry rendered and closed normally; DLAA/status returned the
  existing unsupported-selector response. No NGX dependency is linked there.
- Supersampling conflicts were tested in both orders in the NGX-enabled run;
  the previously selected mode/scale survived rejection.
- No non-NVIDIA adapter or older Vulkan loader was available for physical
  testing. Those rejection paths were reviewed, not claimed as hardware passes.

## Optional anti-aliasing and world render scale (2026-10-11)

Off (default), FXAA, SMAA 1x, TAA and MSAA 2x/4x/8x are implemented.
Independent world render scales are 100% (default), 125%, 150% and 200%. TAA is
opt-in, with real instance/deformation motion history. Vulkan remains
opt-in; GL33 rendering, assets, localization and gameplay behavior are unchanged.
There is no graphics menu, upscaler, dynamic resolution or AA stacking.

Developer evaluator commands (process-local, not persisted):

```sqf
triAA "off"       // also "fxaa", "smaa", "taa", "msaa2", "msaa4", "msaa8"
triAA "motion"    // visualize velocity/rejection while taa is selected
triAA "motion-off"
triRenderScale 150
triAA "status"    // actual/requested mode, actual/requested scale, sample mask
```

Mode changes report `OK` and take effect at the next frame's existing
idle/recreation boundary. Unsupported sample counts return `UNSUPPORTED` and
retain the previous selection; they never silently become a different mode.
Invalid scales return false. TAA requires independent attachment blending;
unsupported devices retain their previous mode. GL33 reports unsupported for
these commands. Capabilities intersect
the exact color and sampled depth/stencil image formats with device framebuffer
and sampled-depth sample-count limits. The RTX 4060 Ti reports 1/2/4/8 support.

### Rendering and algorithms

The existing `BeginWorldEffects` / `FinishWorldEffects` boundary brackets a
separate world target only when AA or render scale requires one. Landscape,
sky, native objects, software TL terrain, transparency, water, fog, projected
shadows and rain complete there. SSAO reads that world's depth. AA/downsampling
then composite world color and depth into the original native-resolution scene
target before cockpit/weapon draws, HUD, map labels and menus. Gamma still runs
once on the completed frame, including overlays. Off at 100% uses the original
scene attachments without a world copy or AA images.

- FXAA uses NVIDIA FXAA 3.11's complete PC quality path, preset 29, 0.5 subpixel
  filtering, 0.125 contrast and 0.0312 dark thresholds. RGB luma is calculated at
  the reference fetch sites to include chromatic edges without a luma-copy pass.
- SMAA 1x uses the upstream HIGH preset with color-edge detection, blending
  weights (including diagonal/corner detection), and neighborhood blending.
  Original area/search table bytes are embedded, losslessly expanded through
  the existing texture uploader, and sampled bilinearly without anisotropy.
  Edge/weight targets are zero-cleared UNORM images. See
  [reference provenance and licenses](../engine/PoseidonVK/ThirdParty/README.md).
- MSAA uses real 2/4/8-sample color and depth/stencil attachments, Vulkan color
  resolves and per-sample projected-shadow stencil tests. Opaque cutout world
  materials use derivative-scaled alpha-to-coverage around the existing cutoff;
  blended smoke, glass and water keep their ordinary blend paths. Sampled depth
  uses a minimum-sample resolve in the shader for SSAO and subsequent depth
  composition. This avoids requiring a newer depth-resolve extension.
- Supersampling integrates the exact source-pixel area of each output pixel,
  including fractional 125/150% footprints. It uses actual rounded-up scaled
  dimensions, not a larger window. Depth conservatively takes the nearest
  covered surface; cockpit depth clears retain their original ordering.

World stencil stays attached throughout world drawing and CSM interruptions;
it is not filtered or used as AA scratch. After the world boundary no projected
shadow pass consumes it. Native-resolution overlay stencil retains its own clear
value. SSAO modulates the resolved world color after the final multisample pass,
so a later resolve cannot overwrite AO. CSM resumes the correct world target
and sample-count pipeline bank. Every boundary invalidates cached draw bindings.
FXAA and SMAA at 100% write directly into the native scene, avoiding an extra
full-frame copy; scaled post-AA uses one reusable intermediate before downsampling.
All GPU target resources follow swapchain lifetime; no GPU history images are
allocated per frame. CPU temporal geometry records are rebuilt each world frame.

### Temporal motion contract and resolve

The previous milestone's missing engine contract is now implemented, not replaced
by camera-only depth reprojection. `Object::_id` is unsuitable: it can be -1,
assigned again by landscape/load code, and changed by `SetID`. Object addresses
are recycled, and both Shapes and model proxy Objects are shared. A small
process-local 64-bit `RenderIdentity` member in Object and Shape gives every
lifetime/copy a fresh identity, independent of serialization. `Object::Draw`
pushes/pops the entire parent/proxy instance path through default-no-op Engine
hooks. GL33 ignores these hooks; no save format or stock asset changes are needed.

- Native immutable Vulkan meshes retain their actual previous MVP, keyed by
  full object path and Shape lifetime (including LOD). This path is not claimed
  to animate vertices: stock animated vehicles/soldiers use the software path.
  Native dynamic meshes explicitly reject motion and do not retain an MVP;
  transform-only history is not valid deformation history.
- Software TL captures the actual post-animation, pre-clipping vertex positions
  in `FaceArray::Draw`. Previous homogeneous clip positions are retained by
  path, Shape and indexed topology; this includes soldier pose, turret, wheels,
  rotor and billboard transforms. Both CPU clip interpolation overloads carry
  the optional previous-position sidecar using the same intersection fraction.
  Pixel-projection conversion avoids applying camera FOV twice.
- Terrain and explicitly static terrain-fitted objects/roads reproject their
  known stationary world positions, including generated/clipped vertices.
  This static-world treatment is not applied to arbitrary moving objects.
- First appearance, changed LOD/topology, missing correspondence, ambiguous
  repeated instance draws, and unsupported effects reject history. Histories
  retain only consecutive world frames, not a persistent entity registry.

The world pass writes RGBA16F motion: XY is **previous UV minus current UV**,
including current/previous jitter, Z is expected previous-camera view depth in
meters, and W is 1 for valid opaque, .75 for valid cutout, 0 for unavailable, or
-1 for reactive coverage. Current sampled depth, jitter and validity are kept
separate from color. This is a usable future reconstruction input contract, not
an integration of DLSS/DLAA/FSR. Debug view maps XY displacement to red/green
around gray (80x UV scale); magenta means rejected, not zero velocity. HUD and
cockpit remain their normal colors. Debug switching also invalidates history.

TAA uses an eight-sample Halton(2,3) world-only projection jitter, two shared
RGBA16F history images on the existing graphics queue, nearest-depth velocity
dilation, previous-view-depth/disocclusion rejection, 3x3 YCoCg neighborhood and
variance clamping, velocity/luminance-sensitive history weighting, and bounded
8% detail restoration. History alpha stores view depth with a negative sign for
unreliable color, preventing reuse after smoke/water moves away. Sky has no
invented motion; only an immediate foreground edge can supply valid coverage.
SSAO resolves before TAA; downsampling and native overlays follow it; final
gamma remains last. Existing command buffers, frame fences, attachment resume
passes and swapchain recreation own all resources.

Alpha-tested foliage uses deformation motion and a lower history weight.
Partially transparent texels, smoke, water, additive/no-depth effects and
untracked draws reject history (including a one-pixel reactive border). Opaque
texels in stock blend-classified materials retain valid motion: rejecting the
entire texture would incorrectly exclude many stock buildings and plants.
Projected shadows preserve the receiver's velocity, with luminance/neighborhood
clamping for changing shadow color; they do not get fictitious caster velocity.

Camera subject/type changes, camera-effect changes, zero-time scripted camera
commits, scripted setPos/setPosASL/setDir on the camera subject (even small
teleports), large positional/directional/FOV discontinuities, NVG switching,
save loading, world cleanup, missing world frames, resize/scale recreation and
AA switching invalidate history. CPU history caches and GPU history validity
are independently gated, so stale records cannot make a reset frame valid.

### Limitations

Unsupported effects remain spatially sampled and can shimmer; smoke/transparency
are deliberately not accumulated. Sky/cloud texture animation and moving shadow
color have no independent velocity. Very thin fences, disoccluded edges and LOD
changes can still flicker, and TAA trades some fine texture contrast for stability.
It is not a promise of trail-free behavior for every mod/effect. No temporal
filter touches native cockpit instruments, weapons, HUD, menus or map text.
There is no SMAA+TAA stacking or vendor upscaler.

Spatial AA reduces visible stair steps but cannot provide temporal stability.
Fine foliage and thin geometry can still shimmer in motion; MSAA does not
supersample texture/shader evaluation. Supersampling is costlier and can soften
texture detail. Cockpit and weapon edges deliberately remain native and unfiltered.
Qualification is on the normal Windows UNORM swapchain; other GPUs and the sRGB
surface fallback are unqualified. Mode/scale changes reuse the existing
device-idle recreation and can hitch while pipelines/resources are rebuilt.

### TAA verification and performance

Vulkan-enabled and GL33-only RelWithDebInfo builds pass. Both pass the existing
`[vulkan-shape],[ShadowMath],[Conventions],[ShutdownOrder]` suites: 74 cases,
812 assertions, including new identity-reuse/copy, software projection and
homogeneous clipping checks. The Vulkan driver-free policy/lifetime executable
passes 311 checks; its intentional teardown-error fixture is not a live error.

Stock gameplay was exercised at 1920x1080 on RTX 4060 Ti / driver 617.14,
using isolated profiles and actual bounded keyboard/mouse input. Motion debug
was inspected before TAA color. Captures include consecutive 90-frame image
sequences (roughly 15-18 captured frames/second, not lossless 60-fps video).
Off/SMAA/TAA infantry comparisons reset position/direction but are not lockstep
AI replays; smoke, enemies and camera easing vary between sequences.

| Scene / local evidence run | Checks actually exercised |
| --- | --- |
| `taa-quality-infantry` | Independent soldier/limb/weapon vectors; running, firing enemies, blood/smoke, village foliage, fences and roofs; moving Off/SMAA/TAA image sequences |
| `taa-hmmwv` | Approximately 96 m driving over two bounded sequences; vehicle versus terrain velocity, dust rejection; TAA + SSAO/CSM, native cockpit at 150% world scale |
| `taa-tank` | Independent turret/hull velocity, turret aiming, about 11.6 m driving; optics and cannon fire (24 to 23 rounds), smoke; SSAO/CSM |
| `taa-cobra` | Rotor startup, takeoff from ground to 38.7 m, forward flight; cockpit/exterior switching, 125% scale, SSAO/CSM; translucent fast-rotor coverage rejected |
| `taa-night` | Night/NVG running; TAA + SSAO/CSM; 200% scale, resize to 960x640, minimize/restore |
| `taa-menu-save` | Main menu to stock Ambush intro/briefing/gameplay; ordinary menu Save/Load at TAA 150%; saved XY [8089.76,5153.36] restored after movement; map text; shoreline water/fog, scripted camera cut/teleport, SMAA/MSAA4/TAA switching |

Those six runs all closed normally (exit 0) with **zero core/synchronization
validation errors and warnings**. `taa-final-night` additionally exercises the
final small-teleport/NVG reset changes with GPU profiling disabled, covering
the ordinary acquire-semaphore wait path; firing/reload was checked (30 to 29
rounds), and its 9,338-frame shutdown also had zero validation warnings/errors.
`taa-gl33` launched the GL33-only binary with no renderer switch, exercised
stock infantry movement, rejected the Vulkan-only commands, and exited 0.
The host loader still reports its pre-existing stale Epic overlay manifest and
disabled third-party layers; these are distinct from Khronos validation output.
Local PNG sequences, action logs,
binary hashes, validation logs and timing JSON are in ignored
`build/shadow-live/taa-*`, not in the shipped assets.

Visual inspection found noticeably steadier foliage/roof edges than Off and
SMAA during the captured movement, with a modest loss of fine texture contrast,
not a blanket blur. No persistent soldier, vehicle or turret silhouette trails
were observed in the inspected sequences. Disocclusions reject instead of
dragging the old silhouette; smoke/water remain unfiltered. Native cockpit,
weapon, NVG overlay, HUD, briefing and map text stay sharp. This is bounded
stock-gameplay qualification on one GPU, not exhaustive coverage of every
animation, effect, frame rate or mod. The limitations above still apply.

`taa-perf` repeats Off/SMAA/TAA twice in the same frozen village setup used
below, native 1080p, projected shadows on, SSAO/CSM off, validation off. Each
mode settles for 14 seconds; the last four two-second profiler windows are
averaged. Existing frame timestamps include world draws, composition, overlays
and final gamma; new timestamps isolate the TAA resolve and are read only after
the existing frame fence, without a new wait.

| Mode | Whole-frame GPU ms, run 1 / run 2 | TAA resolve ms |
| --- | --- | --- |
| Off | 0.961 / 0.849 | — |
| SMAA | 1.369 / 1.426 | — |
| TAA | 1.543 / 1.546 | 0.351 / 0.351 |

Practical TAA overhead here is about 0.58-0.70 ms versus Off and 0.12-0.17 ms
versus SMAA, including motion output/world-target overhead, not just the resolve.
Automatic graphics clocks differed (Off 465/585 MHz, SMAA 705 MHz, TAA 825 MHz;
memory 810 or 5001 MHz). No clocks/power settings were forced. Consequently
these are practical elapsed GPU intervals, not fixed-clock throughput or a
guaranteed FPS gain/loss. Scaling increases both motion/history bandwidth and
resolve work. CPU per-instance/deformation bookkeeping also has a cost; the
renderer is not claimed to be GPU-limited in this scene.

### Earlier spatial-AA verification and performance

The results in this subsection predate the temporal implementation; references
to TAA rejection below describe that earlier executable, not the current mode.

Both Vulkan-enabled and GL33-only RelWithDebInfo builds pass. Each configuration
passes the existing `[vulkan-shape],[ShadowMath],[Conventions],[ShutdownOrder]`
suite (71 cases, 799 assertions). The driver-free Vulkan executable passes 309
checks, including new opt-in defaults, unsupported-mode retention, bounded
scale selection and odd-sized fractional extents. Its intentional teardown-error
fixture is not a live validation failure.

Live qualification used stock missions, isolated profiles, exact-window input,
1920x1080 and NVIDIA RTX 4060 Ti / driver 617.14. Developer commands positioned
the player and froze simulation for matched still comparisons; motion checks
then restored simulation. These are bounded movement sequences with screenshots
and position/ammunition evidence, not a continuous-video temporal-quality study.

| Stock scene | Live checks |
| --- | --- |
| Infantry / Ambush village | Off, FXAA, SMAA and all three MSAA counts; foliage, fences, rooftops, moving soldier; all four scales; SSAO + CSM; shoreline water/fog and camera transition |
| HMMWV | All modes driving (about 56 m total displacement); external and native cockpit; SMAA at 200% |
| Heavy Metal / M1 Abrams | All modes driving with SSAO + CSM; gunner optics and cannon fire, confirmed ammunition 24 to 23 |
| Ground Attack / Cobra | All modes in cockpit and actual flight (altitude above 30 m); external transition with SMAA at 150%; SSAO + CSM |
| Shadow Killer | All modes at night and under NVG, movement, native reticle/text/weapon; firing confirmed 30 to 29; MSAA 8x + 200% resize/minimize/restore |

Final spatial-code qualification runs `aa-q-infantry`, `aa-q-hmmwv`, `aa-q-tank`,
`aa-q-air` and `aa-q-night` all closed normally with exit 0 and **zero validation
errors or warnings**, including synchronization validation. The night and final
infantry runs also contain the final acquire-excluded GPU profiler. Earlier
`aa-final-infantry` includes map-label comparisons and SMAA + 150% resize/restore.
Local PNGs, action receipts, executable hashes and complete validation logs are
under ignored `build/shadow-live/aa-*`; they are not shipped as stock assets.

The final interface-name-only rebuild also passed `aa-menu-save` with profiling
disabled (the normal semaphore wait path): all modes in menus and moving stock
Ambush gameplay, briefing/map UI, and ordinary Save/Load with SMAA at 150%.
Loading restored `[8089.76,5153.36,0.00552177]` after movement; TAA and scale 99
requests left that active selection unchanged. Shutdown reported 19,671 frames,
zero validation errors/warnings and exit 0. Both final configurations launched
GL33 by default without `--render`; the new commands reported unsupported and
both exited normally (`aa-default-vkbuild`, `aa-default-glbuild`).

Visual review found reduced stair steps on rooftops, fences and silhouettes.
SMAA preserves more local texture contrast than FXAA; MSAA coverage smooths
cutout boundaries without applying a texture-wide post-filter. Fractional and
200% downsampling reduce fine-detail aliasing but cost more. HUD, map text,
cockpit instruments and weapon overlays remain sharp/native. No missing world,
stencil-shadow leak, broken water/NVG, or overlay filtering was observed in
these runs. This is not a claim of shimmer-free motion: spatial modes have no
history, and distant leaves/wires can still pop or sparkle. This earlier run
did not assess TAA ghosting.

The physical device supports every requested MSAA count. Unsupported-count
behavior is covered by the driver-free test (explicit rejection and previous
selection retention), **not** a second physical GPU or a forced driver failure.

GPU profiling is opt-in via the existing `CWR_VK_PROFILE` environment variable.
Two timestamps enclose the frame's GPU commands, including gamma. In profiling
mode only, the acquire-semaphore wait is moved to TOP_OF_PIPE so the starting
timestamp does not include waiting for the display's image. Results are read
after the existing frame fence, without an additional wait. Normal rendering
keeps its original late semaphore wait. This is an elapsed GPU command interval,
not an isolated shader-throughput measurement or a CPU/display frame time.

The matched village benchmark uses `[7088,6067,0]`, direction 90, external view,
16:00 clear weather, view distance 900, frozen simulation, projected shadows,
CSM off and SSAO off. Validation is disabled for timing. `aa-perf-infantry`
contains two 12-second sweeps with normal VSync. `aa-perf-unpaced` contains two
20-second sweeps requesting `triSetVsync 0`; despite the directory name and
mailbox selection, measured presentation remains around 162 FPS / 6.17 ms.
The configured CPU cap is 240, not the measured 162. No driver/global settings
or power plan were changed.

Timing statistics below use the second run, discarding the first two two-second
report windows after each mode/scale transition. Ranges are the retained window
means, not per-frame minima/maxima. Automatic GPU clocks varied from roughly
495 to 1950 MHz during sampling. **A fixed-clock incremental GPU cost is not
established by this experiment.** In particular, negative median differences
for lightweight modes must not be interpreted as AA speeding up rendering.

| Mode / scale | Median GPU ms | GPU window range ms | Median difference vs Off/100 ms (clock-confounded) |
| --- | ---: | ---: | ---: |
| Off / 100% | 1.763 | 1.416–1.803 | baseline |
| FXAA / 100% | 1.442 | 0.669–2.148 | -0.321; unresolved cost |
| SMAA 1x / 100% | 1.580 | 0.889–2.086 | -0.182; unresolved cost |
| MSAA 2x / 100% | 1.767 | 1.247–1.790 | +0.004; within variation |
| MSAA 4x / 100% | 1.868 | 1.219–1.869 | +0.106 |
| MSAA 8x / 100% | 1.998 | 1.453–2.019 | +0.236 |
| Off / 125% | 1.782 | 1.253–1.815 | +0.019; within variation |
| Off / 150% | 1.853 | 1.239–1.856 | +0.090 |
| Off / 200% | 1.980 | 1.363–1.981 | +0.217 |

Each row retains 16–18 report windows / 5,187–5,815 submitted frames. CPU/display
frame medians remain 6.173–6.178 ms across the table. These scene-specific results
are not a maximum-quality worst-case budget; combining MSAA, 200%, SSAO and CSM
was exercised for correctness, not exhaustively benchmarked. Reliable fixed-clock
incremental comparisons, other GPUs and a continuous-video motion study remain
unqualified.

## Optional Vulkan cascaded sunlight shadows (2026-10-11)

CSM remains **off by default**. The existing `triEnableShadowMaps` and
`triShadowSet*` developer commands now work with Vulkan; `triDisableShadowMaps`
restores the unchanged projected accumulator. No graphics menu or asset changes.

The backend consumes `Scene::RenderShadowMapDepthPass` / `ShadowCasterSet` and
the existing `BuildShadowCascadesTiered` results. Solid casters use front-face
culling; two-sided foliage/fences use the same 0.5 texture-alpha cutoff as GL33.
The existing draw/shadow LOD and caster-distance rules remain authoritative.
Each fence-retired frame owns a reusable D32 array (one to four layers), views,
framebuffers and descriptor. Solid/alpha streams upload once per frame through
the existing transient pages and are reused by every cascade.

An opt-in backend hook prepares the existing object list and depth maps before
landscape rendering. GL33 retains its original ordering. The depth pass suspends
the scene and resumes with color/depth/stencil LOAD operations, invalidating
the command cache. Native receivers use camera-relative world coordinates;
software-transformed receivers reconstruct them from clip W, projection and
camera rotation, including terrain. Sampling follows GL33's omni/frustum tier
selection, coverage fallthrough, quadratic cascade bias, 3x3 PCF, 15% transition
bands, distance/fog fade and sunlight strength. World-effect boundaries and
material flags exclude UI, sky and cockpit/weapon overlays. Maps reset each frame
and become inactive at night. Opt-in profiling reports cascade depth GPU time.

Verification: Vulkan and GL33-only builds, 56 focused cases / 738 assertions,
and driver-free Vulkan policy/command-cache checks pass. Both configurations
launch GL33 without a renderer override, with shadow maps disabled. GL33 renderer
files, stock assets and localization are unchanged. Software submissions without a shadow receiver keep
the original unlit path (including CSM OFF), rather than counting as native lit
draws or copying unnecessary receiver data.

Stock gameplay at 1920x1080 on the RTX 4060 Ti was exercised with exact-window
input and isolated profiles. Developer commands repositioned actors/set daylight;
these are rendering/gameplay checks, not completed mission playthroughs:

- Infantry village: walking, weapon fire, grenades (stock count 6 -> 4), moving
  soldier silhouettes, building receivers, foliage cutouts, map and pause.
- HMMWV: about 50 m of driving (42 km/h sampled), exterior/interior changes,
  recognizable vehicle shadows, fences and low-sun shadows (sun factor 0.82).
- Heavy Metal: tank movement, exterior/optics, cannon fire (24 -> 23 rounds;
  developer-assisted gunner seat change), smoke, CSM with SSAO, and live switching
  between four/2048 and two/1024 cascades.
- Ground Attack: stock Cobra cockpit/exterior, rotor startup and takeoff to
  about 39 m. HUD and cockpit overlays remain unshadowed; SSAO was also enabled.
- Shadow Killer: sunlight factor zero, no cascade passes, NVG, movement and
  HK fire (30 -> 29 rounds), CSM OFF/ON and SSAO.
- Normal menu -> Ambush -> save -> movement -> load restored the exact saved
  position with CSM still enabled. Water/shoreline with CSM+SSAO, resize,
  minimize/restore, and normal window close were checked separately.

All nine validated Vulkan sessions closed normally with zero Khronos core and
synchronization errors/warnings through teardown (57,649 submitted frames,
including the initial depth-pass development run). The final software fallback
adjustment was rechecked in the 2,500-frame tank firing/SSAO session. No new
water, cutout, smoke, gamma, specular, cockpit or command-cache regression was
observed; this is sampled coverage, not exhaustive certification.

Before/after captures, actions, launch hashes and validation logs are retained
locally under ignored `build/shadow-live/csm-*`. `csm-direct-{infantry,hmmwv,tank}`
contains repeated OFF/ON comparisons; `csm-{infantry,hmmwv,tank,night}-gl33`
contains GL33 shadow-map references. References are representative gameplay
views, not pixel-registered cross-renderer comparisons. Moving captures and
position/ammunition receipts accompany the static comparisons.

Performance: 1920x1080, RTX 4060 Ti, four 2048 cascades, 900 m view distance,
SSAO/validation off, frozen simulation and repeated 12 s OFF/ON phases. Values
are medians of 2 s profile windows (first three startup windows and mixed-toggle
windows excluded; 10-13 samples/mode). FIFO/display pacing limits these results;
depth GPU timestamps cover all cascades, **not** the complete frame GPU cost.

| Scene | Projected frame ms | CSM frame ms | CSM depth GPU ms | Geometry upload ms, OFF -> ON |
|---|---:|---:|---:|---:|
| Infantry village | 6.491 | 6.184 | 0.356 | 0.680 -> 0.441 |
| HMMWV road | 6.172 | 6.172 | 0.276 | 0.247 -> 0.249 |
| Heavy Metal, tank-mounted view | 6.173 | 6.172 | 0.407 | 0.109 -> 0.135 |

Steady-state transient allocations were zero. CSM removes projected draw/upload
work, explaining the village improvement; these are not uncapped throughput
claims. Two frame slots at four/2048 retain about 128 MiB of depth images.
Earlier `csm-perf-*` logs had managed stdout/stderr-pump stalls and are excluded:
repeating with direct file handles (`csm-direct-*`) removed those timing spikes.

Quality/readiness: usable as an opt-in developer enhancement, not a new default.
Keep GL33's four 2048 cascades, 0.00002 bias, 3x3 PCF, 40 m fade and existing
distance/LOD controls. No new scheduler or shadow LOD framework was justified.
Model silhouettes and alpha cutouts follow the shared caster system; terrain
receives but is not added as a caster. Low-detail caster silhouettes, fine
foliage self-shadow striping and finite-resolution shimmer remain visible in
both backends. Camera movement showed no severe cascade instability, but this
does not eliminate every LOD/cascade transition. Software TL reception modulates
its already-combined lighting, so it cannot isolate only the sunlight component
as precisely as a per-light shader. Sky, UI and cockpit/weapon overlays are
deliberately excluded. No claim of all-model/all-campaign coverage is made.

## Earlier backend qualification (before CSM)

Current status: Vulkan remains opt-in. Sustained mission-flow testing now covers
infantry, campaign transitions, tank driving/gunnery, helicopter flight/landing,
night combat and save/load. It exposed and fixed a combat crash from tracers
crossing the camera plane. See the final gameplay section for actual coverage,
assisted transitions and readiness limits; no genuine mission win is claimed.
Gameplay lighting/fog, projected shadows, ordered screen batching and composed
framebuffer gamma remain supported. The measured Infantry map result is
6.55 -> 47.49 FPS with 91.25% fewer screen draws. The dated milestones below are
historical; their earlier unsupported-feature lists are superseded by later work.

Repository migration: `vici1129/cwr-vulkan` is an independent clone with its own
`.git` directory and official `BohemiaInteractive/CWR` upstream. Its main starts
at official 3.05 (`ffc6183`) and replays only the six Vulkan commits from the
historical branch through `f88f733`; no Chinese localization history is included.
The historical Git branches are preserved in a verified local bundle/ZIP. The
linked CWR-RR worktree has been removed through Git after moving its ignored
game/build evidence into this independent checkout. Private tools/dependencies
are under `build/legacy-vulkan/tools/`; preserved old build/client outputs under
`build/legacy-vulkan/` and `dist/legacy-vulkan/`; stock game data and the isolated
renderer profile under `game-local/`. Active builds use `build/vulkan-local/`
and `build/gl33-local/`. No old CWR/CWR-RR folder is required.
Relocation checks: both configurations rebuilt from relocated tools/dependencies;
83 driver-free guards and 44 focused cases / 223 assertions passed. The stock
GL33 initialization check exited 0. The real RTX 4060 Ti indexed-triangle run
presented 3,548 frames, exited 0, and reported zero validation errors/warnings
through teardown (implicit layers disabled only for that process). This is a
diagnostic smoke test, not evidence of Vulkan game-world rendering support.
Historical runtime results below retain their original context; migration checks
are reported separately, not substituted for those artifacts.

`PoseidonVK` is an opt-in SDL3 Vulkan backend registered as `vk`. GL33 remains
the default and reference game renderer. Clear/present, indexed triangle and
native static textured Shape drawing work, including stock P3D models and a
WRP-derived scenery portion. Normal `--render vk` startup now visibly reaches
the menu over the stock intro world. Rendering remains experimental and unlit;
this is not a claim of playable missions. Latest verification is recorded below.

## Build and selection

Configure the normal Windows Clang preset with `-DCWR_HAS_VULKAN=ON` and an
installed/system Vulkan SDK. CMake uses `find_package(Vulkan REQUIRED)` and
`Vulkan::Vulkan`; no Vulkan dependency framework is vendored. The option defaults
to OFF, so normal GL33 builds do not require or link Vulkan. The indexed triangle
also requires the SDK's `glslangValidator`: CMake compiles the two GLSL sources to
embedded SPIR-V 1.0 at build time (no runtime shader compiler). SDL3's `vulkan`
vcpkg feature must be enabled; its Windows default otherwise rejects Vulkan windows.

```powershell
cmake --preset win-x64-clang-rwdi -DCWR_HAS_VULKAN=ON
cmake --build build/win-x64-clang-rwdi --target PoseidonVK PoseidonGL33 PoseidonTests PoseidonVKPolicyTests PoseidonGame
ctest --test-dir build/win-x64-clang-rwdi -R '^PoseidonVKPolicyTests$' --output-on-failure
```

The game clients (Game, GameDemo and Tetris) explicitly register the backend
when enabled. The existing `--render` option accepts `vk`; its default remains
`gl33`. Registration does not initialize SDL or enumerate physical devices.
Factory availability means the backend was compiled/registered, not that a
compatible driver has been verified. Creation reports errors and returns null.
Vulkan has lower factory priority than Dummy, so automatic selection preserves
the existing GL33-to-Dummy fallback instead of selecting an unfinished renderer.

Use the explicit renderer-only smoke mode for live testing:

```powershell
PoseidonGame.exe -C <GOG-Remastered-3.05-directory> --render vk --vulkan-smoke triangle --vk-validation --width 800 --height 600 --timeout 30
```

`--vulkan-smoke clear` tests the original clear/present path; `--vulkan-smoke shape`
constructs a real engine Shape and uses the production geometry virtuals. All modes use the
existing backend factory and SDL window/event loop, stop before game-content and
audio initialization, never fall back to GL33, and exercise normal destruction
on timeout or window close. A disabled Vulkan build rejects the option. Normal
game startup with `--render vk` still encounters explicit unsupported calls.
Use the existing `POSEIDON_USER_DIR`, `POSEIDON_CACHE_DIR` and `POSEIDON_TEMP_DIR`
redirects to keep test writes outside the retail content directory.

## Ownership and frame lifecycle

- `EngineVK` inherits the existing `Engine` directly and implements its window,
  frame and capability seams. It owns an SDL Vulkan window and balances its
  video-subsystem reference. It uses the existing window placement resolver and
  the shared SDL input/focus/fullscreen event pump.
- `VulkanContext` is private to this backend. It owns the instance, SDL-created
  surface, selected device, queues, command pool and all swapchain/frame objects.
  It needs Vulkan headers and the loader, but no engine, SDL or asset types.
- `SwapchainPolicy.hpp` contains only driver-free surface/queue decisions.
- `EngineVK_Unsupported.cpp` centralizes explicit failures for unfinished
  resource and rendering entry points. It does not inherit Dummy's no-op draws.

Instance creation uses SDL's required surface extensions, optional debug-utils
object names and optional portability enumeration. `--vk-validation` requires the
Khronos validation layer; its absence fails initialization with an actionable
error, rather than allowing an unvalidated smoke-test success. Synchronization
validation is enabled when its layer extension is available. Warning/error callbacks
include their message IDs. Loader diagnostics are distinguished from validation
messages. A Vulkan 1.0 device is sufficient; portability subset is enabled when
advertised, with its properties2 instance dependency retained.

Device selection requires graphics and presentation queues, the swapchain
extension, usable surface formats/present modes, and color-attachment image
usage. Discrete GPUs are preferred over integrated GPUs. A combined graphics
and present family is preferred; distinct families use concurrent image sharing.

Swapchain creation chooses a supported format, clamped/fixed surface extent,
bounded image count and supported composite-alpha mode. FIFO is the default;
requested off/adaptive intervals use supported alternatives with FIFO fallback.
Each image has a view, clear-pass framebuffer and presentation semaphore.

Two frames in flight each have a command buffer, acquire semaphore and initially
signaled submit fence. `InitDraw` checks SDL's drawable size, skips minimized or
zero-sized windows, recreates when needed, waits for the frame fence and acquires
an image with a finite timeout. Out-of-date acquire defers recreation without
resetting the fence. Suboptimal acquire is consumed and presented before rebuild.

The render pass transitions an undefined image into color-attachment use and
finally into presentation layout. Every frame begins with opaque black;
`Clear` records a full color clear using the engine's packed RGB convention.
There is no retained framebuffer-content contract. Each swapchain image now has
a D32 (D16 fallback) depth attachment cleared to 1 every frame; `clearZ` also
supports an explicit depth clear.

`FinishDraw` closes and submits the command buffer, then presents. The submit
waits at color-attachment output. Fences reset only immediately before submit.
Present-wait semaphores are indexed by acquired image rather than frame index,
as described in the [Khronos semaphore reuse guide](https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html).
Out-of-date/suboptimal present requests recreation. Other Vulkan errors stop
the backend and release resources; failed recording/submission is not retried
with potentially stranded synchronization objects.

Resize and fullscreen/pixel-size events request deferred recreation. Recreation
waits for device idle, destroys the old image resources, and rebuilds from fresh
surface capabilities. Minimized windows retain old resources until drawable.
Shutdown waits idle, destroys the triangle pipeline before the render pass,
then framebuffers/depth attachments/views/semaphores and swapchain, immutable geometry buffers,
pipeline layout, frame synchronization, command pool, device, surface, instance,
SDL window and the backend's SDL video reference. Partial initialization cleanup
and repeated shutdown are supported. This uses the conventional core-Vulkan
idle-based swapchain teardown; presentation fences/maintenance extensions and
non-blocking swapchain retirement are outside this stage.

The shared `SDLEventWindow.hpp` moved from PoseidonGL33 to
`Poseidon/Graphics/Shared`, retaining its existing behavior and adding drawable
pixel-size events for DPI changes. The existing source-audit test follows the move.
Vulkan uses SDL's fullscreen path and does not initialize the GL ImGui renderer.

## Deliberately unsupported

Immutable opaque untextured Shape/TL submission and depth testing are implemented.
Dynamic/animated geometry, 2D/UI/font rendering, texture bank/loading/upload,
material binding, depth bias, gamma correction and shadow rendering remain unsupported.
Required draw/resource methods throw an actionable `std::logic_error`; neutral
gamma/bias requests are allowed, and unsupported capabilities report false.
Optional engine features otherwise retain the base interface's unsupported
defaults. Monitor enumeration and SDL display-mode requests are implemented;
monitor switching and readback are not.

No GL33 renderer replacement, assets, localization, modern shading, upscaling or
renderer redesign is part of this change.

## Indexed triangle and donor reuse

`EngineVK::DrawTestPattern("triangle")` is allowed only with
`--vulkan-smoke triangle`. Three immutable position/color vertices and uint16
indices **2,0,1** are uploaded into coherent host-visible buffers, bound to a
minimal graphics pipeline and drawn with `vkCmdDrawIndexed`. Dynamic viewport,
scissor and aspect compensation follow the swapchain extent. Both frame slots
share read-only geometry; resize rebuilds the pipeline against the new render
pass, retaining buffers/layout. Device-idle teardown releases all resources.

Selectively adapted from **Deus-Ex (koosoli)**, `koosoli/PoseidonVK` commit
`7523bd5d3afcc13f13e51050d7c717ca103a7c19`: the host-visible allocation/map/unmap
pattern in `BufferVK.hpp/.cpp` and `bootstrap_triangle.vert.glsl` /
`bootstrap_triangle.frag.glsl`. These remain GPL-3.0-or-later with the additional
Bohemia terms in `LICENSE`. Buffer ownership/bounds/failure cleanup were tightened;
the shader UBO/descriptors were removed in favor of 32-byte push constants. The
pipeline, smoke loop and integration adapt to our existing context, frame slots
and per-image present synchronization; donor texture/mesh/framework code was not
incorporated. Official 3.05 renderer seams and working lifecycle code are retained.

The backend-private `BufferVK` helper accepts buffer size/usage independently of
the triangle. It is the minimal preparation for `Engine::CreateVertexBuffer` and
`DrawSectionTL`: GL33's existing `Shape` path fan-triangulates polygons and tracks
per-section index ranges. The Shape milestones below implement those production
engine entry points using the same packing and polygon-fan conventions.

## Live verification (2026-10-09)

- GPU: NVIDIA GeForce RTX 4060 Ti, driver 617.14 (Windows 32.0.16.1714), Vulkan
  1.4.351; installed SDK/Khronos validation layer 1.4.350.0. API requests remain 1.0.
- Used `PoseidonGame` against the authorized local GOG Remastered 3.05 content
  directory with user/cache/temp redirected under ignored `build/vulkan-live`.
  The renderer-only diagnostic does not claim game-world Vulkan compatibility.
- First live clear attempt exposed SDL3's missing Vulkan build feature. After
  enabling it, clear/present and resizing worked. Normal close then exposed an
  inactive progress query unnecessarily calling the unsupported texture bank;
  a guarded short-circuit fixes this shared startup/close seam, with regression test.
- Clear retest: two resize recreations, 230 submitted frames, normal shutdown,
  zero validation errors/warnings. Triangle: visibly correct at 800x600,
  1104x801 and 704x501, also after minimize/restore; 11,038 submitted frames,
  normal close exit 0, zero core/synchronization validation errors or warnings
  including resource destruction. Pipeline recreation occurred for every new
  swapchain; geometry was uploaded once.
- One external loader error remains: a stale Epic Online Services overlay JSON
  path. Implicit overlays were disabled only for the test processes; their
  disabled-layer loader warnings are not validation warnings. No machine overlay
  installation or registry was changed.
- Vulkan-enabled and GL33-only Game/GameDemo/Tetris, GL33/Dummy and the Poseidon
  test target build successfully. Driver-free policy CTest passes all **83** checks (original
  42 plus buffer allocation/map/bind failure cleanup, bounds/lifetime and indexed
  layout/draw guards). Factory: 17 cases in each configuration (ON: 94 assertions,
  OFF: 91); window/close regression: 3 cases / 10 assertions in each. No driver is
  called by these tests.
- Final rebuilt clear/triangle timeout checks both exit 0 with zero validation
  errors/warnings, including teardown. Missing explicit `--render vk` and a
  Vulkan-disabled build both reject the diagnostic with exit 1. Default GL33
  `--check --nosound` exits 0 against the same content; the existing missing
  `biscamel\icamel2.paa` warning is not changed. Import inspection confirms no
  `vulkan-1.dll` dependency in the GL33-only executable.
- Game-local content, localization and resources retain identical before/after
  file-count/size/timestamp snapshots. No assets or distribution settings changed.
  Formatting and `git diff --check` pass; no dependency blocker remains.

Local evidence: `build/vulkan-live/clear/retest.{stdout,stderr}.log` and
`build/vulkan-live/triangle/{stdout,stderr}.log`, with `initial.png`, `resized.png`
and `restored.png`. Other GPUs/platforms, normal Vulkan gameplay and non-blocking
swapchain retirement are not validated by this milestone.

Follow-up error-path fix: mapped pointers become owned only after `vkMapMemory`
succeeds. Terminal backend shutdown returns the final validation count, so the
smoke verdict includes destruction-time errors. Driver-free regressions cover a
failed map with non-null output and an injected teardown-only validation error;
both build configurations and their focused tests pass. The GPU results above
precede this follow-up; no new live run was performed for these error-path fixes.

Explicit-validation follow-up: a missing requested Khronos layer fails before
instance creation. Driver-free tests cover the rejection/error message, enabling
an available requested layer, and unchanged layer-independent operation without
the flag. CLI help now describes the strict requirement; no new live run was performed.

Muted-logging follow-up: smoke success also requires the backend's stored failure
flag to be clear, sampled after `StopAll`/terminal shutdown and before destruction.
Two driver-free verdict tests use real `critical`/`off` logging filters and retain
normal close/timeout, frame, validation and logged-error guards (9 assertions).
No logging subsystem, rendering path or normal window-close behavior was changed;
no new live run was performed.

Shape milestone 1 (2026-10-10): `CreateVertexBuffer` accepts immutable `VBStatic`
and `VBBigDiscardable` Shapes. Position/negated-normal/UV packing, polygon fans,
`VertexIndex` indices and section ranges follow GL33. Dynamic/dirty updates reject
explicitly. GPU allocations are shared with referencing frame slots and released
before device destruction even if their Shape survives shutdown. Vulkan and
GL33-only builds pass; extraction/range rejection tests pass (2 cases, 13
assertions), as do the existing 83 driver-free guards. Shape drawing is still
unsupported at this intermediate milestone; GPU upload is built but not yet
live-exercised.

Shape milestone 2 (2026-10-10): `PrepareMeshTL` reads the scene camera and
model-to-world transform using GL33's camera-relative convention; `BeginMeshTL`,
`DrawSectionTL`, and `EndMeshTL` now record actual indexed mesh draws. The opaque
untextured shader uses the engine projection (0..1 depth, Vulkan Y inversion)
and white or `IsColored` scene constant color. Each swapchain image has a depth
attachment; resize recreates depth/framebuffers/pipelines while retaining mesh
buffers. Textures/materials, animated meshes, blending and unsupported render
flags still reject explicitly. Focused extraction/transform tests pass (3 cases,
19 assertions); 85 driver-free guards pass. The existing live triangle path also
presents correctly with the new render pass (380 frames, zero validation errors
or warnings through shutdown). Live Shape integration follows in milestone 3;
this intermediate commit does not yet claim a visible Shape.

## Engine Shape live integration (2026-10-10)

`--render vk --vulkan-smoke shape --vk-validation` now renders a real `Shape`
through `ConvertToVBuffer` / `CreateVertexBuffer`, `PrepareMeshTL`, `BeginMeshTL`,
`DrawSectionTL` and `EndMeshTL`. The fixture has 8 vertices, 6 quad sections and
36 triangulated indices; two instances use separate rotating model matrices and
a translated/oriented engine camera. A combined section range and nonzero index
offsets are exercised. The farther instance is submitted after the nearer one,
so overlapping visibility depends on depth testing. Color is an opaque scene
constant; normals/UVs retain GL33 packing but are not shaded or sampled. This is
an engine mesh integration check, not normal gameplay or a hardcoded GPU triangle.

Real RTX 4060 Ti verification with Khronos core/synchronization validation:

- Captured and inspected distinct rotating poses at 800x600, 1104x801, 704x501,
  and after minimize/restore. Geometry, section colors, perspective and depth
  occlusion were visible. Resize rebuilt depth/framebuffers/pipelines correctly.
- Released the Shape's vertex-buffer owner while a frame was still recording,
  then re-uploaded next frame. In-flight references kept allocations alive;
  output and validation remained clean.
- The final 22-second timeout run submitted 3,394 / presented 3,391 frames,
  exited 0, and reported zero validation errors/warnings through destruction.
  The final normal window-close run submitted 1,216 / presented 1,213 frames,
  exited 0 and also reported zero validation errors/warnings. Counts differ by
  three during resize/restore; no backend failure was reported. External stale Epic overlay loader diagnostics
  remain; implicit layers were disabled only for the test processes.
- Vulkan and GL33-only builds pass. Focused Shape/factory/smoke/window tests:
  15 cases in each configuration (ON: 93 assertions; OFF: 90). The new Shape
  cases contribute 4 cases / 28 assertions. Vulkan's registered CTest passes
  1/1, with 85 driver-free guards. Root CTest enumeration is blocked by an
  ungenerated Studio test-list file in this targeted build; broad suites were
  not rerun.
- GL33 remains the default. Its initialization check exits 0; default-renderer
  startup initializes the world and normal closure exits 0. The live menu-window
  captures were black, including with the preserved pre-task GL33 executable,
  so visible GL33 menu/gameplay is not claimed by this check. Its renderer code
  was unchanged. Stock game data/resources metadata snapshots were preserved.

Evidence (ignored): `build/shape-live/lifetime-timeout/{result.json,stdout.log,
stderr.log,initial.png,rotated.png,wide.png,small.png,restored.png}`;
normal-close evidence under `build/shape-live/lifetime-close/`; GL33 initialization,
current launch and pre-task comparison under `build/shape-live/gl33-*`.

Remaining limits: immutable static buffers only; opaque full-viewport untextured
draws with default depth state and optional `IsColored`/`DisableSun` flags;
no textures/materials, lights, blending, animation, shadows or UI. Unsupported
flags/lighting and dirty/dynamic updates fail explicitly. `Shape::Draw` still
reaches unsupported per-section material/texture preparation. The smallest next
step is the neutral untextured section-preparation path needed for `Shape::Draw`
itself to consume these buffers; textured game geometry follows separately.

## Historical driver-free verification (initial foundation)

`PoseidonVKPolicyTests` exercises queue selection, extent/image-count bounds,
format/present/composite-alpha fallback, recreation classification and empty
context/invalid-initialization guards. Its 37 checks make no Vulkan API call,
create no instance or SDL window, and enumerate no physical devices. It links
the loader only to compile/link the real lifecycle implementation.

The existing Catch2 graphics/factory tests also test actual backend registration,
priority, idempotence, enum/code lookup and unavailable-backend handling. Those
tests use sentinel creation callbacks and never construct a Vulkan engine.

Verification on Windows used the installed Vulkan SDK 1.4.350.0, MSVC 14.44,
and worktree-local LLVM 21.1.8, CMake 4.4.4, Ninja 1.13.2 and vcpkg. The latter
tools and the unchanged manifest dependencies were installed under ignored
`build/tools` after dependency installation was authorized; nothing is vendored
into the backend. There is no remaining dependency blocker for this build.

- With Vulkan ON: PoseidonVK, PoseidonGL33 (and Dummy in Poseidon), all three
  client executables and all seven unit-test executables built and linked.
- With Vulkan OFF: GL33/Dummy, all three clients and all seven unit-test
  executables built and linked. Import inspection confirms that the GL33-only
  game executable does not import `vulkan-1.dll`; the Vulkan-enabled one does.
- Driver-free policy CTest: 1/1 passed, all 37 checks. The context and tests also
  compiled with MSVC `/W4 /WX`, and for x86 as objects only.
- Graphics/factory/input selection: 348 test cases passed with Vulkan ON and
  OFF. Hidden fixture-generation tests are excluded from this selection. The
  registration test uses real Dummy/Vulkan descriptors, but models GL33 with a
  sentinel because this harness stubs its registration. It also checks that the
  CLI default is still `gl33` and the existing Auto enum value is unchanged.
- Broader suites (excluding external game-data tags): Core 812/812 and
  Foundation 475/475 passed in both configurations. Poseidon 2450/2451 passed
  in both; the unchanged `ModArchive::Unpack rejects a valid archive wrapping a
  corrupt PBO` test throws `No mapping for the Unicode character exists in the
  target multi-byte code page.` No unrelated archive/encoding fix is included.
  Foundation's intentional crash-handler child test emits a diagnostic dump;
  the parent suite passes.
- Vulkan-enabled application suites: Server 7/7, Evaluator 95/95, Tetris 9/9,
  headless software-rendered Studio 50/50 passed.
- New C++ files pass clang-format checks; `git diff --check` passes.

At the initial foundation stage, legacy tests using absolute `/tmp` paths ran through a temporary drive alias
rooted in this worktree's ignored build directory; user/cache/temp directories
were also redirected there. No game executable was launched and no Vulkan
instance, physical device or live clear/present frame was tested. Before/after
file-count/size/timestamp snapshots of `game-local/Remastered`, localization and
resources were identical. Physical driver behavior was unverified at that stage;
the live results above supersede that limitation. These broader historical suites
were not all rerun for the indexed-triangle change.

## Native Shape sections (2026-10-10)

`--render vk --vulkan-smoke shape --vk-validation` now invokes ordinary
`Shape::Draw`, including `ShapeSection::PrepareTL`, neutral mip preparation and
basic unlit diffuse material colors. Textured requests remain explicit errors.
Individual mesh destruction no longer waits for device idle: referencing frames
retain buffers until their fences signal; terminal device teardown still drains.
RTX 4060 Ti live captures (`build/shape-live/native-close`) visibly confirmed
rotation, section colors and depth overlap across resize/minimize/restore.
Normal window close exited 0, 1134 submissions, zero validation errors/warnings
through teardown. Focused tests: 12 cases/83 assertions; policy guards: 85.

## Stock texture uploads (2026-10-10)

`--vulkan-smoke texture` mounts stock `dta/data.pbo` with the existing bank API,
loads `data\domek1_front_okna.pac` and `data\domek1_side.pac` through
`QIFStreamB` and `DecodePAABuffer`, and samples cached RGBA images on native
Shapes. No asset extraction or alternate decoder. Uploads use a staging buffer,
one-time command and fence, then shader-read layout; images/descriptors are held
by referencing frames. Only the original top mip is supported for now.
Two visibly distinct textures, UV orientation and section switching were inspected
in `build/shape-live/texture-close`, including resize/minimize/restore. Exit 0,
1081 submissions, zero validation errors/warnings through shutdown. Decoder and
Shape tests: 26 cases/4219 assertions; policy guards: 85. Alpha textures currently
fail explicitly; stock `domek2_side.paa` decoded successfully as cutout but is not
yet a supported draw. This is the next section-pipeline milestone.

## Textured sections (2026-10-10)

The texture fixture now draws stock PAC opaque and PAA cutout sections through
`Shape::Draw`. RGBA alpha is classified by the engine decoder; cutout discards
below 0.5 while retaining depth writes. A source-alpha blend variant disables
depth writes, with ordering left to the existing scene/section-class passes.
Clamp U/V, repeat and point/linear section samplers are supported. Other effects
and depth overrides remain errors. No lighting or texture-streaming framework.
`build/shape-live/cutout-close`: two textured native Shapes visually inspected
with depth overlap, correct UV orientation and window holes; resize/restore and
normal close passed, 1170 submissions, zero validation errors/warnings. Focused
decoder/Shape tests: 26 cases/4221 assertions; policy: 85 guards. Blend rendering
has not yet been visually verified against a stock translucent asset.

## Real stock P3D models (2026-10-10)

`--render vk --vulkan-smoke models --vk-validation` reads normal game configuration,
mounts stock Data/Data3D PBOs, uses `ModelCache` ODOL loading and `ShapeAdapter`,
and draws `data3d\dum_mesto.p3d` (561 display vertices, 9 sections) and
`data3d\jeep.p3d` (3438 vertices, 50 sections) through native `Shape::Draw` with
the existing `Object` material provider. No renderer-specific model loader.
Diffuse/emissive material modulation is unlit; stock specular fields do not add
a specular lobe. Opaque/cutout sections precede back-to-front blend objects using
the engine section-class filter. Crew/proxy geometry, animation and lighting
are not demonstrated. Arrows move the camera, PageUp/Down elevate, A/D turn.

RTX captures in `build/shape-live/models-close` and `models-scan-close` were
inspected: recognizable building and Jeep, original UVs/section textures,
glass/cutout visibility, depth, resize and restore. Camera movement was visibly
verified using bounded scan-code input (virtual-key-only injection did not
reach SDL). Normal close exited 0: 1440 submissions, zero validation errors or
warnings. Timed 22-second run exited 0: 3321 submissions, zero validation issues
through teardown. Focused model/adapter/Shape tests: 28 cases/126 assertions.
These are real models in an integration fixture, not a loaded game world.

## Stock-world scenery portion (2026-10-10)

`--vulkan-smoke world` reads stock `worlds\eden.wrp` with the same `WrpReader`
used by Landscape's OPRW loader. Four nearby static buildings are selected from
its 56,740 source objects, retaining original WRP matrices and source IDs
16952, 16967, 17061 and 17059. Models: `dum_mesto`, `dumruina`, `dum_rasovna`.
No manually arranged replacements, renderer-specific asset loader, terrain,
mission simulation or gameplay. This is a scenery-only real-content integration
route, not success of the normal Landscape draw flow.
RTX screenshots in `build/shape-live/eden-first` visibly show the stock building
cluster with original textures/placement and depth. Resize/restore/normal close
passed; exit 0, 1144 submissions, zero validation errors/warnings through shutdown.

Normal `--render vk --vk-validation` was attempted without smoke. The first
blocker was configured gamma (now implemented in the fragment shader); the next
is `EngineVK::Draw2D`, called by `ProgressSystem::Draw` during world initialization.
The run did not reach a menu/world. The next practical step is minimal textured
2D primitives for that existing progress/menu flow, not a new UI framework.

## Normal menu and intro world (2026-10-10)

The earlier startup blockers above are superseded. `Draw2D`, pixel/absolute
polygons and GL33-style textured 2D lines now share the existing Vulkan upload,
descriptor, frame-fence and shader path. Reciprocal-W, vertex ARGB, UVs, clipping,
alpha blending and depth are retained. Existing FreeType atlas creation/update
uses RGBA textures; the engine's GL-only atlas validity cast was corrected.
No font system was replaced. Texture interpolation uses decoded engine pixels,
keeps one cached result per source pair, and retires changed images at frame
fences. Missing stock references return null with a warning, matching GL33.
Eight sampling states are shared device-wide: stock preloading exposed and fixed
the original per-image sampler allocation-limit violation.

Software `FaceArray` submission now consumes the engine's already transformed,
lit and clipped TL vertices. Dynamic vertex-buffer requests decline hardware
storage and use this existing engine route; immutable GPU Shapes remain native.
Unused clipped vertices are not converted. Sky and animated texture frames use
the original scene/texture animation systems. Terrain detail uses GL33's 32x UV
and alpha modulation; stock water's specular texture uses its decoded bump sample
and sun direction. Depth bias reuses `ZBiasMath`/projection conversion. Native
materials are intentionally unlit diffuse/emissive, not full light-list rendering.
Vulkan explicitly advertises no projected-shadow support; Scene does not request
that optional pass. Unsupported shadow commands still fail, and GL33's capability
defaults to true. Other unsupported effects remain errors, not successful no-ops.

Normal startup without **any** smoke flag was captured and inspected on the RTX
4060 Ti in `build/shape-live/normal-restore`: recognizable stock intro vehicles,
terrain, original logo, menu labels and UI lines. Wide/small resize and
minimize/restore passed after fixing no-acquired-frame mesh handling. Normal
window close exited 0, 84 submissions/82 presentations, zero Khronos core or
synchronization validation errors/warnings through shutdown. Focused decoder,
Shape/screen, model/adapter and WRP tests: 55 passed, one archived external-data
case skipped, 4355 assertions passed; driver-free policy/lifetime guards: 85.

Limitations: original top mip only, no streaming/eviction, native lighting/fog
approximation, no projected/shadow-map effects, decals or 3D line/point effects.
Normal menu/intro rendering is demonstrated; see the stock mission check below.

## Final integration checks (2026-10-10)

Normal `--render vk --vk-validation --test-mission <stock Missions/01TakeTheCar.ABEL>`
uses the existing isolated mission-test staging and normal simulation/Scene path,
without any Vulkan smoke flag. RTX captures in `build/shape-live/stock-mission-input`
were inspected: textured terrain, town buildings, cutout trees, sky, first-person
M16 and HUD. Bounded Right/Up input visibly turned/moved the player. Wide/small
resize, minimize/restore and normal close passed: 98 submissions/97 presentations,
exit 0, zero Khronos core/synchronization validation errors or warnings. This is
real mission rendering and basic movement, not full campaign/combat qualification.
Radio text contains replacement glyphs; its encoding/font path remains unresolved.

Normal startup's previously ignored `--timeout` now closes the ordinary main loop
cleanly (default zero remains unlimited). A 22-second menu/intro run in
`build/shape-live/normal-timed-fixed` exited 0 after resize/restore, 194/193 frames,
zero validation issues through shutdown. Final triangle, textured native Shape
and real-model smoke regressions also passed visually, including lifecycle checks:
1189, 1183 and 1427 submissions respectively, zero validation issues. The loader
still reports a stale installed EOS overlay manifest; disabled implicit overlay
messages are separate from Khronos validation and no overlay registry was changed.

Vulkan-enabled and GL33-only builds passed. Final focused asset/geometry/factory
tests: 63 passed, one archived external-data WRP case skipped, 4410 assertions;
font/shutdown tests: 18 cases/327 assertions; policy guards: 85. GL33-only factory
and Shape tests: 13 cases/94 assertions. Default GL33 checks passed in both builds;
the GL33-only normal menu/intro was visibly inspected and closed cleanly in
`build/shape-live/final-gl33-menu`. Stock game/resources metadata hashes remain
unchanged; all profiles, staged missions and screenshots are under ignored build
directories. Next useful rendering improvement: original mip-chain upload/sampling
to reduce visible distant-texture aliasing, then broader mission/effect coverage.

## Gameplay improvement: screen states (2026-10-10)

Screen draws now use four independent depth/blend pipeline keys, removing the
no-depth opaque/blend collision regardless of creation order. Six focused Shape
cases/46 assertions passed. RTX menu and Take the Car captures under
`build/gameplay-live/state-*` were inspected for translucent UI/HUD, cutout
foliage and software geometry; resize/restore and close passed, 81/71 submissions,
zero Khronos core/synchronization validation issues through shutdown.

Original stored PAA/PAC mip chains are now exposed by `DecodePAAMipChainBuffer`,
using the same palette/PacLevelMem traversal and RGBA conversion as the unchanged
top-level API. Sequential and OFFS-indexed sources retain valid shorter chains.
Synthetic size/order/alpha/short-chain tests and actual bank-backed `domek1_front_okna`,
`domek2_side`, `detail_dx`, Abel `rwn` and `s3` checks passed: 24 cases/4318 assertions.
Top-only Vulkan texture regression was visually checked with resize/restore/close,
zero validation issues. GPU mip upload follows separately.

Vulkan now uploads every decoded stored level in one staging allocation/copy
submission, transitions the complete chain, and exposes all levels in its image
view. Shared samplers use trilinear filtering, or nearest texel/mip for explicit
point sampling; clamp/repeat remain unchanged. Image-view bounds handle shorter
chains. Dynamic font/UI and interpolated textures retain their single-level path;
alpha classification still uses the original top level, avoiding reclassification
of cutout assets from lower-mip edge alpha. Texture metadata/CPU pixels expose
the original levels too. No generated mips, streaming or cache redesign.
RTX `mips-menu`, `mips-mission`, `mips-models` captures were inspected: distant
terrain speckle is clearly reduced against `state-menu`, original building/road
textures and cutout foliage remain visible, HUD/UI still blend. Movement,
resize/minimize/restore and close passed with zero validation issues through
shutdown (menu 83 and mission 95 submissions). Focused decoder/stock/Shape tests:
30 cases/4364 assertions; policy CTest passes. Both build configurations passed.

Transient profiling (`CWR_VK_PROFILE=1`) measures wall frame intervals/p95, CPU
recording/upload/retirement and fence/acquire/present waits separately. With
Khronos validation enabled, matched intro intervals at about 353 submissions/frame
improved from 117.3 ms / 8.5 FPS to 6.45 ms / 155 FPS; buffer allocations/frame
fell from 706 to zero after warm-up, geometry uploads from 33.7 to 0.20 ms and
retirement from 72.6 to 0.001 ms. GPU execution time is not timestamp-profiled.
Take the Car before: 92-179 ms/frame, 502-878 allocations/frame, 24-43 ms upload;
after: representative settled intervals 10.7 ms / 93 FPS, zero allocations and
0.032 ms uploads, despite about 570 transient submissions/frame. Different mission
views are not a controlled FPS comparison. Settled static textures do not reupload;
occasional font/interpolated image updates remain real uploads/fence waits.
Two frame slots now retain mapped 4 MiB vertex/1 MiB index pages. A completed slot
fence resets cursors; overflow adds a page without replacing recorded buffers.
Immutable P3D allocations are unchanged. Offset-copy guards add two focused checks
(policy total 87). Sustained intro before/after captures, mission movement, resize,
restore, close and timed exit were inspected with zero validation issues. The
first requested 60-second stationary mission exited cleanly after about 30 s
(reason not established);
it supplied profiling but did not pass that duration gate. A 40-second sustained
intro and a 25-second timed mission passed separately. Evidence: `profile-*`.

## Gameplay effects and integration (2026-10-10)

Real firing exposed unsupported transformed 3D lines and projected decals.
Both now reuse the existing textured-quad path, retaining engine colors, UVs,
reciprocal-W and depth. Lines match GL33's three-pixel ribbon; billboards use
GPU clipping. Screen polygons select opaque/cutout/blended state from the
texture and section flags rather than always blending. Weather can restrict
a stored sky chain to its finest level; old images remain fence-owned.
Bank-backed mip-limit regression checks cover five real textures.
Blended billboards reject only near-zero alpha, preserving alpha-fog fades.

The radio replacement glyphs were valid localized UTF-8, not a Vulkan encoding
error. On Windows, missing glyphs may use the installed Microsoft YaHei face
through the existing FreeType atlas. Latin metrics remain unchanged; no fonts,
localization or game assets were edited.

RTX 4060 Ti normal Vulkan startup and stock Take the Car, Infantry and Heavy
Metal were exercised with exact-window input: movement/rotation, optics,
firing, weapon mode/grenade selection, reload input, map and pause/resume.
Take the Car ammunition decreased; Heavy Metal grenade count fell 6 to 5 and
its explosion dust/smoke billboards were visually inspected. Infantry and
Heavy Metal each passed a 60-second sustained interval, resize/minimize/restore
and normal close, with 10211/6598 submitted frames and zero Khronos core or
synchronization validation errors/warnings through shutdown. Settled intervals
were about 7.5 ms / 133 FPS and 12.5 ms / 80 FPS respectively, validation on;
these are sampled scenes, not whole-mission benchmarks. Evidence is under
`build/gameplay-live/{takecar-effects-decals,infantry-effects,heavy-metal-sky-fixed}`.

Final checks: Infantry pause-menu Abort was selected with keyboard input and
exited cleanly (3148 submissions); a final Heavy Metal grenade/resize/restore
run reached its 70-second timed exit (5860 submissions), both zero validation
issues. Normal Vulkan menu/intro timed exit passed too. Both build configurations
pass 49 focused cases (ON 4745 / OFF 4700 assertions) and Vulkan policy CTest.
Default GL33 visibly launched the stock menu in both executables and closed
normally. Stock-data/resource file counts, bytes and metadata hashes are unchanged.
Final evidence: `infantry-abort-final`, `heavy-metal-alpha-final`, `final-*`.

Native Shapes still use approximate unlit diffuse/emissive color and no native
distance fog; shadows, points and full material/effect parity are not implemented.
No campaign or complete mission was finished. The highest-value next rendering
milestone is native Shape lighting/fog using the existing engine inputs.

## Lighting/fog follow-up (2026-10-10)

Software TL now uses the same near-zero alpha rejection as 2D for blended
effects, retaining the opaque cutout threshold only for cutouts. NoZWrite is
independent of blending: opaque depth-read-only screen pipelines have their
own key. RTX Heavy Metal movement, firing/grenade, map/pause, resize/restore
and close were inspected (`build/lighting-live/m1-baseline`), zero validation
issues through shutdown. Shape state tests and policy CTest pass.

Native Shapes now use GL33's sun Direction/Diffuse/Ambient and material
ambient/diffuse/forcedDiffuse/emissive calculation, including DisableSun.
Packed normals retain GL33's negation; inverse-transpose world normals handle
rotation and nonuniform scale. Software TL colors are not lit again. Small
mapped uniform pages are frame-fenced, retained and reused; no synchronous or
per-section buffer allocation. Heavy Metal late-afternoon and Take the Car noon
captures were inspected, with close/resize/restore and zero validation issues
(2178/1236 submissions). A matched GL33 Heavy Metal capture has closely matching
terrain, foliage and weapon brightness/colors. Seven focused cases/58 assertions
and policy CTest pass. Evidence: `build/lighting-live/{m2-*,reference-heavy}`.

Native distance fog now uses the scene's weather/view-distance start/end range
and current fog color, with camera-relative distance and FogDisabled/NoDropdown
honored. Software TL carries its existing Fog8 visibility instead of recomputing
distance; IsAlphaFog effects retain their existing opacity fade, without a second
RGB fog mix. HUD/ordinary 2D remain unfogged. Shared fragment fog is applied before
gamma. Separate native/screen constant caches reuse frame-fenced uniform slices.
Ninjas dawn/fog gameplay and Convoy's distant hills, trees, road and vehicle cab
were inspected on RTX, with resize/minimize/restore and clean close (1594 and
692 submissions, zero validation issues). Ninjas GL33 reference brightness and
colors remain close. Eight focused cases/66 assertions and policy CTest pass.
Evidence: `build/lighting-live/{m3-fog,m3-convoy,reference-fog}`.

Night-eye color response now follows GL33's engine-provided coefficients,
before fog/gamma; the world resets it for HUD/menu drawing. Native Shapes support
up to eight engine-selected point/reflector lights, with GL33's night/material
modulation, attenuation and cone conventions. Stars use the engine's already-lit
screen points as subpixel-weighted quads; moon/flares consume software TL lighting.
Shadow Killer now launches rather than failing on DrawPoints/SpecLighting, and
its stars, moon, terrain and weapon darkness match the inspected GL33 capture.
Helitrain clear morning, Ninjas dawn fog, Heavy Metal dusk and an isolated
03:00 Take the Car copy were inspected against GL33 (stock missions unchanged).
The night town receives real local lights (246 light contributions/frame in the
sampled view) and has zero settled GPU allocations/texture uploads.

A live mission-command test changes visibility 900 -> 300 -> 900 and then fog
to 0.8. Recorded ranges change to 84..280, 264..880 and 64.8..216 metres. It caught
TextureVK's CPU GetPixel wrapping (1,1) to blue sky instead of clamping to the
horizon as PacLevelMem does. Correcting that produces the same grey distant
terrain as GL33. This does not change GPU repeat/clamp sampling. The corrected
run passed resize/restore/close with zero validation issues. Focused ON/OFF
checks pass (53/52 cases, 4776/4726 assertions), plus 89 driver-free policy checks.
Evidence: `build/lighting-live/{m4-*,reference-night*,reference-day,reference-visibility}`.

Final integration: Heavy Metal's 100-second timed run included a 60-second
sustained segment and grenade dust/smoke; Infantry and Take the Car each had
30-second sustained segments with movement, aiming/firing, weapon/reload,
map/pause and resize/minimize/restore. Infantry exited via pause-menu Abort;
Take the Car closed normally. Shutdown reported zero Khronos core/sync errors
or warnings (6887/6409/3810 submissions). Normal Vulkan menu/intro timed exit
also passed (3422 submissions). Real captures, not exit codes alone, were checked.
The old smoke fixtures needed an owned engine sun after lighting was introduced;
they now initialize it, and the cube has actual face normals. Shape, texture,
P3D model, WRP scenery and triangle smoke modes all passed visual/lifecycle checks.
Both builds and focused tests pass; default GL33 visibly opens the stock menu in
both executables. Stock data/resources counts, sizes and metadata hashes are unchanged.

Comparable Heavy Metal samples (800x600, validation on) changed from roughly
12..13 ms / 77..82 FPS to 14..15 ms / 67..72 FPS with lighting/fog. Infantry
settled around 8..9 ms / 111..125 FPS. These are similar-view samples, not a
controlled benchmark; loading, map changes and resize intervals are excluded.
Settled GPU allocations and texture uploads remain zero; transient uploads are
about 0.05 ms/frame in Heavy Metal, with no new synchronous per-draw uploads.
Evidence: `build/lighting-live/m5-*`.

Remaining differences: no object/vehicle shadows or full material specular
response; water parity is still approximate, with a thin bright shoreline strip
in Infantry absent from the GL33 reference. Two small water-state experiments
did not fix it and were reverted. No campaign/full mission was completed, and
night-vision goggles were not exercised. Basic object/vehicle shadow parity is
the highest-value next visual milestone.

## Projected-shadow work (2026-10-10)

The mip decoder retains its decoded prefix at valid one-pixel tails unsupported
by PacLevelMem, while rejecting truncated payloads and invalid dimensions.
Three existing PAA/PAC fixtures reproduce the old failure and now pass; five
sampled retail textures still pass (20 focused cases / 264 assertions).

Native alpha selection now honors IsAlpha/IsTransparent and material opacity.
Blending no longer implicitly disables depth writes: NoZWrite/NoZBuf select
independent native and screen pipelines, following GL33. Blended fades retain
the near-zero threshold; explicit cutout flags use GL33's 192/255 threshold.
Ten focused cases / 88 assertions and 89 policy guards pass. RTX Take the Car
foliage, buildings, weapon/HUD and model-smoke Jeep/building were inspected;
a matching GL33 mission view was captured. Timed and normal-close Vulkan runs
report zero validation errors/warnings. Evidence: build/shadow-live/m1-*.

Projected shadows now consume Scene/Object's existing projected Shapes. A
D32S8 (D24S8 fallback) attachment and per-pass stencil clear retain receiver
depth; EQUAL-zero/INCREMENT exclusion and ZERO/ONE_MINUS_SRC_ALPHA blending
darken each pixel once across all casters. A dedicated late-test fragment shader
keeps texture holes out of the stencil mask. Native and software draws use
engine opacity and shadow-distance inputs, without ordinary lighting/RGB fog.
The normal Scene capability is enabled. Invalid pass nesting/draw state fails.

Stock B02HMMWV at 13:00 visibly casts its projected vehicle shadow on the road
and roadside while driving in third person; nearby scenery/vehicle depth remains
intact. Take the Car's ordinary scene also runs. RTX core/sync validation through
normal close: 5244/4811 submissions, zero errors/warnings. Software shadow draws
were measured; cached native shadow coverage is checked in the next milestone.
Build, ten focused Shape cases and 92 policy guards pass. Evidence: m2-*.

Cached terrain-fitted shadow Shapes now accept the engine's immutable small
discardable buffers; other dynamic geometry still uses software TL. Native
shadow fading follows Scene::FogExponential rather than a linear approximation.
Stock HMMWV runs exercised both moving software shadows and stopped cached
native shadows (5 draws / 174 triangles per frame), with no steady-state GPU
allocations or texture uploads. Matching GL33 vehicle shadow shape/opacity,
resize and minimize/restore captures were inspected. Normal-close validation
was clean; 30 focused decoder/Shape cases (354 assertions) and 92 guards pass.
Evidence: m3-hmmwv, m3-final. A separate low-sun probe exposed an existing
IsLight flare-decal rejection; that is addressed in the appearance milestone.

Low-sun checks exposed IsLight decals being rejected (and software lights using
ordinary alpha blending). Screen/software light draws now use GL33's additive
SRC_ALPHA/ONE blend and disabled RGB fog, with distinct depth/blend pipelines.
Native world lighting is unchanged. Ten Shape cases / 93 assertions pass.

At matching B02HMMWV viewpoints, Vulkan and default GL33 captures at 13:00,
18:00 and 23:00 show matching projected vehicle/scenery shadow direction and
low-sun length. Existing runtime settings enabled object shadows (the isolated
profile originally had objectShadows=0). Buildings, trees and the roadside
monument cast in the ordinary Scene pass. At 19:30/23:00 projected submissions
fall to zero. Nearby nighttime brightness also occurs in GL33. A second Jeep
was placed through the existing script API for an overlapping-caster check;
the combined vehicle/building shadows did not accumulate extra dark bands.
These controlled time/placement probes are not claimed as untouched mission
playthroughs. One malformed probe aborted via the test harness; the corrected
run closed normally with 2809 frames and zero validation errors/warnings.
Evidence: m4-conditions, m4-reference, m4-overlap. Road filtering and foliage
edge differences versus GL33 remain outside this shadow change.

Final RTX 4060 Ti validation: normal --render vk startup, animated menu world,
menu-selected Heavy Metal and walking near its M1A1/building/soldier shadows
ran for six minutes (25888 submissions). Infantry at stock dawn ran 200 seconds
(11545): movement, camera rotation, sights, firing (ammo decreased), reload,
third-person animated shadow, weapon selection, grenade explosion/fading smoke,
map, pause/resume, resize and minimize/restore were visually checked. Stock
Shadow Killer at 03:00 exercised movement/sights/firing with zero projected
shadow submissions; keyboard-selected Mission Abort exited the test-mission
run normally (4771). All these runs reported zero core/synchronization validation
errors/warnings through teardown. HMMWV driving was checked earlier. These are
bounded gameplay checks, not completed missions or full gameplay certification.

Matched 800x600 HMMWV town profiles, same build/stock mission/viewpoint with
isolated object+vehicle shadow settings off/on: the last five settled two-second
windows averaged 15.53/17.31 ms (64.4/57.8 FPS), CPU recording 11.25/12.33 ms,
and window p95 12.13/13.12 ms. Shadows-on submitted 114 cached draws/frame;
both had zero transient allocations and texture uploads in those windows.
Fence time was 0.135/0.161 ms. These are short live samples, not GPU timings or
a deterministic benchmark. No allocator/scheduler changes were warranted.

Both build configurations pass; Vulkan has 30 focused decoder/Shape cases
(357 assertions) plus 92 policy guards, GL33-only has 19 decoder cases
(214 assertions). The GL33-only default menu was captured and closed normally.
Stock data/localization/resources were not edited. Evidence: build/shadow-live/m5-*.
Remaining limitations: legacy terrain-fitted shadows, not general object-to-object
shadow mapping; no exhaustive proxy/LOD coverage. The map remains expensive
(about 5 FPS / 11000 screen submissions in Infantry), and distant white water/
shoreline patches were observed but not investigated in this shadow task.
The next high-value performance step is reducing the map's repeated screen draw
overhead, independently of projected shadows.

## Ordered map batching: alpha prerequisite (2026-10-10)

Software-transformed explicit IsTransparent sections now use GL33's 192/255
cutoff even when decoded texture alpha suggests a different classification.
IsAlpha/IsAlphaFog/additive fades retain near-zero rejection; projected shadows
retain their existing independent alpha policy. Both build configurations and
11 focused Shape cases (131 assertions) pass, plus 92 driver-free policy guards.
RTX Infantry gameplay, map open/close, resize and minimize/restore were inspected
in `build/shadow-live/map-m1-baseline`. This is still the unbatched renderer.

Ordered screen batching now retains one consecutive run of compatible polygons
in CPU vectors, fan-triangulates with 32-bit indices, and uploads once through
the existing frame-fenced transient pages. Image version/sampler, cutoff,
blend/additive mode, independent depth state and scissor must match. Vertex
reciprocal-W, UV, color and fog remain untouched. Gamma, fog color and night-eye
changes flush before mutation; direct draws, diagnostic draws, shadow boundaries,
clears, explicit FlushQueues and frame end also flush. Runs are bounded at 65,536
vertices/196,608 indices; an oversized individual polygon flushes immediately.
Dynamic textures allocate new immutable image versions, with queued and recorded
draws retaining their original version. Native P3D and software shadow geometry
are not batched or reordered.

Both builds, 13 focused cases/156 assertions and 92 policy guards pass. RTX
Infantry map/gameplay, map close, resize/restore and normal close were visually
checked (`build/shadow-live/map-m2-batched`): 2,667 submissions, zero core/sync
validation errors/warnings through teardown. Initial map observations reduce
roughly 11,800 polygons to about 1,030 screen draws; controlled A/B measurements
follow below. `CWR_VK_SCREEN_BATCH=0` disables batching for process-local A/B
profiling; the default is enabled. CWR_VK_PROFILE now distinguishes queued 2D
polygons, emitted batches, all actual screen draws (including software geometry),
and polygons per batch. Existing transient counts still count geometry uploads.

### Real Infantry map comparison

RTX 4060 Ti, 800x600, Khronos core/synchronization validation on, same executable
and isolated profile, stock Infantry default map area/scale. Batching off/on
used `CWR_VK_SCREEN_BATCH=0`/default. Simulation was stopped with the existing
harness after loading for stationary map sampling; these are controlled map
measurements, not mission playthroughs. Five consecutive settled two-second
windows per run were retained before interaction/capture; frame-weighted results:

| Metric | Batching off | Batching on |
| --- | ---: | ---: |
| Frame time / FPS | 152.72 ms / 6.55 | 21.06 ms / 47.49 |
| Existing CPU recording interval | 133.51 ms | 14.13 ms |
| Submitted 2D polygons/frame | 11,790 | 11,836 |
| Actual Vulkan screen draws/frame | 11,809 | 1,033 |
| Emitted 2D batches/frame | 11,790 | 1,014 |
| Polygons/batch | 1.00 | 11.67 |
| Transient allocations/frame | 0 | 0 |
| Geometry upload time/frame | 0.345 ms | 0.105 ms |

This is 7.25x measured FPS and 91.25% fewer screen draws. Small differences in
radio/tutorial overlays account for the 0.39% polygon-count difference; map area,
scale and geometry are matched. The existing record_ms interval includes
fence/acquire/present waits: respectively 2.512/0.951/5.685 ms before and
0.233/0.017/0.476 ms after. It is not a GPU timestamp or pure CPU-cycle metric.
No texture uploads occurred in these sampled windows. Evidence:
`build/shadow-live/map-m3-{off,on,gl33}` and `build/map-perf/m3-matched-samples.csv`.

Default and zoomed Vulkan/GL33 captures were compared for terrain, roads, grid,
tree/building symbols, unit markers, labels, notebook, compass and translucent
overlays. Zooming, right-button panning, different areas and map open/close were
exercised. No missing symbols, stale textures, incorrect order or clipping was
observed. Both Vulkan runs closed with zero validation errors/warnings (680 and
8,904 submitted frames). GL33-only default renderer also displayed the real map
and closed normally. The map is now responsive; remaining cost is dominated by
recording about 1,000 state-separated runs, not allocations or upload bandwidth.
No additional localized hotspot was established that warranted expanding this
change. Redundant command/state binding within those ordered runs is a useful
next profiling target; sorting transparent polygons remains out of scope.

### Gameplay and final regression checks

The existing isolated GOG data/profile setup was used with object and vehicle
shadows enabled in copied profiles. Stock missions were loaded through the normal
mission-test path; no smoke geometry replaced gameplay. Evidence is under
`build/shadow-live/map-m4-*`:

- Infantry: movement/rotation, sights, firing (M16 ammo 30 -> 29), reload,
  weapon switching, grenade throw (6 -> 5), visible explosion/dust/smoke,
  map open/close, zoom-out and pan, pause/resume, third-person animated geometry,
  resize/minimize/restore, and keyboard-selected Mission Abort. Normal shutdown:
  11,746 submissions, zero core/synchronization validation errors/warnings.
- Take the Car: movement/rotation, sights/fire/reload input, foliage and HUD,
  map zoom/pan through town/road/forest areas, pause/resume, resize/restore and
  normal close. 3,814 submissions, zero validation issues through destruction.
- B02HMMWV: third-person driving (position change verified), exhaust/dust,
  moving vehicle and scenery projected shadows, map zoom/pan, pause/resume,
  resize/restore and normal close. 3,195 submissions, zero validation issues.

Comparable stationary 800x600 gameplay samples with the same executable/profile
and batching off/on showed no systematic 3D regression. Representative frame
interval ranges were 11.65..18.08 -> 10.40..12.67 ms in Infantry and
19.54..24.58 -> 13.61..13.88 ms in Take the Car. Corresponding recording intervals
were 10.64..13.55 -> 7.47..9.39 ms and 12.46..15.75 -> 12.96..13.18 ms.
AI, weather, radio overlays and window scheduling vary between live runs, so
these ranges are regression observations, not deterministic whole-mission gains.
Settled samples retained zero recurring transient allocations.

Normal Vulkan menu/animated intro reached a 20-second timed shutdown (exit 0,
1,989 submissions, zero validation issues). Default GL33-only menu/intro was
visually inspected and timed out with exit 0. Texture/Shape, real-model and
triangle diagnostics were visually checked and timed out with exit 0, respectively
1,833 / 1,771 / 1,841 submissions and zero validation errors/warnings. A further
batching-off menu comparison retained the same small intro projection artifact;
it also exited 0 with clean validation. Wide-map rectangular offshore contour
lines were reproduced in GL33 and are not a batching regression.

Final Vulkan-enabled and GL33-only builds pass. Focused Shape/batch/decoder tests:
32 cases / 274 assertions ON, 31 / 269 OFF; all 92 driver-free policy guards pass.
The batching tests cover fan order/attributes, 32-bit indices, bounded runs,
state splits and image-version lifetime. Live mixed UI/software/native/shadow
frames, font updates and teardown showed no missing or unflushed geometry.
All 7,606 stock-data/resource files retained their original count, lengths and
timestamps; localization and GL33 rendering code were not edited. Audio mute and
Caps Lock were restored to their initial states. These are bounded gameplay
checks, not completed missions or exhaustive coverage of all map scales/proxies.

Remaining limitations include pre-existing water/shoreline artifacts and material
parity gaps. Map performance now depends mainly on the remaining ordered draw
runs and CPU command recording; GPU execution is not timestamp-profiled. The
single next performance milestone is measuring and eliminating redundant Vulkan
pipeline/descriptor/viewport/scissor bindings within those runs, while retaining
exact draw order and invalidating cached state at command-buffer boundaries.

## Command recording state cache (2026-10-10)

VulkanContext now remembers only the state recorded in the current command
buffer: pipeline, the two immutable texture/sampler descriptor handles, lighting
set plus dynamic offset, vertex/index buffers plus offsets/index type, viewport,
scissor and all 112 push-constant bytes. Identical state commands are omitted;
draws, screen-batch boundaries, shaders and frame-fence resource ownership are
unchanged. Resetting a command buffer, destroying/recreating the swapchain and
the different-layout diagnostic triangle invalidate the cache. Clear and shadow
boundaries retain Vulkan state legally; changed shadow pipeline/constants still
bind normally. The existing native/screen lighting-data caches remain separate
from the actual descriptor binding cache.

CWR_VK_PROFILE adds emitted state-command counts, submit time, resource-retention
search time and binding-path time (including lighting-data lookup/upload).
command_ms measures elapsed reset/begin through end-command-buffer, excluding
fence/acquire/retirement/submit/present; it includes engine work between draws
and is not a GPU timestamp or pure CPU-cycle measurement. Existing record_ms
retains its previous broader meaning. No new profiling switch was added.

The Vulkan build, 32 focused Shape/batch/decoder cases (274 assertions), and 114
driver-free policy checks pass. The new checks exercise real DrawMesh calls and
verify identical-state suppression, offsets, index types, sampler/image versions,
lighting set/offset changes, clipping, viewport, complete push bytes, native /
screen / shadow transitions, diagnostic-layout restoration and reset. Real
800x600 Infantry map runs have clean core/synchronization validation through
normal shutdown. Matched timing and wider gameplay verification follow below.

### Measured remaining cost and stop decision

RTX 4060 Ti, stock Infantry map, 800x600, isolated copied profile, ordered batching
enabled in both binaries. Baseline is 7d98998 plus the same profiling timers;
cache is e20d527's implementation. Simulation was frozen through the existing
harness after loading, then the real map was opened with M. Each result uses the
first five complete upload-free map windows, weighted by frame count. Evidence:
`build/shadow-live/cmd-matched-{baseline-on,cache-on,cache-off}` and the initial
`cmd-m1-no-validation` baseline; `build/map-perf/commands-matched.csv` holds the
summary. The validation-off pair has identical polygons, batches and draws.

| Per-frame measurement | Validation on: before -> after | Validation off: before -> after |
| --- | ---: | ---: |
| Observed FPS | 52.18 -> 98.26 | 158.28 -> 158.35 |
| Observed frame interval | 19.166 -> 10.177 ms | 6.318 -> 6.315 ms |
| Command recording interval | 12.654 -> 8.458 ms | 2.635 -> 2.450 ms |
| Binding path | 7.210 -> 3.302 ms | 0.315 -> 0.133 ms |
| Retention searches | 0.031 -> 0.031 ms | 0.029 -> 0.027 ms |
| Geometry upload | 0.100 -> 0.096 ms | 0.089 -> 0.087 ms |
| Queue submission | 0.593 -> 0.573 ms | 0.027 -> 0.028 ms |
| Fence wait | 0.268 -> 0.264 ms | 0.002 -> 0.003 ms |
| Acquire | 0.044 -> 0.041 ms | 0.002 -> 0.002 ms |
| Present | 0.481 -> 0.487 ms | 3.396 -> 3.582 ms |
| Actual screen draws | 1,034 -> 1,034 | 1,033 -> 1,033 |
| Screen batches | 1,015 -> 1,015 | 1,014 -> 1,014 |
| Settled GPU allocations | 0 -> 0 | 0 -> 0 |

Important measurement limitation: wall-clock FPS is noisy, not the isolated
cache gain. The validation-on baseline includes stalls outside the measured
recording interval. Two additional validation-off baseline runs using fixed
startup timing (`cmd-matched-baseline-off`, `cmd-repeat-baseline-off`) measured
94.77 and 104.94 FPS, yet their recording intervals remained 2.588/2.725 ms and
their broader record_ms remained 6.093/6.074 ms. Individual settled windows in
those runs also reached about 158 FPS. Do not interpret those stalls, or the
entire validation-on FPS difference, as work removed by the cache. Their source
outside VulkanContext was not established; no unrelated scheduling change was
made. The directly measured benefit is about 4.20 ms of recording with validation
and 0.19 ms without it. Normal validation-off presentation already dominates the
remaining binding work; no meaningful steady FPS increase is claimed there.

Validation-on production state commands/frame (diagnostic triangle commands are
not counted by these DrawMesh counters):

| Command | Before | After |
| --- | ---: | ---: |
| Pipeline | 1,034 | 7 |
| Texture descriptor pair | 1,034 | 1,030 |
| Lighting descriptor + dynamic offset | 1,034 | 1 |
| Vertex buffer + offset | 1,034 | 1,034 |
| Index buffer + offset/type | 1,034 | 1,034 |
| Viewport | 1,034 | 1 |
| Scissor | 1,034 | 15 |
| Push constants | 1,034 | 10 |
| Total | 8,272 | 3,132 |

This eliminates 5,140 commands/frame (62.1%) without eliminating a single draw
in the matched pair. Texture/sampler transitions and transient geometry offsets
really change and are deliberately still bound. Tutorial/radio overlays differ
slightly between runs (11,836 vs 11,790 polygons in the validation-on pair);
the off pair has 11,836 in both. The earlier 1,030..1,034 draw variation also
occurred in the baseline. No batcher or draw-order code changed.

Linear retention searches are only about 0.03 ms on the map and about 0.10 ms
in the checked Take the Car/HMMWV gameplay windows. They do not justify another
cache or ownership change. No additional hotspot optimization was retained or
needed. Further map micro-optimization is not justified by validation-off results.

### Live regression verification

Current evidence is under `build/shadow-live/cmd-*`, separate from the previous
batching milestone. All three gameplay missions used copied profiles with
object/vehicle shadows enabled and unchanged stock GOG assets:

- Infantry: movement and camera rotation, sights, firing (M16 30 -> 29), weapon
  switching, two grenade throws (6 -> 4), visible explosion smoke/dust,
  third-person animated geometry and projected shadow, map open/close/zoom/pan,
  HUD fades, pause/resume, resize/minimize/restore and Mission Abort. Exit 0;
  12,556 submissions, zero core/synchronization validation errors or warnings.
- Take the Car: movement/rotation, aim/fire/reload input, foliage cutouts,
  translucent tutorial/radio HUD, town/forest/road map, map transitions,
  pause/resume and resize/restore. Normal exit 0; 3,498 submissions, zero issues.
- HMMWV: third-person driving (position moved about 11 m), exhaust/dust, vehicle
  and object shadows, map zoom/pan/open/close, pause/resume and resize/restore.
  Normal exit 0; 3,632 submissions, zero issues.

Default GL33-only Infantry gameplay and map were visually compared with Vulkan:
terrain/roads/grid, unit markers, symbols, labels, translucent overlays and
notebook/compass layers remained faithful at the inspected scales and areas.
Zoom/pan/map close worked in GL33. No missing geometry, stale texture, incorrect
ordering or new clipping/flicker was observed. These are bounded checks, not
completed missions or exhaustive scene coverage. GPU execution was not timestamped.

Normal Vulkan menu/animated intro and diagnostic triangle rendered and reached
timed exit 0 with respectively 2,860 / 1,663 submissions and zero validation
errors/warnings through destruction. Default GL33-only menu/intro also rendered
and reached timed exit 0. The loader still reports the pre-existing stale EOS
overlay manifest independently of the clean Khronos validation counters.
Both build configurations pass; focused tests remain 32 cases/274 assertions ON,
31/269 OFF, plus 114 driver-free guards. No GL33 rendering code, stock assets or
localization changed: all 7,606 protected files retain count, length and timestamp.
Original endpoint mute and Caps Lock state were preserved; no game remains open.

The highest-value next visual-parity milestone is the pre-existing white
water/shoreline artifact visible in Infantry, not further map command reduction.

## Water/shoreline cause and targeted correction (2026-10-10)

The white Infantry strip was reproduced against GL33 with the same scripted
camera position/target, dawn time, weather, visibility, 800x600 and copied profile.
A temporary magenta tag on native SpecularTexture draws covered exactly the
white pixels: these are landscape water Shapes, not software geometry, missing
terrain, a surface overlay or a stale UI texture. The tag and trace logging were
removed after diagnosis. Evidence: `build/shadow-live/water-{baseline-vk,
baseline-gl33,trace-vk,trace-gl33,fixed-vk}`.

The actual native section flags are 0x02002000 (SpecularTexture | NoClamp), with
animated `data/more_anim.*.pac` primary textures and `data/specular_dx.paa` as the
secondary texture. LandscapeRender's WaterFlags deliberately do NOT include
IsWater. GL33's captured state is PSDetail (shader 1), SpecularTex (format 2):
TGDetail supplies 32x secondary UVs and PSDetail multiplies lit primary RGB by
secondary alpha * 2. Vulkan had inferred a water bump shader from the texture
source flag alone, decoded secondary RGB as a normal at unscaled UVs, and added
a large clamped light dot product. This erroneous additive term caused the white
patches. Neither a different depth state nor forcing the unused IsWater/TGWater
branch addresses that mismatch. The earlier unsuccessful state experiments
recorded above were not adopted.

Vulkan now derives secondary shader mode from the existing shared render-pass
descriptor, independently of the secondary texture source. This selects the
already-implemented detail-alpha shader operation for ordinary stock water.
Original textures/mips, animated frame selection, material lighting, fog, gamma,
repeat samplers, opaque depth-tested/depth-writing geometry and render order are
unchanged. No colors are substituted and no water geometry/effects are hidden.
The explicit IsWater family remains unsupported rather than being silently
enabled; no speculative new water shader or global sampler/fog change was added.

The original white strip is gone in the matched fixed capture, with a second
bay-facing capture showing ordinary near/distant water and shoreline continuity.
Both builds pass: 33 focused cases / 285 assertions ON, 32 / 280 OFF, plus 114
driver-free policy checks. The new regression locks the distinction between
SpecularTexture and IsWater and preserves sampler/alpha/depth policy. The first
fixed Infantry run closed normally with 10,834 submissions and zero Khronos
core/synchronization errors or warnings. Wider water-condition checks follow.

### Matched water-condition comparison

Six fixed-camera pairs were inspected in `build/shadow-live/water-m3-vk-ready`
and `water-m3-gl33`, using the GL33-only executable with no renderer override.
Both used the same copied shadow-enabled profile, 800x600, frozen simulation,
July 5 1985 and identical camera targets/positions. The original Infantry shore
and west-facing bay were checked at 05:35 (overcast 0.487594, fog 0, visibility
900 m); the bay, opposite coast and a low near-water view at noon (overcast 0.3);
the latter again with fog 0.65 and visibility 300 m. Harness receipts confirmed
matching camera positions and time. Texture-animation phase and NPC positions
are not synchronized, so these are controlled visual comparisons, not pixel diffs.

The erroneous white strip is absent. Ordinary water color/brightness, near-wave
texture, shoreline transitions and fogged horizon are recognizably faithful to
GL33 across these views. No new seam, missing water or depth overlap was seen.
Vulkan still has visibly coarser distant terrain/detail and smoother distant
water than GL33; that existing texture/LOD difference is not concealed by this
fix. Full native material-specular parity and the unused explicit IsWater family
are not claimed. No additional shader or global sampler change was justified by
these comparisons. Both comparison processes closed normally (Vulkan 3,127
submissions, zero core/synchronization validation errors or warnings).

### Water performance and final gameplay verification

Matched profiling used the stock Infantry bay at noon, the same camera and
shadow-enabled profile above, 800x600 on the RTX 4060 Ti. The saved 0b80145
executable was compared with the fixed executable. Each result aggregates the
last five complete two-second windows after settling (frame-weighted times,
FPS from total frames / total elapsed time). No build ran during sampling.
Evidence: `build/shadow-live/water-perf-{before,after}-{on,off}` and the reversed
order `*-off-repeat` pair.

| Validation / sample | Baseline FPS / frame ms | Fixed FPS / frame ms | Command recording ms, before -> after |
| --- | --- | --- | --- |
| On | 158.16 / 6.323 | 158.13 / 6.324 | 2.232 -> 2.223 |
| Off, first pair | 121.18 / 8.252 | 107.13 / 9.334 | 0.652 -> 0.603 |
| Off, reverse-order repeat | 158.42 / 6.312 | 158.28 / 6.318 | 0.605 -> 0.647 |

The first validation-off pair contains intermittent wall-clock stalls in BOTH
binaries outside the measured command interval. The repeat does not reproduce
the apparent FPS loss. Legacy `record_ms` (including presentation/waits) stayed
about 6.01 ms off and 6.03 ms on; settled geometry upload was 0.006-0.008 ms/frame,
with zero transient allocations and no texture uploads in the sampled windows.
Both binaries recorded identical sampled draw/state counts (189 lit draws,
109 screen draws, 14 pipeline / 218 texture-set binds per frame). This is a
visual correction, not a measured performance improvement; the stable repeat
shows no meaningful regression. GPU execution was not independently timestamped.

Bounded real-game checks, with core and synchronization validation enabled:

- Infantry: moved about 4 m, rotated toward/away from shore, aimed/fired/reloaded,
  threw a grenade (6 -> 5 with visible explosion smoke), opened/zoomed/closed the
  map, checked HUD/third-person soldier/projected shadows, paused/resumed, resized
  to 960x640 and minimized/restored. Running water was inspected in two captures
  ten seconds apart; animation/shore movement continued without new white patches.
  Mission Abort exited 0: 12,694 submissions, zero validation errors/warnings.
- Take the Car: movement/rotation, aim/fire (HUD 30 -> 29), foliage and building
  textures, translucent radio/tutorial HUD, map open/close, pause/resume and
  900x650 resize/minimize/restore. Normal close exited 0: 3,287 submissions, zero
  validation errors/warnings.
- HMMWV: cockpit and third-person driving (about 20 m), exhaust/dust, vehicle
  and object shadows, map open/close. A temporary runtime placement at
  [8110,5160,0] facing west added daylight coastal driving/water inspection;
  no mission file was edited and this was not mission completion. Water,
  animated flag and vehicle shadow remained intact across pause/resume and
  900x650 resize/minimize/restore. Normal close exited 0: 7,792 submissions,
  zero validation errors/warnings.

Evidence is under `build/shadow-live/water-game-{infantry,takecar,hmmwv}`.
Final Vulkan menu/animated intro reached timed exit 0 (2,482 submissions, zero
validation issues). Default GL33 in the Vulkan-enabled executable and the
GL33-only executable both rendered their menus/intro and reached timed exit 0.
Both final builds pass; focused tests pass 33 cases / 285 assertions ON and
32 / 280 OFF, plus 114 driver-free guards (their intentional mocked teardown
error is not a live validation error). The stale EOS overlay manifest loader
message remains an unrelated environment issue. All 7,606 protected stock and
resource files retain their initial count, length and modification timestamp;
no localization or GL33 rendering implementation changed.

Next highest-value visual-parity work: investigate distant/oblique texture
filtering and mip/LOD parity with GL33. Its linear samplers enable up to 16x
anisotropy whereas Vulkan currently uses plain trilinear sampling; the observed
distant-water smoothing is a concrete comparison target, not grounds for a
speculative global sampler change in this patch. Full material-specular and
explicit IsWater support remain separate limitations.

## Texture filtering parity (2026-10-10)

The 8b2d705 baseline was reproduced with the existing six Infantry coastal
camera presets, copied profile and 800x600 (`build/shadow-live/filter-before-water`
and `filter-gl33-water`). An anisotropy-only A/B (`filter-aniso-water`) removes
the conspicuous horizontal distant-water smoothing and improves oblique terrain
detail toward GL33 without reviving the white shoreline artifact.

Vulkan now queries/enables samplerAnisotropy when supported and applies
min(16, maxSamplerAnisotropy) to the four existing linear samplers. Magnification
remains linear and minification trilinear, with zero LOD bias and unchanged
clamp/repeat combinations. Unsupported devices retain trilinear filtering.
The four point samplers remain nearest/nearest-mip with anisotropy disabled.
Secondary texture descriptors already use sampler 0 (linear/repeat), independently
of the primary sampler, matching GL33. No sampler manager/settings or new
texture versions were added. The RTX 4060 Ti reports 16x; the six-view run closed
normally with 3,351 submissions and zero core/synchronization validation issues.
The Vulkan build passes and driver-free guards pass 261 checks, including all
eight sampler combinations at fallback/8x/16x and capability-limit handling.

### Stored mip range and AI88 precision

Temporary upload traces established that GL33's TextureSourcePac stops before
either mip dimension reaches 4, whereas Vulkan sampled the entire stored chain.
The Vulkan image view now exposes the same tail (all original mip bytes are
still decoded/uploaded). Examples: detail_dx 128x128 exposes 5 of 7 levels;
specular_dx 256x256 6 of 8; silnice 256x64 4 of 6; more_anim.03 512x512 7 of 8.
Single-level/tiny UI images retain their base. GL33's observed maximum size was
4096 with largestUsed=0; no top-size limit explains these scenes. UseMipmap is
a GL33 residency request, not an explicit shader LOD; Vulkan remains fully
resident. ObjMipmapCoef differs (1.5 versus 1), but has no call sites in this
tree, so it was not changed. No bias, generated mipmaps or streaming was added.

The matched low-angle HMMWV road exposed another concrete cause: anisotropy
alone left broad gray bands. Stock silnice, detail_dx and specular_dx are AI88.
GL33 uploads their native 8-bit intensity/alpha as RG8; the shared CPU decoder
used by Vulkan converted them through ARGB4444, discarding half the bits.
Both memory-chain and file-mip decoding now expand native AI88 directly to
RGBA8. The new synthetic all-256-values/multiple-mip regression failed before
the fix and passes afterward, including alpha precision. Stock alpha classes
remain Blend; classification policy is unchanged. `filter-ai88-road/road-noon.png`
now retains the fine road grain seen in `filter-gl33-road`, unlike
`filter-before-road` and the anisotropy-only comparison. `filter-ai88-water`
also preserves coastal detail without the former white patches.

ShapeSecondaryMode now uses IsMultitexturing(), matching GL33's detail shader
selection and avoiding unused secondary uploads when disabled. Focused tests
cover enabled/disabled detail/specular behavior, explicit IsWater selection,
mip-view bounds and full AI88 bytes. Both builds pass: 34 cases / 301 assertions
Vulkan ON, 33 / 296 OFF; eight stock textures pass 265 assertions ON and the
driver-free Vulkan guards pass 266 checks. Temporary texture/road traces were
removed. No GL33 drawing code, stock assets, shaders or batching changed.

### Final filtering verification and performance

Matched 800x600 camera/date/weather/visibility comparisons used the copied GOG
profile: six Infantry coast views (dawn shoreline/bay, noon bay/second coast,
near water, fog at 300 m) and four HMMWV road/town views (noon, dusk, fog).
`build/shadow-live/filter-{before,aniso,ai88}-water` and the corresponding road
runs retain the before/intermediate/after captures; `filter-gl33-{water,road}`
are the default-GL33 references. Camera geometry/settings match, not mission
NPC/subtitle timing. Oblique water/terrain detail is substantially closer to
GL33; AI88 road bands are gone. Buildings, shoreline transitions and distant
water remain intact. No new white patches or obvious filtering seams were
seen. Live movement did not reveal excessive new shimmering; this is bounded
inspection, not a claim of pixel-identical rendering.

`filter-multi-off-{vk,gl33}` additionally uses a copied profile with
multitexturing=0. Both lose the secondary ground detail as expected while
retaining ordinary water, confirming the engine setting reaches shader choice.
The usual multitexturing=1 profile was not changed.

RTX 4060 Ti, 800x600, existing VSync-on/FPS-cap-240 profile, CWR_VK_PROFILE:

| Resident scene | Core + sync validation | 8b2d705 ms / FPS | Final ms / FPS |
| --- | --- | --- | --- |
| HMMWV oblique road/buildings/foliage | On | 7.952 / 125.75 | 8.010 / 124.84 |
| HMMWV oblique road/buildings/foliage | Off | 6.316 / 158.33 | 6.323 / 158.15 |
| Infantry noon coastal water | On | 6.315 / 158.35 | 6.312 / 158.43 |
| Infantry noon coastal water | Off | 6.317 / 158.30 | 6.335 / 157.85 |

These are means of the first three complete two-second intervals with zero
allocations and texture uploads after the matched camera setup, not whole-run
averages. Road geometry is matched at 615 lit draws / 1,922 shadow triangles;
HUD/script submissions vary. Evidence: `filter-perf-road-{before,after}-{on,off}3`
and `filter-perf-water-{before,after}-{on,off}2`. Later road intervals contain
large stalls and changing scene workloads: final-five interval ranges across
the four road runs were 79.90–158.02 FPS (7.938–12.515 ms validation-on,
6.328–11.630 ms off). These are retained in the logs, not attributed to the
filter change, and not evidence of a speedup. An earlier overlapping diagnostic
attempt (`filter-perf-road-*-on`, no numeric suffix) is excluded.

Road record_ms was 7.665 -> 7.746 on / 6.070 -> 6.078 off; geometry uploads
0.191 -> 0.191 ms on / 0.181 -> 0.190 off. Water uploads stayed 0.007 ms.
All reported resident intervals have zero recurring allocations or texture
uploads. Off-mode present waits are about 4.1 ms road / 5.4 ms water: the
normal runs are display-paced, so these results do not measure uncapped GPU
headroom. No disproportionate cost was demonstrated that warrants reducing
GL33-equivalent 16x anisotropy. No performance optimization was added.

Fresh normal gameplay checks (`filter-game-*`, not mission completion):

- Infantry: about 8 m movement, camera rotation, aim/fire (M16 30 -> 29), map
  open/zoom/close, HUD, third-person soldier/projected shadow, coast water
  captures ten seconds apart, pause/resume, 960x640 resize/minimize/restore.
  Normal close: 10,967 submissions, zero validation errors/warnings.
- Take the Car: about 5 m movement, rotation, aim/fire (30 -> 29), close foliage
  cutouts, translucent radio/tutorial HUD, map open/close, pause/resume and
  900x650 resize/minimize/restore. Normal close: 5,918 submissions, zero issues.
- HMMWV: cockpit/windshield, third-person driving (about 8 m), road/building
  textures, dust, animated flag, vehicle/object shadows, HUD/map, pause/resume
  and 900x650 resize/minimize/restore. Normal close: 6,272 submissions, zero issues.

Khronos core/synchronization validation was enabled through those shutdowns.
Vulkan menu/intro timed shutdown also passed (1,442 submissions, zero issues).
Vulkan-enabled and GL33-only builds and the focused tests above pass; the final
stock case passes 265 assertions ON / 173 OFF. The 266 driver-free guards include
an intentional mocked teardown error, not a live validation failure. Default
GL33 in both executables rendered the menu/intro and reached timed exit 0;
Vulkan remains opt-in. The known stale EOS overlay-manifest loader message is
an unrelated environment warning, not a Khronos validation finding.
All 7,606 protected game/resource files retain their initial count, length and
modification time. Stock missions, textures and localization were not edited.

Remaining differences: sky/cloud appearance and low-sun fence/foliage rendering
are visibly different from GL33 and predate these changes; Vulkan remains fully
resident rather than implementing GL33's texture-size/residency policy. Explicit
IsWater/full material-specular support remains limited as documented earlier.
Highest-value next visual-parity milestone: diagnose the low-sun object-lighting
difference on the matched HMMWV fence, using GL33 as reference, without further
global texture sharpening or speculative fog changes.

## Visual parity: fence winding (2026-10-10)

The matched HMMWV dusk fence was not a specular or alpha-threshold defect.
The stock `data3d\plutek.p3d` uses normal material 0 (specular RGB/power zero),
with `planky3.paa`/`planky3.pac` blended/cutout sections. The close fence is
software-transformed; suppressing only that diagnostic draw removed the
affected pixels. GL33 retains back-face culling in its screen/software pass
specifically to reject coplanar reverse faces. Vulkan had culling disabled in
both paths, allowing the sunlit reverse side to overwrite the shaded front.

Native and screen pipelines now use clockwise front faces and back-face culling,
matching GL33 after the existing Vulkan Y conversion. Projected-shadow pipeline
rasterization is unchanged. Native-only culling did not fix the fence and was
not retained as a partial solution. No colors, normals, alpha thresholds, mip
levels, fog or stock geometry were changed to conceal the artifact.

Evidence under `build/shadow-live/`: `visual-before-fence` versus
`visual-cull-screen` reproduces/removes the orange dusk face; eight matched
`visual-fence-final` / `visual-fence-gl33` views cover noon, 18:00 low sun,
19:00 dusk, 23:00 night, reduced visibility, closer/oblique and reverse views.
The shaded face now matches GL33; the opposite side remains correctly sunlit.
Roads, buildings, sky, stars and sun halo remain visible. The Vulkan run closed
normally with 3,219 submitted/presented frames and zero core/synchronization
validation errors or warnings. The build passes and 269 driver-free guards
include new winding/cull/shadow-policy checks. Temporary tracing was removed.
Foliage silhouette differences remain for separate investigation; these captures
use GL33's actual 0x-MSAA target, so they must not be attributed to MSAA coverage.

### Native material specular

The native lighting block now carries sun-diffuse times material-specular RGB
and the original material exponent. The vertex shader computes GL33's normalized
world-normal/view/sun half-vector response, gated by positive exponent and the
existing sun-enable/DisableSun state. The normal/detail fragment path adds this
separate term after diffuse texture/detail modulation and before night-eye,
fog and gamma. Zero-specular materials, opacity, inverse-transpose normals,
local lights, shadow shaders and water's corrected detail-alpha sampling remain
unchanged. Software/UI vertices supply zero specular, as before; their CPU
lighting currently supplies black specular RGB. No new material system is added.

`visual-material-{vk,gl33}` compares the stock HMMWV, nearby buildings and glass
at noon, low sun, a second angle, night and fog. HMMWV body materials in this
run have no specular term and correctly do not acquire a new gloss. Narrow
diagnostics confirmed nonzero stock native terrain (power 3) and animated-water
(power 6) material terms. `visual-water-{before,spec,gl33}` covers six Infantry
coast views: dawn shoreline/bay, noon bay/second coast, near water and fog.
No white shoreline patches returned. Highlights are view-dependent and subtle
in these views; this is not a claim of a large vehicle brightness improvement.
Vehicle and water Vulkan runs closed normally with 2,180 / 3,386 frames and zero
core/synchronization validation findings. The Vulkan build and 35 focused
cases / 310 assertions pass, including specular color/power, disabled sun,
black sun/material and std140 layout regression checks. Traces were removed.

Further foliage diagnosis: the same `str_jablon.p3d` object at
`[3061.324,230.263,6334.997]` uses LOD 0 / 55 vertices in both renderers.
GL33 loads `jablon_renovace.pac` from mip 2 (64x64); Vulkan retains its 256x256
base. The different stored-mip coverage contributes to the fuller GL33 crown.
Evidence: `visual-lod-{vk,gl33}` and `visual-mip-gl33`. This is not MSAA coverage,
nor evidence for changing the global 192/255 cutoff. GL33's demand-residency
policy remains outside this targeted lighting pass; anisotropy, AI88 precision
and the previously corrected mip-tail bounds are untouched.

### Sky texture interpolation and atmosphere

Matched Infantry coast captures traced a further real mismatch to texture
interpolation, not ClipLightSky/ClipFogSky or sunlight. GL33 uploads the stock
sky interpolation in ARGB1555 using PacLevelMem's integer 255-weight RGB555
arithmetic. Vulkan instead mixed expanded RGBA8 texels in floating point.
This changed the texture's color steps/gradient through weather transitions.
Vulkan now preserves source-format metadata and matches that packed arithmetic
for the corresponding P8/1555/compressed source families. Other formats retain
their existing path. A focused test compares the helper directly against
PacLevelMem::Interpolate over representative channel values and weather factors.

Importantly, GL33's CPU GetPixel sky/fog calculation interpolates the
source colors separately from its quantized GPU upload. Vulkan now retains
those source references/factor too, so matching the visible texture does not
quantize the scene's fog color. Average-color metadata also remains unquantized;
this does not implement GL33's source-header average-color policy.
Existing image-version/frame-fence ownership,
the 1/64 update tolerance, endpoint selection and single-level sky policy are
unchanged. Stock files, sky geometry and weather animation are untouched.

`visual-sky-{spec,packed,gl33}` compares before/after/reference at clear noon,
partial-overcast sunset, full-overcast noon, foggy overcast dawn, clear night
and a moon-facing night view. The fixed screen crop x=5..794, y=105..319 has
mean absolute RGB error 1.984 -> 0.408 /255 at clear noon, 2.310 -> 1.965 at
sunset and 0.718 -> 0.552 at night. These are screenshot observations, not a
universal parity score; cloud movement and mission timing are not bit-identical.
Full-overcast error barely changes (5.005 -> 4.974), as expected when the
endpoint texture is used. Sky/horizon transitions, stars, moon, sun halo and
fog remain present. The final Vulkan weather sweep closed normally with 3,507
frames and zero core/synchronization validation errors/warnings. Both builds
pass: 36 cases / 459 assertions ON, 35 / 454 OFF. No tracing remains.

Cloud geometry/layers, CPU cloud color and alpha-fog handling already follow
the engine; no new cloud system or global fog correction was justified.
Atmospheric parity is partial: GL33 applies gamma after framebuffer composition,
whereas Vulkan applies it per fragment before alpha blending. That remains a
specific compositing difference for translucent clouds/effects at non-unit gamma,
not evidence for altering original weather brightness or alpha thresholds.

### Final visual-pass verification

The final executable (SHA256 `A8249CC6ACA358A515956D5AA62B1E4E4374E8B2631D92AD11189E5881066A6E`)
was tested against the saved baseline executable
(`95555F63812F55F9DED61C60C2BBD3FF890BF534E8241F41011221A8E8E34B12`),
using the isolated GOG 3.05 data/profile, RTX 4060 Ti and 800x600 window.
`visual-perf-{road,water}-{before,final}-{on,off}` contains the existing matched
camera sampler's logs and screenshots. Each number below is the mean of the
first three complete two-second intervals with zero allocations/texture uploads;
loading intervals are excluded. VSync/profile settings are unchanged.

| Scene / validation | FPS before -> final | Frame ms | Recording ms | Geometry upload ms |
| --- | --- | --- | --- | --- |
| HMMWV oblique road / on | 131.20 -> 138.52 | 7.623 -> 7.219 | 7.387 -> 6.971 | 0.187 -> 0.142 |
| HMMWV oblique road / off | 158.46 -> 158.54 | 6.311 -> 6.308 | 6.079 -> 6.077 | 0.177 -> 0.140 |
| Infantry coast / on | 158.55 -> 158.71 | 6.307 -> 6.301 | 6.055 -> 6.058 | 0.007 -> 0.006 |
| Infantry coast / off | 158.43 -> 158.60 | 6.312 -> 6.305 | 6.028 -> 6.027 | 0.006 -> 0.006 |

These support no observed steady-rendering regression, not a claimed speedup.
The road's mission/shadow workload varies between launches (615 lit draws but
91 versus zero software-shadow draws in these intervals). The baseline repeat
`visual-perf-*-repeat-before-*` also exhibits that variation. Later intervals in
both binaries sometimes reach roughly 9-12 ms while recording remains around
6 ms and p95 near 6.2 ms; those application/frame-pacing stalls remain in the
logs and are not assigned to this patch. Validation-off steady samples are
presentation-limited, so they do not measure uncapped GPU headroom. Settled
transient allocations remain zero. Batching, state caching and draw order were
not redesigned or optimized in this pass.

Actual final Vulkan gameplay evidence (`build/shadow-live/visual-game-*`):

- Infantry: movement, camera turn, aim/fire/reload, two grenade throws and visible
  blast/smoke, soldier shadow, map opening/zoom/pan/closing, pause/resume,
  960x640 resize/minimize/restore and Mission Abort. Abort exits this test-mission
  launch normally; 16,563 submitted frames, zero validation findings.
- Take the Car: movement, aiming/firing/reload, buildings and vegetation, HUD/map,
  pause/resume, 900x650 resize/minimize/restore, normal close; 4,262 frames, zero.
- HMMWV: cockpit/windshield and third-person views, about 22 m of driving,
  braking, dust, fence/vehicle shadows, HUD/map, pause/resume and resize/restore;
  normal close, 5,144 frames, zero.
- Heavy Metal (`visual-game-heavy2`): on-foot movement and stock M1 tank views;
  the existing harness placed the player in the driver's seat, followed by
  keyboard driving/braking, vehicle shadows, map, pause/resume and resize/restore.
  Normal close: 9,352 frames, zero. An earlier unsupported `assignedVehicle`
  diagnostic caused a script-error exit and is excluded, not a renderer failure.
- Shadow Killer: stars/moon and dark foliage, night-vision toggle, movement,
  aiming/firing, HUD/map, pause/resume and resize/restore. Normal close:
  14,205 frames, zero. Default GL33's matching starting view and night vision
  are captured in `visual-game-shadow-gl33-repeat` (normal exit 0).

Core/synchronization validation was enabled through each accepted Vulkan
shutdown. Grenade blast smoke and vehicle dust were observed; a runtime
SmokeShell selection attempt did not produce a dedicated smoke-grenade throw,
so that particular effect is not claimed as tested. No mission files were edited.
Additional fixed noon cameras `visual-matched-{takecar,heavy}-{vk,gl33}` compare
the same buildings, foliage, M1 tank, shadows and sky with identical time,
weather, visibility and settings. These complement the HMMWV/Infantry sweeps
above; ordinary initial mission positions alone were not treated as matched.

Both builds pass. Final focused runs pass 459 assertions / 36 cases ON and
454 / 35 OFF; stock mip checks pass 265 / 173 assertions, and the driver-free
suite passes 269 guards (including its intentional mocked teardown error).
Final menu/intro captures `visual-final-menu-{vk,gl33,gl33-only}` render normally
and reach timed exit 0; Vulkan records 1,509 frames with zero validation findings.
Both executables still select GL33 when no renderer option is supplied.
The 7,606 protected stock/resource files retain their original count, lengths
and modification times; localization and assets are untouched. No white
shoreline patches or AI88 road banding returned in the inspected scenes.

Completed fixes are reverse-face rejection, native material specular and packed
sky interpolation. Foliage residency/coverage and atmospheric compositing are
still partial, not solved milestones. Highest-value next visual-parity work:
match GL33's framebuffer-level gamma/composition ordering for translucent
clouds/effects, with matched non-unit-gamma captures before extending scope.

### DXT1 interpolation rounding follow-up

A synthetic DXT1 case exposed a small remaining mismatch in `8f60a83`:
endpoints `0x1000/0x0000`, selector 2, blended halfway toward black produced
red 0 instead of GL33's 8 (the older floating-point result was 6). DXT1-to-RGBA8
decoding followed by five-bit truncation cannot reproduce the legacy decoder's
rounding before interpolation. The existing packed-color test missed that step.

Only DXT1 sources used in packed interpolation now lazily decode their top level
with `PacLevelMem::DecompressDXT1`, the same routine GL33 uses. The result is
cached separately; ordinary pixels, stored mips, alpha classification and CPU
sky/fog lookup retain their existing decode. Other formats and mixed-format
floating-point interpolation are unchanged. No shaders or GPU ownership changed.

The actual TextBankVK regression failed with 0 before the fix and passes with 8
afterward; ordinary red stays 11 and the CPU halfway lookup stays 5.5/255.
Additional checks cover all DXT1 selectors, transparent/equal-endpoint blocks,
partial blocks and truncated payloads. Both builds pass; focused checks pass
826 assertions ON / 814 OFF, stock mip checks 265 / 173. No new gameplay visual
improvement or performance result is claimed; the previous live validation
results were not rerun for this narrowly scoped CPU conversion fix.

## Framebuffer gamma: reproduction (2026-10-10)

This work starts from `45a6033` on main, retaining the repository-owner and DXT1
rounding follow-ups to requested baseline `209657c`. There was no unstaged
documentation edit at the start. The saved pre-change executable is
`build/vulkan-local/apps/cwr/Game/PoseidonGame-gamma-before.exe`, SHA256
`C045FADFEF8232D03806D783D9701A61566EC53D1FC667C44DC7DA02D752EFAA`.

GL33's ApplyGammaPass copies the completed default framebuffer to RGBA8 and
draws a nearest-sampled fullscreen triangle with pow(rgb, 1/gamma). It runs
after scene/HUD composition, with blending/depth/culling disabled. Vulkan's
shape.frag instead applied pow before fixed-function alpha/additive blending,
including software geometry and ordered screen batches. Projected shadows
darkened an already-corrected destination. For example, a 50% white fragment
over black at gamma 2 produced 0.5 instead of sqrt(0.5), approximately 0.707.
This is an ordering defect, not evidence for changing cloud colors or opacity.

The existing Infantry camera/weather sweep was repeated at gamma 0.6, 1.0,
and 1.6, with the same 800x600 copied profile, brightness 1.6, fixed cameras,
date/weather/visibility and frozen simulation. Evidence is in
`build/shadow-live/gamma-before-{vk,gl33}-{06,10,16}`: clear noon, partial sunset,
heavy overcast, foggy overcast dawn, night and moon-facing views. Baseline Vulkan
normal closes reported 3351/2947/3475 frames and zero core/synchronization
validation errors or warnings. The GL33 references also closed normally.

The fixed sky crop x=5..794, y=105..319 has Vulkan/GL33 mean absolute RGB error
in 8-bit levels of 1.953/4.670/2.393 at sunset for gamma 1/0.6/1.6, respectively;
heavy overcast is 5.721/8.459/4.288. These are observations, not a deterministic
parity score: cloud positions and subtitle/mission timing differ between runs.
Non-unit gamma amplifies some differences and masks others. Gamma-1 residuals
are explicitly retained as a separate comparison, not assigned to gamma.

An actual SmokeShell throw was also observed in `gamma-smoke-before-vk-16`:
the initial full Infantry inventory rejected the added magazine. Removing
HandGrenade magazines in the disposable runtime session allowed SmokeShell
selection; a mouse throw consumed it and produced a visible white smoke plume.
No stock mission or asset was edited. The run closed normally with 10512 frames,
zero validation findings. Further matched smoke/HUD comparisons and corrected
renderer results follow below; selection/eval success alone is not smoke coverage.

### Framebuffer gamma implementation

VulkanContext now owns one sampled scene-color image per swapchain image, with
the same extent/format and the existing depth/stencil attachment. The original
render pass still contains every native Shape, software TL draw, sky/water,
cloud/particle, projected shadow and ordered screen/HUD batch. EndFrame flushes
the last batch, ends that pass, then draws one fullscreen triangle into the
acquired swapchain image. Its shader applies pow(max(rgb, 0), 1/gamma) once,
after all ordinary alpha/additive blending and stencil shadow exclusion.
The Shape fragment shader no longer performs gamma; its old push-constant slot
is padding, so gamma changes no longer split batches or mutate scene constants.
The latest gamma setting applies to the whole finished frame, as in GL33.

Color-write to shader-read and prior-sample to next-color-write dependencies
cover target reuse. Final rendering retains the acquire wait, per-image present
semaphore and original frame-fence submission. The independent gamma layout
invalidates the command-local scene cache. No scene ordering, blend factors,
alpha thresholds, fog, material lighting, texture descriptors or asset lifetime
changed. All added images, views, framebuffers, descriptor sets/pool, pipeline
and layout are reused; idle-based swapchain recreation/shutdown owns cleanup,
including partial initialization. No recurring frame allocations are added.

The RTX surface remains B8G8R8A8_UNORM, so there is no automatic sRGB conversion
in either pass. The sampled image uses the exact swapchain format rather than
introducing an sRGB target. An sRGB-only surface retains its original blend
space, with hardware decode on sampling and encode on final output; that
fallback was not live-tested on this UNORM surface. Format/usage support is
queried before creating each color image. Shaders target SPIR-V/Vulkan 1.0.
Nearest/clamp sampling reuses the existing point sampler. Gamma 1 (including
GL33's 0.999..1.001 tolerance) skips pow but retains the simple fullscreen pass;
performance measurements below decide whether a second rendering path is needed.

Both builds pass. Focused Shape/decoder checks pass 820 assertions / 37 cases ON,
808 / 35 OFF; eight stock texture mip checks pass 265 / 173 assertions. The
driver-free suite passes 286 checks, including latest-gamma constants, identity
tolerance, acquired-image/full-extent selection, one final triangle/pass and
scene-cache invalidation. Its intentional mocked teardown error is not a live
validation failure.

`gamma-after-vk-{10,06,16}` repeats all six matched sky views and additionally
resizes to 960x640, minimizes/restores and closes normally. Respectively
3780/3509/3854 submitted frames, zero Khronos core/synchronization warnings or
errors through destruction. `gamma-live-update` changes the real Graphics UI
slider 1.0 -> 1.6 -> 0.6 -> 1.0, including resize/restore at 0.6; the setting and
whole-frame brightness update visibly without stale/black frames. Normal close:
9095 frames, zero validation findings. GL33 drawing code remains untouched and
Vulkan remains opt-in. Broader effect/gameplay/performance verification follows.

### Cloud and translucent-effect comparison

The same six sky views were compared before/after against GL33. Selected sky-crop
mean absolute RGB errors (8-bit levels; smaller is closer) are:

| View | Gamma | Before | After |
| --- | ---: | ---: | ---: |
| Partial sunset | 1.0 | 1.953 | 1.807 |
| Partial sunset | 0.6 | 4.670 | 1.460 |
| Partial sunset | 1.6 | 2.393 | 1.542 |
| Heavy overcast | 1.0 | 5.721 | 5.273 |
| Heavy overcast | 0.6 | 8.459 | 5.065 |
| Heavy overcast | 1.6 | 4.288 | 4.111 |
| Foggy dawn | 0.6 | 1.461 | 0.995 |
| Moon-facing night | 1.6 | 1.140 | 0.454 |

All six crops improve in these runs at all three settings, but this is not a
deterministic image test. In particular, the small gamma-1 movement is capture
variation, not a claim that identity gamma fixes clouds. The identity shader
copies the same normalized scene color without a power operation. No visible
gamma-1 transparency, horizon, shoreline or HUD regression was found.

Shared Landscape::DrawClouds builds the same layered software geometry and
TLVertexTable::DoCloudLighting supplies cloud brightness, alpha and alpha-fog
to both backends. Weather::MoveClouds advances cloud position by speed * deltaT;
freezing after different loading intervals does not make positions identical.
This explains shifted cloud features, but does not establish the entire cause
of the remaining heavy-overcast RGB difference. That residual is not assigned
to gamma, nor concealed with a cloud-color/opacity adjustment. Existing texture
detail/LOD differences elsewhere also remain outside this fix.

`gamma-effects2-{before,after,gl33}-{10,06,16}/smoke-plume.png` captures actual
SmokeShell plumes at the same coastal camera and weather. Runtime inventory
queries verified that a mouse throw consumed SmokeShell; merely selecting or
issuing a script fire command was not counted. The corrected smoke blends with
the already-composed shoreline/sea before gamma, with no opaque replacement,
missing layers or new edge halo. Particle positions/ages differ, so no numerical
smoke-parity score is claimed. Rectangular billboard edges are also visible in
GL33 and are not evidence for changing Vulkan alpha thresholds. Initial
`gamma-effects-*` selection-only captures are excluded. The 0.6 corrected run
completed via its normal 100-second timeout after a later unsuccessful frag
grenade attempt; its smoke capture is valid, its frag attempt is not coverage.
Every corrected smoke run reports zero validation findings through shutdown.

`gamma-material-{vk,gl33}-{10,16}` repeats the HMMWV noon, low-sun, side, night
and fog views. Glass retains the reference transparency, body lighting/specular
and projected shadows remain visible, and non-unit gamma affects the completed
vehicle/background composition. Vulkan normal closes: 2129/2098 frames, zero
core/synchronization warnings or errors. Smoke HUD/pause captures and the live
Graphics gamma-slider test retain readable fonts and translucent overlays.
No additional rendering change was justified by these matched comparisons;
cloud/weather simulation, alpha thresholds, blend factors and stock assets
remain untouched.

### Final gameplay and regression verification

The isolated GOG 3.05 installation, RTX 4060 Ti, copied profiles and exact-window
bounded input helpers were reused. No game data, mission, model or texture was
edited. Before/after inventories of 7596 stock-data files and 10 repository
resource files match in path, size and modification time. Audio was disabled for
these runs; audio parity is not claimed. The system-awake work lease did not
keep the display awake or change the power plan.

All five requested missions were exercised at gamma 1.6 with Khronos core and
synchronization validation:

| Evidence under build/shadow-live | Observed coverage | Submitted frames |
| --- | --- | ---: |
| gamma-play-infantry2 | Movement, views, HUD/map/zoom, pause/resume, resize/restore, real HandGrenade smoke and ground dust, Mission Abort | 4120 |
| gamma-play-takecar2 | Movement, rifle fire/reload, HUD/map/zoom, pause/resume, resize/restore, normal close | 2092 |
| gamma-play-hmmwv | Driving, exterior/interior, shadows, HUD/map/zoom, pause/resume, resize/restore, normal close | 2750 |
| gamma-play-heavy | On-foot controls, existing M1 driver/interior/exterior, driving/exhaust/shadows, map, pause/resume, resize/restore, normal close | 3164 |
| gamma-play-shadow | Movement/mouse look, firing/reload, night vision, HUD/map/zoom, pause/resume, resize/restore, Mission Abort | 2632 |

Each completed with exit code 0 and zero validation warnings/errors through
resource destruction. An abort/close can leave the final submitted frame
unpresented; that is not a fence or validation failure. These are bounded
gameplay checks, not complete mission playthroughs.

Evidence was checked rather than inferred from input receipts: the initial
keyboard fire attempt left ammunition unchanged and is not counted. Actual
mouse fire in Take the Car changed M16 30 -> 29, then reload restored 30.
Infantry's stock HandGrenade count changed 6 -> 5 and `grenade-effect-0.png`
shows the dark explosion plume and pale ground dust. This is separate from
the sustained SmokeShell coverage above. HMMWV's driving capture is partly
occluded by foliage; Heavy Metal provides an unobstructed tank/exhaust view.
One unprotected Take the Car run ended after the player was killed; it also
shut down cleanly. The repeat and later scripted scenarios disabled player
damage only in disposable runtime state, and Heavy Metal used the existing
test harness to enter an existing M1. AI, weather simulation and stock missions
were not rewritten.

`gamma-fence-{vk,gl33}-16` repeats the road, town, sun/night/fog and opposite
fence views. The reverse fence remains correctly visible with reference-like
cutouts, and the low-sun material/shadow views retain their previous behavior.
Vulkan closed after 3466 frames with zero findings. Residual foliage detail is
still visible and is not attributed to the final gamma pass. The sky/coast,
vehicle and map captures plus the existing focused tests cover packed sky
interpolation/DXT1 rounding, the shoreline correction, AI88 precision, native
material specular, anisotropic/mip policy, projected shadows, fonts and ordered
screen batching/state-cache behavior without changing those implementations.

`gamma-weather-live` resumes simulation and requests a ten-second overcast/fog
transition at the coastal camera. Successive captures show the horizon becoming
fogged while scene/cloud rendering continues; this is not a claim that the
legacy cloud simulation finishes every layer transition in ten seconds.
Normal close: 2334 frames, zero findings. The separately frozen heavy-overcast
and dawn views above cover those completed visual states.

Timed Vulkan menu, indexed-triangle and engine-Shape diagnostics
(`gamma-{menu,triangle,shape}-vk`) exit 0 with 1472/1627/1687 frames and zero
validation findings. Both `gamma-default-gl33-{on,off}` builds launch GL33
without `--render`, render their menu and exit 0 on timeout. Vulkan remains
opt-in. Both build targets and the focused suites pass as listed above; the
final suite rerun again passes 286 driver-free checks, 820/808 Shape/decoder
assertions and 265/173 stock-mip assertions. No GL33 source was changed.

### Gamma performance and remaining limits

`gamma-perf-{road,water}-{10,16}-{on,off}-{before,after}` contains 16 sequential
RTX 4060 Ti runs at 800x600, with the same copied profile/VSync setting and
existing camera sampler. The saved pre-change binary is `45a6033`, not a fresh
checkout of `209657c`; its two retained follow-ups are repository documentation
and DXT1 source-decoding rounding, not a different steady draw/gamma pipeline.
The corrected binary SHA256 is
`4FED6E00BB66A074D096E294E5E8C4C21722618EDCE15AC2DAA3DB0991E8BD4E`.

As in the preceding baseline report, these are means of the first three complete
two-second intervals with zero transient allocations and zero texture uploads.
They exclude startup, not inconvenient frame times. All later intervals remain
in the logs and were also inspected (including the last eight-window summaries).
The road's mission/shadow/overlay work is not fully deterministic, so its
confounded pairs are explicitly identified rather than interpreted as isolated
gamma cost.

| Scene | Gamma | Validation | Frame ms before -> after | CPU command ms before -> after |
| --- | ---: | --- | --- | --- |
| HMMWV road | 1.0 | On | 7.711 -> 8.485 | 6.458 -> 7.143 |
| HMMWV road | 1.0 | Off | 6.303 -> 6.324 | 1.738 -> 1.781 |
| HMMWV road | 1.6 | On | 8.157 -> 7.034 | 6.837 -> 5.936 |
| HMMWV road | 1.6 | Off | 9.745 -> 6.326 | 1.594 -> 1.756 |
| Infantry coast | 1.0 | On | 6.301 -> 6.311 | 2.003 -> 2.019 |
| Infantry coast | 1.0 | Off | 6.316 -> 6.309 | 0.603 -> 0.613 |
| Infantry coast | 1.6 | On | 6.312 -> 6.327 | 2.147 -> 2.212 |
| Infantry coast | 1.6 | Off | 6.321 -> 6.308 | 0.599 -> 0.634 |

The first road gamma-1 validation-on pair has 9 versus 80 screen batches, despite
615 lit draws and 97/91 native/software shadows in both selected windows. At
gamma 1.6 with validation, software shadows are 91 versus zero. The gamma-1.6
validation-off baseline already contains application/frame-pacing gaps (its
p95 frame interval is 6.197 ms despite the 9.745 ms average frame period).
None of those three pairs establishes a gamma slowdown or speedup. Later
windows in both binaries also show roughly 9-14 ms average frame periods while
recording/p95 remain much lower, as in the preceding baseline work. These
pre-existing pacing/workload differences were not changed in this task.

The reverse-order gamma-1 road repeat (`gamma-perf-road-10-on-repeat-*`) runs
the corrected executable first. Its selected windows have the same 615 lit
draws and 97/91 native/software shadows, with 11 versus 10 screen batches:
frame time 8.076 -> 8.081 ms, command recording 6.795 -> 6.805 ms and p95
8.379 -> 8.402 ms (numbers are always before -> corrected). This much closer
workload comparison does not reproduce the apparent 0.774 ms initial increase.
The reverse-order gamma-1.6 validation-off road pair likewise measures
6.325 -> 6.319 ms (command recording 1.865 -> 1.838 ms), with 615 lit draws,
97/91 shadows and 6 versus 1 screen batches. All 20 performance runs, including
these four repeats, close normally with zero core/synchronization findings
where validation is enabled and zero settled transient allocations.

The coast keeps 189 lit draws, zero shadows and one screen batch in all selected
windows. Together with the validation-off gamma-1 road sample, it shows no
meaningful ordinary steady-frame regression in the tested scenes. The small
gamma-1 differences do not justify a direct-to-swapchain fast path: identity
skips pow, and the single simple composition path is retained. These are
presentation-limited measurements around 158 FPS, not uncapped GPU timing or
claims about higher resolutions. No per-frame scene-target allocation occurs.

Remaining limits: the full heavy-overcast gamma-1 residual is not yet explained;
shifted cloud/particle phases and existing texture/LOD detail differences are
not fixed by framebuffer gamma. The sRGB-only surface fallback remains untested
on this UNORM display. The loader still reports the pre-existing missing Epic
overlay JSON and notices for intentionally disabled implicit layers; these are
separate from the zero Khronos core/synchronization findings. No renderer code
or machine-wide layer configuration was changed to conceal them.

The highest-value next parity milestone is a narrowly matched gamma-1 cloud
layer investigation: record identical cloud phase, per-layer textures/mips and
software vertex colors/fog before proposing any additional rendering change.
Do not compensate for those differences with arbitrary opacity/color tuning or
a new weather/particle system.

## Cloud and foliage input parity (2026-10-10)

This investigation starts at `24f691f` with a clean working tree. The isolated
GOG 3.05 data, RTX 4060 Ti, 800x600 window and existing Infantry coastal camera
are reused. It establishes a capture correction, not a new cloud-rendering fix.

### Matched clouds: phase and camera target both matter

`Weather::MoveClouds` integrates `_cloudsSpeed * deltaT` into `_cloudsPos`.
Freezing after loading leaves different accumulated positions in each backend.
A small temporary diagnostic pins only DrawClouds' local `cPos` to 123.25;
it does not change weather simulation or stock data. It logs layer/model,
position, azimuth/scale, ordered submissions, texture/resident mip, spec flags,
transformed positions, UVs and packed CPU colors including alpha-fog.

That phase-only comparison (`build/shadow-live/cloud-phase-{vk,gl33}-10`)
reduces heavy-overcast sky-crop error from the previous 5.273 to 1.041 /255.
The remaining transformed Y/Z differences exposed another capture variable:
`CamSetTargetVec` uses `GetPos`, which adds `SurfaceYAboveWater`. This coastal
target is over water, so different simulation clocks produce different target
heights and camera pitches even with identical script coordinates. Identical
camera position/daytime alone is therefore insufficient evidence of a match.

The existing `triSetSimTime 60` command, after `setAccTime 0` and before camera
creation/target assignment, removes that variable. With both controls,
`cloud-locked-{vk,gl33}-{10,06,16}` has identical cloud trace sequences in all
five weather/time states at all three gamma settings. Heavy overcast includes
35 candidate Draw calls, 17 surviving software submissions and 68 lit vertices;
all corresponding positions, UVs and packed RGBA values compare exactly.
The three layers retain their original ordering, transforms and 0.1 scale.
The four `mrak_war_{1,3,4,5}.paa` textures all use resident mip 0 in both paths
(256x128 except `_3`, 256x256). This is not the foliage residency discrepancy.

`DoCloudLighting` supplies identical sunlight/accommodation, brightness and
alpha, with SkyFog8 already attenuating vertex alpha. Both backend submissions
use flags `0xac128`: ordinary source-alpha blending, no additional RGB fog,
1/255 alpha cutoff and the original disabled-depth semantics. GL33's ordered
alpha queue and Vulkan's immediate software submissions preserve the same
cloud order. No missing software attribute or cloud-specific shader is needed.

The same sky-only crop x=5..794, y=105..319 gives mean absolute RGB differences
in 8-bit levels (not full-frame scores):

| Synchronized view | Gamma 1.0 | Gamma 0.6 | Gamma 1.6 |
| --- | ---: | ---: | ---: |
| Clear noon | 0.097 | 0.123 | 0.073 |
| Partial overcast noon | 0.361 | 0.457 | 0.264 |
| Partial-overcast sunset | 0.408 | 0.385 | 0.351 |
| Heavy overcast noon | 0.216 | 0.281 | 0.155 |
| Foggy overcast dawn | 0.120 | 0.127 | 0.100 |

Heavy-overcast gamma-1 error thus falls about 96% relative to the previous
unsynchronized 5.273 result without changing the renderer. Visual inspection
agrees: cloud features, translucency, horizon fog and sunset halo align.
The small residual is not bit-exact parity, but no visible cloud defect has
been isolated from it. It does not justify color/opacity tuning or changes to
packed sky interpolation, alpha thresholds, gamma, fog or the weather system.
Mission subtitles/timing outside the crop remain different and are excluded.

Both diagnostic builds pass. The three synchronized Vulkan sweeps close
normally with 2791 / 2714 / 2975 submitted frames and zero Khronos core or
synchronization warnings/errors through shutdown. GL33 references also exit 0.
These probes are diagnostic-only, not a proposed production rendering change.

### Foliage: residency explains one view, not every remaining edge difference

The previous apple-tree texture attribution was incomplete. The traced
`str_jablon.p3d` at engine position (3061.3242, 230.26288, 6334.9966) uses
`kura_jablon_asi_ne.pac` and **`jablon.pac`** in LOD 0 (55 vertices), not
`jablon_renovace.pac`. The latter is resident elsewhere in the scene. Forcing
only that texture to full resolution in a diagnostic GL33 build did not change
the target crown. Section-level tracing was necessary to identify the texture.

GL33's `PrepareTexture` / `UseMipmap` requests depend on projected texture area
and distance. The target's `jablon.pac` initially loads 64x64, then 128x128 in
the town view; approaching to 12 m loads 256x256 and retains it on subsequent
views. `jablon_renovace.pac` was 64x64 with coarser requested mips. The observed
budget was only about 6.8 MB of 543 MB, with no allocation/copy-pressure boost;
the largest-mip setting and 4096 texture-size limit were not restricting these
256 textures. This is demand-driven residency, not evidence of exhausted VRAM.
Vulkan retains the authored 256x256 base plus stored mip chain.

In the gamma-1 town view, requesting mip 0 for the actual apple textures in GL33
reduces the target-crown crop's mean RGB difference from 9.892 to 0.816 /255
(x=210..489, y=165..399). The reference then shows the same finer leaf detail as
Vulkan. The 12 m comparison is already close (0.778 /255, crop 180,102..619,499)
because unmodified GL33 has loaded the full texture by then. No production
residency override is retained, and Vulkan is not deliberately downgraded.

The sweeps also cover 25 m, 65 m, reverse and side views, adjacent vegetation,
town buildings, fence fronts/backs, low sun, night and fog. The 25 m crown still
differs with full residency: 10.207 /255 in crop 280,170..519,409, using the
later `foliage-target-vk` trace. Its side view is close, 0.493 /255 in crop
270,160..549,439; the earlier Vulkan side capture was not comparably close.
Sampled target traces show LOD 0 and matching section textures, but are not a
per-capture-frame proof of identical geometry/state. Nearby trees also change
LOD within the frozen-camera runs. These residuals are **not explained by the
town-view residency result**, nor sufficiently repeatable to assign exclusively
to filtering. A strictly draw-matched geometry/LOD and filtered-cutout comparison
remains necessary. Do not call this a confirmed Vulkan decoder bug or change a
global cutoff to conceal it.

The existing opt-in stock-bank test now includes the four cloud textures and
`jablon`, `jablon_renovace`, `n_strom_13` and `krovi6`. It checks authored mip
dimensions and translucent-versus-cutout classification, and compares every
legacy foliage mip's binary alpha with the Vulkan decoder: all agree exactly.
The foliage sources are DXT1. GL33 uploads them compressed, so RGB from a
forced legacy RGB555 decode is not an appropriate reference for normal foliage
sampling. Both backends use trilinear/16x anisotropic sampling without LOD bias;
Vulkan's sampled-mip bound already matches GL33's stop before a dimension <=4.
These checks rule out missing stored mips or differing source alpha masks, not
every possible driver filtering or draw-state difference.

Evidence is in ignored `build/shadow-live/foliage-{trace-vk,trace-gl33,
full-gl33,full-family-gl33,target-vk}`. All 13-view runs exit normally; the two
Vulkan traces submit 5703 and 5265 frames with zero validation findings through
shutdown. Temporary source probes and the GL33 full-res override were removed
after archiving their patch under `build/cloud-evidence/temporary-probes.patch`.
No runtime renderer change is justified by this investigation so far.

Both probe-free builds pass. Focused rendering/decoder tests pass 820 assertions
in 37 cases with Vulkan enabled and 808 in 35 with Vulkan disabled; the expanded
stock test passes 684 / 504 assertions respectively. The driver-free Vulkan
policy suite passes 286 checks (including its intentional mock teardown error).

### Probe-free gameplay and final verification (2026-10-11)

The final binaries have no engine/shader differences from `24f691f`. Vulkan
remains opt-in; GL33, stock resources and localization are unchanged. The
retained implementation work is the focused extension of the existing stock
test, not a visual compensation or new capture/streaming framework.

On the RTX 4060 Ti with the isolated GOG 3.05 installation:

| Run under `build/shadow-live/` | Gamma | Submitted frames | Exit | Core/sync findings through shutdown |
| --- | ---: | ---: | ---: | --- |
| `foliage-final-infantry` | 1.0 | 4566 | 0 | 0 errors, 0 warnings |
| `foliage-final-hmmwv` | 1.6 | 2131 | 0 | 0 errors, 0 warnings |
| `foliage-final-shadow` | 0.6 | 2196 | 0 | 0 errors, 0 warnings |
| `foliage-final-shadow-bright` | 1.6 | 2041 | 0 | 0 errors, 0 warnings |
| `foliage-final-shape-smoke` (timed exit) | default | 1677 | 0 | 0 errors, 0 warnings |

The gameplay runs exercise bounded movement/camera input, firing/reload inputs,
HUD/map and map zoom, pause/resume, 960x640 resize, minimize/restore and normal
window-close teardown. Infantry's stock HandGrenade count decreases after a
throw; this is not a claim of new SmokeShell or matched explosion coverage.
HMMWV's position changes from (3105.91,6335.33) to (3078.48,6339.36), with vehicle,
fence, bushes and building rendering inspected. Shadow Killer's gamma-0.6 view
is too dark for useful foliage judgment; the extra gamma-1.6 run visibly verifies
night vegetation, moon/stars and the green night-vision overlay. Map/briefing
fonts and transparent HUD overlays remain intact. Coast/cloud, low-sun fence
front/back, building and foliage screenshots from the matched sweeps supplement
these live checks. No new shoreline, shadow or sky-interpolation defect was
observed; exact filtering/coverage parity is not claimed.

Both Vulkan-enabled and GL33-only RelWithDebInfo builds pass. Each starts the
default GL33 menu without `--render` and exits 0 on its timer
(`foliage-final-default-gl33-{on,off}`). Focused suites listed above pass in both
configurations, including existing gamma, AI88, mip-bound, native-lighting and
render-state policy coverage. The pre-existing missing Epic overlay JSON loader
notice remains separate from Khronos findings; no system layer settings changed.

There is no changed runtime rendering behavior to performance-benchmark in
these commits: all phase/logging/residency experiments are removed. No new
frame-time delta or speedup is claimed; the previous gamma baseline remains the
relevant measured result. Gameplay profiling around input, map and resize events
is not presented as steady-state GPU performance. Source comparison against
`24f691f` confirms unchanged renderer, synchronization and command-state caching.

Before/after metadata inventories (path, size, UTC modification ticks) match for
all 7596 isolated stock files and 10 repository resource files. No unrelated
working-tree edit was present at task start. Only this document and the existing
stock test are retained changes; diagnostic captures remain ignored locally.

The remaining 25 m foliage comparison is a historical visual follow-up, not a
gameplay compatibility priority. Preserve Vulkan's full-resolution textures;
neither texture streaming nor global alpha/color tuning is warranted.

## Sustained gameplay and mission flow (2026-10-11)

Baseline `89714c8`, crash fix `4890bfa`, isolated GOG 3.05 assets, copied KyouKyou profiles, windowed
800x600/960x640. Normal menu selection and briefings were used for the coverage
below; direct mission startup was used only for crash replay. All Vulkan runs
enabled Khronos core and synchronization validation. Audio was disabled.

| Stock mission | Gameplay and lifecycle actually exercised |
| --- | --- |
| Steal the Car | Genuine objective attempt through town/buildings/vegetation, aiming/fire/reload, weapon switching and a confirmed frag throw (6 -> 5); enemy killed the player before reaching the car. Pause/save, movement away, load restoring position and resumed play; death/load and retry; abort to menu. |
| 1985 Training | Tutorial look/move stages, run to the repair truck and back (~140 m), action tutorial and first/third-person views. Development save succeeded; ending was then **script-triggered**, followed by stock cutscenes and the next briefing. |
| 1985 Flashpoint | Normal briefing, stock helicopter passenger flight/landing/unload, squad movement ~300 m through terrain/forest, ~4 minutes of simulation before load. Menu save/load restored the post-landing state and play resumed. **Script-triggered** ending reached Combined Arms briefing; campaign selection persisted across Vulkan and default-GL33 restarts. |
| Heavy Metal | Normal entry into M1A1 as driver, ~110 m driving into forest, interior/external views, moving shadows/exhaust, exit to foot, menu save/load/resume. Development-assisted gunner seat change; cannon (24 -> 23) and machine-gun fire. **Script-triggered** ending displayed debriefing and returned to menu. |
| Ground Attack (`04Helitrain`) | Walk to Cobra and enter as pilot, rotor startup, cockpit/external views, takeoff to ~70 m and ~1 km flight. Cannon, Hellfire and FFAR consumption confirmed; airborne menu save/load restored flight. Existing auto-hover action assisted a controlled landing (alive, zero damage, near-zero speed). Abort/menu; mission objectives not completed. |
| Shadow Killer | Normal night briefing, NVG, ~200 m approach, combat/reload, genuine enemy death and load/resume. Later protected combat against stock AI, with development-set dawn/overcast/fog/rain, exposed the tracer crash. Saved-state replay tested the fix, repeated save/load, map/HUD, resize/minimize/restore and shutdown. |

There were **zero genuine completions**. The Steal the Car attempt failed in
combat; tutorial stages and helicopter landing succeeded without rewriting
mission logic. `triEndMission 'end1'` checks are simulated progression, not wins
or proof that stock victory conditions work. No gameplay solver or mission
edits were introduced. Protection, seat placement, weather/time changes and
saved-state replay were limited to explicitly assisted coverage. A SmokeShell
throw was attempted but consumption was not verified; it is not counted.

### Combat crash and fix

Shadow Killer crashed twice in `Object::DrawLines -> EngineVK::DrawLine ->
SubmitScreen -> ScreenGeometry`. The first dump contains reciprocal-W values
`+0.220856249` and `-0.419688940` on the same tracer ribbon. The diagnostic replay
confirmed another positive/negative pair with projected, non-clipped vertices.
Vulkan's positive-only check threw on the behind-eye endpoint; this is valid
homogeneous geometry, not a missing texture or resource-lifetime failure.

`ScreenGeometry` now accepts finite **nonzero signed** reciprocal-W and retains
its sign for GPU clipping, matching GL33's existing `1 / aRhw` shader path.
Zero/non-finite inputs remain errors. No GL33, shared clipping, mission or
renderer architecture change is needed. The captured-coordinate regression
test fails on the baseline and passes with the fix, including the positive-W
near-plane intersection. Default GL33 replayed the pre-crash save under fire
and exited normally. Fixed Vulkan diagnostics processed two negative-W tracers
without throwing and shut down with 10,737 submitted/presented frames and zero
validation errors/warnings. Temporary instrumentation was removed.

### Verification and limits

Both Vulkan-enabled and GL33-only RelWithDebInfo builds pass. Focused shape,
PAA, factory, window metrics/placement suites pass: 74 cases/1,693 assertions
with Vulkan, 72/1,498 without; explicit stock-mip checks pass 684/504 assertions.
The driver-free Vulkan policy/lifetime suite passes all 286 checks (its deliberate
mock teardown error is expected). No broad performance campaign was run.

The initial multi-mission flow ran ~16 wall-clock minutes and exited normally
with 71,449 submitted/presented frames, zero validation findings; Flashpoint
private memory stayed ~2.630-2.634 GB with 802 handles in the sampled forest
segment. The broader vehicle/night session subsequently crashed as described
above, so it is **not** a clean shutdown-validation pass. Frame profiling showed
zero steady-state transient allocations between loading/movement events; these
bounded samples are not a claim of leak-free multi-hour play.

The final uninstrumented replay ran ~9 minutes across Shadow Killer, Heavy Metal,
Ground Attack and Steal the Car. It repeated combat/save/load, simulated
debrief/menu, tank driving/save/load/cannon fire, airborne load/cameras/missile
fire, a fresh normal-menu airborne save/load and zero-damage auto-hover-assisted
landing. One piloting attempt crashed the helicopter; it did not crash the
renderer. Temporary harness loading from that death screen left its UI open,
so replay was restarted from live gameplay; normal menu loading restored both
gameplay and UI correctly. Combat memory samples were 1.7877/1.7847 GB with 799
handles across another load (asset caches grew when changing missions).
Resize/minimize/restore and normal exit passed: 37,360 submitted / 37,359 presented,
zero validation errors/warnings through shutdown. The one unpresented submission
is consistent with the resize/out-of-date presentation path, not an outstanding
frame at shutdown. Both builds also started the default GL33 menu without a
renderer argument and timed out normally, exit 0 (`gameplay-final-default-gl33-{on,off}`).

One initial Steal the Car death showed a gray background and a shared-engine
"Ground drawing segment too big" warning, but death controls/load worked.
Subsequent Vulkan/GL33 death replays and the natural Shadow Killer death did not
reproduce it; a final Vulkan enemy-caused death at the original location also
rendered correctly. It remains an unisolated observation, not a verified fix.

Assessment: these checks support real single-player gameplay beyond rendering
smoke tests, but Vulkan should remain opt-in. Complete mission wins, an unassisted
campaign, multi-hour stability, fixed-wing aircraft and multiplayer are not
qualified by this run. No stock assets, missions or localization were changed;
all 7,596 stock files matched the starting path/size/UTC-mtime inventory.
Ignored evidence lives in `build/shadow-live/mission-flow-vk2`,
`mission-breadth-vk`, `tracer-{diag-vk,reference-gl33,fixed-diagnostic-vk}` and
`mission-final-vk` (screenshots, actions, profiles, logs and crash dumps).

## Optional SSAO: depth access milestone (2026-10-11)

The existing per-swapchain depth/stencil target is now sampled-capable and
stores depth after the scene pass. A separate depth-only view/point-clamp
descriptor leaves the depth/stencil attachment view and projected shadows
unchanged. The same D32S8/D24S8 preference is retained, requiring both attachment
and sampling support. Early/late depth writes become visible to fragment
sampling in depth/stencil-read-only layout at pass end. Views/descriptors follow
the existing idle-before-recreate/shutdown lifetime. No extra depth render pass
or visual effect is enabled by this milestone; gamma and UI ordering are unchanged.

Both builds and existing focused shape/PAA/factory/window tests pass; the Vulkan
policy suite passes 286 checks. Live Steal the Car and Heavy Metal checks cover
geometry, shadows, firing, map, 800x600 -> 960x640 resize and minimize/restore.
`ssao-depth-resize-vk` visibly resumed gameplay and exited 0 with zero core/sync
validation errors or warnings through shutdown. Evidence is ignored locally
under `build/shadow-live/ssao-depth*`.

### Basic SSAO pass

SSAO is opt-in with `CWR_VK_SSAO=1` before launch. The existing developer
console/harness can toggle/tune it with `triSSAO [1,1.2,1.5,0.03,80]`
(enabled, strength, radius metres, bias metres, fade distance metres); use
zero for enabled to disable. Invalid/nonfinite parameters are rejected without
changing state. GL33 returns false and its rendering code is unchanged.

One fullscreen draw reconstructs view positions/normals from depth, samples a
fixed eight-direction kernel, and multiplicatively blends restrained occlusion
into existing scene color. World geometry/shadows/transparency precede AO;
cockpit/weapon overlays, HUD/map/UI and the unchanged final gamma pass follow.
The world composite runs before the cockpit pass; interior depth clears also
guard against replacing its depth projection early. Menu/briefing/map/pause
rendering skips AO. A compatible LOAD pass
resumes color/depth/stencil afterward. No extra images, geometry pass, per-frame
allocations, blur, temporal history or material changes are introduced.

Initial Steal the Car off/on, movement/fire/reload, map/pause and resize/restore
checks passed (`ssao-basic2-vk`): normal exit 0, 4,224 submitted / 4,223 presented,
zero core/synchronization validation errors/warnings through shutdown. Both
builds pass; stock-backed focused tests pass 1,693 Vulkan / 1,498 GL33 assertions;
driver-free policy tests pass 295 checks, including AO toggle/parameter guards.
Broader visual tuning and performance qualification follow separately.

### Stock-scene tuning and gameplay checks

The opt-in defaults are strength **1.2**, radius **1.5 m**, bias **0.03 m**,
with fading from **48 to 80 m** and a 35% modulation ceiling. Paired 1920x1080
captures showed modest vehicle/ground, wall/fence and foliage contact shading,
without broad black outlines or replacing the original projected shadows.
The fixed sea-view off/on captures were identical. GL33 airfield, tank exterior
and nighttime references were also checked (qualitative, not pixel parity).

`ssao-quality2-vk` exercised normal single-mission menus/briefings and transitions
between these stock scenarios in one validation-enabled process:

- **Infantry / Ambush:** village/fence movement, aiming, firing/reload, pause,
  normal Save/Load restoring position and resumed movement.
- **HMMWV:** village/wooded terrain driving, interior glass and exterior cameras.
- **Heavy Metal:** M1 driver movement, gunner optics and cannon fire (24 -> 23).
- **Ground Attack:** airfield, Cobra cockpit/exterior, rotor, takeoff to ~20 m,
  cannon/missile fire (500 -> 499 / 8 -> 7), normal airborne Save/Load.
- **Shadow Killer:** loaded later combat, movement/fire/reload, NVG on/off,
  map, resize to 1280x720 and minimize/restore.

Developer assistance relocated Infantry to its village, selected vehicle seats,
protected some combat checks and restored a prior Shadow Killer combat save.
Fixed/frozen views were used only for image comparisons, not claimed as played
mission progression. This was not a mission-completion test. Normal shutdown
returned 0 with 62,340 submitted / 62,338 presented and zero Khronos core/sync
errors or warnings; the two unmatched presentations coincided with recreation.
Local raw captures/logs/profiles are under `build/shadow-live/ssao-quality2-vk`,
`ssao-reference-gl33` and `ssao-tank-gl33` (ignored, not stock-asset changes).

### Performance and final qualification

RTX 4060 Ti, driver 617.14, 1920x1080, warmed fixed views, VSync off, existing
240 FPS cap not reached, validation off. Medians of the existing two-second
profile intervals over 12-second off/on samples (`ssao-final-perf3-air-vk`,
`ssao-gpu-tank-vk`):

| Scene | Frame p95 off / on | AO GPU ms/pass | AO CPU recording ms/frame | New steady allocations/frame |
| --- | --- | --- | --- | --- |
| Ground Attack airfield | 6.227 / 6.216 ms | 0.263 | 0.012 | 0 |
| Heavy Metal exterior | 6.198 / 6.210 ms | 0.342 | 0.013 | 0 |

Whole-frame interval averages were noisy (final airfield 6.360 -> 6.366 ms, tank
13.429 -> 11.175 ms); repeated earlier pairs also changed sign. They do **not**
establish an end-to-end FPS penalty or speedup. The GPU interval and AO-specific
CPU recording cost are the useful isolated measurements; no optimization was
made on the basis of the noisy averages. There is no extra geometry/depth pass:
sampling reuses the original depth. Depth store/layout overhead was not isolated
from the full frame. Two timestamps per frame slot are allocated only when
`CWR_VK_PROFILE` is enabled and the graphics queue supports them, read after the
existing fence without a new wait, and destroyed with frame resources. The
`gpu_ms/pass=-1` diagnostic means no completed sample, not negative GPU time.

The final instrumented HMMWV check repeated driving, assisted exit/re-entry,
normal Save/Load, map, AO toggles and resize/minimize/restore: exit 0, 7,106
submitted / 7,105 presented, zero core/sync errors/warnings through shutdown.
SSAO with profiling disabled also passed live movement/fire/reload and shutdown
(3,519/3,519, zero messages). A further Steal the Car grenade throw (6 -> 5)
and developer-spawned stock smoke effect rendered with usable HUD and transparent
particles; its timed normal exit was clean (11,624/11,624, zero messages).
Final ordering review caught infantry optics bypassing the interior depth-clear
hook. Moving AO before the entire cockpit pass excludes these scopes and weapon
overlays too. Sniper Team M21 off/on optics, firing/map checks and Ground Attack
airborne cockpit/load checks passed with zero core/sync messages through normal
shutdown (7,693/7,693 and 4,904/4,904). Separate paired cockpit captures confirm
unchanged instruments. A later malformed harness ammo query ended one diagnostic
run in test-mode script-error shutdown; it is not counted as normal-exit evidence.
Both builds and default GL33 startup pass. Final focused suites pass 74/72 cases
(1,693/1,498 assertions), stock-mip checks 684/504 assertions and Vulkan policy
guards 295 checks. All 7,596 stock files retain their starting path/size/mtime.

Limitations: this is deliberately modest depth-only SSAO, not ambient lighting
replacement. Hidden/off-screen geometry cannot occlude; thin/cutout surfaces and
screen edges are approximate, with no temporal stabilization or blur. Transparent
world effects receive modulation based on the opaque depth behind them; they
have no separate AO mask. No serious transparency, water, shadow, UI or gamma
regression was observed in these checks, but arbitrary mods, unusual world-view
crops and multi-hour sessions are not qualified. Cockpit/weapon overlays are
excluded along with UI. Enablement is process-local/developer-console only,
not yet a persistent graphics-menu option. Keep the enhancement opt-in.
