# Stock PBR receivers for RTGI

Public base: `78e39d9`. This extends the existing developer-only, default-Off
PBR/RTGI path. It does not replace artwork, change the BRDF or adjust light gain.

## Receiver contract

Enhanced mode already gave qualified PBR v3 textures the same bounded stock
reflectance and separate global ambient/sun coefficients used by ray hits.
However, only a successful small model/LOD catalog lookup promoted an opaque
draw to an ambient-replacing receiver. Ordinary classified buildings and roofs
could illuminate other receivers but could not themselves use transport.

Both native and captured CPU-lit submissions now call the same receiver policy
after stock/catalog material setup. Promotion requires all of:

- An existing exact PBR v3 identity or a successfully validated catalog entry.
- The original opaque world/depth-writing/non-water gate, with no cutout,
  blending, uncertain lighting flags or unsupported secondary shader.
- An active PBR material with original normals and separately prepared lighting.
  CPU triangles still require a common captured material slot. Inside/half-lit,
  shining, missing-normal, mixed-slot and unsupported local-light ranges retain
  their existing fallback. Packed, already-lit vertex RGB is never albedo.
- The existing linear night-eye response gate at final draw submission.
- A valid fragment normal and a successfully captured ambient contribution.
- A fragment inside the shared 45 m receiver bound. The same C++ constant is
  sent to raster capture and the GI trace/filter, rather than reserving ambient
  on distant geometry that cannot receive transport.

Catalog success no longer overrides a failed opaque-world prerequisite. The
fragment uses one readiness decision for **both** negative-depth records:
captured ambient and bounded diffuse reflectance/normal/fog. A material identity
alone cannot reserve ambient or produce a receiver record. Depth, coverage and
normal reconstruction checks still guard the subsequent transport/filter pass.

The actual global ambient is the existing PBR ambient term, independently
captured from the sun-independent base and multiplied by fog transmittance.
Composition remains `(reflectance * incoming - capturedAmbient) * fade` in HDR.
Direct sunlight, local lights, emission and in-scattered fog are not removed.
RTAO and SSAO reject the negative ambient tag, so neither multiplies the
replacement indirect light a second time. No composition algorithm is changed.
Outside the receiver range, ambient remains positively tagged and ordinary AO
continues to apply. A matched AA-Off distant control exercises both SSAO and
RTAO: each has a nonzero effect versus AO-Off, while GI On versus its AO-only
image has **zero changed pixels**. The lighting is not merely bypassing AO.

The calibrated directional stock sky/cloud model, real sunlight/sky visibility,
bounded reflectance, one surface bounce, 25 m transport, 80 m visibility,
45 m receivers and existing temporal reconstruction are unchanged. There is
still one primary sample per eligible half-resolution receiver, not a higher
sample count used to compensate for missing materials.

## Which materials can receive

The existing 23-entry allowlist is unchanged. **22 identities** can now receive
when the preceding lighting/geometry gates pass:

| Family | Existing texture identities |
| --- | --- |
| Rock | `skala_piskovec2` |
| Roof tiles | `strecha_tasky_vln`, `strecha_tasky_vln90`, `tasky_tmavsi` |
| Weathered wood | `prkna_dark`, `prkna_dalsi`, `telegraf_sloup` |
| Worn plaster/brick and closed-barn matte facade/door sections | `omlacena_bocni_zed`, `stodola_f`, `stodola_f2`, `stodola_s` |
| Stone/mortar | `kamennazed` |
| Concrete | `betondlouhastenabez`, `betonpoprask_side`, `betonpoprask_top`, `plot_beton_01_side`, `plot_beton_02_side`, `plot_beton_03_side`, `plot_beton_0301_side`, `plot_beton_0302_side`, `plot_beton_01_top`, `plot_beton_03_top` |

Names are the exact existing `data\\*.pac` keys, not substring/prefix rules.
The original inspection classified the closed barn's plaster/brick and rough
timber sections for the same conservative matte dielectric response; this does
not promote glass/hardware atlases or the different open-barn materials.
Supplemental maps are not required. Catalog fingerprint/model/LOD checks and
optional-map handling are retained.

