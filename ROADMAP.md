# CWRR Rendering Roadmap

CWRR is an independent community Vulkan rendering project based on the official *Arma: Cold War Assault Remastered* (CWR 3.05) source. This roadmap covers **graphics work**, not engine releases or game content.

This is a direction, **not a timetable or a promise**. Priorities may change, and features can be dropped if their real-gameplay benefit does not justify the cost. The original GL33 renderer remains available; new Vulkan effects are opt-in.

## Implemented

- **Vulkan foundation:** native and software world rendering, textures, transparency, sky/weather integration, existing engine lighting and fog, and gamma handling.
- **Image quality:** FXAA, SMAA, MSAA, TAA, AMD FSR 3.1.4 temporal upscaling, and world render scaling.
- **HDR and atmosphere:** FP16 linear-HDR world rendering, tone mapping, bloom, atmospheric perspective, screen-space sunlight shafts, and CSM-shadowed volumetric sunlight/fog.
- **Sunlight shadows:** cascaded shadow maps (CSM), contact-hardening soft CSM filtering, short screen-space contact shadows, and hybrid hardware ray-query sunlight shadows.
- **Soft RT sunlight:** verified default-off four-ray solar-disc sampling in the shared RT scene, with geometric contact hardening, world-anchored sampling and unchanged conservative composition. Independent of CSM PCSS; requires explicit RT sunlight enablement. Malden/Everon captures, AA/effect checks and RTX 4060 Ti 1080p/1440p costs are documented; fine four-ray grain and existing RT coverage limits remain.
- **Shadowed local lights:** verified optional shadows for up to two existing nearby spotlights/headlights, with 1024x1024 perspective maps, PCF and bounded live casters. Default Off, no RT requirement; Malden/Everon geometry-cast shadows, AA/effect coexistence and 1080p/1440p costs are documented. Point lights and stock emissive beam meshes remain unshadowed.
- **Cloud shadows:** completed weather-synchronized stock-cloud coverage of direct sunlight, including CSM volumetric sunlight; independent default-off control, no RT requirement.
- **Volumetric clouds:** verified optional half-resolution, three-layer density/scattering from stock weather; replaces the visible flat clouds, shares cloud-shadow coverage and water sky-reflection shading. Default Off, independent controls, no RT requirement for visible clouds; soft detail and stock background ordering remain deliberate limits.
- **Occlusion:** SSAO and optional hardware ray-traced ambient occlusion (RTAO).
- **Dynamic RT geometry:** verified default-off nearby vehicle and on-foot soldier coverage in the shared AO, sunlight and textured water ray-query scene. Reuses live animation and fence-safe bounded BLAS/TLAS updates; Malden/Everon evidence and 1080p/1440p costs are documented. Crew/equipment proxies, blended effects and destroyed wrecks remain excluded.
- **Water:** textured hardware ray-query reflections and depth-aware coastal refraction/absorption, both independently selectable. Sky/cloud reflections milestone complete: optional directional stock-sky and moving-cloud sampling on water-ray misses.
- **Rain ripples:** verified optional world-anchored impact rings driven by actual rain and simulation time, sharing the existing water normal with ordinary shading, RT/sky reflections and coastal refraction. Default Off, no RT requirement; Malden/Everon weather, AA, pause and dry-parity checks plus RTX 4060 Ti 1080p/1440p costs are documented. Fine detail fades with distance; stock waves and shoreline geometry remain.
- **Shoreline foam:** verified optional broken, softly animated patches at shallow terrain coasts, reusing the existing water capture/composition with independent refraction selection. Default Off, no RT requirement; Malden/Everon beach, rock, bay, weather, AA and preservation checks plus RTX 4060 Ti 1080p/1440p costs are documented. Missing depth, grazing views and steep contacts conservatively omit foam; stock geometry remains unchanged.
- **Wet surfaces:** completed optional rain-driven darkening and restrained sunlight sheen for reliably classified roads and ground, using stock surface identities and simulation time. Default Off, no RT requirement; grass/foliage, known mixed grass tiles and unclassified building/vehicle atlases are excluded. Malden/Everon weather, lighting, AA, restoration and RTX 4060 Ti 1080p/1440p costs are documented.
- **Underwater rendering:** completed optional depth-dependent absorption/scattering in the existing world composite, driven by actual animated sea level and stock lighting/fog. Default Off, independent of wet surfaces, no RT requirement; UI and above-water views remain unchanged. Malden/Everon depth, crossing, weather, AA and combined-operation checks plus 1080p/1440p costs are documented. Stock one-sided water geometry and transparent-depth limitations remain.

HDR10 surface negotiation and SDR fallback exist, but **physical HDR10 display output is not yet verified**. See the [development notes](docs/vulkan-foundation.md) for the actual coverage, limitations, performance and verification of individual features.

## Final major experiments

- **Physically based material rendering (PBR):** investigate material response and texture/roughness inputs before attempting an asset-wide conversion. The stock game does not supply a modern PBR material set.
- **Ray-traced global illumination (RTGI):** explore indirect bounce lighting only after the scene and material representation can support useful hit shading, acceptable performance and stable temporal behavior.

These are intentionally **last**, not prerequisites for the nearer-term milestones. Non-water reflective materials belong with the material work rather than a premature general reflection system.

## Working principles

- Preserve stock missions, weather and visibility behavior, game data, the GL33 backend and ordinary rendering when an effect is disabled.
- Extend existing Vulkan passes, CSM and shared ray-query geometry where practical; avoid new frameworks for a single effect.
- Validate visually in real Malden/Everon gameplay, including movement, relevant AA modes and feature combinations. Measure GPU/CPU cost where it matters.
- Keep [docs/vulkan-foundation.md](docs/vulkan-foundation.md) as the detailed implementation and test record; this file stays short.

**Private NVIDIA DLSS/DLAA work is not a public roadmap deliverable.** Redistribution of the NGX integration remains unresolved under the applicable licensing terms.
