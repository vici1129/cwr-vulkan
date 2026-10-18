# Graphics quality and visual tuning

The [RT-water transition follow-up](rt-water-transitions.md) diagnoses and
corrects the shoreline rejection seam on top of `aadbc5e`. Its current visual
and performance evidence is separate from the historical measurements below.
RT water is now an explicit Water Quality choice in unified Video Settings;
corrected volumetric sunlight is an independent Off/On preference.

## Player menu (updated 2026-10-19)

The unified [Video Settings page](video-settings.md) implements the exact Vulkan
group mappings below in its GRAPHICS section. DISPLAY and IMAGE QUALITY contain
display modes, VSync/frame cap, brightness/gamma, AA/upscaling/scale, view distance
and scene detail on the same scrollable notebook page. GL33 shows its meaningful
legacy controls instead of Vulkan-only groups. Original mod Video resources
retain a compatibility fallback. No original PBO or asset is edited.

Overall **Standard** selects Standard shadows/clouds/water and AO Off;
**Enhanced** selects Enhanced shadows/clouds/water and AO On. A matching set of
individual choices is recognized as its preset; all other combinations show
**Custom**. New Vulkan settings default to Standard. Existing display/detail
preferences are independent of these presets. Developer-created partial groups
are displayed as Custom; select a supported group or preset before applying.
All supported shadow choices enable the original object/vehicle caster gates
needed by CSM, including profiles migrated from the old Shadows Off setting.
These are runtime overrides: Vulkan saves its shadow choice only in
`vulkanShadows`, without changing the legacy `shadowQuality` tier or the profile's
`objectShadows`/`vehicleShadows` preferences. Unified Video and the compatibility
Video dialog preserve those saved GL33 preferences too; shadow-gate edits in a
Vulkan session remain process-local. GL33 continues to load and save its own
shadow preferences normally, including profiles without explicit shadow fields.
Disabling these gates through legacy controls is observed as a Custom shadow
group; the new Shadow Quality control owns their enabled state when applied.

Selections are staged: browsing does no resource recreation. **Apply** sends one
complete bundle, coalesces deferred resources into the next frame's rebuild,
checks renderer acceptance/fallback, then saves `vulkanShadows`,
`vulkanClouds`, `vulkanWater`, `vulkanAO` and independent `vulkanVolumetricLighting` in the existing per-user
`graphics.cfg`. **OK** does the same and returns; **Cancel/Escape** discards
unapplied selections. During activation, controls wait for completion. Rejected
requests and resource failures show an error and actual renderer choices without
saving the failed bundle. A file-write failure is reported and Apply can retry.
The unified page also stages its independent display/image/detail selections;
display changes retain the existing timed Keep/Revert confirmation.

Saved Vulkan choices are requested during the existing graphics configuration
boot path, after device creation and before ordinary world rendering. The four
automatic Vulkan effects (atmosphere, classified wet surfaces, underwater optics,
allowlisted PBR) start enabled with their existing parameters and gates. They do
not affect GL33. `tri` effect commands remain available and process-local; the
menu observes their actual group state when it opens and after external changes.
Player presets do not change bloom, volumetric sunlight, sun shafts,
RTGI, RTAO, AA/upscaling, render scale, or internal artistic tuning. Existing
shared RT scene consumers retain dynamic geometry while any consumer needs it.
Water offers Standard, Enhanced and Ray Traced: the last adds the corrected
hardware/sky/cloud reflections and supported dynamic geometry to Enhanced and
derives Custom. Volumetric Lighting defaults Off; its independent On/Off choice
does not change preset recognition and is preserved when selecting a preset.

**Ray Traced Shadows and Water require initialized ray-query capability.** Without it the
selectors offer only Standard/Enhanced and their hints explain the
`CWR_VK_RTAO=1` startup/restart requirement or hardware limitation. A previously
saved RT component falls back to Enhanced for that process; the saved
preference is preserved until the player explicitly applies another choice for
that component (or a raster preset). Changing independent lighting does not erase it.
Availability is based on device negotiation and independent MRT blending, never
GPU brand. This menu does not change the device bootstrap. Player-friendly RT
startup opt-in is a focused follow-up. AA/upscaling and render scale are now
available on the unified page with actual capability checks. FSR retains its
existing build and `CWR_VK_FSR=1` startup requirements.