The 23rd painted-signpost identity stays PBR/reflector-only rather than widening
uncertain cutout receiver coverage. Terrain remains a reflector **only**: its
colored CPU lighting is not treated as separable PBR ambient. Mixed house/window
atlases, foliage and unsupported lighting modes retain legacy shading/AO.

### Receiver diagnostics

`triRTGI 'status'` reports actual eligible half-resolution receiver pixels by
family and native/CPU route, separately from ray-hit material counters. Four
unused bits above the existing 20-bit oct-normal code carry this diagnostic
class; all 24-bit integers are exactly representable in the existing RGBA32F
record. Original normal precision, RGB/fog/depth and the positive-depth water
ABI are unchanged. MSAA rejects mixed-class samples at material boundaries.
There are no new render targets or pipelines. The existing counter readback
grows by 64 bytes; the stock material identity and reflectance remain the
original shared raster/ray contract, not the diagnostic class.

## Gameplay qualification

The existing muted original-mission harness uses copied profiles, ordinary
brightness, 1400 m visibility and TAA. It visits Malden's village tiled-roof
view and Everon's original closed barn, beyond the previously cataloged walls.
Morning (09:00), noon, overcast noon and 17:30 dusk are checked on July 5, 1985.
The preserved `78e39d9` executable is the actual baseline, not a diagnostic
switch pretending to reproduce it. Runtime hashes, cameras, original screenshots,
counter snapshots, profiler intervals and focused tests are in the
[qualification receipt](evidence/rtgi-receivers/qualification.json).

### Coverage actually received, not just ray hits

Completed-frame counts at confirmed 1920x1080 world dimensions:

| Ordinary morning view | `78e39d9` receivers | New receivers | Families / route | Share of half-resolution sample grid |
| --- | ---: | ---: | --- | ---: |
| Malden village roofs | 0 | 87,195 | 87,116 roof + 79 wood, native | 16.82% |
| Everon closed barn | 0 | 92,506 | 91,377 facade + 1,129 wood, native | 17.84% |

These are view-specific eligible **half-resolution receiver pixels**, after
coverage/depth/normal/distance checks, not percentages of every game surface.
Neither view has a catalog receiver; their participation cannot be attributed
to the old demonstration walls. Family sums match the receiver counter.

The source side also remains real transport. The same morning roof frame has
7,728 qualified material hits and 7,470 sun-visible secondary hits; the barn has
40,266 qualified hits and 28,682 sun-visible hits. At the barn, a later isolated
frame includes 31,986 terrain hits, of which 27,988 receive sunlight. Nearby
stock ground therefore contributes through actual visible paths, not an added
screen-wide light. Terrain itself remains untagged as a receiver.

Previously cataloged walls still work. Malden's CPU-lit stone view retains
215,051 receivers versus 214,867 in its baseline snapshot. Everon's concrete view
retains 113,408 CPU-lit catalog receivers versus 113,416 at the baseline's
different sample phase, and gains **6,736 additional captured CPU-lit stock
concrete receivers**. Thus the extension is exercised on both native and
CPU-lit geometry, not merely enabled by a unit-test flag.

The original Malden house/window atlas is a useful negative control: its broad
facade does not become a receiver because it resembles plaster. Some inspected
wood/plaster sections also retain unsupported original lighting modes. Their
fallback is intentional; they are not counted as a new facade demonstration.
Rock and the other table identities are policy-enabled under the same contract,
but this live milestone specifically demonstrates tiles, barn facade/door,
wood and additional CPU-lit concrete rather than claiming a separate rock trial.

### Matched original-artwork comparisons

Full-resolution, unamplified captures:

