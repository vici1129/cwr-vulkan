# Stock PBR coverage and CPU lighting inputs (2026-10-19)

Public base: `e9c4a06` (verified against origin/main before work). This replaces
the five-identity/native-only coverage limit with inspected rough dielectric
sections on both native and CPU-transformed geometry. The existing GGX BRDF,
light calibration, texture sampling, presets and settings are unchanged.

## Asset investigation

Stock `DTA/Data.pbo`, `DTA/Data3D.pbo`, `Worlds/abel.wrp` and `Worlds/eden.wrp`
were inspected read-only with the current PoseidonTools. Decoded original pixels
were viewed locally, and section identities were compared across visual LODs.
No extracted or replacement game assets are included in this change. Local
inspection outputs and source SHA-256 receipts are in `build/pbr3-inspection/`.

The practical obstacles were not a missing BRDF:

* Small stationary objects can use CPU T&L, not just terrain and animated models.
  `zed_kamenna` and the wooden members of `lampazel` are examples. The old screen
  shader explicitly supplied a zero normal and the PBR classifier never ran.
* CPU material lighting still has original normals, material modes and selected
  engine lights. Packing its summed lighting into ARGB discards both the unlit
  material inputs and the individual light responses. HDR color alone does not
  restore them. Feeding that color into the BRDF would apply lighting twice.
* Sections and vertex lighting modes are different boundaries. One texture
  section can include normal, inside, half-lit and shining vertices. A section
  name alone cannot authorize replacement of its lighting.
* `domek1_side`, `domek1_fr`, `cihldum_back` and similar house images combine
  masonry with windows, frames, doors and/or gutters. They remain legacy.
  Transparent foliage/fence impostors and pallet LOD atlas substitutions remain
  excluded. The open and closed barns use different materials; one does not
  qualify merely because the other was inspected.
* Images contain authored dirt, cracks, mortar relief and baked shadows. There
  is no recoverable normal/roughness map or trustworthy measured albedo. These
  details stay in the original color image. We do not invert baked lighting or
  synthesize surface normals from image contrast.

Representative placed model counts from the stock world tables:

| Model | Malden | Everon | Relevant coverage |
| --- | ---: | ---: | --- |
| `zed_kamenna` | 130 | 39 | Stone/mortar wall and fired-clay cap, both visual LODs |
| `lampazel` / `lampadrevo` | 12 / 0 | 0 / 20 | Dedicated wood members; hardware stays legacy |
| `dum_rasovna` | 11 | 56 | Separate worn plaster/exposed brick side section, all five visual LODs |
| `stodola` | 0 | 10 | Closed barn plaster/brick, rough timber door and tiled roof, all three visual LODs |
| `zed` / `zed2` | 0 / 0 | 4 / 73 | Concrete panels, pillars and caps; matching identities across visual LODs |
| `molo_beton` | 1 | 1 | Dedicated weathered concrete side/top, all three visual LODs |

These are placed **models**, not a claim that every polygon or every instance
passes the lighting gates. They describe ordinary stock coverage rather than
newly placed material demonstration assets.

## Material policy

`PBRMaterialVK.hpp` remains an exact stock-identity allowlist, shared by the two
Vulkan geometry routes. No prefix matching, material database, texture mutation
or new shader system is introduced. New decisions are supported by pixels and
section usage, not translations of texture names.

All candidates are nonmetallic with F0=0.04. These roughness values are
conservative unmeasured assumptions, not extracted optical properties:

| Images under `data/` | Perceptual roughness | Evidence and limits |
| --- | ---: | --- |
| `kamennazed.pac` | .90 | Repeated stone/mortar with a clay cap strip; no polished stone inferred |
| `telegraf_sloup.pac`, `prkna_dalsi.pac` | .85 | Visible weathered grain/boards; no varnish inferred |
| `tasky_tmavsi.pac` | .90 | Dedicated repeated tile image on the closed barn |
| `omlacena_bocni_zed.pac` | .90 | Separate worn plaster/exposed brick, without windows or hardware |
| `stodola_f.pac`, `stodola_f2.pac`, `stodola_s.pac` | .90 | Worn plaster/brick facade and rough timber door; both are compatible with this conservative matte dielectric response; no glass/exposed metal promoted |
| `betonpoprask_side.pac`, `betonpoprask_top.pac` | .90 | Weathered concrete quay, authored staining retained |
| `plot_beton_01_side.pac`, `plot_beton_02_side.pac`, `plot_beton_03_side.pac`, `plot_beton_0301_side.pac`, `plot_beton_0302_side.pac`, `plot_beton_01_top.pac`, `plot_beton_03_top.pac`, `betondlouhastenabez.pac` | .90 | Concrete panels/caps, including painted camouflage and original stains; no optical properties inferred from camouflage colors |

