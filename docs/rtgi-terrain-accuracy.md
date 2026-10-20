# Terrain GI material accuracy and cost

Base: public `3200901046af66b89315a2147d9fe59e0adc7dc1`. PBR/RTGI remain
developer-only and default Off. Original pixels, sky calibration, occlusion,
ambient replacement, one-bounce sampling, distances and reconstruction quality
are retained.

## Original material provenance

Terrain ray hits now use the same `terrainDiffuse` reflectance function as
native and CPU terrain receiver records. It combines the original texture,
unlit tint and repeating detail alpha, applies the existing .5 maximum-component
stock bound and .04 dielectric F0, then the existing wet-diffuse attenuation.
No lit vertex color is inverted or used as albedo. Stock detail alpha is sampled
as linear data at 32x the original UV, independently repeating even when the
base transition tile clamps. It is not interpreted as RGB relief or a normal map.
The original central-difference shading normals and actual triangle normals
remain separate; the latter still govern offsets and geometric hemispheres.

The palette distinction matters: `GenerateSegment` currently creates terrain
vertices with `ClipAll`, selecting palette entry **0** on the CPU-colorized
route. The ordinary native route has the existing neutral material tint.
`GetRandomColor` also returns a spatial palette entry while supplying UV jitter,
but **that color is not used by these raster draws**. Applying it only to GI
would invent a material mismatch. Ray materials therefore use neutral native
tint or the original unlit CPU entry 0 times two, following `DoLightingColorized`.
The CPU receiver sidecar continues to read the actual per-vertex palette index
before clipping; no raster palette or artwork is changed.

The terrain material cache includes the native/CPU route and detail availability.
Wetness uses the existing texture response and current wet-rain/sea-level inputs,
without rebuilding terrain geometry as rain or waves change. Water, potential
coastal triangles, cutouts and unsupported surfaces retain their previous paths.

This remains a bounded stock-reflectance approximation, not measured albedo.
Ray texture/detail lookup uses the available original base mip; raster uses
screen-footprint filtering. Terrain ray geometry remains the existing local
height field, while raster selects its normal terrain LOD and skirts.

## Focused cost changes

Immutable object buffers no longer scan every triangle for terrain receiver
ranges when constructed or updated. Only an eligible GI landscape-ground draw
requests the ranges, lazily from its retained immutable buffer snapshot. An
unchanged dirty notification keeps them; a replaced snapshot invalidates them.
GI Off and ordinary objects/water do not allocate or scan these ranges. Triangle
order, dry/non-dry splits and receiver coverage are preserved.

A conservative translated shape bounding sphere also skips receiver splitting
when the entire shape lies beyond the existing 45 m receiver limit, with a
1 cm outward tolerance. This does not shorten the receiver or ray ranges: no
fragment in that sphere could pass the unchanged per-fragment distance gate.
The ordinary terrain draw and its direct/local lighting remain present.

Terrain capture also reuses its already-computed nonlinear ambient difference
when its separate-sun `aoReference` is exactly `base`. This avoids duplicate
sRGB/night-eye conversions; object PBR and nonseparate lighting keep their
original expression. Terrain patch comparisons reuse scratch vector capacity
across the collection, still checking every height, normal-stencil and texture
input before deciding whether the cached geometry is unchanged.

CPU terrain qualification exposed another existing geometry cost: each clipped
software polygon uploaded the entire transformed mesh table. In Everon's yard,
the CPU water submission failed while allocating another transient page, also
reproduced with the unchanged `3200901` executable. Submission now compacts the
referenced vertex IDs, retains sharing and original triangle order, and copies
all existing lighting/normal/palette/motion fields by their original source ID.
Only referenced vertices are uploaded. A reusable sparse remap avoids clearing
the entire table for each polygon. No water or terrain coverage is discarded.

The existing profiler now reports `Vulkan world: raster_gpu_ms` and
`RTscene_cpu_ms/frame`. World raster time includes terrain/objects and receiver
attachments, before water optics and RT dispatch. It is not an isolated material
record timer: compare GI Off/On in the same view, alongside the existing trace,
filter, temporal, command, upload, draw and binding counters. These intervals can
overlap GPU scheduling effects and should not be subtracted as an exact energy
or execution-time budget.
The trace interval includes hit shading and the existing sky quadrature, not
only hardware ray traversal. The temporal interval also includes its existing
5x5 spatial neighborhood; the filter interval includes upsampling/composition.

## Inspecting real bounce paths

