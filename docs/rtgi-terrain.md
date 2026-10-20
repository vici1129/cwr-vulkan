# Stock terrain RTGI reception

Public base: `1a2339d`. Developer-only/default-Off PBR and RTGI retain original
artwork. Terrain's existing ray-reflector path and the calibrated sky/sun,
one-bounce estimator, visibility rays and temporal reconstruction are unchanged.

## Lighting and material contract

Terrain stays on its original raster lighting path; identity `1000` does **not**
turn it into an object GGX material or reinterpret lit vertex RGB as albedo.
Its new receiver record contains original texture color, independent original
material/palette tint, existing scalar detail modulation and wet-diffuse
attenuation. The shared `materialDiffuse` function bounds the artistic linear
color to a .5 maximum component and applies the same .04 dielectric F0 as terrain
ray hits. Wet attenuation follows that bound. These are unmeasured reflectance
approximations, not calibrated physical albedo or de-lit stock artwork.

There are two real submission routes:

- **Native terrain**, used by the ordinary tested configuration: capture the
  actual global ambient from the existing material lighting, separately from
  direct sun, forced diffuse, emission and local lights. Retain its original
  neutral material tint, vertex normals, stock UVs and repeating 32x detail alpha.
  Do not introduce CPU palette variation into this previously neutral route.
- **CPU-colorized terrain**: reuse `DoLightingColorized`'s RTAO ambient sidecar,
  `sun->AmbientPrecalc() * palette * 2`, before packing. Carry the original
  outward normal and **unlit** palette multiplier in an optional sidecar through
  CPU clipping. Invalid endpoints do not interpolate into valid receivers. Both
  separately captured ambient and sunlight must exist at every submitted corner.

The CPU route also needs actual original geometry at the right world position.
Its centered landscape segments now include their existing translation in the
view transform and local-light positions. Original normals need no translation.
Software dry terrain supports its existing DetailTexture alpha operation; it
does not substitute an invented texture or infer relief from brightness.
The matching software water fallback also retains scalar specular-detail alpha
at 32x UV, never an invented RGB normal map, and remains a non-GI water draw.

Capture still evaluates the actual nonlinear legacy-light difference on the
sun-independent base, including detail/wet attenuation, night-eye response and
fog transmittance. The existing HDR composition replaces that difference:
`(receiverReflectance * incoming - capturedAmbient) * distanceFade`. Direct sun,
sun shadows, local light, wet highlights, emission and in-scattered fog are not
removed. There is no new ambient light, gain or exposure term.

Receiver reflectance includes original palette/detail appearance where present;
ray-hit terrain retains its previous conservative stock-color proxy unchanged.
That endpoint approximation is deliberately not claimed to be a fully matched,
measured terrain BRDF. No source-side reflectance or ray geometry code changes.

## Eligibility and fallback

An explicit landscape-ground boundary distinguishes real terrain from fitted
roads/objects. Existing opaque, depth-tested/depth-writing, valid-texture and
supported-lighting gates remain prerequisites. Water draws, cutouts/blends,
animated textures, forced/unsupported native lighting and invalid inputs retain
their legacy path. The nonlinear night-eye fallback also remains in force.

All original triangle corners must be above **maximum tide + maximum wave**
(5.25 m), with finite original upward terrain normals. Native geometry caches
ordered dry/non-dry triangle ranges with its existing buffer snapshot; draw
splits preserve texture sections and original index order. Rejecting an entire
large coastal segment would incorrectly discard its inland ground. Instead,
potential-water triangles retain ordinary ambient while proven dry triangles
can receive. CPU clipping is conservative at the corresponding boundary.

The existing wet-surface response may contain a small nonzero rain value even
in a clear scene. Requiring exactly zero would eliminate ordinary terrain.
Instead, use its **actual existing** diffuse attenuation in both ambient capture
and reflectance, preserving the independent wet highlight. This is not a GI
strength adjustment. Water and the potential coastal band are still excluded.

