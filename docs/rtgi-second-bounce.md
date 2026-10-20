# Optional second diffuse surface bounce

Base: public `fe5f7a5`. One bounce remains the default and recommended baseline.
The second bounce is retained as a developer experiment: it produces real,
localized additional transport, but the visible improvement with the current
stock reflectance proxies is small. It is not a broad image-quality upgrade.

## Controls and transport

```sqf
triPBR "enhanced";
triRTGI "on";
triRTGI "bounces-2"; // select two surface bounces, reset temporal history
triRTGI "bounces-1"; // restore the baseline, reset temporal history
triRTGI "status";
```

The bounce selector does not enable GI or change component selection. It is
session-only, with no saved setting, menu or preset change. Existing `sky-only`,
`sun-only` and `no-sun` masks also apply to the second endpoint. GL33 continues
to report these Vulkan-only controls as unsupported.

A qualified first hit launches one independently seeded, cosine-weighted diffuse
continuation, bounded to the same 25 m per edge as the first ray. Receivers remain
bounded to 45 m. Both endpoints share the original material reconstruction,
palette/stock texture sampling, reflectance eligibility, terrain wetness and
direct-light estimator. Sun and calibrated directional sky retain their existing
80 m visibility tests and cloud response. Geometric normals control offsets,
sun-facing tests and hemisphere rejection. Rejected directions are absorbed,
without resampling or renormalizing. Dynamic hits/occluders mark history unsafe;
the marker cannot flip back to safe when another dynamic hit is encountered.

For first/second reflectances `rho1`, `rho2`, the added incoming receiver sample is
`rho1 * rho2 * (secondSunEpi + secondVisibleSkyEpi)`. Each diffuse edge samples
`pdf = cos(theta)/pi`, cancelling the Lambert factor. Receiver reflectance is
still applied once by the existing composition pass. There is no extra pi,
distance attenuation, exposure change or artistic gain.

A continuation miss contributes **zero**, including an unobstructed sky miss:
the first endpoint already estimates direct visible sky independently. An
unsupported second hit also contributes zero, with no ambient/color fallback.
The first endpoint's existing unsupported-material ambient fallback is preserved.
Direct sunlight and sky at the first endpoint remain unchanged and are added once.

One half-resolution primary sample, existing receiver capture, reconstruction,
temporal/spatial filtering and ambient replacement remain in use. No new image,
pass, rendering framework, material coverage or replacement artwork was added.
The two-bounce specialization uses a bounded loop to share traversal and shading;
the one-bounce specialization compiles out the continuation. An initial duplicated
implementation cost another 1.95 ms in Everon at 1440p; it was replaced by this
shared loop before qualification.

## Measured GPU cost

RTX 4060 Ti, driver 617.14; original Malden/Everon missions and existing ground
cameras; TAA, 100% world resolution, 1400 m view distance, RT sunlight/cloud
shadows on. Volumetric clouds and water optics are off for these matched timings.
Both modes are warmed before one/two/two/one measurements; each interval settles
for 4 seconds and samples for 6 seconds, followed by two GI-off intervals.
Values average interval medians. No builds or second game ran during timing.

| Scene | Frame, one / two (ms) | Additional frame time (ms) | Trace, one / two (ms) |
| --- | ---: | ---: | ---: |
| Everon 1080p | 2.818 / 3.127 | +0.308 | 0.503 / 0.806 |
| Everon 1440p | 4.782 / 5.326 | +0.544 | 0.834 / 1.366 |
| Malden 1080p | 4.534 / 4.485 | -0.048, inconclusive | 0.561 / 0.592 |
| Malden 1440p | 5.741 / 5.791 | +0.050 | 0.678 / 0.723 |

Everon 1440p's incremental GI overhead rises from **1.821 to 2.366 ms**, about
30%, rather than retaining its existing approximately 1.8 ms budget. Its filter
and temporal scopes stay about 0.354 and 0.184 ms. Malden is CPU submission bound;
at 1080p world-raster timing also shifts from 2.050 to 2.011 ms. The negative total
delta is **not a speedup**: tracing adds 0.030 ms, while clock/supply variation
obscures the small whole-frame cost. Raw interval samples and runtime hashes are
retained. The Everon 1440p run preceded a host-only change to lazily create just
the selected pipeline pair; all four runs use the same final transport shader.