Existing component modes remain `on`, `no-sun`, `sun-only`, and `sky-only`.
`triRTGI 'probe-on'` additionally records 64 sparse paths from the **existing**
8x8 screen grid; it adds no rays or lighting. `triRTGI 'probe'` returns the last
completed frame as JSON, and `probe-off` disables the optional writes/readback.
It is disabled by default and during all timings.
Its writes and path bookkeeping are specialized out of the ordinary GI
pipeline; the optional diagnostic pipeline is created only when requested.

Each of the 64 entries contains twelve four-float records, in this order:

| Record | XYZ / RGB | W |
| --- | --- | --- |
| 0 | Receiver world position | Temporal sample phase + 1 |
| 1 | Receiver sampling normal | Receiver material family |
| 2 | Existing primary ray direction | 0 untraced, 1 miss, 2 unsupported hit, 3 eligible hit |
| 3 | Actual hit world position | Scene kind (2 = terrain) |
| 4 | Hit shading normal | Material identity (1000 = terrain) |
| 5 | Actual geometric normal | Hit distance |
| 6 | Original sampled linear texture | Available-input flags |
| 7 | Bounded diffuse reflectance after wetness | Sun cosine after geometric-side rejection |
| 8 | Visible incident direct sunlight | Sun/cloud visibility |
| 9 | Outgoing reflected direct sunlight | Sun direction X |
| 10 | Outgoing reflected skylight | Sun direction Y |
| 11 | Total outgoing bounce | Sun direction Z |

Zero entries are ineligible receivers. The probe reports individual Monte Carlo
paths before filtering, ambient composition, AA and output conversion. It is
not an exhaustive pixel capture or a substitute for examining the normal scene.

## Qualification

Measurements and preservation receipts accompany the final implementation in
`docs/evidence/rtgi-terrain-accuracy/qualification.json`. Raw local runs use the
existing copied-profile, muted, original Malden/Everon mission harness under
`build/shadow-live/accuracy-*`. Camera sweeps are real mission render-camera
checks, not a claim of completing either mission.

The first Malden 1080p intervals changed draw counts while caches settled; those
samples are retained separately. Both executables were rechecked after scene
and pipeline warmup. Malden is CPU-bound, so whole-frame GPU timestamps also
reflect command-supply effects; they are not a standalone ray-traversal speedup.

Cross-launch weather is another real control: the original sky/cloud inputs
can differ while GI-Off pixels remain nearly identical. The receipt preserves
the unoccluded sky quadrature with each weather comparison. Use the separately
matched morning component captures and raw paths to attribute material/bounce
changes; do not attribute every cross-launch brightness difference to this patch.

### Measured lighting

Native/CPU agreement is checked at actual matching ray hits, not by inverting
screenshots. **52** matching Everon ground paths have CPU/native reflectance
ratios `[1.00392108, 1.00392166, 1.00392179]`, agreeing with the original neutral
CPU palette multiplier `256/255 = 1.00392157`. The native route stays neutral.
Original detail alpha and wet attenuation remain in both reconstructions.

In the matched morning captures, Everon's barn facade changes **-.125%** in
linearized SDR luminance versus the baseline, its shaded-ground ROI **+.099%**.
Malden's shaded ground changes **+.011%**, and its stone-wall ROI **-.001%**.
These are subtle material/sampling differences, not a new global fill light.
The separately reported Malden open-ground difference is **+1.63%**; its surface
bounce component is unresolved, so that difference is not attributed to improved
terrain reflectance. Full weather comparisons retain their sky-state caveat.

| Matched ROI | Direct-sun bounce (`on - no-sun`) | Reflected sky (`on - sun-only`) |
| --- | ---: | ---: |
| Malden shaded ground | -.002% | +.161% |
| Malden stone-wall region | +.772% | +.723% |
| Everon shaded ground | +.019% | +.477% |
| Everon barn facade | +2.965% | +3.810% |

Percentages are relative to the corresponding component-disabled image, from
four interleaved captures per mode. Tiny signed ground sun differences remain
unresolved. The baseline barn already measured **+2.963%** direct-sun bounce:
there was no missing sun-transport term to repair or reason to add gain.

