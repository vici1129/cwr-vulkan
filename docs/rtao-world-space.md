# RTAO world-space coverage (2026-10-19)

This milestone starts at public main `afe900a`. RTAO remains a developer command,
default **Off**, with no saved setting, preset membership or player menu entry.
SSAO remains independently available and its saved preference survives an RTAO
round trip. RTGI remains parked; its implementation and shaders are unchanged.

## What was actually limiting the old effect

The old receiver was any valid world-depth pixel. Terrain already **received**
RTAO; adding another terrain receiver flag alone would not fix coverage. The
important omissions were on the other side of the rays and at composition:

- AO rays used instance mask `1`. Terrain patches, when collected for another
  RT consumer, used mask `2`, shared with the extended foliage/cutout subset.
  An AO-only scene did not collect terrain at all. Ground could receive a wall's
  occlusion, but could not occlude the lower wall, a rock face or a vehicle.
- The 2 m, distance-weighted ray bound contributed little from ground to a
  standing-height vertical receiver. This limited nearby structural occlusion
  even after adding the real terrain triangles.
- The final pass multiplied completed world color. That also attenuated direct
  sunlight, local lights, emissive light and fog. Increasing that multiplication's
  strength would make the image darker without repairing its lighting meaning.
- Frustum culling was **not** the missing coverage: static spatial buckets and
  the existing dynamic fallback already collect eligible off-screen occluders.

The exclusions are not all bugs. Alpha-blended effects, hidden/proxy geometry,
animated/damaged static meshes and terrain-fitted immutable meshes cannot safely
be promoted to opaque casters. Building window/door cutouts already traverse the
real texture alpha; this test remains unchanged. Foliage cards and non-building
cutout fence sections remain outside the AO mask; eligible rigid opaque fence
members, posts, trunks, walls and rocks still participate.

## Rendering change

The existing shared collector, immutable mesh buffers, BLAS cache and per-frame
TLAS ownership are retained. AO now requests the existing local 16x16-cell terrain
patches even without RT sunlight/water. Heights, cell diagonals and invalidation
are exactly those used by the existing terrain RT path. Terrain gains a separate
AO bit (`4`); AO rays use `1|4`, leaving the foliage-only subset excluded. The
AO-only object budget reserves terrain slots within the existing 1,024-instance
cap. The scene still uses an 80 m neighborhood, or the existing 160 m shared
neighborhood when another RT consumer needs it.

AO alone now sets a **4 m** ray radius. Strength `.85`, the `.55` maximum
occlusion, 3.5 cm normal offset, four world-anchored cosine samples, half-resolution
trace, bilateral upsampling and 60 m receiver fade are retained. There is no new
temporal history, random frame sequence or sample-count increase.

Lighting correctness requires one ambient record at the existing world boundary.
It belongs to the existing AA targets and is created only while RTAO is On:

- Native shapes supply their authored global ambient term, excluding forced
  sunlight. The legacy path records the difference in its existing linear-light
  conversion; PBR records its existing ambient contribution without changing its
  BRDF, material selection or lighting calculation.
- Colorized terrain carries its actual CPU-computed ambient contribution,
  including colorization, in an optional sidecar through CPU clipping. Other
  software lighting has no invented ambient estimate.
- The fragment stage records ambient radiance after the existing material/detail,
  wetness, CSM and night-eye response, times fog transmittance, plus raster view
  depth. Fog in-scattering is not part of this record. Local lights and emission
  are not included in the amount to subtract.
- Unsupported/translucent draws invalidate the record; cutouts retain raster
  discard/coverage. Water does not receive AO. MSAA requires matching surface
  depth and lighting across samples before modifying resolved color.
- Composition subtracts `capturedAmbient * (1 - aoVisibility)` from HDR RGB and
  preserves alpha. It does not multiply completed direct/emissive/fog lighting.
  Existing CSM/RT sunlight composition still runs in its established order.

The new record uses the existing MRT/pipeline infrastructure, with an additional
FP16 attachment and five color output locations. Independent blending and at
least five color attachments are checked before accepting On. AA recreation
handles toggles and releases the record when Off. No extra AS system, scene
database, persistent renderer setting or private SDK is introduced.
The existing sun/water MRT layout is reused, including its carrier targets in an
AO-only launch; full-frame timing below includes this allocation/layout choice.

## Follow-up composition fixes

The volumetric-cloud world-composite pipeline now declares the fifth color slot
required by the RTAO pass, with writes disabled for that slot. Legacy non-PBR AO
extracts ambient from the sun-independent lighting base whenever sunlight is
captured, including its night-eye difference. This prevents AO and RT sunlight
from subtracting overlapping nonlinear lighting contributions. RTAO Off, PBR,
sunlight visibility and the ray-tracing algorithms are unchanged. Source-contract
and arithmetic regression checks cover both defects. The Vulkan build and 76
focused cases (2,610 assertions) pass, as do 624 driver-free policy checks. All
six RTAO-Off fragment variants compile to byte-identical SPIR-V versus `85288c0`.