## Lighting, noise and preservation

Original ground and sheltered courtyard/wall views were compared on both islands
in morning, overcast and dusk; roof views were checked in morning light.
Sparse probes verify the actual
two-surface paths, including sunlit terrain reflecting through qualified stock
walls and original material color products. Every checked miss/unsupported
second endpoint has zero additional contribution. Reflectance-product and sum
checks use the emitted float diagnostics, before filtering and composition.

`probe-on` / `probe` / `probe-off` reuse the existing 8x8 sparse path grid, adding
no rays. Each path now contains 96 floats (24 vec4s). Rows 0-11 retain the original
48-float prefix. Row 12 is continuation direction/status (0 not traced/rejected hemisphere,
1 miss, 2 unsupported hit, 3 qualified); rows 13-16 hold its position/kind,
normal/material identity, geometric normal/distance and stock color/availability.
Row 17 is the added incoming radiance, row 18 the total. Rows 19-23 mirror rows
7-11 for the second endpoint: reflectance/cosine, incident sun/visibility,
reflected sun, reflected sky and their outgoing sum. Status also reports eight
second-bounce counts: rays, hits, eligible, rejected, sun rays/blocked and sky
rays/blocked. Timing measurements disable these optional sparse probe writes.

The fixed-sample one/two/one checks have identical first 48 probe fields and
exactly matching receiver counts/family/native/CPU breakdowns in all six views.
Returning to one bounce reproduces each original screenshot exactly. Two bounces
only add positive pixels: about **3.35%** of Everon's yard and **4.00%** of Malden's
courtyard view; the rest remain unchanged. This rules out uniform brightening.
The positive differences are small: whole-content linearized SDR luminance rises
only 0.0255% and 0.0353%, respectively. The fixed Everon roof view is effectively
unchanged. These static one-sample images establish transport, not final quality.

TAA reduces the visible sampling grain, but additional path variance remains.
Across three settled captures, courtyard wall ROI mean luminance rises about
0.055% / 0.154% / 0.075% in morning/overcast/dusk; Everon's barn facade rises
0.050% / 0.079% / 0.067%. Broad ROI variation is often comparable with these gains;
some other ROIs change slightly negatively between unmatched temporal phases.
There is no claim of reduced noise or a clearly visible benefit in every scene.
The current conservative reflectance proxies compound at two hits, and retain
baked stock shading; inventing brighter albedo or extra gain would invalidate
this comparison. One bounce remains the practical choice, especially in Everon.

Focused existing checks passed:

- Vulkan: 64 cases / 49,163 assertions; GL33: 63 / 49,123; 687 driver-free Vulkan
  policy/lifetime checks. The existing history audit now checks the idempotent
  unsafe marker, and policy checks cover the independent bounce selector/default.
- Four loop-shader Vulkan runs presented 92,867 frames with synchronization
  validation and zero errors/warnings. The initial experiment's 24,850 frames
  also had zero validation findings.
- Two-bounce TAA/FSR Quality camera movement, MSAA4, wet terrain and RTAO on both
  islands; SSAO composition, PBR v3 restoration, and live textured RT water with
  active refraction in Malden. Existing lighting and water paths were not edited.
- Rebuilt GL33: Vulkan controls remain unsupported; the existing stock-world ROI
  has zero changed pixels across PBR toggles.
- 59 original player/fixture profile files are unchanged; four stock asset hashes
  match the preserved source baseline. Private DLSS and SDKs were not edited.

The broad `[Graphics]` invocation additionally selected an unrelated hidden DDS
fixture-generation test that expects 64 pixels from a 32-pixel fixture; that test
failed and was left unchanged. Root CTest discovery also requires an unbuilt
Studio suite, so the focused existing test executables were invoked directly.

[Measurements, path examples, controls and validation](evidence/rtgi-second-bounce/qualification.json)
include the limitations and all interval medians. The checked-in original TAA
captures and explicitly amplified static comparisons are in the same directory.
Task-local helpers/raw captures remain under `build/local-tools/graphics-quality`
and `build/shadow-live/bounce2-*`; no new general test infrastructure was added.