The sparse Everon paths establish the geometric reason. Ground receivers sample
their upward hemisphere; the nearby flat ground is not in it. In the final
yard check, none of **1,178** unique terrain-receiver grid/phase paths hit
eligible terrain. The nearby wall and elevated barn checks likewise record none.
Facade paths do point downward toward sunlit ground. One observed path reaches
`[2219.1,22.32,7398.08]` from the barn, with shading and geometric normals
`[0,1,0]`, sun cosine **.747442**, cloud/sun visibility **.954690**, diffuse
reflectance `[.0514165,.0619915,.0289224]`, visible incident sunlight
`[.938337,.827944,.243293]`, and outgoing sunlight
`[.048246,.0513256,.00703662]`. The outgoing value is the componentwise product,
with positive, correctly colored ground radiance. This explains measurable wall
bounce alongside negligible ground-to-ground bounce without changing transport.

### Measured performance

RTX 4060 Ti, TAA, 100% world resolution; existing Off/On/On/Off intervals, four
seconds settling and six seconds sampling per interval, two medians averaged
per mode. The rechecked runs warm the scene/pipeline caches first. Cloud volume
and water optics are Off during timings; ordinary sky/weather, cloud shadows and
RT sunlight remain. All raw interval samples are in the receipt.

| Ground scene | Baseline full GI cost | Updated full GI cost |
| --- | ---: | ---: |
| Malden 1920x1080, warmed recheck | 1.034 ms | .327 ms |
| Malden 2560x1440 | 1.513 ms | 1.650 ms |
| Everon 1920x1080 | .911 ms | .931 ms |
| Everon 2560x1440 | 1.850 ms | 1.885 ms |

The gain is not universal. In CPU-bound Malden 1080p, GI-On lit draws fall from
**1,366 to 1,028**, lighting bindings from **700 to 339**, and CPU command recording
from **13.022 to 11.374 ms**. This is real avoidable draw/driver work, with the
same receiver bound and retained terrain coverage. It is not a claim that the
ray estimator itself became .707 ms cheaper. Original initial baseline timing
was .960 ms; the variable initial updated intervals and the warmed recheck are
both retained, rather than presenting a negative apparent GI cost.

At Everon 1440p, updated trace/hit shading, filter/composition, and temporal
reconstruction average **.848 / .351 / .234 ms**. World raster is **1.213 ->
1.602 ms** with GI Off/On; scene preparation is **.073 -> .090 ms CPU** and
transient geometry upload about **.007 ms CPU**. The intervals are distinct
scopes, not an exact additive attribution of every whole-frame difference.
The corrected material lookup adds a small trace cost there. Malden 1440p still
costs more overall; this patch does not claim a broad GPU speedup.

### Gameplay and preservation

The final `1DC03AB6...` executable completes **120,243** presented Vulkan frames
across the four final runs, including the Everon native/CPU/native check:
**zero validation errors or warnings**, with synchronization validation enabled.
Earlier failing diagnostic runs are explicitly excluded and retained locally.
The CPU route is still a correctness fallback with many small draw submissions;
compaction fixes the repeated upload/allocation failure, not its entire cost.

Both islands complete TAA/FSR translation/yaw and ordinary distance/LOD sweeps.
Physical Malden player strafing moves about **1.16 m** under each AA mode, with
GI active. Headlights use two actual local shadow maps. Live coastal waves keep
BLAS builds fixed at **954**, shade terrain reflection hits, and retain refraction
with `fallback=none`. Dry/uncertain coastal receiver gates remain unchanged.
Fog/rain, clouds, night-eye fallback, SSAO/RTAO and MSAA4 are also exercised through
the existing focused helper. Stock palette, transition tiles and detail remain
recognizable in the original unamplified captures.

PBR v3 equations and eligibility are unchanged; its cross-launch Vulkan control
ROI luminance varies by at most **.33%** in these checks. The GL33 stationary
world control has **zero changed pixels** across the existing PBR command probes.
Vulkan focused tests pass **64 cases / 48,860 assertions**, GL33 **63 / 48,820**,
and existing driver-free policy/lifetime guards **684 checks**. Policy tests
deliberately inject failures; they are separate from actual gameplay validation.
The four original PBO/world preservation hashes still match. Original player
settings and private DLSS/NGX remain untouched; no replacement artwork is added.

Original comparisons: [Malden before](evidence/rtgi-terrain-accuracy/malden-yard-before.png),
[after](evidence/rtgi-terrain-accuracy/malden-yard-after.png),
[Everon before](evidence/rtgi-terrain-accuracy/everon-yard-before.png),
[after](evidence/rtgi-terrain-accuracy/everon-yard-after.png).
These cross-launch frames retain the weather qualification above. The
[qualification receipt](evidence/rtgi-terrain-accuracy/qualification.json) includes
the matched component measurements, actual path data, profiler samples, tests,
CPU route captures, motion/coast checks and preservation evidence.