The same valid-fragment decision tags both captured ambient and material data.
Original shading normals, depth/coverage reconstruction and the shared **45 m**
receiver bound remain mandatory; the existing distance fade is unchanged.
Actual fragment geometry must also be non-vertical and agree with the original
shading normal. Vertical LOD skirts borrow upward normals but are artificial
curtains, not the height field; they keep their original ambient.
Negative ambient tags keep RTAO/SSAO from multiplying replacement GI again.
Outside reception, normal legacy ambient and AO remain available.

No extra G-buffer or transport pipeline is added. Native/CPU terrain use the
previously spare receiver classes 0/8 in the existing 24-bit record. Normal
precision and the positive-depth water ABI are unchanged. `triRTGI 'status'`
reports `receiverPixels(native/CPU)=...,terrain:N/C,` independently of terrain
**ray-hit** source counters. The optional software vertex tint is four floats.

## Verification scope and limitations

The existing original Malden/Everon mission harness uses copied profiles,
muted exact-window capture, ordinary brightness and grounded render cameras.
Morning, noon, overcast noon and dusk compare GI Off, the actual preserved
`1a2339d` reflector-only executable and this terrain receiver implementation.
Camera/contact receipts restore temporary ground probes atomically. These are
real mission render-camera checks, not mission completion or player-input tests.

Stock texture weathering, soil/grass transitions, UV orientation and scalar
detail remain original. Unsupported house/window atlases, foliage and coastal
triangles still retain their old lighting; they are not counted as receivers.
Original foliage-card artifacts and filtering blur are not fixed here.

GI remains approximate: baked artistic shading, engine-relative sky energy,
finite 25 m transport / 80 m visibility, one surface bounce, spatial/temporal
noise and receiver-distance fade. There is no indirect specular or reflected
local-light transport. A brighter legacy screenshot is not physical ground truth.

## Matched gameplay results

The [qualification receipt](evidence/rtgi-terrain/qualification.json) contains
executable hashes, grounded cameras, actual receiver/source counters, original
screenshots, component measurements, profiler samples and validation receipts.
Only the final `2C9ADD82...` Vulkan binary is used for the final qualification;
the reflector-only control is the actual `1a2339d` runtime (`CFC13CDC...`).

Completed morning-frame coverage, after normal/depth/distance validation:

| Ground-level view | Before total receivers | After total | New terrain receivers | Terrain share of half-resolution grid |
| --- | ---: | ---: | ---: | ---: |
| Malden village yard, 1080p | 0 | 339,551 | 339,551 | 65.50% |
| Everon barn yard, 1440p | 135,038 | 664,898 | 529,790 | 57.49% |

These are view-specific eligible sample pixels, not percentages of the entire
game. The Malden stone-wall ground view gains 161,184 terrain samples; the Everon
concrete-wall ground view gains 118,433. Native/CPU counters distinguish actual
reception from source hits. In the separate live CPU check, terrain changes from
native reception to **340,489 CPU-colorized receiver samples**, then back to
native. Original world placement, detail UVs and shading remain recognizable.

The CPU check first reproduced the unchanged baseline's unsupported software
terrain-detail exception. Ground detail support then exposed the matching water
detail gap; both are corrected narrowly, with centered world placement retained.
The final native/CPU/native run exits normally with zero validation errors or
warnings. Earlier diagnostic crashes are not included in final qualification.
The software route is a correctness fallback, not a native-performance claim.

Existing receivers remain active: the old Malden roof control has 87,116 native
roof samples, stone has 215,010 captured CPU catalog samples, and the Everon
1440p controls have 162,539 native facade and 201,555 CPU catalog concrete samples.
The latter also retains 11,968 CPU stock-concrete samples. Resolution-scaled
coverage agrees with the prior milestone; this is not a claim of pixel-identical
1080p/1440p captures. Original object PBR and ray-reflector code are unchanged.

### Original, unamplified comparisons