A short 1080p Malden validation run with PBR Off, RTAO, RT sunlight and volumetric
clouds confirmed active clouds with no fallback across TAA, MSAA 4x, FSR Quality
and AA Off, plus night/day and RTAO Off/On transitions: 6,658 presented frames,
zero validation errors or warnings. Local evidence is retained
in `build/shadow-live/rtao-surgical-20261019/`. This is a combination smoke check,
not a before/after gameplay reproduction of the black-shadow defect.

## Stock gameplay review

The baseline executable identifies itself as `3.05-afe900a`, SHA256
`28DDDC594404FA198E75CE1194A3B826D07C6BF14AC7A5FB25062F511321251A`.
The qualified candidate SHA256 is
`EB8BE1B22C53387804B5E02B40AC2C59E7CBE94F89F770D7968639C81CDD28C1`.
These are muted direct developer launches using stock Malden `01TakeTheCar.ABEL`
and Everon `06Ninjas.EDEN`, isolated copies of the existing profile, 12.5 m terrain
grid, 1,400 m view distance, noon clear/overcast conditions, and ordinary exposure.
They are not Steam-route qualification.

Compare the ordinary, unamplified 1:1 image crops (Off / SSAO / previous RTAO /
improved RTAO): [Malden wall](evidence/rtao-world-space/malden-wall.webp),
[rock](evidence/rtao-world-space/malden-rock.webp),
[UAZ](evidence/rtao-world-space/malden-uaz.webp),
[off-screen wall](evidence/rtao-world-space/malden-offscreen.webp),
[Everon brick building](evidence/rtao-world-space/everon-brick.webp), and
[barn](evidence/rtao-world-space/everon-barn.webp).
The [capture manifest](evidence/rtao-world-space/screenshots.json) records original
PNG hashes, crop rectangles and lossless output hashes. All source captures remain
under the corresponding local `build/shadow-live/rtao-*` run.

The useful change is the lower wall/door/vehicle response to real ground, along
with nearby recesses and structural shading that survive loss of the occluder's
screen samples. Sunny ground need not be darker than previous RTAO: its direct
sunlight is preserved. Off-screen occlusion remains useful, especially under
overcast ambient lighting; this was already possible in the previous ray scene.
The new advantage over that implementation is terrain participation and correct
composition, not a claim that off-screen collection was newly added.

The UAZ is the mission's existing undamaged `aU`, not a replacement mesh. Its
fresh-launch position varied; the unmatched early captures were discarded. The
matched pair restores the authored mission position `[7279.009277,8009.358398,0]`
and heading 235, allows one second to settle, then freezes. Both builds report
`[7278.94,8009.35,0.00204468]`. Dynamic geometry is independently On for this
comparison; RTAO does not silently enable that feature.

Useful camera setups, in SQF coordinates unless marked raw engine X/up/Z:

| Scene | Camera | Target / direction |
|---|---|---|
| Malden wall | `[3092,6339,1.7]` | `[3080,6350,1.7]` |
| Malden house | raw `[7116,73,6085]` | raw `[-1,-.1,-.2]` |
| Off-screen wall | raw `[7111,72.5,6082]` | raw `[1,-.7,0]` |
| Malden rock | `[7215,7708,1.7]` | `[7223,7704,1.2]` |
| UAZ | `[7283,8006,1.2]` | `[7279,8009,.5]` |
| Everon brick | `[4903,7090,1.7]` | `[4930,7107,1.7]` |
| Everon barn | `[5264,9372,1.7]` | `[5253,9378,1.7]` |
| Everon yard | `[5265,9373,1.7]` | `[5274,9383,1.2]` |

Repeated four-state comparisons covered AA Off, TAA and FSR Quality. Actual player
strafing and mouse turns covered TAA/FSR on both islands, including disocclusion
and settled views. No new persistent trails, dark silhouette halos, filled alpha
holes or broad foliage-card occlusion were observed in those sequences. Four-ray
grain remains visible at close range with AA Off; TAA/FSR reduce it. Sampling was
not changed merely to increase apparent contrast.

CSM, RT sunlight, overcast weather, ordinary fog, night and MSAA4 were exercised
with Off/On/Off comparisons. Frozen static scene Off images restore exactly on
both islands, and the SSAO preference round trip is pixel-identical. One initial
vehicle comparison includes changing background foliage; that pair is not used
as a full-frame identity claim. See [restoration checks](evidence/rtao-world-space/off-restoration.json).
Static wall/ground regions also match the **previous executable** with AO Off,
with zero differing RGB values in all five checked regions:
[baseline Off checks](evidence/rtao-world-space/baseline-off.json).
The Malden SSAO preference check excludes a fading radio subtitle; the Everon
check matches the full captured window. Runtime [states](evidence/rtao-world-space/states.json)
and actual [movement receipts](evidence/rtao-world-space/movement.json) are retained.