| Scene / condition | PBR v3 / GI Off | Enhanced / GI Off | Enhanced / GI On |
| --- | --- | --- | --- |
| Malden roofs morning | [PNG](evidence/rtgi-receivers/malden-morning-v3.png) | [PNG](evidence/rtgi-receivers/malden-morning-enhanced.png) | [PNG](evidence/rtgi-receivers/malden-morning-gi.png) |
| Malden roofs noon | [PNG](evidence/rtgi-receivers/malden-noon-v3.png) | [PNG](evidence/rtgi-receivers/malden-noon-enhanced.png) | [PNG](evidence/rtgi-receivers/malden-noon-gi.png) |
| Malden roofs overcast | [PNG](evidence/rtgi-receivers/malden-overcast-v3.png) | [PNG](evidence/rtgi-receivers/malden-overcast-enhanced.png) | [PNG](evidence/rtgi-receivers/malden-overcast-gi.png) |
| Malden roofs dusk | [PNG](evidence/rtgi-receivers/malden-dusk-v3.png) | [PNG](evidence/rtgi-receivers/malden-dusk-enhanced.png) | [PNG](evidence/rtgi-receivers/malden-dusk-gi.png) |
| Everon barn morning | [PNG](evidence/rtgi-receivers/everon-morning-v3.png) | [PNG](evidence/rtgi-receivers/everon-morning-enhanced.png) | [PNG](evidence/rtgi-receivers/everon-morning-gi.png) |
| Everon barn noon | [PNG](evidence/rtgi-receivers/everon-noon-v3.png) | [PNG](evidence/rtgi-receivers/everon-noon-enhanced.png) | [PNG](evidence/rtgi-receivers/everon-noon-gi.png) |
| Everon barn overcast | [PNG](evidence/rtgi-receivers/everon-overcast-v3.png) | [PNG](evidence/rtgi-receivers/everon-overcast-enhanced.png) | [PNG](evidence/rtgi-receivers/everon-overcast-gi.png) |
| Everon barn dusk | [PNG](evidence/rtgi-receivers/everon-dusk-v3.png) | [PNG](evidence/rtgi-receivers/everon-dusk-enhanced.png) | [PNG](evidence/rtgi-receivers/everon-dusk-gi.png) |

The original tiles, orientation, roof seams, plaster weathering, brickwork and
timber details remain recognizable. GI gives the roof planes different fill,
and the barn cooler sky illumination with darker occluded regions beneath the
eaves. It does not brighten every surface to the same level. Direct sun remains
the existing GGX/shadow contribution, independent of the replaced ambient.

Linearized **SDR screenshot** ROI means quantify those observations; these are
after AA/output, not raw HDR calibration or a target for lighting energy:

| Condition | Left roof GI change | Right roof GI change | Barn facade GI change | Barn gable GI change |
| --- | ---: | ---: | ---: | ---: |
| Morning | +1.32% | +5.50% | -29.30% | -18.54% |
| Noon | -1.49% | +3.10% | -20.20% | -11.62% |
| Overcast | -7.42% | +1.47% | -30.40% | -20.55% |
| Dusk | -4.86% | -.74% | -26.40% | -15.87% |

Percentages compare GI On with enhanced GI-Off. The useful change is spatial
and directional illumination, not a promise of greater total brightness.
Orientation and occlusion legitimately make the barn darker than its old
uniform ambient. No gain was added to cancel that difference. PBR v3 and
enhanced GI-Off are almost identical in these already-qualified matte regions;
this change does not alter their raster BRDF or artwork.

Six interleaved captures per component mode compare `On` with `sky-only`, which
suppresses all qualified surface bounce but retains visible primary sky:

| Newly receiving region | Surface-bounce luminance increment | Relative addition versus sky-only |
| --- | ---: | ---: |
| Barn facade, 1080p | .008593 | 6.58% |
| Barn gable, 1080p | .006028 | 3.89% |
| Left roof, 1440p | .002590 | 1.22% |
| Right roof, 1440p | .001737 | .67% |

This isolates combined sunlight/reflected-skylight surface transport, not direct
sunlight alone. The ordinary barn now visibly participates in that transport.
The untouched adjacent ground control changes only -.020% in this isolation;
Malden's unqualified facade control changes -.030%. Those small sampling/cloud/
foliage differences must not be called terrain receivers or screen-wide GI.

### Motion, distance and GPU cost

Real mission simulation advances while the render camera translates/yaws with
TAA and FSR Quality at 1440p. Moving and settled frames preserve the original
surface detail without new obvious roof/facade light trails or glow in these
checks. Existing foreground foliage-card artifacts and filtering blur remain.
These are render-camera checks, not player-input responsiveness or mission
completion tests.

