# Enhanced stock material and one-bounce GI vertical slice

Base: public main `85288c0`. The optional path replaces the inspected stone
wall's painted color with original authored albedo and normal/roughness maps.
It connects the same material to raster, eligible CPU-lit geometry and ray hits.
It does not convert an asset library or modify original archives.

## Controls and scope

`triPBR 'enhanced'` opts into the stone-wall catalog. `on` remains PBR v3;
`off` remains legacy. `triRTGI 'on'` is a separate developer control, default
off. `triRTGI 'sky-only'` is a transport isolation diagnostic: traversal and
sun visibility remain active, but enhanced-hit surface radiance is suppressed.
It is not a recommended visual mode. There is no artist-adjustable GI gain.

The [material example](../materials/enhanced/stone-wall/README.md) documents its
unmeasured authored parameters and regeneration. Exact model path, visual LOD,
section and source geometry/pixel fingerprints prevent a mod using the same
texture name from silently inheriting the enhancement. Missing maps and
unsupported material/vertex modes fall back. Private DLSS/NGX is unrelated.

## Material and light contract

Raster and hits share original supplemental sRGB albedo, linear normal and
roughness data, metallic and F0. The existing GGX/Smith/Schlick kernel gains
colored metal Fresnel and diffuse energy exclusion; v3 uses its dielectric
wrapper/defaults. Ambient, direct sun, selected point/spot lights, emission and
specular remain separate. Captured CPU lighting supplies material/light inputs;
already-lit packed vertex RGB never enters the BRDF.

Normals are reconstructed in object space from position/UV derivatives in
raster and triangle edges in RT, with the **same** tangent frame and mirrored-UV
handedness. The complete mapped normal is inverse-transpose transformed, which
also handles nonuniform object scale. Degenerate UVs retain the original normal.
Macro normals remain separate for offsets, hemisphere tests and denoising.
Ray hits retain base-mip bilinear sampling; raster uses ordinary filtered mips.
Secondary texture-footprint/mip selection is a remaining limitation.

The wall is terrain-fitted and reports `Object::IsAnimated`, so the old static
ray scene excludes it. The enhanced catalog now captures its existing
`Animate`/`Deanimate` pose, including offscreen walls. Per-object immutable
snapshots are reused while vertex/index bytes agree, refreshed on terrain/pose
changes and pruned outside the bounded scene. Original normals, topology and UVs
are retained. This exception applies only to explicitly enhanced, validated
static walls, not arbitrary animation, damaged models or terrain proxies.

Enhanced raster and RT use identical incident sun/ambient coefficients and
the ordinary engine brightness/color filter once. The existing Lambert-reference
calibration (`pi * engine light` for a normalized BRDF) is retained. These engine
lights are not calibrated photometric measurements.

## Indirect composition

One cosine-weighted diffuse path samples the authored receiver normal. A known
enhanced hit reflects its authored diffuse reflectance times directly incident
sunlight, with actual geometric hemisphere, alpha-tested RT sun visibility and
cloud visibility. Receiver reflectance is applied after filtering incident
radiance. There is no extra inverse-square factor on transported radiance,
hit-distance gain, second surface bounce or inferred texture-RGB albedo.

Adoption is deliberately partial. Unconverted endpoints have unknown outgoing
radiance and **retain their legacy ambient share**, rather than becoming black
absorbers. Upper-hemisphere misses use an ambient-calibrated environment;
finite-range misses retain the same ambient boundary. Only known material
transport replaces that share. This hybrid fallback is not a claim of complete
physical ground/building/foliage transport. Rejected-material counters expose
how much transport is still unsupported.

The raster pass captures the global ambient response around the sun-independent
base, including night-eye response and fog transmittance. A negative-depth
record reserves that ambient for GI, excluding it from RTAO/SSAO. After valid
reconstruction, GI composes `indirect - captured global ambient`; direct/local
light, emission and fog in-scattering survive. Rejected samples keep raster
lighting. A distance fade cross-fades the replacement, not its energy gain.