## Verification and cost

Final Vulkan validation runs closed after **25,928 Malden** and **10,628 Everon**
submitted frames with **zero core/synchronization errors or warnings**. The earlier
2 m exploratory candidate also closed cleanly after 42,280 frames. Runtime hashes,
launch arguments and shutdown receipts are in
[runtime receipts](evidence/rtao-world-space/runtime-receipts.json).

The public Vulkan and GL33 builds pass. Focused Vulkan shape tests pass 720
assertions / 30 cases, shared GL33 shape tests 680 / 29, graphics tests excluding
hidden fixture generators 42,762 / 384, and driver-free policy/lifetime tests 624
checks. The new regression verifies ambient sidecar clipping and release, and the
uniform layout test verifies the appended field offsets.

A broader default run is **not fully green**: 2,526 passed, 7 failed, 4 skipped.
Failures reference missing external `packages/Remaster` / `packages/Demo`
localization data and a Windows Unicode-path exception in `ModArchive`. Explicitly
selecting hidden fixture generators also exposes an existing synthetic DXT5 test
expecting 64 pixels for a 32-pixel fixture. These files and asset data were not
changed for RTAO. See the [full-suite log](evidence/rtao-world-space/all-tests.txt),
[graphics checks](evidence/rtao-world-space/graphics-tests.txt),
[shape checks](evidence/rtao-world-space/vulkan-shape-tests.txt), and
[policy checks](evidence/rtao-world-space/policy-tests.txt). Original logs are also
retained locally under `build/rtao-*tests.txt`.

GPU profiling uses the existing completed-frame and BLAS/TLAS/trace/filter
timestamps on RTX 4060 Ti 8 GB, driver 617.14, VSync Off, validation disabled.
Each state warms for two seconds and samples four seconds in Off / On / On / Off /
SSAO order, separately with TAA and FSR Quality. The
[measurements](evidence/rtao-world-space/measurements.json) retain raw samples and
GPU clock/power state. Full-frame results include ambient capture and composition;
trace/filter timestamps alone do not include that raster work. Dynamic GPU clocks
vary between states, so these are measured gameplay costs, not fixed-clock FPS
claims. Static house/yard timing is not an upper bound for a battle's dynamic BLAS
updates.

Median GPU milliseconds (all completed samples pooled within each state):

| Island / output | AA | AO Off | SSAO | Previous RTAO | Improved RTAO | New trace + filter |
|---|---|---:|---:|---:|---:|---:|
| Malden / 1080p | TAA | 1.813 | 2.156 | 2.212 | 2.241 | .301 |
| Malden / 1080p | FSR Quality | 2.093 | 2.119 | 2.180 | 2.191 | .170 |
| Malden / 1440p | TAA | 2.271 | 2.457 | 2.828 | 3.031 | .424 |
| Malden / 1440p | FSR Quality | 2.227 | 2.303 | 2.468 | 2.594 | .195 |
| Everon / 1080p | TAA | 2.039 | 2.115 | 2.097 | 2.169 | .399 |
| Everon / 1080p | FSR Quality | 2.020 | 2.094 | 2.113 | 2.110 | .205 |
| Everon / 1440p | TAA | 2.121 | 2.197 | 2.286 | 2.571 | .463 |
| Everon / 1440p | FSR Quality | 2.123 | 2.175 | 2.204 | 2.296 | .212 |

The measured full-frame increase over AO Off is .089–.759 ms. The largest case
is Malden 1440p TAA, .203 ms above previous RTAO. The two On windows agree closely;
the [numeric summary](evidence/rtao-world-space/cost-summary.json) preserves exact
values. Native TAA traces at output resolution; FSR Quality uses 1280x720 input
at 1080p and 1706x960 at 1440p. Cached static BLAS/TLAS timestamp overhead in these
settled scenes is approximately .0001 ms per bracket, not a build cost claim.

## Remaining bounds

This is bounded near-field AO, not full sky visibility or GI. Raster LOD versus
visual LOD0, four-sample noise, conservative mixed-MSAA rejection and the existing
bias can still miss thin contacts. Terrain-fitted/deformed static models need
their **posed** geometry; simply adding their immutable source mesh would give
incorrect occlusion. Proxy doors, damaged static models and excluded cutout fence
cards likewise remain incomplete. The shared scene has no reliable opaque versus
foliage classification for every excluded card, so this milestone does not fill
those holes with solid rectangles or guessed hulls.

Retain the coverage/composition improvement as developer-only RTAO, default Off.
No RTGI, PBR BRDF, RT water/shadow algorithm, assets, graphics presets, settings
pages or private DLSS/NGX files are changed by this milestone.
