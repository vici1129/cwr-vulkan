# Diffuse sky in stock RTGI

Public base: `a16aced`. This extends the same developer-only, default-Off,
one-bounce ambient replacement. It changes illumination, not artwork, material
classification, receiver eligibility, exposure, presets or player settings.

## Lighting model

The sky boundary now uses the **original directional skydome lookup** already
constructed for RT water: actual dome geometry/UVs, the active original weather
texture, `SkyColor`, the broad `SunSkyColor` atmospheric lobe, and the existing
eye/brightness convention. Readiness is separate from the water sky-reflection
toggle. RTGI requests weather data even with water/cloud-rendering options Off.
If that lookup is unavailable, original zenith/horizon colors provide a bounded
directional gradient; it does not substitute uniform ambient.

The three existing legacy cloud-coverage maps composite cloud radiance over the
sky, with their current color, opacity, layer heights and weather. A coarse mip
footprint avoids sampling distant high-frequency cloud detail with one probe.
GI uses real world origins, not the skydome's compressed raster coordinates.
It does not add a volumetric cloud march or change RT water's lighting model.

Direct sunlight remains a separate delta source, using the original sunlight
coefficient, geometric/shading cosine, alpha-tested shadow query and cloud-sun
visibility. The broad atmospheric sky lobe contains no solar disk. Sunlight is
not also added to the sky lookup as a second direct-light term.

For a receiver's unchanged cosine-weighted transport sample:

1. An upward geometry miss evaluates sky radiance in that sampled direction.
   A visibility-only continuation checks the interval from the 25 m transport
   bound to 80 m, so a farther roof/tree cannot become a false sky opening.
   Empty space below the horizon is not a sky emitter.
2. A qualified material hit retains its original bounded stock reflectance.
   Its outgoing diffuse light is reflectance times the sum of direct sunlight
   and visible diffuse sky illumination.
3. Exactly one independently scrambled cosine-weighted sky probe samples the
   hit's shading hemisphere. It must also face the real geometric hemisphere
   and upper sky. A real 80 m alpha-tested query rejects blocked directions.
   A blocker terminates this visibility probe; its material is not shaded.

With cosine sampling, sky irradiance divided by pi is the mean visible sky
radiance. Lambert's factor and the sampling PDF cancel at the hit and receiver;
there is no extra pi, distance attenuation, GI gain or ambient term on qualified
hits. An occluded probe contributes zero, not a dimmed ambient constant. This is
still one **surface** bounce, not another diffuse bounce or more primary samples.

The existing spatial/temporal reconstruction filters the resulting radiance.
History invalidation now also tracks dome changes, sky colors and cloud color,
in addition to the prior sun/ambient/weather checks. Dynamic sky blockers mark
history unsafe using the established dynamic-hit mechanism.

## Diagnostics

All diagnostic component switches are binary, and reset temporal history:

| `triRTGI` mode | Primary visible sky | Direct-sun surface bounce | Reflected skylight |
| --- | --- | --- | --- |
| `on` | yes | yes | yes |
| `sun-only` | yes | yes | no |
| `no-sun` | yes | no | yes |
| `sky-only` | yes | no | no |

`sky-only` retains its original meaning: suppress **all surface bounce**. The
new `sun-only` is not the old `a16aced` model: primary sky is directional and
visibility-tested in all four modes. Use the preserved baseline executable for
the actual old/new comparison. Sun counters describe evaluated visibility even
when the sunlight contribution is masked; secondary sky queries are skipped
when their component is disabled.

Status separates primary sky continuation rays/blocked/visible/below-horizon
misses from secondary sky rays/blocked/visible/rejected hemisphere samples.
The old sunlight/material counter prefix and history offsets are unchanged.
The readback allocation and descriptor cover the complete extended counter data.

Unsupported hit materials still retain the established legacy ambient fallback.
No additional receiver is promoted. CPU-lit ranges lacking reliable captured
lighting, including the tested nighttime local-light fallback, remain legacy.

## Gameplay qualification

Matched original Malden courtyard and Everon compound views use the existing
muted direct mission harness, copied profiles, ordinary brightness, 1400 m
visibility and TAA. These are real mission render-camera tests, not mission
completion or player-input responsiveness qualification.

The comparison is enhanced raster GI-Off, the actual `a16aced` executable, and
the new sky model. Morning, noon, overcast noon, 17:30 dusk and midnight are
checked. Component-isolation captures separately quantify warm direct-sun
bounce and cooler reflected skylight; legacy ambient brightness is not a target.

Full-resolution, unamplified matched captures:

