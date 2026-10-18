# RT-water reflection transitions

Follow-up to the [graphics-quality visual gate](graphics-quality.md), starting
from public main `aadbc5e`. RT water remains a process-local developer feature;
the unified Video Settings menu and its Water choices are unchanged.

## Diagnosis

The documented Malden `[3500,5,3240,1,-.08,0]` and Everon
`[5100,5,3750,0,-.08,1]` views reproduce a conspicuous strip of original water
between the coast and reflected detail. Increasing receiver reach or Fresnel
does not address its main cause.

`shape.frag` decoded `specular_dx.paa` RGB as a signed normal. That stock asset
is **AI88 intensity/alpha detail**, with white RGB and a varying alpha channel;
original raster water uses its alpha at 32x UV. The stock-backed decoder test
now verifies that property. For a flat sea triangle, the old normal was therefore
`normalize(vec3(.06,1,.06))`, a constant diagonal tilt of about 4.85 degrees.
It was not an animated normal map. This affected both native and software water.

Temporary actual-game shader probes separated receiver fade, hit length, normal
and rejection reason. The near-shore strip was rejected by
`dot(incident, normal) >= 0`, well inside the receiver limit. The old Fresnel
approaches .425 at that boundary, so this test switched the entire contribution
abruptly between zero and a large weight. A grazing ray reflected about the
biased normal can also travel below the sea rather than into the reflected
scene. The visible reflected-terrain silhouette is not, by itself, evidence of
the 70 m ray cutoff: sampled nearby hits there were well inside that range.

There were two independent continuity defects in the bounded fallback path:

- A textured hit at the 70 m ray limit changed immediately to weather sky on a
  miss. The 60–80 m **receiver** fade does not fade this **hit-distance** event.
- The 3x3 filter averaged unassociated RGB and alpha. Zero-alpha early exits
  contained black RGB, darkening partly covered reflections a second time.

The existing raster composition remains `mix(originalWater, reflection, weight)`
in linear HDR, after refraction/foam and before temporal reconstruction. The
Fresnel coefficients .025/.4 were not the source of the rejection discontinuity.
Sky/cloud sampling already follows the engine weather; its color was being
switched off together with geometry at the bad normal boundary.

## Correction

The existing water surface attachment, one-ray query, shared scene and filter
are retained. There is no new reflection system or history buffer.

- With RT water active, reflect about the actual water triangle normal plus
  existing rain slopes. The stock texture supplies scalar detail, not XYZ
  normals or a new height field. Original diffuse frames, 32x detail alpha,
  geometry, shoreline UV animation and wave behavior remain untouched. RT Off
  selects the previous normal/optics path exactly. No new texture samples.
  With RT active, refraction and foam consume the corrected surface normal;
  their existing bottom capture, coastal gates and composition remain intact.
- Smooth the contribution to zero over the last .06 of front-facing cosine and
  .03 of above-horizon reflected direction. Back faces, underwater receivers and
  rays directed beneath the sea retain original water, without an abrupt
  nonzero Fresnel step at the gate.
- Fade shaded geometry into the **same** weather-sky fallback over 50–70 m of
  reflected ray travel. Misses and unavailable hit materials use that fallback
  too. Receiver fade stays **60–80 m**, ray maximum **70 m**, scene radius
  **160 m**; no additional rays or AS coverage are requested.
- Filter premultiplied reflection radiance and coverage, then unassociate for
  the existing source-alpha blend. Keep the depth/water rejection kernel and
  MSAA foreground rules.
- Evaluate reflected sky/clouds only when a miss, unavailable material or partial
  hit fade actually needs them. Near fully shaded hits skip the discarded cloud
  march. Cloud lighting on geometry hits remains active.

The water marker's existing first component distinguishes RT geometric normals (2) from
the preserved non-RT optics (1). No descriptor, image format, push-constant ABI,
resource lifetime or scene-collection budget changes. Shared dynamic geometry
remains independent of RT sunlight. GL33 and private DLSS/NGX are untouched.