The measurements and visual gates below remain the authoritative mappings and
qualification record. The original report describes the pre-menu state.
Menu implementation checks and unretouched screenshots are recorded in the
[menu verification evidence](evidence/graphics-menu/README.md).

This pass starts at public main `cdf53e1` and proposes a compact Vulkan settings
design. It does not implement a menu or persistence. GL33 and the private
DLSS/NGX checkout are outside the change set. All screenshots are actual stock
game rendering; all timing claims below distinguish GPU work from CPU work and
presentation pacing. There is no SSR implementation or SSR setting.

**Outcome:** two raster presets, three compact groups and one AO toggle; optional
RT sunlight, but no first-menu RT water, bloom or volumetric-lighting control.
All numerical tuning candidates were rejected or withheld for insufficient
evidence. This is a documentation/evidence change: renderer constants and shipped
defaults remain unchanged. The proposed automatic/enhanced mapping supplies the
useful remastered differences without a new artistic intensity ladder.

## Audit and proposed ownership

Most optional effects currently default **Off** and use process-local developer
commands. The proposal below is not a claim that those defaults have already
shipped. Renderer capability and requested/active state must be distinguished.

| Effect | Classification | Existing behavior, dependency and boundary |
|---|---|---|
| Atmospheric perspective | Automatic candidate | Vulkan shape shader tints the original fog's in-scattered light. Original extinction, view distance and horizon endpoint remain; fades with weather/night/NVG. No extra pass or RT requirement. |
| Wet surfaces | Automatic candidate | Rain-driven, classified road/soil/rock/sand response with about two seconds of simulation-time smoothing. Excludes mixed grass, foliage, clothing and unidentified atlases. No shelter tracing, puddles or accumulated wetness system. No extra pass. |
| Underwater | Automatic candidate | Existing world composite, camera below animated sea and above seabed; HUD excluded. No extra pass, caustics or replacement underside geometry. |
| Limited PBR | Automatic candidate | Five explicitly supported stock texture identities, native opaque geometry only; dielectric response. Does not convert arbitrary stock materials. No new assets or RT requirement. |
| Bloom | On/Off candidate | Four-level HDR pyramid, threshold 1.25, strength .15; world only, before tone mapping. SDR works; an HDR display is not required. Bright source response is deliberately small. |
| Ambient occlusion | On/Off candidate | Existing SSAO at strength 1.2, radius 1.5 m, bias .03, fade 80 m. Full-resolution depth pass. RTAO is a separate implementation, not automatically selected on an RT GPU. |
| Volumetric lighting | On/Off candidate | Existing half-resolution, 48-step, 400 m sunlight integration plus CSM depth. Weather/night/NVG gates remain. Surface shadows Off would not eliminate its CSM depth dependency. |
| Sun shafts | Hidden alternative | Quarter-resolution screen-space approximation; volumetrics suppress it. Do not expose two overlapping sunlight toggles. |
| CSM, soft CSM, contact and local shadows | Grouped shadow candidates | CSM remains the distance fallback. Soft CSM changes direct/ambient separation as well as penumbrae. Contact is screen-space and short-range. Local shadows select at most two 1024-square spotlights within 160 m. |
| Hardware sunlight | Optional shadow candidate | Four-ray soft sunlight plus bounded dynamic scene, with CSM retained. Shared ray-query capability is required. Near-field geometry, alpha/proxy and dynamic budgets limit coverage. |
| Volumetric clouds and cloud shadows | Grouped cloud candidate | Half-resolution visible volume replaces stock cloud layers. Original coverage/drift/time/weather are shared with broad cloud shadows; the shadow is not an exact projection of every density lobe. NVG/failure uses stock clouds. |
| Water refraction and shoreline foam | Grouped water candidate | Share the original opaque-color/depth capture. Raster coastal terrain only; missing bottom/unsupported geometry retains original water. Foam does not wrap arbitrary objects. |
| Rain ripples | Water subfeature | Actual water, driven by simulation rain/time; no standalone player switch. No geometry displacement or RT requirement. |
| RT water and weather-sky misses | Optional water candidate | Textured bounded ray hits, 80 m receiver/70 m ray ranges, stock fallback. Sky reflections require RT water. Volumetric clouds also increase the water miss-shader cost. No SSR or planar-reflection tier. |
| RTGI | Developer only | Preserve the current optional one-bounce prototype and history behavior. Not an overall-preset dependency. |
| RTAO, hard-vs-soft RT, ray-scene budgets | Developer only | Avoid a second AO quality ladder and caster-budget UI. Capability does not prove a player-visible benefit in every scene. |

