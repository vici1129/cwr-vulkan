# RTGI sky irradiance convention

Subsequent stock receiver adoption is documented in [stock RTGI receivers](rtgi-receivers.md).

Public base: `ef9c3d9`. Developer-only, default-Off RTGI still replaces captured
ambient on qualified receivers. No artwork, receiver/material classification,
exposure, player profiles, presets or private DLSS code changes.

## Contract investigation

The original engine has two independently authored quantities, not a calibrated
atmosphere. `LightSun::Recalculate` sets ambient to `BackgroundColor * ambientI`
and sky color to `BackgroundColor * backgroundI + colorFull * .5`. Their solar
elevation clamps differ. Weather also selects different stock dome artwork.
Consequently the visible sky's linear pixel values are not already in the
lighting units of the ambient coefficient.

The enhanced raster path multiplies bounded stock diffuse reflectance by
`legacyLightToLinear(MainLight()->Ambient() * GetAccomodateEye())`. This is a
Lambert reference **E/pi**, not sky radiance per direction. The ray contract
decodes that same coefficient. Sunlight is likewise a Lambert lighting reference;
the raster GGX path deliberately compensates its normalized BRDF by pi. Cosine
sampling cancels the Lambert pi in transport: no missing pi was found.

Stock sRGB textures decode once through their image format. Legacy light
coefficients use the shared sRGB-plus-linear-headroom conversion after the
existing eye/brightness factor. The stock dome multiplication follows the
visible engine's convention. `InitDraw` uses the existing brightness/night-eye
response, not a newly introduced exposure controller. Original cloud lighting's
two eye factors also occur in its CPU lighting path; removing one only in RTGI
would not make it consistent. None of those conventions is changed here.

## Conversion, not another ambient light

One 64-lane GPU workgroup integrates 1,024 deterministic cosine-weighted lookup
directions about world up, without tracing rays. Let `D` be the mean linear
**dome-only** appearance, `A` the engine's decoded ambient E/pi reference, and
`Y` linear luminance. RTGI uses:

```
k = Y(A) / Y(D)
Lsky(x, direction) = k * stockDomeAndCloudAppearance(x, direction)
```

This anchors unobstructed horizontal dome E/pi luminance to the existing engine
lighting convention, not to final screenshot brightness. A scalar retains sky
chroma and angular variation. It applies equally to primary sky misses and
secondary visible skylight at qualified material hits. Direct sunlight, stock
reflectance and unknown-hit legacy fallback are unchanged. `A` is never added
as a second contribution. Near-black dome data avoids division by zero without
inventing an emitter.

Local clouds, materials, receiver orientation and scene visibility are **not**
in the denominator. Cloud layers retain their original radiance/opacity and
spatial weather modulation; the complete cloudy hemisphere need not equal the
dome reference. A roof or wall can never trigger a compensating normalization.
The existing alpha-tested 25 m transport / 80 m visibility, one secondary sky
probe, geometric/shading hemispheres and 32-phase reconstruction remain intact.
There are no additional transport rays, surface bounces or gain controls.

### Pre-composition diagnostics

`triRTGI 'status'` now reports raw dome/weather/up and camera-facing vertical
hemisphere integrals, the ambient E/pi reference and their derived conversion.
The vertical integral is an **unobstructed hypothetical normal**, not a wall ROI.
All integral RGB values in status are before calibration; multiply by `k` for
the lighting values. The vertical integral includes the lower-horizon zero.

A second tiny workgroup sparsely reads the existing raw GI, qualified receiver
and captured-ambient images. It reports means of incoming radiance, replacement
radiance, captured ambient and reflectance-times-fog, plus the valid sample count.
These generic frame samples precede temporal resolve, composition, AA and output
mapping. They are noisy individual-frame estimates over all sampled receivers,
not pure shaded-wall measurements. They add no rays and change no image pixels.

## Qualification

The original Malden courtyard and Everon concrete compound use the existing
muted mission harness, copied profiles, ordinary brightness, 1400 m visibility
and TAA. Dates are July 5, 1985 at 09:00, 12:00, overcast 12:00, and 17:30.
The actual preserved `ef9c3d9` executable supplies the baseline images; an
identity-conversion instrumented build first measured the old lighting contract.
The final executable and all raw receipts are identified by SHA256 in the
[qualification receipt](evidence/rtgi-sky-calibration/qualification.json).

### Energy before raster composition

Final 1,024-direction GPU integration gives identical dome/reference values
at both locations. These are linear E/pi luminances in engine-relative units,
not output-image measurements:

| Condition | Unscaled dome | Engine reference | Derived k | Calibrated dome |
| --- | ---: | ---: | ---: | ---: |
| Morning | .448971 | .903368 | 2.01209 | .903368 |
| Noon | .447771 | .903368 | 2.01748 | .903368 |
| Overcast | .416225 | .903368 | 2.17038 | .903368 |
| Dusk | .337076 | .218415 | .64797 | .218415 |