## Evidence and verification

Current captures, matched-state receipts, profiler samples and qualification
limits are in [the evidence package](evidence/rt-water-transitions/README.md).
The normal/rejection probes were temporary and are absent from the final build.

The hard rejected-water strip is removed in both documented coast views and
the verified above-water low-angle repeats. Short stock-scene camera sweeps,
TAA/FSR Quality, grounded player input, shallow/deep water, sunset/overcast,
feature toggles and no-RT startup checks retain useful reflections and original
water fallback. The sampled RT-Off water region is pixel-identical to baseline
in all twelve frozen comparisons; full windows differ slightly in sky/HUD.
These are developer-assisted stock gameplay checks, not mission completions.

Both builds pass. Focused stock-backed suites pass **2,069 assertions / 84 cases**
with Vulkan and **1,823 / 80** with GL33; driver-free Vulkan guards pass **517**
checks (including the intentional teardown-error fixture). Water compute/filter
single/MSAA and water MRT regular/temporal SPIR-V all validate. Malden/Everon
core and synchronization validation stays clean through shutdown; separate
low-view, grounded movement and no-RT runs also report zero errors/warnings.
Default GL33 launches stock Malden, rejects RT water and exits normally.

## RTX 4060 Ti cost

Driver 617.14, stock GOG 3.05, native AA Off, SDR, VSync Off, view distance 1400,
same saved world/camera/weather per island. Enhanced raster shadows, SSAO and
water are active, with RT water/sky/dynamic scene but RT sunlight/RTAO/RTGI Off.
Each launch samples cloud volume On/Off/Off/On, settling 3 seconds and measuring
6.2 seconds per state. Values are medians of the existing two-second profiler
windows, not individual-frame percentiles. No build/capture runs during sampling.

| Coast / output | Enhanced clouds: GPU ms baseline -> fixed | Delta | Stock clouds: GPU ms baseline -> fixed |
|---|---:|---:|---:|
| Malden / 1080p | 3.208 -> 3.570 | +0.363 | 2.316 -> 2.367 |
| Everon / 1080p | 3.512 -> 3.857 | +0.345 | 2.227 -> 2.226 |
| Malden / 1440p | 5.440 -> 6.422 | +0.982 | 3.605 -> 3.612 |
| Everon / 1440p | 6.382 -> 6.872 | +0.490 | 3.580 -> 3.596 |

The combined-cloud increase is real and is accepted for this developer feature;
it is not free. Correcting the reflection direction changes which rays reach
terrain versus the expensive volumetric sky. The removed bad-normal rejection
also admits about **4.9% / 6.2%** more valid Malden/Everon water pixels, while
maximum ray count per pixel, reach and AS neighborhood remain fixed. No new
texture fetch is needed for the normal correction. Stock-cloud differences
are small/noisy; a negative delta is not evidence of a speedup.

Shared RT CPU collection/build medians remain about **.002/.046 ms** on Malden
and **.005/.024 ms** on Everon. Whole CPU recording varies by at most about
.17 ms across these pairs; no systematic extra CPU collection work is added.
Steady allocation counters remain zero and stationary AS structures are reused.
Raw pass timings, CPU/presentation intervals and automatic clock/power snapshots
are retained in the evidence. 1440p is a verified 2560x1440 world/output drawable
on a 1080p desktop, not physical full-monitor 1440p inspection.

## Remaining limits

This is still bounded, approximate water reflection. Distant reflected geometry
converges to weather sky, then original water outside receiver coverage. It is
not a complete reflected island. The original terrain UV/LOD approximation and
simple hit lighting can differ from raster terrain; this correction does not
add detail materials, local-light transport or hidden unsupported geometry.
Excluded blended/proxy/dynamic-texture geometry can reveal a supported hit or
sky behind it. Reactive water still limits long temporal trails at the expense
of some fine reflected-edge shimmer. No new denoiser or temporal framework is
introduced. Keep RT water developer-only pending broader GPU/mod qualification.
