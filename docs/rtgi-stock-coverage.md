# Stock RTGI reflector coverage

This records the `a16aced` material-coverage milestone. For the subsequent
directional sky boundary and visibility-tested reflected skylight, see
[diffuse skylight](rtgi-diffuse-skylight.md).

Public base: `86a5f09`, verified against origin/main before work. This extends
the existing one-bounce, ambient-replacing developer RTGI path. No artwork,
original assets, PBOs, private DLSS/NGX, presets or default feature states change.

## What prevented useful transport

The old ray scene already included the ground, but RTGI required a nonzero
enhanced-catalog identity. Ground and ordinary qualified PBR v3 sections failed
that gate. At the original Everon camera, a fresh baseline frame recorded 53,555
hits, only 2,557 eligible hits and 50,998 rejected materials. Most omitted hits
were nearby terrain, not absent ray intersections.

The original concrete hits also have a real illumination limitation. In that
morning frame only 348 secondary sun queries ran and 303 were blocked; many
other concrete normals face away from the sun. Expanding material coverage must
not turn those paths into unoccluded light.

Terrain's existing RT data was sufficient for coarse reflection color, not for
this material qualification. Its vertex normals were placeholders. Shared UV
vertices used global repeating coordinates even on clamped four-part terrain
tiles. Its cache did not track terrain texture-ID changes. These inputs are
corrected before terrain is admitted as a diffuse reflector.

## Shared materials, separate source and receiver eligibility

`PBRMaterialVK.hpp` now gives the **unchanged 23-entry PBR v3 allowlist** explicit
shared identities. Qualified opaque static ray sections can use those identities
without an enhanced model/LOD catalog entry. Supported original normals/UVs,
ordinary vertex lighting modes and existing render flags remain required.
Colored/emissive modes, cutouts, foliage impostors, mixed window/metal atlases,
missing inputs, arbitrary animation and damaged geometry keep conservative
fallback. Terrain fitting still requires the validated catalog exception.

That allowlist includes the original sandstone rock, stone/mortar wall,
plaster/brick facades, concrete wall/panel/pillar/cap textures, rough timber,
roof tiles and painted signposts. It does not classify new object atlases.
Landscape ownership separately qualifies dry opaque ground, including the
original Malden `abel` and Everon `eden` grass/soil transition tiles. The receipt
lists the actual decoded terrain identities encountered in these runs.

In enhanced mode, native and captured CPU-lit v3 sections use the same bounded
stock diffuse proxy and incident-light convention as the ray path. Classic
`triPBR 'on'` retains its original BRDF/material coefficients and coverage.
The original GGX, local-light selection/cones/shadows, emissive, fog and night-eye
paths remain. No already-lit vertex color becomes albedo.

This source adoption **does not broaden GI receivers**. Only the existing
catalog-qualified receivers reserve independently captured global ambient.
Other qualified stock surfaces can reflect light while retaining their ordinary
raster/AO composition. Optional albedo/normal/roughness overlays remain independent
and keep their model/LOD/pixel/geometry compatibility checks.

## Terrain reconstruction

The RT mesh uses the original terrain-grid heights and the same 01--10 cell
diagonal as `Landscape::SurfaceY`. Its shading normals reproduce the original
raster LOD0 height stencil, outward-facing, rather than using `(0,1,0)` on slopes.
Actual triangle normals remain separate for offsets and geometric hemisphere
checks. The existing ray normal reconstruction validates/interpolates the stencil.

Per-cell vertices preserve clamped texture seams. UVs use the same parent land
cell, four original random UV offsets, bilinear interpolation, physical UV
conversion and wrap/clamp policy as `LandscapeRender`. An edge can therefore
have U=1 in one clamped tile and U=0 in its neighbor. Original decoded pixels
use the existing sRGB bilinear hit sampler; no texture is generated or replaced.

Only landscape-owned opaque, non-water terrain whose corners exceed the raster
renderer's maximum tide/wave bound is qualified. Potential shoreline/submerged
cells remain geometry/visibility contributors with legacy material fallback.
This is conservative near coasts. Height stencils (including the normal halo),
texture IDs, landscape/grid identity and hit-data/GI mode invalidate snapshots.
Animated sea level is deliberately **not** a cache key: waves must not rebuild
the terrain BLAS every frame.

