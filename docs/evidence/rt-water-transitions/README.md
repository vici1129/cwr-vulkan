# RT-water transition evidence

See the [root-cause and correction report](../../rt-water-transitions.md).
Baseline is public main `aadbc5e`; the final candidate uses the same public
checkout and stock GOG 3.05 data. Private DLSS/NGX, original assets and player
profiles are outside this change. All test profiles are isolated copies.

## Unretouched captures

Every PNG is an original complete game-window capture, copied byte-for-byte.
There is no crop, contrast change, retouch or generated artwork. Source run,
executable SHA-256 and image SHA-256 are in [screenshots.json](screenshots.json).

| View | Current baseline RT On | Corrected RT On | Corrected RT Off |
|---|---|---|---|
| Malden shoreline/pier, noon | [Before](malden-before.png) | [After](malden-after.png) | [Original](malden-rt-off.png) |
| Everon coast, noon | [Before](everon-before.png) | [After](everon-after.png) | [Original](everon-rt-off.png) |
| Malden verified low angle, noon | [Before](malden-low-before.png) | [After](malden-low-after.png) | Local raw run |

[Malden sunset](malden-sunset.png) and [Everon overcast](everon-overcast.png)
show the retained weather response. The dark, angular reflected terrain
silhouette at sunset is real bounded terrain geometry with approximate shading;
it is not evidence that every coastline now looks like a smooth mirror.

The two clearly labeled **diagnostic shader outputs** are actual game captures,
not ordinary shading or accepted visual comparisons:

- [Old normal](diagnostic-normal.png): the same diagonal normal throughout the
  sea despite stock detail variation. The stock-backed test independently proves
  white intensity RGB and variable alpha in `specular_dx.paa`.
- [Rejection reason/ranges](diagnostic-rejection.png): yellow is the early
  back-face rejection; magenta is non-water, cyan fog rejection, gray receiver
  beyond 80 m. Traced pixels encode receiver distance in red and hit ray length
  in green; blue marks hits beyond 69 m. The coastal strip is yellow, not gray.
  These probes are removed from the final executable.

## Matched state and timings

