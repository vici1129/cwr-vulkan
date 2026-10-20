# RTGI GPU performance follow-up

Base: public `b61d327236b2c9639bd8a458ca60b3d19cc42955`. PBR/RTGI remain
developer-only and default Off. This changes existing shader work, not the
renderer, pass scheduling, material coverage or lighting model.

## Bottleneck and retained changes

The fresh Everon 1440p baseline spends about **0.848 ms** in GI tracing/hit
shading, **0.351 ms** in filter/composition, and **0.239 ms** in temporal/spatial
reconstruction. World raster also rises from **1.209 to 1.576 ms** with GI.
Tracing includes sky evaluation and diagnostics, not just hardware traversal.
The temporal scope includes the existing 5x5 spatial neighborhood. These scopes
are not an exact additive budget.

Four small optimizations were retained after measuring their incremental effects:

- Receiver capture reuses the already-sampled/converted stock terrain detail
  factor for sunlight and ambient. It selects the final enhanced ambient
  reference before the nonlinear night-eye conversions, rather than calculating
  an intermediate difference and overwriting it. Ineligible ambient fragments
  skip calculations whose result was unused. Existing zero attachment writes
  still invalidate unsupported foreground coverage. The isolated capture change
  reduced Everon's GI-on world raster scope by about **0.10 ms**. Some savings
  also apply with GI Off, so this is not all an incremental GI-overhead saving.
- The six dispatch-constant authored light colors are decoded once per 8x8
  workgroup using the existing `legacyLightToLinear` function and initialization
  barrier. Direction-dependent sky texture coordinates, lobe, combined nonlinear
  sky tint, clouds and calibration remain unchanged. This saved about **0.04 ms**
  in Everon tracing; it had little additional effect in Malden.
- Trace diagnostics omit global atomic adds of zero. Counts and categories are
  retained, not sampled or disabled. This saved about **0.05 ms** in Malden
  tracing; Everon's isolated difference was too small to claim a gain.
- The 8x8 reconstruction group shares its existing 12x12 input footprint,
  including the same clamped two-pixel border. Original float radiance, decoded
  positions and normals are reused across outputs. Every output still processes
  the **same 25 taps in the same order**, with unchanged weights, rejection,
  moments, variance clipping, dynamic rejection and history accumulation. This
  is transient workgroup scratch, not a persistent cache, new image or history
  architecture. At 1440p it saved about **0.05 ms** in both scenes' temporal scope.

The same attachment formats, full receiver records, ray count, half-resolution
GI, 25 m transport, 45 m receivers and 80 m visibility remain. Reflectance,
palette, stock textures/UVs, normals, directional skylight, sun bounce,
one-bounce transport and ambient replacement are unchanged. No precision,
filter-quality or coverage reduction was used.

An additional GI-only out-of-range ambient guard was tested, including explicit
preservation of ordinary RTAO's range. It did **not** give useful world-raster or
total-GPU savings in either scene. The guard and its flag plumbing were removed.
Its measurements are retained alongside the accepted experiments.

## Matched measurements

RTX 4060 Ti, driver 617.14, original Malden/Everon missions and the existing
ground cameras, TAA at 100% world resolution, 1400 m view distance. Clouds volume
and water optics are off for these ground timings; stock sky/cloud shading and
RT sunlight remain on. This is the same focused setup as the preceding terrain
qualification, not a reduced-GI configuration.

Each run warms GI for 10 seconds, then measures Off/On/On/Off with 4 seconds of
settling and 6 seconds of sampling per interval. Values below average the two
interval medians. There were no concurrent builds, unit tests or image analysis
during timing. Sparse probes were off. The original executable was retained and
rechecked at 1440p before final qualification; those rechecks reproduce the
initial baseline. Raw samples and runtime hashes are in
[qualification.json](evidence/rtgi-gpu-performance/qualification.json) and its
adjacent GPU receipts.

All values are milliseconds. “GI cost” is the same-run GI-on minus GI-off frame
timestamp, not the sum of the individual pass scopes.

| Scene | GI-off before → after | GI-on before → after | Incremental GI cost before → after |
| --- | ---: | ---: | ---: |
| Everon 1080p | 2.166 → 2.126 | 2.919 → 2.783 | +0.753 → +0.657 |
| Everon 1440p | 3.104 → 2.959 | 4.950 → 4.763 | +1.847 → +1.804 |
| Malden 1080p | 4.287 → 4.258 | 4.573 → 4.527 | +0.286 → +0.269 |
| Malden 1440p | 4.478 → 4.414 | 5.993 → 5.726 | +1.515 → +1.312 |

The Everon 1440p GI-on frame improves by **0.188 ms (3.8%)**, while its
incremental GI overhead improves by only **0.043 ms (2.3%)**. Both numbers matter:
the texture reuse also makes GI Off faster. This uses the fresh +1.847 ms
baseline, not the earlier +1.885 ms measurement from a different run.

