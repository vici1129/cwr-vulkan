# CWRR Rendering Roadmap

CWRR is an independent community Vulkan rendering project based on the official *Arma: Cold War Assault Remastered* (CWR 3.05) source. This roadmap covers **graphics work**, not engine releases or game content.

This is a direction, **not a timetable or a promise**. Priorities may change, and features can be dropped if their real-gameplay benefit does not justify the cost. The original GL33 renderer remains available; new Vulkan effects are opt-in.

## Implemented

- **Vulkan foundation:** native and software world rendering, textures, transparency, sky/weather integration, existing engine lighting and fog, and gamma handling.
- **Image quality:** FXAA, SMAA, MSAA, TAA, AMD FSR 3.1.4 temporal upscaling, and world render scaling.
- **HDR and atmosphere:** FP16 linear-HDR world rendering, tone mapping, bloom, atmospheric perspective, screen-space sunlight shafts, and CSM-shadowed volumetric sunlight/fog.
- **Sunlight shadows:** cascaded shadow maps (CSM), contact-hardening soft CSM filtering, short screen-space contact shadows, and hybrid hardware ray-query sunlight shadows.
- **Occlusion:** SSAO and optional hardware ray-traced ambient occlusion (RTAO).
- **Water:** textured hardware ray-query reflections and depth-aware coastal refraction/absorption, both independently selectable.

HDR10 surface negotiation and SDR fallback exist, but **physical HDR10 display output is not yet verified**. See the [development notes](docs/vulkan-foundation.md) for the actual coverage, limitations, performance and verification of individual features.

## Next

### 1. Cloud shadows

Use the existing moving cloud/weather information to modulate **direct sunlight** across terrain and objects. Keep the effect consistent with time of day, overcast, CSM and volumetric sunlight. Do not replace the stock cloud renderer for this milestone.

### 2. Sky and cloud reflections on water

Improve water-ray misses beyond the current horizon/zenith gradient, reflecting the game's actual weather sky and cloud appearance where feasible. Preserve the original animated water, refraction and restrained reflection strength.

## Later candidates

These are candidates, **not approved implementation commitments**.

| Feature | Intended scope |
| --- | --- |
| Volumetric clouds | Explore a convincing optional 3D-cloud treatment compatible with existing weather and sky. Only proceed if the visual gain and cost are worthwhile. |
| Shadowed local lights | Start with a bounded number of important night spotlights/headlights, not every dynamic light. |
| Dynamic RT casters | Investigate moving soldiers, vehicles, animation and damage in the existing ray-query scene without unbounded AS rebuilding. |
| Soft RT sunlight shadows | Explore penumbrae for RT-only silhouettes; current PCSS softens CSM, **not** the single-ray RT result. |
| Targeted weather/water polish | Rain ripples, shoreline foam, underwater effects or wet surfaces only where stock assets and gameplay justify them. |
| Optional presentation features | Depth of field, motion blur, color grading and dynamic resolution only if there is a clear use case. |

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