The mismatch reverses at dusk: this is not an unconditional brightness increase.
The initial 256-direction measurement also found the daytime deficit and dusk
excess; its raw receipt is retained separately. No conversion factor was fitted
to a wall ROI, legacy screenshot or desired percentage of brightness.

The complete morning cloudy/up integral is **not** forced to .903368: Malden
is .662226 before conversion (1.332459 afterward), Everon .596050 (1.199307
afterward). Those different cloud fields retain their authored effect. The
unobstructed camera-facing vertical sky E/pi is .749837 / .688620 after
conversion, already below uniform ambient because of orientation. Actual sparse
qualified-receiver incoming means are .655805 / .692740. Their replacement
radiance means are .056077 / .037756 versus captured ambient .079518 / .050051.
These last numbers are noisy frame-wide means, not wall ROI integrals.

Real morning transport remains visibility-dependent:

| Scene | Eligible / rejected hits | Sun-visible hits | Secondary sky visible / blocked / rejected hemisphere |
| --- | ---: | ---: | ---: |
| Malden | 108,586 / 47,088 | 64,323 | 34,775 / 51,804 / 22,007 |
| Everon | 52,908 / 32 | 29,472 | 40,063 / 11,561 / 1,284 |

Primary upward continuations also encounter 1,349 / 2,386 blockers. No blockers
were disabled and no receiver was promoted. The residual morning darkness is
consistent with the represented angular sky, geometric visibility and bounded
one-bounce reflectance, rather than a remaining factor-of-two sky conversion
error. That does not prove the bounded scene or artistic inputs are complete
physical ground truth; unrepresented illumination remains a limitation.

### Matched gameplay comparisons

The linked full-resolution images are unamplified. Table values are shaded-wall
ROI means from **linearized SDR output after AA/tone mapping**, not the HDR
measurements used for calibration. Stochastic phases, HUD and foliage vary
slightly; this is not a pixel-identical image-difference test.

| Scene / condition | Enhanced GI-Off | `ef9c3d9` RTGI | Calibrated RTGI |
| --- | ---: | ---: | ---: |
| Malden morning | [.110473](evidence/rtgi-sky-calibration/malden-morning-off.png) | [.052781](evidence/rtgi-sky-calibration/malden-morning-old.png) | [.081653](evidence/rtgi-sky-calibration/malden-morning-calibrated.png) |
| Malden noon | [.109771](evidence/rtgi-sky-calibration/malden-noon-off.png) | [.052943](evidence/rtgi-sky-calibration/malden-noon-old.png) | [.081204](evidence/rtgi-sky-calibration/malden-noon-calibrated.png) |
| Malden overcast | [.109865](evidence/rtgi-sky-calibration/malden-overcast-off.png) | [.052223](evidence/rtgi-sky-calibration/malden-overcast-old.png) | [.089472](evidence/rtgi-sky-calibration/malden-overcast-calibrated.png) |
| Malden dusk | [.026711](evidence/rtgi-sky-calibration/malden-dusk-off.png) | [.026721](evidence/rtgi-sky-calibration/malden-dusk-old.png) | [.019361](evidence/rtgi-sky-calibration/malden-dusk-calibrated.png) |
| Everon morning | [.049696](evidence/rtgi-sky-calibration/everon-morning-off.png) | [.018934](evidence/rtgi-sky-calibration/everon-morning-old.png) | [.037384](evidence/rtgi-sky-calibration/everon-morning-calibrated.png) |
| Everon noon | [.067038](evidence/rtgi-sky-calibration/everon-noon-off.png) | [.037829](evidence/rtgi-sky-calibration/everon-noon-old.png) | [.057106](evidence/rtgi-sky-calibration/everon-noon-calibrated.png) |
| Everon overcast | [.050481](evidence/rtgi-sky-calibration/everon-overcast-off.png) | [.018365](evidence/rtgi-sky-calibration/everon-overcast-old.png) | [.039027](evidence/rtgi-sky-calibration/everon-overcast-calibrated.png) |
| Everon dusk | [.048544](evidence/rtgi-sky-calibration/everon-dusk-off.png) | [.052892](evidence/rtgi-sky-calibration/everon-dusk-old.png) | [.047469](evidence/rtgi-sky-calibration/everon-dusk-calibrated.png) |

Morning shade rises 54.7% / 97.4% versus the old model, leaving it 26.1% / 24.8%
below GI-Off. This improves ordinary wall readability while preserving the
original stone/weathering and concrete painted patterns. The shaded and sunlit
walls retain different color/illumination. Dusk instead falls 27.5% / 10.3%
versus old RTGI, as the energy correction requires; it is not brightened back
to legacy. Overcast changes sky color and weakens the warm sunlight contribution.

