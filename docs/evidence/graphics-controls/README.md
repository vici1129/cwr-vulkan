# Final Vulkan graphics controls (2026-10-19)

The later [automatic startup verification](../automatic-capabilities/README.md)
supersedes this snapshot's RT/FSR environment prerequisites. Normal Vulkan
launches now detect supported capabilities automatically.

Implementation starts from public main `d03ed0c`. The [Video Settings guide](../../video-settings.md)
describes the compact player controls. This integration reuses the corrected
[RT-water renderer](../../rt-water-transitions.md) and
[volumetric sunlight](../volumetric-road/README.md); no shaders, sample counts,
ray budgets, acceleration-structure coverage or rendering algorithms change.

## Implementation

- Water Quality 0/1/2 maps to original water, Enhanced refraction/foam/ripples,
  and Enhanced plus RT reflections/weather sky/dynamic geometry. RT water and
  soft RT sunlight are independent; shared resources stay enabled for either
  player consumer and existing developer RTAO/RTGI consumers.
- Volumetric Lighting is one independent boolean in the existing renderer-state
  exchange and `vulkanVolumetricLighting` in `graphics.cfg`. Default Off. Preset
  recognition ignores it and preset selection preserves it, like independent AA.
- Both use the existing staged/deferred apply, actual-state check and config
  transaction. A rejected/partial activation cannot be saved as successful.
  RT capability is negotiated at startup, not inferred from GPU brand. Missing
  capability shows raster choices and the `CWR_VK_RTAO=1` restart reason.
- Restart testing found a lazy-initialization dependency: saved lighting On
  could create the volumetric pipeline before any mesh had created the shared
  lighting descriptor layout. The existing layout initialization is now a small
  idempotent helper called by both consumers; ownership and shutdown are
  unchanged. Initial pre-fix Everon/no-RT runs each reported four layout/pipeline
  validation errors and are retained as diagnosis, not accepted validation.
- Startup fallback and saving are component-specific. An unavailable saved RT
  water/shadow preference survives independent lighting/AO/display changes;
  explicitly applying its raster alternative or a preset replaces it. GL33
  does not apply or overwrite these Vulkan preferences.
- Existing localization keys/fallback lookup and notebook row marquee are used;
  no original assets or community replacement-resource policy changes.

## Automated verification

Public Vulkan and GL33 `RelWithDebInfo` builds use `build/local-build.ps1`.
The focused configuration/UI, shadow-isolation, Vulkan shape/AA, decoder,
factory and window suites pass: **2,895 assertions / 189 cases** on Vulkan and
**2,649 / 185** on GL33. Driver-free Vulkan policy/lifetime guards pass
**520 checks**; their synthetic teardown-validation fixture is not a live GPU
finding. Existing constructor-order compiler warnings are unrelated.

New checks cover all water/shadow combinations, staging, Cancel, OK, independent
lighting/preset recognition, old-file defaults, round-trip persistence, missing
capability, component-specific fallback preservation, GL33 isolation and an
incomplete deferred activation that must restore without saving.

## Live verification

Direct diagnostic launches use the existing stock GOG missions, copied isolated
profiles, `--nosound`, public FSR, RTX 4060 Ti and Vulkan core/synchronization
validation. These are bounded gameplay checks, not mission completion or broad
GPU/mod certification. Captures are unretouched game-window PNGs.

The menu matrix exercises all three Water choices, independent RT shadows,
return to Enhanced/Standard, exact Standard/Enhanced/Custom recognition,
lighting Off/On, staged Cancel, applied OK and normal restart persistence.
Everon also exercises both consumers during actual world rendering: sunlight
queries continue after water is disabled, and water queries/shaded hits continue
after sunlight RT is disabled. Dynamic geometry remains active in both cases.
Both enabled together retain the same bounded scene and supported animated mesh.
An existing developer RTGI consumer also retains dynamic geometry when both
player RT components return to Enhanced; it is disabled again after that check.

Matched RT Off/On coast views, low-height camera sweeps, sunset/overcast and
grounded native keyboard traversal cover Malden shoreline/pier and Everon with
TAA and FSR Quality. The documented correction remains present; original-wave
texture and RT-Off paths are unchanged. Malden's fine 3.125 m road view remains
free of the repeated volumetric terrain-self-shadow bands, and the forest view
retains eligible scattering and terrain shadows. Early paused capture attempts
were excluded and repeated with world-dispatch/scattering evidence.

No new performance campaign was necessary: integration adds no frame rendering
work beyond the already-qualified selected effects. Prior renderer measurements
and bounded-coverage/material limitations remain applicable; no new universal
performance or artifact-free claim is made.

Missing-RT startup restores Enhanced water/shadows in the menu, preserves both
saved RT choices through lighting/AO Apply, and only replaces water after an
explicit Standard/Enhanced Apply. Developer requests report the startup reason.
GL33 exposes neither Vulkan control and preserves RT water/shadows, lighting and
FSR while saving its own FPS preference. Returning to Vulkan restores both RT
components and lighting On; GL33 still rejects Vulkan effect commands.

All final Vulkan runs close normally with **zero core/synchronization validation
errors and warnings**, including saved lighting On before the first mesh draw.
The host's pre-existing loader messages for disabled implicit layers and a stale
overlay manifest are not validation callbacks; no loader/overlay configuration
was changed.

| Accepted final run | Submitted / presented frames | Validation errors / warnings |
|---|---:|---:|
| Malden menu / coast / road / forest / traversal | 16,302 / 16,302 | 0 / 0 |
| Everon restart / coast / traversal / shared consumers | 9,936 / 9,936 | 0 / 0 |
| Missing RT startup / preference preservation | 4,223 / 4,223 | 0 / 0 |
| Return from GL33 / saved RT and lighting restored | 3,462 / 3,462 | 0 / 0 |

See [runtime receipts](runtime-receipts.json), [accepted live status](live-status.json),
[GL33 isolation](gl33-isolation.json), and focused test output for
[Vulkan](vulkan-tests.txt), [GL33](gl33-tests.txt) and [policy guards](policy-tests.txt).

## Captures

- [Applied compact controls](menu.png), [missing RT restart hint](unavailable.png).
- Malden shoreline/pier with TAA: [RT Off](malden-water-off.png), [RT On](malden-water-on.png).
- Everon coastline with FSR Quality: [RT Off](everon-water-off.png), [RT On](everon-water-on.png).
- Corrected [fine-terrain road with volumetrics On](road-volume-on.png) and
  [eligible forest sunlight](forest-volume-on.png).

Local raw logs, receipts, action history and additional captures are retained
under `build/shadow-live/graphics-finalize-*`. The following checked-in evidence
contains the accepted captures/status, not player profiles or private SDK data.