Code anchors: `VulkanContext.hpp` contains defaults; `VulkanContext.cpp` negotiates
capabilities; `EngineVK_Atmosphere.cpp` and `EngineVK_Weather.cpp` supply original
environment data; `WetSurfaceVK.hpp` and `PBRMaterialVK.hpp` bound material coverage.
The feature sections of [the renderer record](vulkan-foundation.md) retain prior
implementation and qualification evidence, including per-effect GPU timestamps.

## Measurement method

RTX 4060 Ti 8 GB, driver 617.14, Core i7-12700F, Windows, RelWithDebInfo, stock
GOG 3.05 data. Direct development launches use isolated copied profiles and sound
disabled; these are not Steam-route or mission-completion tests. No driver,
clock, power-plan, private SDK or original game-data changes are involved.

Native AA Off/100% scale and SDR isolate effect costs. Real Malden
`01TakeTheCar.ABEL` and Everon `05HeavyMetal.Eden` supply geometry and simulation.
Fixed cameras and frozen simulation make each within-session comparison matched;
separate movement runs check ordinary input and temporal AA. View distance is
1400 m, four 2048-square CSM cascades with the existing profile's effective reach.

Each state settles for 3 seconds, then records 6.2 seconds of the existing
two-second profiler windows. Final combined trials run Standard/Enhanced/RT;
coast and Malden town repeat in reverse order. Earlier exploratory combined
trials repeat every view in reverse order. Individual trials run Off/On/On/Off. Screenshots are taken
after sampling, never during a measured window. No builds run during sampling.
GPU clocks remain automatic and are recorded. Negative deltas are not speedups.
CPU frame interval includes pacing; CPU command recording is reported separately.
Frozen steady-state cost excludes traversal/animation and cold resource creation.
1440p is a verified 2560x1440 drawable on the 1080p desktop, not a claim of a
full-monitor 1440p visual inspection.

Rain density, smoothed wetness and cloud phase are logged, not assumed equal
across launches. Frozen simulation preserves recent wetness even after a dry
weather command. Comparisons within a frozen state are matched; comparing two
resolutions or two executable launches is not an exact pixel/state replay.

The initial Malden road view was labeled `village` by the local runner. It is
reported as **road**, with a separate `town` capture and measurement for the
actual village. An initial run ended normally after moving the player and
advancing simulation; only its completed fixed samples are retained. Subsequent
fixed-view runs leave the mission actor in place.

## Proposed menu and mappings

Use two overall presets, **Standard** and **Enhanced**, plus Custom when a
player changes a component. Enhanced is the recommended starting point for the
tested RTX 4060 Ti; this is a proposed mapping, not newly persisted behavior.
Both use the same artistic constants and automatic improvements. Neither selects
hardware RT, changes resolution/AA, or increases color/bloom intensity.

| Control | Choices | Exact mapping |
|---|---|---|
| Overall | Standard / Enhanced / Custom | Standard selects the three Standard groups and AO Off. Enhanced selects the three Enhanced groups and AO On. RT remains an explicit component choice. |
| Shadows | Standard / Enhanced / Ray Traced | Standard: four 2048 CSM cascades, existing coverage and projected fallback. Enhanced: add soft CSM, contact shadows and up to two local spotlight shadow maps. Ray Traced: Enhanced plus soft RT sunlight and bounded dynamic ray geometry; retain CSM. |
| Clouds | Standard / Enhanced | Original stock layers / volumetric layers plus synchronized broad cloud shadows. No extra step-count, resolution or density levels. |
| Water | Standard / Enhanced | Original above-water shading / refraction + shoreline foam + rain ripples. Underwater optics remain automatic in every choice. Hold the tested Ray Traced candidate out of the initial menu; see the visual gate below. |
| Ambient occlusion | Off / On | SSAO at its existing fixed parameters. Do not silently substitute RTAO or enable RTGI. |

