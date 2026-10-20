# Two additional original stock materials

Public base: `95924b9`. This is a small enhanced-PBR/first-bounce coverage
improvement, not a general replacement of stock lighting. PBR and RTGI remain
developer-only and default Off. One bounce remains the default; the optional
second-bounce implementation is unchanged.

## Investigation and retained scope

The existing sparse grid found 457 rejected paths in 1,066 sampled Malden yard
grid/phase paths; 379 were alpha-tested cutouts. These must not become opaque
diffuse reflectors. Other views included opaque unclassified hits, but nearest
world-object hints are not exact texture identification. Low eligibility alone
does not justify adopting foliage, water, trunks or arbitrary building atlases.

Original decoded textures and `statek_kulna.p3d` section reports identify two
useful, separable surfaces:

| Original texture | Section | Enhanced identity / receiver family |
| --- | --- | --- |
| `data\tasky_p90.pac` | Clay-tile roof, ClampV | 128 / roof |
| `data\budova2_stit2.pac` | Weathered plaster/brick blank gable, ClampU + ClampV | 129 / facade |

Both are original 256x256 DXT1 textures whose decoded alpha is entirely 255.
The gable file contains DXT1 transparent-mode blocks, but no transparent decoded
texels; actual decoded opacity and existing runtime checks remain authoritative.
The stock model occurs 49 times in Everon and five times in Malden's original
world inventories. This is not a claim that every instance/LOD qualifies.
Neighboring `budova2_s`, `budova2_s2` and door-bearing `budova2_stit` sections
remain legacy. Mixed window/door/hardware atlases such as `domek1_side`,
`domek2_side`, `cihldum_back` and `stodola_new_side` are not single safe materials.

Broader brick/wood/roof candidates were investigated and not retained. In
particular the inspected small and regular brick houses still fail the existing
per-vertex/lighting prerequisites in the tested Malden views. Their original
lighting is preserved, rather than recovering albedo from already-lit colors or
relaxing the normal/lighting gates. Broad town-building coverage remains limited.

## Shading contract

The unchanged 23-entry v3 table retains its original identities and behavior.
Only `triPBR "enhanced"` adds the two exact texture keys to raster PBR, using the
existing rough roof/plaster dielectric class: roughness 0.90, F0 0.04, metallic 0.
There are no inferred maps, new textures, reflectance multipliers or exposure
changes. Original stock color/detail, authored geometry normals and existing
bounded, unmeasured diffuse appearance proxies remain in use.

Qualified opaque static ray sections use the same identities and material
reconstruction. Native and captured CPU-lit raster paths use the existing
receiver families. Classification alone cannot reserve ambient: full ordinary
lighting, independent captured ambient, valid original normals and the existing
opaque-world marker are still required. Dynamic/colored/emissive/cutout and
unsupported lighting paths retain their existing fallbacks. Ambient is replaced,
not added twice; visibility-tested sun and calibrated directional sky are not
re-estimated by a new lighting path.

As explicitly approved, these materials may become new receivers. The number of
eligible pixels and consequent visibility queries can therefore grow. Samples
per eligible pixel, half-resolution dispatch, 25 m transport, 45 m receivers,
80 m visibility, wetness response, temporal/spatial filters and all shader
transport code are unchanged. There is no new primary sampling rate.

```sqf
triPBR "enhanced";
triRTGI "bounces-1";
triRTGI "off";      // enhanced raster response, original ambient
triRTGI "sky-only"; // visible sky boundary, no surface bounce
triRTGI "on";       // existing first-surface sun + reflected sky
```

## Gameplay comparison

The qualification receipt links full-size, unamplified original-mission
screenshots from standing Malden/Everon farm cameras, plus matched enhanced
GI Off, sky-only and GI On components. Ordinary screenshots were inspected first.

| Scene | Main baseline GI | New enhanced, GI Off | New sky-only | New GI On |
| --- | --- | --- | --- | --- |
| Malden | [before](evidence/rtgi-material-participation/malden-before.png) | [off](evidence/rtgi-material-participation/malden-enhanced-off.png) | [sky](evidence/rtgi-material-participation/malden-sky-only.png) | [on](evidence/rtgi-material-participation/malden-gi-on.png) |
| Everon | [before](evidence/rtgi-material-participation/everon-before.png) | [off](evidence/rtgi-material-participation/everon-enhanced-off.png) | [sky](evidence/rtgi-material-participation/everon-sky-only.png) | [on](evidence/rtgi-material-participation/everon-gi-on.png) |

The change is local material participation, not uniform brightening. Ambient
replacement can darken a previously overfilled facade even while sunlit ground
adds genuinely positive, naturally colored light. Roof appearance changes also
include enhanced raster response and direct visible sky: they must not all be
credited to reflected sunlight.

Numerical ROI comparisons are linearized SDR screenshot luminance, not an HDR
energy measurement. Different TAA phases/cloud motion can perturb small deltas;
sparse probes establish actual hit material, geometric normal, visibility and
outgoing color separately. Existing concrete courtyard walls provide a control.

| Morning gable ROI | GI On vs GI Off | Sun bounce: On vs no-sun | All surface bounce: On vs sky-only | New vs main GI On |
| --- | ---: | ---: | ---: | ---: |
| Malden | -20.03% | +7.17% | +17.45% | -23.70% |
| Everon | -8.71% | +1.11% | +2.44% | -13.06% |