While night-eye is active, draws retain ordinary raster lighting and AO rather
than recording GI receivers. The captured ambient already includes night-eye,
but the reconstructed indirect radiance does not yet share that response; even
the ambient fallback would otherwise change brightness and color. Daytime GI
replacement resumes when the night-eye response is inactive.

The shared non-water receiver carrier is RGBA32F only while GI is enabled. It
holds bounded diffuse reflectance, a packed 10-bit octahedral shading normal,
fog and original depth; positive depth retains the water ABI. GI also reserves
the existing ambient carrier. Clouds preserve these records. The ray material
ABI appends map addresses/parameters without changing existing field offsets.

The half-resolution estimator uses one ray, 32 temporal phases, a macro-plane
bilateral filter and up to 16 frames of validated history. New light/weather,
camera cuts, invalid motion/depth/normals and dynamic-hit dependency reject
history. Centimetre-wide shared random seeds previously correlated whole image
blocks; subpixel world seeds avoid that estimator defect without adding rays.

## Qualification

Actual stock `01TakeTheCar.ABEL`, 1920x1080, ordinary profile brightness,
1400 m visibility, TAA, RT sunlight and real mission objects. Standing cameras
look into the stone-wall courtyard near `[3128,228.1,6338]` and
`[3130,228.2,6345]`, with both sunlit masonry and nearby shaded masonry. No
demonstration geometry, disabled visibility, exposure override or specular/GI
gain was used. Direct harness launches are not Steam/mission-completion tests.

The settled, counterbalanced nine-capture-per-mode morning isolation measured
shaded-wall linear luminance .172006 with surface bounce suppressed and .178652
with it enabled: +.006646 (3.86%). The sunlit region changed +.000436; an
unmodified-building control changed -.000330. This demonstrates a local surface
bounce, not global brightening. These are rendered-image measurements, not
radiometric ground truth. The unconverted-material fallback remains active.

The final Vulkan validation run presented 29,595 frames with zero errors and
zero warnings, including TAA, MSAA 4x, FSR Quality, RTAO/SSAO, clouds/cloud
shadows, local headlights/shadows, fog/rain, night and working RT water/refraction.
GL33-only build, focused tests and a stock-mission rendering control pass too.
The selected live wall exercises CPU terrain fitting; native transform behavior
is covered by shared shader compilation and math tests, not a separate native
gameplay material demonstration. Fifty focused local Vulkan cases (35,837
assertions), 39 GL33 cases (1,391 assertions), 624 driver-free checks and eight
staged shader variants without pre-existing edits pass.

A short counterbalanced 1080p TAA profiler sample measured mean interval medians
of 5.155 ms with GI off and 5.615 ms with it on (+.460 ms). GI trace/filter/history
scopes averaged .535/.206/.208 ms. The timestamp scopes are not additive and
this short sample is not a broad performance guarantee.

[Legacy](evidence/enhanced-materials-rtgi/malden-legacy.png),
[PBR v3](evidence/enhanced-materials-rtgi/malden-v3.png),
[enhanced raster](evidence/enhanced-materials-rtgi/malden-enhanced.png),
[enhanced + RTGI](evidence/enhanced-materials-rtgi/malden-gi.png) and
[bounce-suppressed diagnostic](evidence/enhanced-materials-rtgi/malden-sky.png)
use the same standing view and ordinary exposure. The
[qualification receipt](evidence/enhanced-materials-rtgi/qualification.json),
pixel isolation, frame intervals and mode/status records retain the exact
runtime identities and numerical evidence.

Validation artifacts and final checks are recorded in that evidence directory;
raw captures, complete validation output and profiler logs remain under the
corresponding `build/shadow-live/enhanced-pbr-*` runs. The first source-lookup,
attachment-format and terrain-fitting defects were fixed before qualification.

This is a working material/transport foundation. The new relief and rough
dielectric response are visible; the authored rubble pattern still needs artistic
refinement. Indirect changes are modest, with residual stochastic noise and
limited material coverage. The comparisons do not establish a production-ready
modern remake or complete physical GI, and the path remains developer-only.