| Scene / condition | Enhanced / GI Off | `1a2339d` reflector-only terrain | New terrain reception |
| --- | --- | --- | --- |
| Malden morning | [PNG](evidence/rtgi-terrain/malden-morning-off.png) | [PNG](evidence/rtgi-terrain/malden-morning-reflector-only.png) | [PNG](evidence/rtgi-terrain/malden-morning-receiver.png) |
| Malden noon | [PNG](evidence/rtgi-terrain/malden-noon-off.png) | [PNG](evidence/rtgi-terrain/malden-noon-reflector-only.png) | [PNG](evidence/rtgi-terrain/malden-noon-receiver.png) |
| Malden overcast | [PNG](evidence/rtgi-terrain/malden-overcast-off.png) | [PNG](evidence/rtgi-terrain/malden-overcast-reflector-only.png) | [PNG](evidence/rtgi-terrain/malden-overcast-receiver.png) |
| Malden dusk | [PNG](evidence/rtgi-terrain/malden-dusk-off.png) | [PNG](evidence/rtgi-terrain/malden-dusk-reflector-only.png) | [PNG](evidence/rtgi-terrain/malden-dusk-receiver.png) |
| Everon morning | [PNG](evidence/rtgi-terrain/everon-morning-off.png) | [PNG](evidence/rtgi-terrain/everon-morning-reflector-only.png) | [PNG](evidence/rtgi-terrain/everon-morning-receiver.png) |
| Everon noon | [PNG](evidence/rtgi-terrain/everon-noon-off.png) | [PNG](evidence/rtgi-terrain/everon-noon-reflector-only.png) | [PNG](evidence/rtgi-terrain/everon-noon-receiver.png) |
| Everon overcast | [PNG](evidence/rtgi-terrain/everon-overcast-off.png) | [PNG](evidence/rtgi-terrain/everon-overcast-reflector-only.png) | [PNG](evidence/rtgi-terrain/everon-overcast-receiver.png) |
| Everon dusk | [PNG](evidence/rtgi-terrain/everon-dusk-off.png) | [PNG](evidence/rtgi-terrain/everon-dusk-reflector-only.png) | [PNG](evidence/rtgi-terrain/everon-dusk-receiver.png) |
| Malden wall/ground | [PNG](evidence/rtgi-terrain/malden-wall-off.png) | [PNG](evidence/rtgi-terrain/malden-wall-reflector-only.png) | [PNG](evidence/rtgi-terrain/malden-wall-receiver.png) |
| Everon wall/ground | [PNG](evidence/rtgi-terrain/everon-wall-off.png) | [PNG](evidence/rtgi-terrain/everon-wall-reflector-only.png) | [PNG](evidence/rtgi-terrain/everon-wall-receiver.png) |

The ordinary Everon yard now has visibly cooler, spatially varying ground fill,
while tree/building shade remains identifiable and original grass/soil patterns
are preserved. Malden changes more subtly and can become darker beneath cover;
it is not a screen-wide brightness increase. Linearized **SDR screenshot** ROI
means, after AA/output, quantify this without serving as radiometric calibration:

| Condition | Malden open ground | Malden shaded ground | Everon open ground | Everon shaded ground |
| --- | ---: | ---: | ---: | ---: |
| Morning | +3.19% | -3.57% | +16.01% | +24.44% |
| Noon | +2.26% | +.10% | +14.59% | +12.61% |
| Overcast | -.89% | +3.19% | +47.96% | +40.88% |
| Dusk | +3.42% | -3.47% | +9.95% | +8.78% |

Percentages compare new reception with reflector-only terrain, using the same
camera, weather and original artwork. They are not targets for legacy brightness.
The GI-Off shaded-ground control varies by at most .30% in Malden across these
matched runs; minor cloud/foliage/sampling differences are retained in the receipt.

### Sunlight versus sky and surface bounce

Four interleaved captures per diagnostic mode separate primary visible sky,
direct-sun surface bounce (`On - no-sun`) and reflected skylight
(`On - sun-only`). `On - sky-only` measures combined surface bounce. These are
output-space observations, not an exact additive HDR energy budget:

| Ground ROI | Direct-sun bounce change | Reflected-sky change | Combined surface bounce change |
| --- | ---: | ---: | ---: |
| Malden shade | +.020% | +.036% | +.121% |
| Everon shade | +.006% | +.486% | +.516% |
| Everon facade-adjacent ground | -.049% | +.953% | +.955% |