The existing binary `sun-only`, `no-sun` and `sky-only` modes were checked in
all four conditions. Six interleaved morning captures per mode isolate:

| Region | Direct-sun bounce Y | Reflected-skylight Y | Relative sun / sky addition |
| --- | ---: | ---: | ---: |
| Malden shaded stone | .002809 | .002886 | 3.56% / 3.66% |
| Everon concrete panel | .000750 | .001222 | 2.05% / 3.38% |

`On - no-sun` isolates sunlight bounce; `On - sun-only` isolates reflected sky.
These output-space increments are not extra ambient. Absolute direct-sun bounce
is close to the previous milestone's .002844 / .000755; reflected sky roughly
doubles with the independently derived daytime conversion. Malden's sun-bounce
RGB is .003786/.002745/.000562 versus reflected sky .002750/.002938/.002770:
warm sunlight and relatively cooler skylight coexist. Original stock material
color still modulates both. See per-mode ranges and all ROIs in the receipt.

### GPU, movement and preservation

Existing profiler, RTX 4060 Ti, confirmed TAA world dimensions. Two interval
medians per mode are averaged in Off/sun-only/On/On/sun-only/Off order, three
seconds settling and four seconds sampling. No build runs concurrently. Water
and volumetric cloud rendering are Off for timing, but weather/sky data stay live.

| Scene / world resolution | GI-Off frame ms | Full GI frame ms | Total GI cost ms | Extra secondary-sky trace ms |
| --- | ---: | ---: | ---: | ---: |
| Malden 1920x1080 | 5.100 | 5.706 | +.606 | +.218 |
| Malden 2560x1440 | 4.914 | 5.625 | +.711 | +.210 |
| Everon 1920x1080 | 3.739 | 4.124 | +.385 | +.053 |
| Everon 2560x1440 | 3.675 | 4.208 | +.533 | +.053 |

All GI modes include lookup calibration; trace scope includes its integration.
Whole-frame cost also includes the small post-trace HDR diagnostic reduction.
The last column isolates secondary sky queries against `sun-only`, not calibration
overhead against `ef9c3d9`. Short whole-frame differences include clock/cache
variation (especially the bracketed Off intervals); these are not an optimization
claim or a guarantee of resolution scaling. Scope times need not add exactly.

Running-mission camera translation/yaw and settled frames were inspected with
TAA and FSR Quality at 1440p. Original wall details remain recognizable without
new obvious wall-light trails or uniform glow in these checks. Existing foreground
foliage-card artifacts, texture blur and one-sample noise remain. The derived sky
scale stays stable during motion; the checks did not reveal a new cloud-history
problem requiring changes to temporal reconstruction. BLAS counts remain stable
at 202 / 252 within both AA sweeps. Live coastal waves also retain count 553,
with terrain reflection hits shaded and refraction active without fallback.

The four final Vulkan runs presented **57,249 frames, zero validation errors and
zero warnings**. Focused Vulkan tests: 46 cases / 13,992 assertions; GL33: 45 cases
/ 13,952 assertions; driver-free policy/lifetime guards: 684 checks. The actual
calibration math is exercised for upward/downward energy conversion, chroma,
zero inputs, visibility loss and separation from sunlight. Both public builds
complete; existing compiler warnings are separate from runtime validation.

RT sunlight, RT water, RTAO/SSAO, clouds, fog/rain, night-eye fallback, real Jeep
headlights/local shadows, TAA/MSAA4/FSR and restored PBR v3 were exercised. GL33's
stationary PBR control has zero changed pixels. Four original PBO/world hashes
still match. Original profiles/assets and private DLSS/NGX were not modified.
These are real mission render-camera checks, not mission completion or player
input-responsiveness qualification. Raw local runs remain under
`build/shadow-live/calibration-*`; no new testing framework was introduced.

## Remaining physical limits

This is a defensible **engine-relative convention**, not measured atmospheric
radiometry. The engine ambient reference is artistic, and its weather response
does not describe a physical sky. Scalar calibration intentionally retains
original sky chroma rather than forcing its RGB to the ambient RGB. Original
cloud layers are an appearance approximation. Stock material RGB still contains
baked shading and uses the existing bounded, unmeasured reflectance proxy.

Vertical/covered surfaces can legitimately remain darker than legacy uniform
ambient. Finite scene/visibility bounds, missing lower-hemisphere geometry,
unsupported material fallbacks, one surface bounce, terrain palette tint and
approximate temporal filtering remain limitations. This change does not claim
complete GI, calibrated moonlight or exhaustive enclosed-room qualification.
