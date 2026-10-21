# Stock-faithful enhanced PBR and one-bounce RTGI

This page records the `86a5f09` catalog/material foundation and its qualification.
For the subsequent terrain and PBR v3 reflector expansion and current evidence,
see [Stock RTGI reflector coverage](rtgi-stock-coverage.md).
For the optional second diffuse surface bounce, controls, cost and limitations,
see [Second diffuse bounce](rtgi-second-bounce.md). One bounce remains the default.
For the subsequent enhanced-only original roof/gable coverage and comparisons,
see [stock material participation](rtgi-material-participation.md).
For an eleven-location, four-island coverage survey and ranked next targets,
see [original-world reconnaissance](rtgi-world-survey.md).
For the opt-in shared source/triangle classifier and its conservative pilot limits,
see [automatic material qualification](automatic-material-qualification.md).

Public base: `9f74c7f` (origin/main verified before work). `triPBR 'enhanced'`
uses the original stock color textures by default. `triPBR 'on'` retains stock
PBR v3 and its complete existing allowlist; `off` retains legacy rendering.
Enhanced PBR and `triRTGI 'on'` remain independent developer controls, default
off, with no graphics-menu or preset changes.

The procedural stone-wall albedo, unrelated relief/roughness maps, generator
and packaging target have been removed. No replacement artwork ships. The four
matching generated-map copies in the public build/runtime were moved to
`build/archive/stock-materials-retired-art-20261019/`, after verifying their Git
blob identities. Original PBOs, source assets and private DLSS are untouched.

## Material contract and coverage

Each catalog entry has independent optional albedo, XYZ tangent normal and
red-channel perceptual-roughness paths. Empty or unavailable inputs retain
stock color, the original geometry/shading normal and scalar roughness. A
normal map does not imply a roughness map or vice versa. Optional data is linear;
only color is sRGB. Normal RGB and roughness R are packed into the existing
data sampler, using available authored mip levels. Base bytes are retained for
lazy ray-data upload if RT is enabled after raster material loading. No normals
are inferred from painted contrast, cracks, mortar or weathering.

| Family / stock model | Original sections under `data/` | Visual LODs |
| --- | --- | --- |
| Stone/mortar with clay cap, `data3d/zed_kamenna.p3d` | `kamennazed.pac` | .5, 1 |
| Weathered/painted concrete, `data3d/zed2.p3d` | `plot_beton_03_top.pac`, `plot_beton_0302_side.pac`, `plot_beton_02_side.pac` | .5, 1; panel/pillar also 4 |

These are existing PBR v3 classifications. All four enhanced identities use
roughness .90, metallic 0 and F0 .04. They have **no supplemental maps**. Exact
model, section, LOD, vertex/section counts, source geometry fingerprint and
decoded stock pixel dimensions/fingerprint must agree. Shared names on another
model, changed assets and memory/collision LODs cannot inherit the enhancement.
Mixed-material building/window atlases and the distinct `zed3` wall remain
outside the enhanced catalog. Other PBR v3 surfaces continue to work as before.

Native sections and eligible CPU-lit triangles call the same catalog and BRDF.
The CPU path retains original normals, material modes, emissive data and
selected point/spot light inputs. Already-lit packed vertex RGB never becomes
albedo. Unsupported vertex modes, clipping across material slots and unsupported
light sets keep their established fallback. Native submission now accepts a
validated section of a multi-section model, rather than requiring the complete
model to be one draw. Per-section enhanced state is reset before classification.

The existing GGX/Smith/Schlick kernel, sunlight visibility, local-light attenuation,
cones/shadows, emissive terms, fog, weather and night-eye handling remain in use.
Enhanced raster and hits retain the bd1d103 common incident sun/ambient convention
and the existing Lambert-reference calibration, with no exposure or light gain.
PBR v3 retains its original light/material coefficient convention.

## Unmeasured stock reflectance

Original RGB contains artistic color, dirt and baked shading. It is **not** a
physical albedo measurement. `material_diffuse.glsl` implements a conservative
appearance proxy: clamp linear RGB to [0,1], then scale all channels together
only when their maximum exceeds .5. This preserves chroma and texture layout,
never brightens dark pixels, and bounds the assumed dielectric reflectance.
The .5 bound is an explicit unmeasured assumption, not recovered asset metadata
or a claim to remove baked light. Bright painted details can have a compressed
lighting response; baked dark details remain dark.

Raster diffuse/ambient and ray-hit diffuse share this function, the same stock
identity, F0 and metallic exclusion. Receiver records use it too. Specular keeps
the existing GGX response; emissive keeps its original meaning. Texture sampling
and UVs are unchanged. Secondary rays still use base-mip bilinear color, whereas
raster uses the original filtered mip chain; secondary texture footprints remain
unimplemented. More accurate reflectance requires asset-specific evidence.

## Transport and fallback

Material identity and scalar roughness are valid without a normal-map address.
Optional normal and roughness channels have separate presence flags. The ray
material ABI and existing RT-water reflection color contract are retained.

The source wall geometry remains terrain fitted. The existing Animate/Deanimate
snapshot path now admits a multi-section fitted model only when **every** section
passes the explicit catalog checks. Original positions, normals, UVs and topology
are retained; snapshots refresh when fitted bytes change. This does not admit
arbitrary animated or damaged models.

One cosine-weighted diffuse ray uses actual hit visibility and alpha-tested
sun visibility, macro-normal hemisphere checks and cloud shadows. Known stock
hits return bounded diffuse reflectance times direct incident sunlight. There
is no hit-distance gain, screen-wide GI term, added ray count or global brightness
boost. `triRTGI 'sky-only'` keeps traversal and sun visibility but suppresses
known-hit surface radiance, solely for bounce isolation.