The camera distance/visual-LOD sweep steps through offsets 0, 12, 28, 42, 65,
100 m and returns to the original view. Dedicated classified roof/facade
sections continue through the normal renderer's LOD selection; no special
model/LOD receiver catalog is required. The unchanged 45 m receiver fade returns
distant geometry to ordinary ambient without black surfaces. This is not a
per-object forced enumeration of every LOD or an adoption of mixed far atlases.
The harness uses the engine's own ground placement to keep the distance camera
above terrain, atomically restoring the temporary player-position probe before
simulation observes it; camera/contact receipts are retained.

Everon's motion sweeps keep BLAS builds at 75. Malden's new roof view increments
by two per sweep (96->98, 98->100). The actual preserved `78e39d9` control
reproduces exactly +2 (78->80, 80->82). That pre-existing behavior is documented,
not silently called stable or fixed by receiver adoption. Live coastal waves
keep the build count at 663, with real terrain reflection hits shaded and
refraction active without fallback.

Existing GPU profiler, RTX 4060 Ti, confirmed world dimensions, TAA. Two interval
medians per mode are averaged in Off/On/On/Off order, three seconds settling and
four seconds sampling. Water/volumetric cloud rendering are Off during timing;
the original sky/weather inputs and cloud shadows remain live.

| Ordinary scene / world resolution | GI-Off frame ms | GI-On frame ms | Total GI cost ms |
| --- | ---: | ---: | ---: |
| Malden roofs 1920x1080 | 4.229 | 4.784 | +.556 |
| Malden roofs 2560x1440 | 4.450 | 4.798 | +.348 |
| Everon barn 1920x1080 | 2.027 | 2.265 | +.238 |
| Everon barn 2560x1440 | 2.751 | 3.766 | +1.016 |

These are **full GI versus GI-Off** costs, including the existing sky model,
capture, filtering and diagnostics, not isolated receiver-policy overhead versus
`78e39d9`. New receivers naturally trace more total samples; the per-receiver
estimator is unchanged. Short whole-frame deltas include clock/cache variation,
especially the bracketed Malden Off intervals, and are not an optimization or
resolution-scaling claim. No heavy build runs concurrently with timing.

### Validation and preservation

The four final Vulkan runs presented **111,388 frames, zero validation errors
and zero warnings**. Focused Vulkan tests: 50 cases / 14,488 assertions; GL33:
49 cases / 14,448 assertions; driver-free policy/lifetime guards: 684 checks.
Both public builds complete. Existing compiler warnings are separate from runtime
Vulkan validation. Tests exercise the actual shared receiver policy and record
class math, preserve every normal bit, reject unknown/terrain identities and
failed prerequisites, and pin the paired ambient/material output guards and
shared receiver bound plus existing AO ordering. The loader also emits
pre-existing external overlay-manifest/disabled-layer diagnostics; those are
not Vulkan validation errors. Validation **and synchronization validation** are
confirmed enabled in the actual gameplay logs.

RT sunlight, RT water/refraction, RTAO/SSAO, clouds, fog/rain, night-eye fallback,
real Jeep headlights/local shadows, TAA/MSAA4/FSR and restored PBR v3 were
exercised. GL33 renders its original scene and its stationary PBR control has
zero changed pixels. Four original PBO/world hashes still match. Original
profiles, player settings/presets, source assets and private DLSS/NGX were not
modified. Local raw runs remain under `build/shadow-live/receiver-*`; no new
testing framework or material editor was added.

## Limits

This is incremental receiver adoption, not comprehensive physical GI. Original
texture RGB contains baked artistic shading; its bounded reflectance is not
measured albedo. Sky calibration is engine-relative, not measured atmosphere.
One surface bounce omits further diffuse transport, indirect specular and
reflected local-light illumination. Finite scene/visibility bounds and the
unchanged receiver-distance fade remain visible limitations.

Unsupported atlases/modes intentionally retain a different legacy ambient
model. Thin or discontinuous geometry can fail the existing depth-normal
reconstruction and retain ordinary ambient for that sample. This is not a
reason to promote uncertain normals or turn off occlusion. Original foliage
card artifacts, texture filtering blur and one-sample temporal/spatial noise
are not fixed by this milestone.