The very small signed sun differences are not a resolved positive direct-sun
bounce measurement. Most of the visible improvement here is directional sky
fill, with a modest measurable reflected-skylight addition near Everon's facade.
Do not describe the tiny Malden component differences as visible warm bleeding.
The morning Everon frame still reports 93,554 eligible surface hits and 51,633
sun-visible secondary paths across the whole view, versus 156/133 in this
particular Malden yard. Those whole-view counters are not per-ROI attribution.
No visibility check, transport gain or source classification was relaxed to
make the component subtraction positive.

## Motion, cost and preservation

At 1440p, real mission simulation advances while the render camera translates
and yaws under TAA and FSR Quality. Sampled moving/settled frames preserve terrain
patterns without new obvious ground-light trails or glow in these checks.
This is not a continuous-video audit. BLAS build counts stay at 105 in Malden
and 574 in Everon through both sweeps. Grounded 0/12/28/42/65/100 m distance
sweeps exercise ordinary terrain/material LOD selection and return to the start;
they do not force every LOD independently. Skirts/uncertain boundaries remain
legacy rather than becoming illuminated curtains.

Existing profiler, RTX 4060 Ti, TAA/100% world resolution, Off/On/On/Off order;
four-second settling and six-second intervals, two interval medians averaged per
mode. Volumetric clouds and water optics are Off during the ground-view timings;
stock sky/weather and cloud shadows remain active. No build overlaps timing.

| Ground view / world resolution | GI Off ms | GI On ms | Full GI cost ms |
| --- | ---: | ---: | ---: |
| Malden 1920x1080 | 4.390 | 5.406 | +1.017 |
| Malden 2560x1440 | 4.465 | 6.028 | +1.563 |
| Everon 1920x1080 | 2.181 | 2.894 | +.713 |
| Everon 2560x1440 | 3.100 | 4.929 | +1.830 |

These are full GI costs including capture, material records, transport, filtering
and diagnostics, not isolated receiver-policy overhead. Total primary samples
increase with the newly eligible ground area; the estimator remains one sample
per eligible half-resolution receiver. Short whole-frame timings include clock
and cache variation; this is not an optimization claim.

The **previous elevated Everon barn view** was separately rechecked at 1440p:
unchanged `1a2339d` measures 2.743 -> 3.769 ms (**+1.026 ms**), reproducing the
published +1.016 ms. Its trace/filter/temporal sum is .649 ms, so the old whole-
frame difference must not be labeled ray-query cost alone. The existing path
also captures ambient/material data and uses the higher-precision GI attachment.
This is a scope distinction, not a complete measured attribution of the residue.

The updated same view measures 2.745 -> 4.091 ms (**+1.345 ms**), now including
100,838 terrain receiver samples. Trace/filter/temporal total .837 ms. GI-Off
cost is essentially unchanged; no clear new pathological regression was isolated.
The extra receiving area, raster record work and filtering have a real cost,
which remains documented rather than hidden by changing quality or gain.

The four final native runs present **62,957 frames**, plus **6,585** in the final
native/CPU/native check: **zero Vulkan validation errors or warnings**, with
validation and synchronization validation confirmed enabled. Pre-existing
external overlay-manifest/disabled-layer loader notices are separate and were
not changed. Earlier failing diagnostic probes are excluded from this count.

Focused Vulkan tests: **52 cases / 14,540 assertions**; GL33: **51 / 14,500**;
driver-free policy/lifetime guards: **684 checks**. Both public builds pass.
Tests exercise actual receiver-range order/bounds, invalid terrain inputs,
palette/normal clipping, record precision and ambient/material output guards.
Policy tests deliberately inject errors; actual gameplay logs establish runtime
validation results.

RT sunlight, local Jeep headlights/shadows, RTAO/SSAO, fog/rain, clouds, night-eye
fallback, TAA/MSAA4/FSR and water/refraction were exercised. The live coastal
wave check keeps BLAS builds at 711, shades actual terrain reflection hits and
retains refraction without fallback. Its low/uncertain shore does not become a
GI receiver. GL33 renders the original scene and its stationary PBR control has
zero changed pixels. Four original PBO/world SHA256 hashes still match.

Original assets, player settings/presets and private DLSS/NGX are untouched.
Raw local runs remain under `build/shadow-live/terrain-*`; no testing framework,
material editor or replacement-art pipeline was added. This establishes useful
terrain reception, not comprehensive or physically finished GI.