Malden's newly receiving shaded gable has visibly less flat ambient fill and
warmer, localized ground illumination. The sparse sample includes 72 ground-to-
facade paths, 70 sunlit. Its 32 sampled gable reflector hits are backfacing to
the morning sun and correctly contribute **no sunlight**, only visible sky.
Everon's sample includes 37 sunlit ground-to-facade paths, 22 sunlit new plaster
hits and three sunlit new roof hits. The roof hits' mean diffuse reflectance is
`[0.1868, 0.08994, 0.08156]`, and reflected sun is
`[0.22784, 0.09679, 0.02579]`: the original reddish color is transported, not an
invented neutral fill. These are sparse path samples, not whole-scene averages.
All checked new-source paths retain valid position/UV/albedo/original vertex
normal metadata and the existing bounded reflectance/ray distance.
Across 57 unique new-source grid/phase paths, emitted sunlight also equals
original reflectance times incident sunlight, and outgoing light equals sun
plus reflected sky, within float diagnostic precision.

The Everon roof ROI rises 7.99% with GI, but only about 0.19% over sky-only;
most of that visible improvement is sky/ambient response, **not sun bounce**.
Everon's gable bounce is subtle at normal scale. The Malden gable is the clearer
visible transport improvement; neither view demonstrates a broad city-lighting
solution or highly saturated color bleeding. Old courtyard/concrete control
ROIs change about +0.16%/-0.98% against main, comparable to temporal/cloud
variation, not a demonstrated control-surface upgrade. The static Malden v3
gable ROI is pixel-identical; Everon's v3 ROI differs by mean RGB
`[0.338, 0.250, 0.108]` code values across separate runs. V3 code/material
coefficients are unchanged; do not mistake that small scene variation for
bit-exact full-frame replay.

## GPU cost and preservation

RTX 4060 Ti, NVIDIA 617.14; same farm cameras, TAA, 100% world resolution,
1400 m visibility, RT sun/cloud shadows on, water optics/volumetrics/AO off.
Each main/final runtime warms GI for 10 seconds, then measures Off/On/On/Off;
each interval settles four seconds and samples six. Values average the two
interval medians. Probe writes are off, and no build or other game runs during
timing. Runtime status confirms actual 1920x1080 and 2560x1440 world/output sizes.

| Scene | GI On GPU frame, main / new (ms) | GI trace, main / new (ms) | New trace + filter + temporal (ms) |
| --- | ---: | ---: | ---: |
| Malden 1080p | 6.421 / 6.244 | 0.481 / 0.583 | 1.217 |
| Malden 1440p | 6.498 / 6.498 | 0.450 / 0.554 | 1.297 |
| Everon 1080p | 3.260 / 3.285 | 0.202 / 0.294 | 0.522 |
| Everon 1440p | 3.490 / 3.662 | 0.182 / 0.275 | 0.550 |

The new receivers/sources add **0.092-0.103 ms** to tracing and approximately
**0.14-0.17 ms** across trace/filter/temporal scopes. Filters are unchanged but
process more eligible pixels. Everon 1440p's full-frame cost increases 0.172 ms;
GI On minus Off rises from 0.478 to 0.649 ms. At 1080p its measured full-frame
delta is only +0.025 ms. Malden's total deltas (-0.177 / -0.000 ms) do **not**
establish a speedup: CPU command recording is around 15-16 ms, raster scope and
clock/supply variation obscure the additional GI work. GI Off/On overhead in
that CPU-bound 1080p run is 2.282 / 2.160 ms before/after, versus 0.523 / 0.672 ms
at 1440p; do not infer a resolution scaling law from those idle-sensitive GPU
timestamps. Raw samples, separate CPU/world scopes and all interval medians are
retained rather than hiding the inconclusive total-frame results.

These farm views are not the denser Everon ground camera from the earlier
approximately 1.8 ms GI measurement. They cannot establish that scene's new
budget or a universal performance bound. The measured extra work is affordable
for this narrow developer-only adoption; broader material participation needs
its own scene evidence. No extra quality/ray-budget reduction was used.

Both public Vulkan and GL33 builds succeeded. Focused existing shader/material/
receiver tests pass: Vulkan 65 cases / 49,203 assertions, GL33 64 / 49,163;
687 driver-free Vulkan policy/lifetime checks pass (their intentionally injected
validation failure is not a game validation finding). Nine Vulkan comparison/
timing runs presented 132,810 frames with synchronization validation and zero
errors/warnings. Existing smoke checks exercised both bounce modes, TAA/FSR
movement, MSAA4, wet RTAO, SSAO/weather, RT sunlight/cloud shadows and textured
RT water with active refraction. GL33's existing stock-world ROI has zero changed
pixels across PBR toggles; Vulkan-only GI/bounce/shadow controls stay unsupported.
This is focused regression coverage, not an exhaustive gameplay test matrix.

All 59 original player/fixture configuration hashes and four original stock
asset hashes match the preserved baseline. Private DLSS remains clean and
remote-free; its SDKs/build snapshots, original artwork and settings were not
edited. No transport shader, second-bounce implementation, default, preset or
new render pass changed.

Final measurements, GPU intervals, preservation and validation are recorded in
[the qualification receipt](evidence/rtgi-material-participation/qualification.json).
Task-local investigation captures remain under `build/shadow-live/participation-*`;
the existing mission, counter, probe, screenshot and profiler tools were reused.
No rendering framework or general test infrastructure was introduced.