| Scene / condition | Enhanced GI-Off | `a16aced` RTGI | Diffuse-sky RTGI |
| --- | --- | --- | --- |
| Malden morning | [image](evidence/rtgi-diffuse-skylight/malden-morning-off.png) | [image](evidence/rtgi-diffuse-skylight/malden-morning-old.png) | [image](evidence/rtgi-diffuse-skylight/malden-morning-sky.png) |
| Malden noon | [image](evidence/rtgi-diffuse-skylight/malden-noon-off.png) | [image](evidence/rtgi-diffuse-skylight/malden-noon-old.png) | [image](evidence/rtgi-diffuse-skylight/malden-noon-sky.png) |
| Malden overcast | [image](evidence/rtgi-diffuse-skylight/malden-overcast-off.png) | [image](evidence/rtgi-diffuse-skylight/malden-overcast-old.png) | [image](evidence/rtgi-diffuse-skylight/malden-overcast-sky.png) |
| Malden dusk | [image](evidence/rtgi-diffuse-skylight/malden-dusk-off.png) | [image](evidence/rtgi-diffuse-skylight/malden-dusk-old.png) | [image](evidence/rtgi-diffuse-skylight/malden-dusk-sky.png) |
| Malden night | [image](evidence/rtgi-diffuse-skylight/malden-night-off.png) | [image](evidence/rtgi-diffuse-skylight/malden-night-old.png) | [image](evidence/rtgi-diffuse-skylight/malden-night-sky.png) |
| Everon morning | [image](evidence/rtgi-diffuse-skylight/everon-morning-off.png) | [image](evidence/rtgi-diffuse-skylight/everon-morning-old.png) | [image](evidence/rtgi-diffuse-skylight/everon-morning-sky.png) |
| Everon noon | [image](evidence/rtgi-diffuse-skylight/everon-noon-off.png) | [image](evidence/rtgi-diffuse-skylight/everon-noon-old.png) | [image](evidence/rtgi-diffuse-skylight/everon-noon-sky.png) |
| Everon overcast | [image](evidence/rtgi-diffuse-skylight/everon-overcast-off.png) | [image](evidence/rtgi-diffuse-skylight/everon-overcast-old.png) | [image](evidence/rtgi-diffuse-skylight/everon-overcast-sky.png) |
| Everon dusk | [image](evidence/rtgi-diffuse-skylight/everon-dusk-off.png) | [image](evidence/rtgi-diffuse-skylight/everon-dusk-old.png) | [image](evidence/rtgi-diffuse-skylight/everon-dusk-sky.png) |
| Everon night | [image](evidence/rtgi-diffuse-skylight/everon-night-off.png) | [image](evidence/rtgi-diffuse-skylight/everon-night-old.png) | [image](evidence/rtgi-diffuse-skylight/everon-night-sky.png) |

### Separate light contributions

Six captures per diagnostic mode at the original morning views give these
linearized screenshot-luminance differences. `On - no-sun` isolates sunlight
bounce; `On - sun-only` isolates reflected skylight. These are post-AA/output
measurements, not calibrated in-engine radiometry.

| Region | Direct-sun bounce | Reflected skylight | Relative sun / sky addition |
| --- | ---: | ---: | ---: |
| Malden shaded wall | .002844 | .001441 | 5.64% / 2.78% |
| Everon concrete panel | .000755 | .000605 | 4.15% / 3.29% |

The percentages use their respective component-Off image as denominator, not
legacy brightness. Absolute sunlight transport remains close to the earlier
milestone (.00286 Malden, .000739 Everon); the changed denominator must not be
mistaken for amplified bounce. The direct-sun RGB increment is strongly warm
(Malden .00384/.00277/.00060); skylight is substantially cooler relative to it
(.00138/.00146/.00139). Stock surface color still modulates both contributions.

Real completed-frame morning visibility diagnostics:

| Scene | Eligible hits | Sun-visible hits | Secondary sky visible / blocked | Primary sky visible / blocked |
| --- | ---: | ---: | ---: | ---: |
| Malden | 107,934 | 64,242 | 34,540 / 51,351 | 57,084 / 1,331 |
| Everon | 53,251 | 29,729 | 40,323 / 11,657 | 53,043 / 2,395 |

Another 22,043 / 1,271 hit probes respectively face the lower/geometrically
invalid sky hemisphere and contribute zero. These are not unoccluded ambient
samples. The courtyard and compound contain actual sky-blocked endpoints;
covered portions remain dark rather than getting the same fill as open ground.
This is not an exhaustive enclosed-room qualification of unsupported materials.