## Transport and diagnostics

Original texture RGB remains a bounded, **unmeasured** appearance approximation.
Raster enhanced PBR and hits share `materialDiffuse`: preserve chroma, never
brighten baked dark pixels, cap the largest linear component at .5, then apply
dielectric/metal diffuse exclusion. Terrain does not infer albedo from its lit
vertex colors. Its texture proxy also does not reconstruct the original terrain
palette tint or baked lighting; color bleeding is approximate.

Primary diffuse sample count, cosine/PDF, one bounce, range, gain, temporal
filtering and ambient-replacement composition are unchanged. Newly qualified
hits execute the existing real, alpha-tested secondary sun query and cloud
visibility. More sun queries now run because more hits have usable materials,
not because rays per sample or bounces were increased. Unknown hits retain their
legacy ambient share. Known shadowed hits do not invent sunlight.

Status now separates terrain, ordinary stock PBR and catalog eligible/sunlit
hits, backfacing endpoints and missing color. `sunVisible` counts geometrically
unblocked sun queries; per-source `sunlit` additionally requires nonzero cloud
visibility. The counter descriptor advertises the whole enlarged buffer; the
existing counter prefix and history fields retain their offsets. RT-water
reflection shading retains its original contract.

## Gameplay evidence

The original Malden stone courtyard and Everon concrete compound were compared
in original missions, using the established muted direct harness, copied
profiles, 1400 m visibility and ordinary brightness. These are real mission
renders, not Steam/mission-completion qualification. Legacy, PBR v3, enhanced
raster, enhanced sky-only and enhanced GI captures use matched standing cameras.
No demonstration geometry, exposure adjustment or amplified difference image
is needed to inspect them.

Selected unamplified images, per-frame source counters, nine-sample-per-mode
bounce measurements, original-source hashes, runtime hashes and profiler samples
are in [the qualification receipt](evidence/rtgi-stock-coverage/qualification.json).
The initial catalog milestone's 1.42% result is a baseline, not evidence for this
implementation. The old authored-wall 3.86% result is unrelated.

Morning 1080p completed-frame counters, baseline executable versus this build:

| Scene | Hits | Eligible hits | Rejected materials | Sun queries | Blocked | Sun-visible |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Malden before | 155,362 | 42,396 | 112,966 | 40,779 | 7,501 | 33,278 |
| Malden after | 155,674 | 108,586 | 47,088 | 106,801 | 42,478 | 64,323 |
| Everon before | 53,555 | 2,557 | 50,998 | 348 | 303 | 45 |
| Everon after | 53,049 | 53,015 | 34 | 50,853 | 21,070 | 29,783 |

These are stochastic snapshots, not phase-locked counts. Primary ray counts and
receiver coverage remain effectively unchanged (Malden about 215,000, Everon
113,400 receivers). The new eligible endpoints are actual intersections, not
screen-wide illumination. Their breakdown is:

| Morning source | Malden eligible / sunlit | Everon eligible / sunlit |
| --- | ---: | ---: |
| Original terrain | 65,673 / 30,754 | 50,530 / 29,741 |
| Additional stock PBR v3 | 610 / 470 | 0 / 0 |
| Existing catalog | 42,303 / 33,099 | 2,485 / 42 |

Everon's morning concrete is still genuinely poorly illuminated: only 42
catalog endpoints pass sunlight visibility, close to the old 45. The major new
source is the visible sunlit ground beside the shaded wall. At noon, terrain
has 50,374 sunlit endpoints and the catalog 950; no occlusion was bypassed.
Malden also exercises non-catalog v3 sources, not just the existing wall entries.

Nine captures per sky-only/on mode measure **5.07%** added linear luminance on
Malden's shaded wall and **2.78%** on Everon's panel. These signals exceed the
observed sample ranges (Malden sky .05632--.05645 versus on .05919--.05931;
Everon sky .026515--.026569 versus on .027244--.027317). Warm RGB deltas and
Everon's upper/lower panel gradients are recorded separately in the receipt.
An unsupported distant Malden building control changes only .22% between modes.

Matched, unamplified 1080p morning views:

| Scene | Legacy | PBR v3 | Enhanced, GI Off | Sky-only | RTGI On |
| --- | --- | --- | --- | --- | --- |
| Malden | [image](evidence/rtgi-stock-coverage/malden-legacy.png) | [image](evidence/rtgi-stock-coverage/malden-v3.png) | [image](evidence/rtgi-stock-coverage/malden-enhanced.png) | [image](evidence/rtgi-stock-coverage/malden-sky.png) | [image](evidence/rtgi-stock-coverage/malden-gi.png) |
| Everon | [image](evidence/rtgi-stock-coverage/everon-legacy.png) | [image](evidence/rtgi-stock-coverage/everon-v3.png) | [image](evidence/rtgi-stock-coverage/everon-enhanced.png) | [image](evidence/rtgi-stock-coverage/everon-sky.png) | [image](evidence/rtgi-stock-coverage/everon-gi.png) |

The visible change includes darker, more spatially varying shade, with local
warm surface fill. Total GI is not a brightness increase: replacing the old
ambient assumption with visible/occluded ground transport darkens parts of these
walls. Surface-bounce isolation and net GI-Off comparisons are reported separately.
Texture patterns, camouflage, cracks, stains and UV orientation remain recognizable.
The bounce itself is still modest, especially on Everon; improved coverage is
not a claim of complete or production-ready GI. Those measured wall regions are
46.34% and 45.07% darker than enhanced raster GI-Off, respectively. Incomplete
sky/multiple-bounce response is an important remaining limitation, not something
to conceal with a gain increase.

Running-mission camera translations/yaw exercise TAA and FSR Quality at 1440p;
stationary tests use both 1080p and 1440p. During the final live sweeps, BLAS build
counts remain unchanged within each AA mode (Malden 207, Everon 252), verifying
that waves/camera motion do not cause terrain reconstruction churn. Existing
foliage aliasing, low-resolution stock texture blur and some estimator noise
remain. These are render-camera tests, not player input responsiveness tests.

On the RTX 4060 Ti, existing GPU-profiler timestamps give the following short
stationary TAA measurements. Each value averages the two interval medians in
an Off/On/On/Off sequence, with three seconds settling and four seconds sampling.
Cloud and water effects are disabled for these isolated timing intervals.

| Scene / world resolution | GI-Off frame ms | GI-On frame ms | Difference ms |
| --- | ---: | ---: | ---: |
| Malden 1920x1080 | 5.060 | 5.728 | +0.669 |
| Malden 2560x1440 | 5.221 | 5.722 | +0.501 |
| Everon 1920x1080 | 3.584 | 4.083 | +0.499 |
| Everon 2560x1440 | 3.645 | 4.200 | +0.555 |

Clock/cache variation makes these short whole-frame differences non-monotonic;
they are not a scaling guarantee. The receipt retains interval samples and
separate trace/filter/temporal scopes, with confirmed world dimensions.

The four final Vulkan runs presented **69,046 frames with zero validation
errors or warnings**, including the live AA sweeps and Malden combination smoke.
Live coastal waves preserve the BLAS build count at 374 while terrain reflection
hits remain shaded and refraction reports no fallback. RT sunlight, RTAO/SSAO,
clouds, weather/fog, night-eye, MSAA4 and restored PBR v3 were exercised; local
headlights were additionally checked before the final shoreline/cache-key fix.
GL33's unsupported developer PBR commands produce zero changed pixels in the
stationary control region. The four original PBO/world hashes still match.

Focused tests cover the preserved allowlist, shared identities, clamped UV seams,
original terrain normal stencil, map independence, material compatibility and
native/CPU normal/lighting contracts. No new testing framework was added.
Vulkan: 44 cases / 1,665 assertions; GL33: 43 cases / 1,625 assertions; driver-free
policy/lifetime guards: 684 checks. Both public builds complete. Existing
compiler warnings remain; the zero-warning statement above concerns Vulkan
runtime validation, not compiler diagnostics.

Remaining physical limits include artistic/baked texture shading, assumed optical
properties, missing terrain palette modulation, base-mip secondary sampling,
partial object coverage, finite visibility range, no indirect specular, no second
bounce, and an ambient boundary rather than a measured sky. In particular, a
known hit reflects direct sunlight here; its own skylight/multiple-bounce response
is incomplete. Substantial differences from legacy ambient are not radiometric
ground truth. Broader accurate GI still requires further work.
