# Volumetric sunlight: terrain/road shadow correction

Public main baseline: `7dfc1f9`. The original
[Malden reproduction](../graphics-quality/README.md#reproduction-recipe) is used
with stock `01TakeTheCar.ABEL`; Everon uses `05HeavyMetal.Eden`. Volumetrics
remain developer-only, default Off. No Video Settings changes are included.

## Diagnosis

The repeated dark bands are **terrain CSM self-shadowing**, exposed when active
volumetrics add a landscape caster to the shared sunlight maps. They are not
darkening from the additive half-resolution scattering composite.

Controlled checks at the documented road camera `[7116,78,6100,0,-.12,1]`:

1. Volumetrics Off removes the bands. On reproduces them with `scattering=on`,
   960x540 integration, 48 steps, sunlight 0.526876 and CSM reach 388.889 m.
2. Leave scattering On but set `triShadowSetDarkness 1`: the bands disappear.
   Restore .35 and they return. This isolates the surface CSM comparison, not
   ray marching, depth reconstruction or bilateral upsampling.
3. The reproduction's actual landscape grid is 12.5 m, already matching the old
   caster grid. Exact sampling alone therefore cannot fix this case. Ten times
   the normal receiver bias still leaves bands; 100 times clears them but is
   unsuitable for ordinary object shadow contact. Cascade settings remain fixed.
4. A terrain-only two-sided slope-bias pipeline, first at factor 1, still bands;
   factor 2 clears this view with the original receiver bias. A 3x3 nearest PCF
   tap can be 1.5 texels away in each axis. The general bound is
   `1.5*(abs(dz/dx)+abs(dz/dy)) <= 3*max(abs(dz/dx),abs(dz/dy))`, so the final
   terrain pipeline uses slope factor 3, plus one depth-resolution unit.
5. With sufficient slope bias but the old 8 m grid, 3.125 m terrain still has
   false dark patches across the road and hillside. Sampling the actual grid
   with the landscape's 01--10 diagonal clears them. The old resampling can put
   a caster above the visible surface; bias alone cannot correct that geometry.

These independent geometry/bias checks explain both the standard-grid bands
and the additional fine-detail errors. Unchanged cascade selection/blending,
marching, depth comparisons and reconstruction work with corrected casters;
none needs an algorithm rewrite. Temporary diagnostic variants are not shipped.
[diagnosis.json](diagnosis.json) retains their executable identities, commands,
actual landscape-grid readouts and hashes of the local diagnostic captures.

## Correction

`EngineVK_Shadow.cpp` uses `GetTerrainGrid`/`ClippedData` from the existing
landscape, with the same cell diagonal as `SurfaceY` and rendered terrain.
The optional, bounded patch is cached at 25 m anchors (or one coarser terrain
cell) with enough margin to preserve its original radius between anchors. Live detail/island changes still
invalidate the mesh. Shared indexed vertices replace duplicated triangle lists
and reverse faces, reducing uploads and vertex transformations. The same
immutable mesh is retained through existing frame fences and reused in all
cascades; there is no additional shadow-map pass or per-frame terrain upload.

`VulkanContext_Shadow.cpp` selects one additional terrain-only, no-cull pipeline
with the PCF-footprint slope bias. Existing solid/cutout sunlight and local-light
pipelines keep their original culling and bias. Hills still occlude sunlight;
building and vegetation casters retain their existing geometry and filtering.
Scattering energy, atmosphere, ambient light, PCF size, cascades, fog/visibility,
TAA/FSR and all shaders remain unchanged. GL33 has no source changes.

## Reproduction and measurement

Use `build/local-build.ps1 -Project Vulkan -Action Build`, public FSR enabled,
an isolated copied profile, `CWR_VK_PROFILE=1`, `CWR_VK_FSR=1`, no RT opt-in,
`--nosound`, stock test mission and the existing developer harness. Freeze with
`setAccTime 0`, use 1400 m view distance, SDR, AA Off, 100% render scale, VSync
Off. Set every optional feature explicitly: only the four existing automatic
effects and ordinary CSM are On; toggle `triVolumetricFog` alone. CSM stays at
four 2048 cascades, bias .00002, darkness .35, fade 40 and the original reach.
The exact commands and requested/active states accompany the raw samples.

Road: noon, weather .45, fog/rain 0, camera above. Forest: 06:00, weather 0,
fog .6, camera `[6800,79,5500,1,.05,0]`. Everon: noon, weather .45, fog/rain 0,
camera `[9448,28,1580,-1,.02,0]`. Do not move the mission actor for fixed views.
Fine-detail diagnostics explicitly use `setTerrainGrid 3.125`; the existing
`triGetTerrainGrid` query reports the preferred profile grid (12.5), not that
live script override. Submitted geometry and the action log identify the trial.

Each timing state settles 3 s, then samples 6.2 s of existing two-second GPU/CPU
profiler windows, in Off/On/On/Off order. Captures occur afterward. No build runs
during sampling. GPU clocks stay automatic and are logged; negative whole-frame
deltas are not claimed as optimizations. CPU frame intervals include pacing;
CPU recording is separate. The 2560x1440 drawable is verified, on a 1080p desktop.

Screenshots are unretouched full-window captures. Off/On views within a frozen
session are matched. Before/after executable comparisons match camera, date,
weather requests, visibility and rendering settings, but are separate launches:
mission HUD/player distance, small weather evolution and foliage LOD can differ.
Those differences are not credited to the correction. No amplified difference
images are used as photographs.

## Unretouched before/after

All pairs have volumetrics **On and scattering active**, including the fine-grid
pair. The normal-grid road pair removes the repeated self-shadow pattern; the
fine-grid pair also removes the false broad patch near the middle of the road.
Everon's hillside loses the same fine artificial striping while the building's
attached ground shadow remains. Forest shafts and their foliage occlusion remain
visible at the original intensity. The launch-matching limitations above apply.

| Stock view | Baseline main | Corrected |
|---|---|---|
| Malden road, 12.5 m grid | [Before](road-before.png) | [After](road-after.png) |
| Malden road, 3.125 m grid | [Before](road-fine-before.png) | [After](road-fine-after.png) |
| Malden dawn forest | [Before](forest-before.png) | [After](forest-after.png) |
| Everon hillside/building | [Before](everon-before.png) | [After](everon-after.png) |

[screenshots.json](screenshots.json) records source run, filename, executable hash
and SHA-256. Each published PNG is byte-identical to its original window capture.
Additional Off/On, diagnostic and validation captures remain locally in the named
`build/shadow-live/volume-banding-*` runs.

## Measured cost

RTX 4060 Ti 8 GB, driver 617.14, i7-12700F, Windows, RelWithDebInfo with public
FSR. Values are pooled medians of the completed profiler windows, milliseconds,
**baseline -> corrected**, with volumetric scattering active. CSM includes the
existing object casters plus terrain, not just terrain. Timings exclude validation.

| View / output | Volumetric GPU pass | CSM depth GPU | Whole GPU frame |
|---|---|---|---|
| Malden road / 1080p | 1.026 -> 1.053 | .328 -> .274 | 2.209 -> 2.188 |
| Malden road / 1440p | 1.559 -> 1.551 | .291 -> .229 | 3.076 -> 2.983 |
| Malden forest / 1080p | .773 -> .781 | .486 -> .420 | 2.295 -> 2.241 |
| Malden forest / 1440p | 1.388 -> 1.390 | .485 -> .418 | 3.632 -> 3.566 |
| Everon hillside / 1080p | .902 -> .932 | .469 -> .413 | 2.211 -> 2.187 |
| Malden road, 3.125 m grid / 1080p | .865 -> .864 | .427 -> .593 | 3.471 -> 3.640 |

The volumetric shader cost is essentially unchanged. Normal-grid CSM work is
lower after removing duplicated faces and indexing shared vertices. The finest
terrain setting needs more accurate caster triangles: its additional measured
cost is about **.17 ms GPU / 4.9% whole-frame**, largely in CSM. This is accepted
to remove the fine-grid geometry error. A preliminary unindexed accurate patch
cost about 1.08 ms in CSM in this view; indexing reduced that to .59 ms before
qualification. That preliminary binary is not a final timing source.

CPU command recording at the fine grid is 3.337 -> 3.314 ms; normal-grid 1080p
road is 2.784 -> 2.776 ms. Paced CPU frame intervals remain about 6.17 ms (the
corrected 1440p road is 6.20 ms). All sampled steady states have zero new streaming
allocations/frame. The normal-grid terrain upload is 7,225 shared vertices plus
42,336 indices, replacing 76,800 duplicated vertices. The fine-grid upload is
109,561 vertices plus 653,400 indices, cached across the 25 m anchor interval.
The profiler's `vertices/frame` includes submitted terrain index references,
not unique vertex-shader invocations. These are short fixed-view measurements,
not traversal-tail, sustained-battle or other-hardware performance guarantees.

[measurements.json](measurements.json) contains raw profiler windows, active
status and GPU clock/power snapshots; [summary.json](summary.json) contains
median/min/max/count. The final sources are `volume-banding-indexed-1080`,
`volume-banding-indexed-1440` and `volume-banding-fixed-everon`. Baselines are
`volume-banding-baseline-repeat-1080`, `volume-banding-baseline-1440` and
`volume-banding-baseline-everon`. The first exploratory 1080p forest sweep was
interrupted and repeated; none of its partial samples is used here.

## Focused verification

- Final Vulkan + public FSR and GL33-only builds pass using `build/local-build.ps1`.
  `[vulkan-shape],[vulkan-aa],[ShadowMath]`: Vulkan **1,601 assertions / 71 cases**;
  GL33 **1,561 assertions / 70 cases**. Driver-free Vulkan policy/lifetime suite:
  **517 checks**. Its intentional synthetic teardown/resource failures are not
  live renderer failures.
- Validation-enabled stock Malden gameplay: **20,323 submitted / presented
  frames**, normal shutdown, **zero core/synchronization validation errors or
  warnings**. The startup receipt confirms both validation modes On. The loader
  separately reports an existing missing Epic overlay manifest and explicitly
  disabled implicit layers; these startup diagnostics are retained in the receipt
  and are not hidden inside the renderer's zero counters.
- Exact-window one-second forward input produces measured player displacement
  with both native TAA and FSR Quality. Thirty 1.2 m camera steps in each mode
  cross a terrain anchor at the 3.125 m grid. Inspected captures show no renewed
  road bands, obvious trails or shadow pops. FSR Quality really dispatches at
  1280x720 -> 1920x1080; the half-resolution volume is 640x360 in that mode.
  These are bounded input/sequence checks, not a long temporal-quality campaign.
- The dawn forest is also checked with soft CSM/contact shadows active. Clear
  dawn -> noon -> rainy/foggy overcast -> sunset -> night -> dawn transitions
  preserve the source gates: scattering On in clear daylight, Off with zero
  sunlight in the storm and after sunset, then On again at dawn. Recorded rain
  wetness is allowed to evolve normally. An additional 18:30 low-sun Off/On pair
  preserves the long object shadows across the road; those legitimate shadows
  are distinct from the removed fine self-shadow pattern.
- All fixed comparisons keep the actor in place. Movement uses an isolated
  copied profile, `--nosound`, bounded input and restored Caps Lock. No original
  assets, gameplay, user profile, private DLSS/NGX work, shader tuning or menu is
  changed. The retained public Vulkan runtime is the qualified executable.

[runtime-receipts.json](runtime-receipts.json) includes commands, per-trial active
states, input displacement, transition checks, startup and shutdown receipts.
These direct development launches are not Steam-route or mission-completion
tests. The baseline executable SHA-256 is
`442D285339F79252610B1F732F79A49D7FB373EE5D8770649093872754AEEA1F`;
the final measured/validated executable is
`431A152705C81084445F4F19C4941625C679F9F1EE06F4436AB0BA4D49B39A5E`.