The original rock (.85), painted signpost (.60/default and its existing legacy
power hint), roof tiles (.90, both orientations) and dark wood (.85) retain
their parameters. No brightness, F0 or roughness adjustment was made to amplify
the result. Unsupported sections retain their original shading.

## Preserving the missing CPU inputs

The existing `DoMaterialLightingP`/SIMD equivalent calls an optional Engine
capture hook before CPU lighting. Its default is a no-op; only EngineVK
implements it. It captures only Shapes containing an allowlisted section, only
while PBR is on and during ordinary world drawing.

For eligible material ranges it retains the existing ShapeLighting payload:
ambient/emissive, diffuse sun, forced-sun separation, sun direction, and the
complete selected point/spot light set (at most the existing eight). Unsupported
light types or overflow leave the range legacy, rather than silently dropping
lights. Existing night scaling, falloff, cones and local shadow slot assignment
are reused. CPU light preparation/color packing continues unchanged for fallback.

A TL sidecar holds original outward world normals and the captured material
slot. Normals use the same sign and inverse-transpose transform as native Shape
geometry, including nonuniform scale. Clipping interpolates these normals but
invalidates vertices crossing different material slots. Only ordinary vertex
lighting mode zero is eligible; inside/half-lit/shining modes stay legacy even
when an exposure multiplier could make their diffuse coefficient exceed one.
At submission, triangles with a common valid material slot use the existing
PBR fragment branch; incompatible triangles retain CPU-lit color. Consecutive
material runs preserve original face order. The common single-run case adds no
index copy or extra draw. Three floats extend the Vulkan screen vertex only;
the engine's legacy TL vertex layout and GL33 streams are unchanged.

The shader reconstructs camera-relative position with the existing screen
projection and interpolates the captured normal. PBR **replaces** CPU-lit color;
it never uses that color as albedo. The original texture, per-light material
coefficients and existing normalized BRDF are the inputs. Ambient/emissive,
direct sun and local light remain separate. CSM/cloud/RT visibility attenuate
sun diffuse and specular together through the existing composition. The existing
ambient output is valid for captured CPU materials too. No AO/RT algorithm,
RTGI, water shader, lighting preset or private integration changes are made.

Fog, weather, night-eye response and temporal outputs follow shading as before.
Classified wet ground/roads retain their existing response; this does not add a
wet clearcoat to roofs/walls. Off skips capture and executes the original color
and lighting branches, with original attribute offsets preserved.

## Visual qualification

Real stock missions `01TakeTheCar.ABEL` and `06Ninjas.EDEN` were run with copied
profiles, muted game audio and the existing local harness/exact-window capture
helpers. These are direct developer gameplay launches, not Steam-route or
mission-completion qualification. Original gameplay assets were not modified.
The five-texture reference executable was preserved before building the change.

Matched Off/previous/expanded comparisons cover Malden street stone/wood,
plaster and roofs, and Everon stone walls, closed barns and
concrete military-compound walls. Daylight, dawn, shade, ordinary view distance,
headlights, RT sunlight/local shadows, rain/wet ground and fog are checked.
Open-barn/mixed-atlas scenes are useful negative controls, not claimed coverage.
The quay's original texture/LOD data was inspected, but the attempted Malden
camera did not establish a visible quay; it is not used as visual acceptance.
The strongest practical improvement is resolving headlight cones across large
CPU-lit wall triangles: the old vertex result can miss nearly the entire light
pool. Daylight gives a continuous rough dielectric response to walls and timber
without polishing masonry. Texture cracks, color differences and stains remain.

TAA and FSR Quality are checked with actual bounded strafe and mouse input on
both islands, followed by settled captures. Camera retreats check the same
wall identities at 8, 15, 30, 60, 100 and 150 metres. Existing low-poly geometry,
texture LOD changes, CSM stair steps and foliage aliasing remain; this is not a
claim to eliminate them or certify every temporal history case.

Local raw evidence is under `build/shadow-live/pbr3-*`; selected original PNGs,
receipts, parity statistics and profiler samples are retained alongside this
record in `docs/evidence/pbr-coverage/`.

| Normal gameplay-distance view | Off | Previous five-texture PBR | Expanded |
| --- | --- | --- | --- |
| Malden street stone, timber and nearby buildings | [PNG](evidence/pbr-coverage/malden-street-off.png) | [PNG](evidence/pbr-coverage/malden-street-previous.png) | [PNG](evidence/pbr-coverage/malden-street-expanded.png) |
| Everon concrete compound walls | [PNG](evidence/pbr-coverage/everon-concrete-off.png) | [PNG](evidence/pbr-coverage/everon-concrete-previous.png) | [PNG](evidence/pbr-coverage/everon-concrete-expanded.png) |
| Everon closed barn plaster/brick, rough timber and roof | [PNG](evidence/pbr-coverage/everon-barn-off.png) | [PNG](evidence/pbr-coverage/everon-barn-previous.png) | [PNG](evidence/pbr-coverage/everon-barn-expanded.png) |
| Malden wall under ordinary Jeep headlights | [PNG](evidence/pbr-coverage/malden-headlights-off.png) | [PNG](evidence/pbr-coverage/malden-headlights-previous.png) | [PNG](evidence/pbr-coverage/malden-headlights-expanded.png) |