Both versions restore the same binary stock-world checkpoint per island, freeze
simulation, then explicitly set camera, noon, overcast .45, fog/rain 0 and
the feature bundle. Checkpoint SHA-256 is in [runtime receipts](runtime-receipts.json).
The main cameras and original launcher/profiler method are documented in the
[previous reproduction recipe](../graphics-quality/README.md#reproduction-recipe).

The added low view uses `[3500,4,3240,1,-.025,0]` on Malden and
`[5100,4,3750,0,-.025,1]` on Everon. Both report underwater depth zero.
The earlier `low-day`/`validate-*-low` labels at **1.5 m** are underwater checks,
not above-water low-angle qualification; the `rt-water-low-*` runs supersede them.

Complete sampled profiler logs/status and per-window values are in
[measurements.json](measurements.json); medians/min/max/count and automatic
GPU clocks/power are in [summary.json](summary.json). Source executable hashes:

- Baseline: `431A152705C81084445F4F19C4941625C679F9F1EE06F4436AB0BA4D49B39A5E`.
- Final: `61524864683FC5B57FE8A8F5CC0AFB6E6445A8FAC47EEED6EACFCD8FA2C4EDBF`.

All final measurements use that same final executable. Exploratory alpha-slope
and diagnostic builds are excluded. RT water is active with Enhanced water and
SSAO/Enhanced raster shadows throughout; volume clouds/shadows use the existing
weather. The cloud volume alone is switched On/Off/Off/On. RT sunlight stays Off,
so the shared dynamic scene is also exercised independently of sunlight.

Additional pass/CPU medians for **Enhanced clouds**, baseline / corrected:

| Coast / output | Water trace/shade GPU ms | Filter/blend GPU ms | Whole CPU command recording ms | CPU frame interval ms |
|---|---:|---:|---:|---:|
| Malden / 1080p | 1.147 / 1.515 | .078 / .078 | 4.833 / 4.955 | 6.463 / 6.659 |
| Everon / 1080p | 1.349 / 1.701 | .077 / .077 | 3.096 / 3.093 | 6.173 / 6.174 |
| Malden / 1440p | 2.000 / 2.642 | .200 / .210 | 4.908 / 4.898 | 6.571 / 6.627 |
| Everon / 1440p | 2.346 / 2.941 | .198 / .213 | 3.119 / 3.224 | 6.396 / 6.889 |

GPU timestamps describe submitted work; CPU frame intervals include presentation
and pacing. Automatic graphics clocks were generally 2835–2850 MHz, memory
9201 MHz; no clock or power-plan change. Do not add isolated pass costs to
predict a combined workload or interpret a tiny negative delta as a speedup.
Warm frozen-world samples exclude movement, animated dynamic BLAS rebuilding
and cold resources. The root report records the whole-frame GPU deltas.

## Live qualification and limits

[States](states.json), [physical-input receipts](inputs.json) and
[runtime receipts](runtime-receipts.json) distinguish requested/active state,
actual AA dimensions, shared dynamic geometry and normal shutdown. Validation
startup explicitly enables both Khronos core and synchronization validation.

| Final validation run | Submitted / presented | Errors / warnings |
|---|---:|---:|
| `rt-water-validate-malden` | 7790 / 7790 | 0 / 0 |
| `rt-water-validate-everon` | 8011 / 8011 | 0 / 0 |
| `rt-water-low-final-malden` | 5900 / 5900 | 0 / 0 |
| `rt-water-low-final-everon` | 2262 / 2262 | 0 / 0 |
| `rt-water-walk-malden` | 2592 / 2592 | 0 / 0 |
| `rt-water-no-rt` | 659 / 659 | 0 / 0 |

Coverage includes shallow pier/beach and deep miss-heavy water, high/verified
low views, day/sunset/overcast, TAA/FSR Quality/MSAA4, animated waves and 24-step
moving-camera sweeps for TAA and FSR. Refraction, foam, sky, dynamic scene and
RT water are independently toggled. Unsupported/unavailable hit material keeps
the same sky fallback by construction; excluded geometry remains excluded.
The no-opt-in process rejects RT water, retains Enhanced raster water and records
no water rays. [GL33's receipt](gl33-receipt.json) confirms a stock Malden launch,
the Vulkan-required rejection and normal exit 0.

Grounded player checks relocate the protected actor to `[3575,3240]` on Malden
and `[5100,3860]` on Everon, verify zero ground clearance, center the ordinary
player camera through the saved LookCenter key, then send bounded movement
inputs with TAA and FSR. Position changes and clearance are recorded. Simulation
is frozen between input blocks to avoid the stock out-of-area mission timeout.
Earlier water placements are not counted as grounded walks. The exploratory
Malden low-view process later reaches that stock timeout after simulation is
left running; its exit is normal, not a renderer crash or mission completion.

The conspicuous rejected-water strip is absent in the corrected coast and
above-water low views and sampled camera traversal. No new persistent reflection
trail or broad coverage popping was observed. Fine thin-edge shimmer can remain
on reactive reflected geometry; short sweeps/screenshots cannot establish
flicker-free multi-hour gameplay. Terrain triangle/UV/lighting approximations
remain visible, especially in dark sunset reflections. Distant geometry fades
to weather sky and original water; arbitrary mods, other GPUs and physical HDR
are not qualified. Keep the feature developer-only.

[RT-Off comparisons](off-comparisons.json) record a pixel-identical water region
`(10,700)-(1900,1060)` in all twelve original frozen Off comparisons. Whole-window
sky/HUD differences remain small but nonzero, so full-window identity is not
claimed. Numeric inspection does not alter the published images.

Focused stock-backed suites pass [2069 assertions / 84 cases](vulkan-local-tests.txt)
and [1823 / 80](gl33-local-tests.txt). Driver-free Vulkan policy checks pass 517;
their intentional mock teardown error is not a live validation finding. Six
water/MRT SPIR-V variants pass SDK validation. Both public builds pass. No new
menu, SSR, denoiser, temporal framework, AS coverage or ray-count tier is added.

Ignored raw material, the launch/measurement helpers and all additional capture
frames remain under `build/shadow-live/rt-water-*` and
`build/local-tools/graphics-quality/water_*.py/ps1` in the original public checkout.