Resolution, window/display mode, VSync/frame cap, AA/upscaling and world render
scale form a separate display section. Preserve the existing supported AA
algorithms and device sample-count checks; TAA is a practical native-resolution
starting point, with FSR Quality a separate reconstruction choice. The effect
benchmark primarily uses AA Off and is not an end-to-end TAA/FSR FPS promise.
Do not expose motion-debug modes, private DLSS, dynamic resolution or SSR.
Existing view-distance and object/terrain-detail controls have a legitimate
scene-complexity/visibility purpose; do not relabel them as shader quality or
silently change them with these new presets.

Automatic in Vulkan: atmospheric perspective, classified wet surfaces, bounded
underwater optics and the existing five-texture PBR allowlist. Retain their
current numerical response, weather/time/NVG gates and fallback behavior.
Rain ripples belong to Water Enhanced rather than a separate weather slider.
Sky reflections and dynamic RT geometry are dependencies owned by the relevant
RT choice, not additional player switches. If either shadow or water RT needs
the shared scene, turning off the other must not tear that scene down.

**Volumetric lighting remains developer-only and Off in both presets.** The
original matched Malden road view exposed repeated dark bands from terrain
added to the shared CSM maps. The focused
[terrain-shadow correction](evidence/volumetric-road/README.md) diagnoses the
surface self-shadowing and fine-detail resampling errors, corrects terrain
geometry/bias, and records new road/forest/Everon checks and costs. It preserves
the existing atmospheric tuning. This milestone adds no player control; eventual
exposure still needs only On/Off, not a sample-count ladder. Do not automatically
enable screen-space shafts as a substitute. The older measurements and rejected
tuning trials below remain historical evidence from before that correction.

**Leave bloom hidden and Off for this proposed first menu.** Existing source
energy produces little or no visible difference in the reviewed views; previous
headlight/fire qualification also found roughly one-code-value changes. The
stronger live trial does not establish a meaningful improvement. Keep its
developer command and current constants. If broader source tests later justify
exposure, use On/Off, not multiple strengths or an Ultra appearance variant.

RTGI, RTAO, detailed diagnostics and unsupported rendering modes remain developer
only. RT water also stays experimental for this first menu. No intermediate cloud/water/shadow tiers are invented, and there is no
overall Ray Traced or Ultra preset. The tested RT candidates explicitly trade
near-field reflected/caster detail for extra GPU and CPU work; they do not
promise complete scene coverage or a general replacement for CSM.

**RT water has a meaningful difference but does not yet pass the visual gate.**
Both coastline comparisons show reflected terrain/pier detail that original
water lacks, but also an abrupt boundary between reflected and original water.
The limited receiver/ray range is a source-level lead, not a diagnosed fix.
Keep the measurements and developer controls; do not present this as an
unqualified higher-quality water tier. A future Ray Traced choice would map to
Enhanced plus `RTReflections`, `SkyReflections` and shared `RTDynamic`, after
the transition is qualified in shoreline traversal. Increasing Fresnel cannot
repair that coverage boundary.

## Capability and application contract

RT means Vulkan 1.2 with `rayQuery`, `accelerationStructure`,
`bufferDeviceAddress`, `shaderInt64`, the required acceleration-structure formats,
and a compute-capable graphics queue. The current bootstrap requests that device
path through `CWR_VK_RTAO=1`; despite its historical name, this is the shared
capability opt-in for sunlight, water and GI. A supported GPU with the opt-in
absent cannot enable RT in an already-created device. Restart is required to
change that bootstrap decision. Once capability is initialized, effect toggles
are live; resource recreation can stall and temporal history resets.

FSR 3.1.4 similarly requires the public optional build and `CWR_VK_FSR=1`
at startup. Runtime status, not the NVIDIA brand or a menu label, determines
availability. Changing to FSR requires world render scale 100%; FSR's own mode
sets its internal resolution. Existing independent render scale is supersampling
at 100/125/150/200%, not a general 50-200% slider or dynamic resolution system.
MSAA choices must obey the device's actual color/depth sample support. None of
these decisions changes bloom, color, fog or material strength.