The preserved `e9c4a06` executable and expanded build with PBR Off match exactly
in the tested Malden stone-wall ROI `(100,600)-(900,820)` of the original window
capture ([record](evidence/pbr-coverage/original-off-parity.json)). Settled
same-session Off/On/Off comparisons restore the world region exactly; subtitles
can expire independently of the frozen simulation. First cold-camera images
with still-streaming LOD changes are excluded from parity claims. Two Malden
movement attempts ended with the stock mission during prolonged distant-player
simulation; a fresh run bounded simulation around actual input and completed
both AA modes. Neither interrupted run is counted as a completed motion test.

## Verification and cost

Vulkan focused Shape/AA/shadow tests: **1,624 assertions / 74 cases**. GL33:
**1,584 / 73**. Driver-free Vulkan policy/lifetime guards: **624 checks**.
New checks exercise inverse-transpose agreement, normal sign and nonuniform
scale, clipping with compatible/incompatible material modes, fallback identities
and separate CPU draw telemetry/reset. The policy test's intentional teardown
validation error is an injected fixture, not a live Vulkan error.

The changed screen vertex shader compiles and passes SPIR-V validation for
Vulkan 1.0. Real-island runs use Vulkan validation; live receipts and GPU cost
measurements are recorded with the retained evidence. Timing uses the existing
GPU/CPU profiler, warmed alternating Off/On/On/Off blocks, TAA at 100% scale,
vsync off and frozen midday views, with validation disabled and no simultaneous
builds. The 1440p window exceeds the 1080p desktop: its verified 2560x1440 targets
qualify render workload cost, not physical 1440p presentation quality.

Test PC: RTX 4060 Ti, Core i7-12700F, Windows. The final implementation artifact
tested before committing has SHA-256
`3703476A6B1E230B0C904806FA60E6AAC2E69EBA44AFEB6962A89C710C13FB5D`;
the preserved five-texture reference is
`3AB11F9484ABF8983CCA7BD8F761AFA7932E8CA9C04B179C9C2139A059B4B237`.
Four final-artifact validation runs presented **33,962 frames**, with zero
validation errors/warnings ([receipts](evidence/pbr-coverage/receipts.json)).
The previously qualified Everon shadow/weather pass adds 27,082 frames with
zero errors/warnings; its binary precedes the closed-barn side/door identities,
not a claim that those new identities were present in that run. Startup loader
messages about a stale external EOS manifest/disabled implicit overlays are
distinct from the Vulkan validation callback totals.

GL33 rebuild and live Off/On/Off control return Unsupported for PBR and change
zero world-region pixels ([record](evidence/pbr-coverage/gl33-isolation.json)).
Original source archive/world SHA-256 values were rechecked unchanged after all
tests ([receipt](evidence/pbr-coverage/source-hashes.json)). Temporary test copies
and their settings remain separate from the player's profiles. Keyboard Caps
Lock was restored after bounded physical input checks.

Existing-profiler medians (milliseconds; [raw samples and sessions](evidence/pbr-coverage/cost.json)):

| Stock scene / output | GPU Off -> expanded | Delta | CPU frame Off -> expanded | CPU commands Off -> expanded |
| --- | ---: | ---: | ---: | ---: |
| Malden street / 1920x1080 | 2.1325 -> 2.1308 | -.0017 | 6.196 -> 6.204 | 4.101 -> 4.078 |
| Malden street / 2560x1440 | 2.2108 -> 2.2121 | +.0013 | 6.192 -> 6.197 | 3.969 -> 4.089 |
| Everon concrete / 1920x1080 | 2.1384 -> 2.1369 | -.0015 | 6.174 -> 6.176 | 3.041 -> 3.059 |
| Everon concrete / 2560x1440 | 2.2050 -> 2.2096 | +.0046 | 6.175 -> 6.176 | 3.016 -> 3.052 |

The GPU differences are within normal run/clock noise, not evidence of a speedup.
No additional pass, image or descriptor is allocated; warmed profiler intervals
report zero Vulkan allocations/frame. CPU frame measurements remain near the
existing pacing floor, so they are not an unpaced CPU-throughput certification.
These two stock views establish reasonable cost for the tested coverage, not a
universal bound for every mission or light count.

This provides useful common material coverage and recoverable CPU lighting
inputs, not calibrated physical lighting throughout the game. Mixed atlases,
unsupported lighting, directional sky/environment specular and RT hit-material
parity still need their own evidence/design. RTGI remains parked.