Weather changes the components, not a global GI multiplier. For example,
Malden's single-pair noon sunlight/secondary-sky increments are .00382/.00136;
overcast changes them to .00026/.00183. Everon's are .00157/.00060 versus
.000069/.000665. Midnight has no qualified receivers in these CPU-lit views:
ordinary night-eye/local-light rendering is retained, with image differences
at sampling noise level. No artificial night glow is introduced.

### Visual outcome and honest brightness comparison

The missing hit-skylight term is now present, visibility-dependent and measurable.
Dusk shade gains useful visible-sky fill: the tested wall means rise about 96.5%
on Malden and 21.7% on Everon versus the old RTGI model. Patterns, camouflage,
weathering and UV orientation remain original, with neutral/cool sky fill and
warmer surface bounce coexisting. No artwork or color-bleeding filter is added.

This **does not eliminate the old 45--46% shade deficit by matching legacy
ambient**. Directional primary sky plus actual occlusion removes some of the old
uniform miss illumination. In morning, those regions are 10.25% / 30.41% darker
than `a16aced`, or 51.75% / 61.81% darker than GI-Off. At dusk they are instead
0.87% / 9.15% above GI-Off. These are observed results, not exposure targets.
The change is not uniform brightening, and this milestone is not a claim of
physically calibrated brightness or universally finished visual refinement.

### Cost, movement and preservation checks

Existing GPU profiler, RTX 4060 Ti, TAA at confirmed world dimensions. Two
interval medians per mode are averaged in Off/sun-only/On/On/sun-only/Off order;
three seconds settling and four seconds sampling each. Water and volumetric
cloud rendering are Off during timing; weather data and sky lighting stay live.

| Scene / world resolution | GI-Off frame ms | Full GI frame ms | Total GI cost ms | Extra secondary-sky trace ms |
| --- | ---: | ---: | ---: | ---: |
| Malden 1920x1080 | 5.035 | 5.652 | +.618 | +.213 |
| Malden 2560x1440 | 4.956 | 5.730 | +.774 | +.233 |
| Everon 1920x1080 | 3.679 | 4.130 | +.451 | +.056 |
| Everon 2560x1440 | 3.654 | 4.204 | +.549 | +.046 |

The final column compares trace scopes against `sun-only`, which already has
the new primary sky lookup/visibility work. It is **not** total skylight overhead
versus `a16aced`. Short whole-frame deltas include clock/cache variation; scope
times need not add exactly and non-monotonic resolution scaling is not a guarantee.

Running-mission camera translation/yaw and settled frames were inspected with
TAA and FSR Quality at 1440p. Original wall details remain recognizable without
new obvious wall trails/glow in these captures. Existing foliage-card artifacts,
texture blur and one-sample noise remain. BLAS build counts are stable within
both AA sweeps (Malden 202, Everon 252), rather than churning with camera/waves.
Live coastal waves also hold the build count at 621, with terrain reflection
hits shaded and water refraction active without fallback.

The four final Vulkan runs presented **63,358 frames, zero validation errors
and zero warnings**. Focused Vulkan tests: 45 cases / 13,980 assertions; GL33:
44 cases / 13,940 assertions; driver-free policy/lifetime guards: 684 checks.
Both public builds complete. Existing compiler warnings are separate from
runtime Vulkan validation. GL33's stationary PBR control has zero changed pixels.
RT sunlight, RT water, RTAO/SSAO, clouds, fog/rain, night-eye, real Jeep local
headlights/shadows, TAA/MSAA4/FSR and restored PBR v3 were exercised. Four original
PBO/world hashes still match; private DLSS/NGX and original profiles remain untouched.

[The qualification receipt](evidence/rtgi-diffuse-skylight/qualification.json)
contains runtime hashes, per-frame counters, all component measurements, source
preservation hashes, profiler intervals, motion receipts and remaining limits.
Raw local runs remain under `build/shadow-live/skylight-*`; no new testing
framework or replacement assets were introduced.

## Limits

The stock sky and light coefficients are authored appearance, **not measured
physical radiance**. Stock surface RGB still includes baked artistic shading
and uses the existing bounded reflectance proxy. The cloud model is the existing
layer approximation, not a calibrated atmosphere. No normalization to legacy
ambient or global compensation is applied.

Visibility is finite, scene collection is bounded, and unsupported materials
still form a legacy boundary. Empty below-horizon misses contain no represented
outgoing radiance. More distant geometry, additional surface bounces, terrain
palette reconstruction, indirect specular and ray mip footprints remain future
work. Sky-blocked areas can legitimately stay dark; one sample also leaves
temporal/spatial noise. Unchanged nighttime fallback is not evidence of newly
implemented moonlight transport.