Renderer selection remains separate: GL33 keeps its existing appearance and
settings. Vulkan-only controls are unavailable in GL33. No renderer-selection,
resolution, display-mode or HDR capability changes are implemented here. HDR10
requires a negotiated compatible swapchain/output; linear HDR lighting/bloom
also work with SDR. HDR brightness/calibration is a display concern, not a
quality tier. Private DLSS/NGX remains outside the public menu proposal.

Keep diagnostic commands available to developers: shadow resolution/cascades,
bias/distance/darkness, AO radius/strength, bloom threshold, motion visualization,
ray/texture probes, pass counters, forced fallback paths, and RTGI history flags.
They do not become arbitrary player sliders simply because a command exists.

## Why these groups earn a control

- **Shadows:** the enhanced filter is subtle at noon; it is not an excuse for
  three raster resolution tiers. Existing dawn building/forest comparisons show
  penumbra changes and preserved ambient light, and actual nighttime spotlights
  add caster silhouettes absent from Standard. See the matched
  [soft-CSM evidence](vulkan-foundation.md#optional-contact-hardening-csm-2026-10-12)
  and [local-light pair](evidence/local-shadows/malden-on.png). RT sunlight adds
  near-field tree/building detail in the new village pair. None replaces the
  distant CSM fallback. Keep one enhanced raster group and one optional RT group.
- **Clouds:** volume changes silhouettes, internal shading and large-scale
  light/shade; this is conspicuous, not a sample-count micro-tier. It costs a
  dedicated pass and becomes especially expensive in RT water misses. Sunset
  lobes are visibly softer than stock layers: Enhanced is an optional appearance,
  not a claim that every original sky looks worse. Preserve Standard.
- **Water:** shoreline translucency/absorption and terrain-edge foam distinguish
  Enhanced when a bottom/shore is visible. The low, far-facing coast benchmark
  is less revealing than the previously qualified
  [Malden shallow-water pair](evidence/water-refraction/malden-on.png).
  Shared captures and additional image storage justify grouping these features
  rather than making all water enhancements automatic. Rain ripples alone do
  not earn a tier. RT reflection detail earns further development, not exposure
  while the visible transition remains.
- **AO:** modest wall/fence/ground contact shading merits one preference toggle,
  supported by the existing stock-scene and vehicle tests; it is deliberately
  not a dramatic darkening filter. No evidence supports multiple SSAO strengths
  or an RTAO tier. In the new village view its difference is subtle; do not use
  that view to claim a large visual or FPS improvement.
- **Automatic features:** their bounded material/weather/camera domains and
  existing fallback tests support automatic Vulkan use at unchanged constants.
  This is not blanket PBR, arbitrary-mod wetness support or new underwater
  geometry. See the existing PBR, wet-surface and underwater qualification in
  the renderer record. The new rain group timing below is an aggregate check,
  not an isolated cost estimate for a submerged camera or every material.

The old nighttime fixed view has no selected local lights (`active=0`). Its low
cost must not be advertised as the cost of two active local shadow maps. Reused
active-light measurements are 1.767 -> 2.205 ms GPU at 1080p and 1.887 ->
2.269 ms at 1440p; corresponding paced CPU intervals are 6.189 -> 7.158 and
6.188 -> 6.758 ms. Those prior controlled tests are explicitly separate from
this pass's new fixed-view table.

## New performance evidence

Milliseconds, median of completed profiler windows. Each triple is **Standard /
Enhanced / RT stress candidate**. RT stress enables *both* RT sunlight and RT
water; it is not a proposed overall preset, and its water component failed the
visual gate. All three have the same four automatic features. Bloom, volumetric
lighting, screen-space shafts, RTAO and RTGI are Off. These are actual whole-GPU
frame measurements, not sums of theoretical effect costs or reciprocal-FPS claims.

| Island / view | 1080p GPU ms | 1440p GPU ms |
|---|---|---|
| Malden village | 1.676 / 2.211 / 3.005 | 1.982 / 3.075 / 5.011 |
| Malden road | 1.982 / 2.128 / 2.605 | 2.088 / 2.659 / 4.369 |
| Malden sunset | 1.141 / 2.047 / 2.318 | 1.751 / 2.292 / 3.831 |
| Malden rain | 1.746 / 2.123 / 2.417 | 1.963 / 2.631 / 4.324 |
| Malden coast | 1.721 / 2.252 / 3.681 | 2.109 / 3.193 / 6.663 |
| Malden night | 1.259 / 1.834 / 2.119 | 1.849 / 2.093 / 3.125 |
| Everon village | 1.967 / 2.215 / 2.880 | 2.092 / 3.180 / 4.917 |
| Everon sunset | 1.156 / 2.009 / 2.224 | 1.645 / 2.180 / 3.581 |
| Everon rain | 1.853 / 2.208 / 2.758 | 2.004 / 3.134 / 4.824 |
| Everon coast | 1.718 / 2.213 / 3.648 | 2.057 / 3.233 / 6.622 |
| Everon night | 1.738 / 2.026 / 2.231 | 1.872 / 2.203 / 3.588 |

CPU command recording (not total CPU frame time), same triple order:

| Island / view | 1080p CPU command ms | 1440p CPU command ms |
|---|---|---|
| Malden village | 3.219 / 3.213 / 3.788 | 3.210 / 3.319 / 3.755 |
| Malden road | 2.788 / 2.770 / 3.081 | 2.993 / 2.765 / 3.284 |
| Malden sunset | 4.359 / 4.509 / 4.688 | 4.483 / 4.638 / 4.739 |
| Malden rain | .774 / .730 / 1.002 | 1.139 / 1.107 / 1.441 |
| Malden coast | 4.646 / 4.648 / 4.781 | 4.662 / 4.675 / 4.971 |
| Malden night | 2.959 / 2.921 / 3.199 | 2.809 / 2.756 / 3.292 |
| Everon village | 1.448 / 1.397 / 1.587 | 1.412 / 1.403 / 1.853 |
| Everon sunset | .385 / .259 / .316 | .323 / .273 / .371 |
| Everon rain | .762 / .718 / .945 | .809 / .833 / 1.041 |
| Everon coast | 2.662 / 2.649 / 2.928 | 2.680 / 2.879 / 3.215 |
| Everon night | 1.493 / 1.450 / 1.632 | 1.482 / 1.464 / 1.647 |

The logged CPU/wall frame interval mostly remains near 6.17 ms. Malden sunset/
coast are around 6.25-6.42 ms; the 1440p RT coast rises to 6.769 ms on Malden
and 6.635 ms on Everon. The relatively flat wall intervals do not imply that
effects are free, or that disabling them necessarily raises actual FPS.
There were zero new steady streaming allocations/frame in these measured rows.
Dense active AI battles, cold shader/resource creation, VRAM pressure, other
GPUs, arbitrary mods and long traversal tails are not characterized here.

### Isolated expensive effects

Whole GPU frame Off -> On, ABBA trials on Malden, Standard as the base. RT water
uses Enhanced raster water but stock clouds; these isolated deltas cannot be
added to predict the combined volumetric-cloud/RT-water workload.

| Group / active view | 1080p GPU ms | 1440p GPU ms | Interpretation |
|---|---|---|---|
| Automatic group / rain | 2.006 -> 1.753 | 2.109 -> 2.101 | No reliable incremental cost isolated; not a speedup. |
| Soft/contact/local / village | 2.013 -> 1.875 | 2.052 -> 2.144 | Local lights inactive; reuse active-night figures above. |
| Soft RT sunlight + dynamic / village | 1.991 -> 2.123 | 2.046 -> 2.656 | Meaningful shadow detail and extra CPU collection/recording. |
| SSAO / village | 2.020 -> 1.893 | 2.059 -> 2.086 | Whole-frame clock confounding; use pass timestamps as well. |
| Bloom / sunset | 1.634 -> 1.452 | 1.927 -> 1.731 | Added pass, no justified visual gain; negative delta is not an optimization. |
| Volumetric sunlight / active dawn forest | 1.980 -> 2.213 | 2.162 -> 3.369 | Scattering confirmed active; not the inactive sunset trial. |
| Volumetric clouds + cloud shadows / sunset | 1.755 -> 1.921 | 1.767 -> 2.195 | Visible cloud replacement; no sunlight cloud-shadow contribution at this hour. |
| Refraction + foam + rain details / coast | 2.040 -> 1.954 | 2.170 -> 2.181 | Small difference in this far-facing view; prior shallow-water measurement is more revealing. |
| RT water + weather sky + dynamic / coast | 2.124 -> 2.125 | 2.215 -> 2.828 | Stock-cloud case; does not represent the more costly combined cloud volume. |

The RT sunlight trial's CPU command median rises 3.227 -> 3.692 ms at 1080p
and 3.203 -> 3.672 ms at 1440p. Raster shadow grouping at 1440p changes
3.191 -> 3.210 ms; water changes 4.659 -> 4.718 ms. These short frozen-view
trials do not isolate simulation or moving-skinned-geometry CPU cost.
The active volumetric pass itself is approximately .86 / 1.40 ms (1080/1440),
independently confirming substantial work despite clock-shifted whole-frame
deltas. Prior refraction capture+optics timings were .314 / .357 ms in the
shallow raster-only beach view; prior SSAO passes were .263-.342 ms in active
stock airfield/tank views. Different scenes, clocks and renderer revisions mean
these are context, not interchangeable measurements.

Raw samples, status strings, GPU clock/power snapshots and per-group median /
minimum / maximum / sample counts are in [the evidence directory](evidence/graphics-quality/)
and [summary.json](evidence/graphics-quality/summary.json). Final table sources:
`quality-malden-1080-final`, `quality-malden-1440-final`,
`quality-everon-1080-final`, `quality-everon-1440-resumed`. The interrupted
`quality-everon-1440-final` has only 17 completed rows and is not substituted
for the resumed run. Earlier `enhanced`/`ray` labels include bloom and volume;
only `recommended`/`recommended-ray` mean the final mappings used above.

### Reconstruction is separate

Additional Malden coast samples, same baseline executable and Enhanced mapping.
Each cell is **GPU / paced CPU frame / CPU command** milliseconds. These are
short settled windows, not a full AA comparison or a traversal FPS test.

| Effects / AA | 1080p output | 1440p output |
|---|---|---|
| Enhanced, native TAA | 2.377 / 7.001 / 5.804 | 3.467 / 6.811 / 5.881 |
| Enhanced, FSR Quality | 2.174 / 6.346 / 6.053 | 2.715 / 6.309 / 6.033 |
| Enhanced + RT sunlight only, native TAA | 2.632 / 6.251 / 5.988 | 4.356 / 6.294 / 6.038 |
| Enhanced + RT sunlight only, FSR Quality | 2.260 / 6.393 / 6.112 | 3.104 / 6.342 / 6.052 |
| Enhanced + both RT candidates, native TAA | 3.862 / 6.250 / 5.989 | 6.974 / 7.007 / 6.303 |
| Enhanced + both RT candidates, FSR Quality | 2.555 / 6.375 / 6.053 | 4.094 / 6.310 / 6.025 |

FSR Quality internally renders 1280x720 / 1706x960. It reduces the measured GPU
work most clearly in the 1440p RT-water stress case, while CPU recording remains
around 6 ms. Do not automatically select it just because a quality preset is
chosen. Standard layers, volumetric clouds, RT, TAA and FSR remain independently
meaningful decisions; a stronger bloom value is not a reconstruction setting.

## Visual tuning decisions

No numerical change is accepted merely because it produces more contrast.
The same existing weather, date, fog, camera and original materials are retained;
no exposure/color-grade replacement, new algorithm or artwork edit is involved.

| Trial | Current -> stronger candidate | Decision criterion |
|---|---|---|
| Bloom | threshold 1.25 -> 1.15, strength .15 -> .22 | Reject: negligible or absent visual change in both islands' five views. |
| Shadow presentation | CSM darkness .35 -> .30 (lower means darker) | Reject: mainly darkens the road; no demonstrated improvement in useful separation. |
| Atmospheric perspective | cool multiplier (.78,.94,1.16) -> (.75,.94,1.20); forward weight .65 -> .70 and sun factor 1.3 -> 1.35 | Withhold: subtle shift, no sufficiently controlled evidence of improved depth. |
| Cloud light/dark balance | ambient/height .20+.22h -> .16+.24h; direct base .40 -> .44 | Withhold: slightly more contrast does not clearly improve the already soft sunset lobes. |
| Cloud shadows | weight .55 -> .60 | Withhold: no demonstrated tactical/lighting benefit beyond the existing broad shadow pattern. |
| Volumetric sunlight | radiance factor .18 -> .21; luminance budget .30 -> .33 | Reject for defaults: added forest haze/brightness does not qualify the known road interaction. |
| Wet roads | darkening/specular (.16,.10) -> (.18,.12), exponent 48 unchanged | Withhold: rain/fog and LOD replay differed across launches; no clean material-response acceptance evidence. |
| Water reflection balance | grazing Fresnel coefficient .40 -> .48, base .025 unchanged, raster and RT together | Reject for defaults: more assertive reflections retain the conspicuous transition. |

The runtime bloom trials use **current -> stronger -> restored** captures in one
frozen session. All ten restored 1080p full-window images are pixel-identical to
their current image. At night and sunset the stronger pair changes zero pixels
on both islands; most other changes are at most one 8-bit code, with a maximum
of two on the Everon coast. The separate Malden combined darkness/bloom trial
also restores identically in all five views; its road maximum change is four
codes, with no night/rain/sunset difference. These are image-difference counts,
not perceptual-quality scores. The unretouched screenshots and numerical
comparison records are retained; no exaggerated difference image is used as a
before/after photograph.

The remaining constants were compiled into a temporary candidate executable,
then compared with fresh baseline launches using the same six camera/date/weather
requests on each island. The additional dawn forest request deliberately activates
volumetrics; the five representative views otherwise leave it Off. These are
**exploratory comparisons, not accepted matched before/after evidence**: the
recorded cloud phase/rain/fog differ slightly between processes, and Malden road
foliage LOD visibly differs. In particular, the rain view's changed distant
visibility cannot be credited to atmosphere tuning. Current/stronger/restored
runtime pairs provide the stricter matching for bloom and darkness. No parameter
change passed the acceptance threshold, so all seven edited renderer files were
restored exactly to main before final tests. There are no accepted visual changes
requiring a new before/after claim.

## Verification and handoff

- Fresh Vulkan + public FSR RelWithDebInfo build of main; candidate and restored
  builds also completed. Focused `[vulkan-shape],[vulkan-aa],[ShadowMath]`:
  **1,601 assertions / 71 cases passed**. Vulkan policy/lifetime: **504 checks**.
  Intentional synthetic unavailable-resource diagnostics are not live failures.
- Restored-source real gameplay input checks: Malden RT stress **1,609** frames,
  Everon RT stress **964**, Malden with RT bootstrap disabled **1,108**; each
  closed normally with equal submitted/presented counts and **zero core/sync
  validation errors or warnings**. AA Off, TAA, FSR Quality and MSAA4 each received
  a bounded one-second forward input with measured player displacement. These
  are short movement/compatibility checks, not full mission or long AA quality
  qualification; some views pass through close stock foliage.
- A below-sea Malden camera reports roughly 4.4 m depth, followed by dry land.
  The no-RT process keeps raster enhancements available and reports unavailable
  RT capability rather than silently enabling a hardware path. RTGI stays Off
  throughout this pass; its implementation/history behavior is unchanged, not
  newly requalified. Prior broader AA/NVG/save-load/GL33 qualification remains
  separately documented in the renderer record.
- No rendering code, GL33 code, original assets/gameplay, user profiles, menu,
  persistence or private DLSS/NGX changes remain. Existing private dirty files
  were preserved by working in an isolated worktree. Development runs use copied
  profiles, `--nosound`, exact-window input and restored Caps Lock; no persistent
  power/display settings were changed.

Benchmark executable SHA-256:
`9915FE7AFA03353E4F1C5D2CC8A08AE2AA336E059FE964BC7AA87CEAA26DB1ED`.
Temporary rejected candidate:
`E732D9A33F18684EB374495382FB44A3357AB229A5D419B266ADE0237EB493B5`.
Restored-source validation executable:
`0828F8249D579F98AFE4490F37A4959CB040CE96DB814D82740B64B4B11E88B1`.

[Evidence index and reproduction](evidence/graphics-quality/README.md) contains
unretouched image links, hashes, camera locations, raw timing samples, tuning
state and validation receipts. Known visual gates remain explicit future work;
neither a new reflection transition nor a volumetric CSM fix is smuggled into
this parameter-only milestone.