Unsupported hits have unknown outgoing radiance and retain their legacy ambient
share. Finite-range misses retain the same ambient boundary. They do not become
black absorbers or emit inferred texture light. This remains a partial hybrid
transport solution, not complete physical scene GI.

Raster captures only its global ambient response, including fog transmittance.
The existing negative-depth record reserves it for GI, excluding it from AO.
After valid spatial reconstruction, composition replaces that ambient with the
indirect estimate. Direct/local light, emission and fog in-scattering survive.
Rejected reconstruction retains raster lighting; distance fading cross-fades the
replacement. Night-eye retains ordinary raster/AO until transport shares that
response. Temporal history, disocclusion checks and one-ray filtering are retained.

## Current gameplay qualification

Evidence for this stock-preserving implementation is kept separately under
`docs/evidence/stock-materials-rtgi/`. Raw captures, action/status receipts,
profiler logs and validation output are in `build/shadow-live/stock-pbr-*-final-20261019`.
These are muted direct developer launches of original missions, with copied
profiles, ordinary brightness, 1400 m visibility and real mission geometry;
they do not certify Steam launching or mission completion.

Current results and exact runtime identities are recorded in the qualification
receipt alongside the comparisons. Matched legacy, v3, enhanced stock and
enhanced+GI captures cover morning and noon, direct/shaded faces, material
boundaries and the original atlas orientation. The stock stone cap and masonry,
and the concrete camouflage, cracks, stains and pillar/cap detail remain visible.
RGB detail correlations against v3 exceed .99 in the registered wall regions;
this supports preservation of detail, not physical albedo accuracy. Expected
lighting differences remain because enhanced materials use the shared incident
light convention and bounded reflectance assumption.

| Scene | Legacy | Stock PBR v3 | Enhanced stock | Enhanced + RTGI |
| --- | --- | --- | --- | --- |
| Malden stone wall | [PNG](evidence/stock-materials-rtgi/malden-legacy.png) | [PNG](evidence/stock-materials-rtgi/malden-v3.png) | [PNG](evidence/stock-materials-rtgi/malden-enhanced.png) | [PNG](evidence/stock-materials-rtgi/malden-gi.png) |
| Everon concrete | [PNG](evidence/stock-materials-rtgi/everon-legacy.png) | [PNG](evidence/stock-materials-rtgi/everon-v3.png) | [PNG](evidence/stock-materials-rtgi/everon-enhanced.png) | [PNG](evidence/stock-materials-rtgi/everon-gi.png) |

The counterbalanced morning bounce isolation used nine captures per mode.
Malden shaded-wall linear luminance increased from .090464 with surface bounce
suppressed to .091745 with it enabled: +.001281, or **1.42%**. The corresponding
sunlit region changed +.000023 (.02%); an unmodified-building control changed
-.000336 within its sampling variation. The shaded-wall sample ranges do not
overlap. This supports a modest local stock-surface bounce, not screen-wide
brightening or a claim that net GI must brighten every wall.

Everon reported 33 enhanced instances and eligible concrete hits. Its panel
isolation changed -.000011 (-.02%), below the observed sample variation. It
demonstrates shared stock-material participation and fallback, **not** a proven
positive visual bounce there. No gain, ray count or artwork was changed to
amplify either result.

Standing developer-camera translations/yaw with TAA and FSR Quality keep the
wall in frame on both islands. The stored moving and settled views show intact
texture orientation; residual foliage aliasing, stock low-resolution textures
and estimator noise remain. These are render-camera motion checks, not player
input responsiveness tests. Earlier downward-looking infantry captures are
excluded. Camera retreats at 8/30/60/100 m inspect material/LOD boundaries;
GI retains its normal receiver-distance fade. Selected walls exercise captured
CPU lighting/terrain fitting; native semantics also pass the common shader and
inverse-transpose tests, without claiming a separate native gameplay example.

Both final Vulkan runs completed: 14,330 Malden + 13,253 Everon presented frames,
zero validation errors and zero warnings. The Malden smoke covers RT sunlight,
RTAO/SSAO, clouds/cloud shadows, headlights/local shadows, fog/rain, night-eye,
MSAA4 and textured RT water/refraction, then restores stock PBR v3. Vulkan and
GL33 builds pass; focused checks pass 42 cases/1,631 assertions and 41 cases/1,591
assertions respectively, plus 684 driver-free checks. GL33 Off/On/Enhanced/Off
retains the original wall with zero changed pixels in the control region. SHA256
checks confirm the original two PBOs and two worlds match the inspection receipts.

Short stationary 1080p TAA profiler intervals, ordered off/on/on/off, measured
RTGI-off/on mean frame medians of 4.918/5.583 ms on Malden (+.665 ms) and
3.649/4.050 ms on Everon (+.401 ms). Trace/filter/history scopes averaged
.407/.205/.211 ms and .089/.124/.114 ms respectively. Scopes are not additive;
these short intervals are not general performance guarantees. Runtime hashes,
per-interval samples, status counters and pixel regions are in the
[qualification receipt](evidence/stock-materials-rtgi/qualification.json).

The earlier authored-wall 3.86% measurement
in `evidence/enhanced-materials-rtgi/` is historical and does **not** qualify this
implementation. It must not be reused as a stock-material result.

Remaining limits include baked texture shading, assumed optical properties,
partial endpoint coverage, short sun/transport range, a calibrated ambient
boundary rather than a measured sky, one diffuse bounce, residual stochastic
noise, no indirect specular and no secondary-ray mip footprints. This advances
the shared material/lighting infrastructure without declaring PBR or GI finished.