| 1440p GI-on scope | Everon before → after | Malden before → after |
| --- | ---: | ---: |
| Trace/hit shading | 0.848 → 0.807 | 0.727 → 0.674 |
| Filter/composition | 0.351 → 0.352 | 0.314 → 0.316 |
| Temporal/spatial reconstruction | 0.239 → 0.185 | 0.218 → 0.171 |
| World raster | 1.576 → 1.481 | 2.477 → 2.313 |

Filtering/composition did not improve. At 1080p, Everon's trace/temporal scopes
change from 0.512/0.134 to 0.479/0.104 ms; Malden's from 0.572/0.164 to
0.566/0.130 ms. The JSON retains all scopes, including CPU command, geometry,
RT-scene preparation, capture/draw and binding counters.

Malden remains CPU-submission-bound: at 1440p its GI-on command time is about
10.69 → 10.57 ms, geometry upload about 0.036 → 0.035 ms, and RT-scene preparation
about 0.183 → 0.187 ms. Lit draws stay **1028**. Everon's GI-on draw count stays
**227**, command time about 2.85 → 2.87 ms, and geometry upload about 0.007 ms.
There is no claimed CPU optimization or equivalent gameplay-FPS gain here.
CPU supply and dynamic GPU clocks affect whole-frame timestamps, particularly
Malden/1080p; small incremental differences are not universal speedups.

## Lighting and coverage qualification

There is no intended lighting change. Linearized SDR screenshot ROI means in
the final GI-on TAA comparisons change by **-0.145% to +0.168%** across the
checked ground, wall and barn regions. Everon's shaded ground changes -0.031%,
its barn facade +0.020%; Malden's shaded ground -0.085% and stone wall -0.100%.
These are output-space observations with temporal sampling and slightly
different cloud inputs, not a claimed radiometric change or an added GI gain.
The stock dome/reference and calibration agree; weather quadrature is recorded.
The non-temporal GI-on ROI differences are similarly small, up to 0.164%.

All six non-temporal receiver counts **and family/native/CPU breakdowns match
exactly** between original and final executables: Everon yard/wall/barn
664894/340832/265164; Malden yard/wall/stone 603475/446466/385617. These are the
existing eligible half-resolution trace-sample diagnostics, not a new
full-resolution coverage measurement. Roof views retain about 96,800 Everon and
154,900 Malden roof samples; their small count differences reflect different
TAA jitter phases. The raster material/coverage rules have not changed.

The native/CPU toggle check finds **53 matching terrain hit paths**. Median
CPU/native reflectance ratios are `[1.00392140, 1.00392164, 1.00392157]`, agreeing
with the original `256/255` palette multiplier. Original palette/detail and the
bounded reflectance model remain, rather than inferring albedo from lit color.

GI-off PBR v3 controls agree within 0.127% in the selected ROI means; enhanced
controls within 0.193%. An initial Malden yard control differed by about 2% but
used a different preceding view sequence. Repeating the **unchanged original
executable** through the final yard/wall/stone/roof sequence reproduces the final
yard result within 0.127%. Both comparisons are retained; the initial difference
is not attributed to a shader lighting change or an optimization benefit.

The existing focused helpers also checked:

- TAA/FSR camera movement on both islands, plus bounded physical strafe and mouse
  input in Malden. The player moved about 1.16 m / 1.15 m, with keyboard state
  restored. These are real mission checks, not a claim of mission completion.
- Ground-distance/LOD sweeps through 0/12/28/42/65/100 m and back, roof receivers,
  native/CPU terrain and return to native.
- Direct/local lighting, two active headlight shadow maps, GI with RTAO, MSAA4,
  fog/rain, night-eye fallback, and restoration to PBR v3.
- Live coastal waves with textured reflection hits and refraction active without
  fallback. BLAS builds remain **786 → 786** over that live interval. Water and
  uncertain coastal triangles keep their existing paths.
- A rebuilt GL33 runtime: Vulkan-only PBR controls remain unsupported, and its
  checked world ROI has **zero changed pixels** across the toggles.

The final four Vulkan runs presented **54,681 frames**, with synchronization
validation enabled and **zero errors or warnings**. Baseline, experiment and
targeted control validation receipts are also retained. Existing focused tests
pass: Vulkan **64 cases / 49,161 assertions**, GL33 **63 / 49,121**, and
**684** driver-free Vulkan policy/lifetime checks. The added checks stay in the
existing shader tests and cover lighting-reference equivalence, shared-tile
border indexing, barrier ordering and unchanged directional decoding.

Source asset hashes (`Data.pbo`, `Data3D.pbo`, `abel.wrp`, `eden.wrp`), 51 player
configuration/profile files and all eight source fixture-profile files are
unchanged. Test profiles were isolated copies. Private DLSS/NGX work and SDKs
were not modified. No original artwork or persistent player setting was edited.

## Remaining cost and limits

The result is a modest GPU improvement, not a large RTGI redesign. Everon 1440p
still costs about **+1.80 ms** in this setup. Tracing/sky/hit shading is still the
largest individual GI scope, and full-resolution receiver traffic plus
composition remain substantial. Unsupported foreground invalidation writes,
formats, filters, visibility and coverage were deliberately retained. These
short warmed mission measurements do not establish performance on every scene,
GPU or driver, and do not remove Malden's separate CPU draw bottleneck.
