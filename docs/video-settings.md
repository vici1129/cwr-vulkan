# Video settings

Options > **Video Settings** is one scrollable notebook page for both renderers.
The old Display/Graphics IDCs and original Video entry point route here too.
Mouse, keyboard and controller use the existing notebook navigation; section
headings are skipped by focus and Cancel stays pinned at the bottom.

Selections are staged, not applied while browsing. **Apply** activates and saves;
**OK** does the same and returns; **Cancel**, Escape or controller Back discards
unapplied edits. Display changes use the existing 15-second Keep/Revert modal.
Revert, Escape and timeout restore the previous display without committing the
other staged settings. Failed activation is not saved. A write failure is shown
explicitly and Apply can retry.

Exclusive-fullscreen monitor changes start at the target output's Native
resolution/default refresh, rather than reusing a mode owned by the old monitor.
This also applies when returning to the original monitor before Apply.
An ordinary window move or resize preserves staged gamma/FPS and other non-display edits;
it does not replace a deliberately staged mode/monitor/window-size selection.
If saving fails, subsequent edits, Retry and Cancel compare against the last
live-applied values, separately from the saved rollback values.

## Consolidation audit

| Previous option/path | Unified behavior |
|---|---|
| Display page monitor, mode, resolution, refresh | Kept in DISPLAY. Borderless uses desktop size; refresh is editable only in exclusive fullscreen. Mode/resolution/refresh requests are checked against the active renderer before saving. |
| VSync, frame cap, brightness, gamma on Graphics and original Video | One copy in DISPLAY. Brightness/gamma retain their original ranges. Unsupported adaptive VSync falls back to On and reports the effective interval. |
| Aspect handling and HUD width limit | Kept in DISPLAY. Original stretched HUD disables the irrelevant width clamp. |
| Old AA and supersampling | One AA/upscaling selector and a short render-scale selector in IMAGE QUALITY, adapted to the renderer's capabilities. |
| Gameplay and original Video visibility controls | One preferred View Distance and Respect Mission Visibility pair in IMAGE QUALITY. The mission-limit policy and existing `game.cfg`/profile sinks are unchanged. |
| Terrain and object detail | Kept as actual grid spacing and LOD-bias values, not invented effect quality tiers. The finest 3.125 m terrain setting retains its original compatibility warning. |
| Legacy frame-rate / visual-quality sliders | Hidden in ordinary Video: modern frame cap, LOD bias and detail controls replace that presentation. Their engine/profile behavior is not deleted; the compatibility dialog still supports them. |
| Vulkan Overall/shadow/cloud/water/AO | Kept together in GRAPHICS with the [validated mappings](graphics-quality.md). Presets never change display, AA, view distance, scene detail or particles. |
| Original object/vehicle shadow toggles | GL33 has two meaningful independent On/Off controls. Vulkan has its grouped Shadow Quality only; its temporary caster gates never overwrite saved GL33 preferences. |
| Particle/smoke tiers | One On/Off control for original cloudlets, smoke, dust and muzzle flashes. The old intermediate tier had no distinct behavior. |
| Multitexturing | Detail Textures on GL33 only. Vulkan does not expose this legacy compatibility switch. |
| Hardware T&L and W-buffer | Hidden from ordinary Video; the original implementations remain available to legacy/mod code. GL33 reports no W-buffer capability and HWTL was not an interactive toggle. |
| Blood | Remains under Gameplay, with language and subtitle options. |
| Graphics -> Display & detail -> original Video | Removed from ordinary navigation. The old overlapping Graphics and Vulkan Quality page implementations are removed. Display mode-list/switching code is reused, not replaced with a second settings manager. |

## Anti-aliasing, upscaling and scale

Windowed resolution values are logical window sizes; display scaling can make
the drawable pixel extent larger. Capture, persistence and rollback use the
window setter's logical units, not the scaled drawable size. Exclusive modes use
their actual pixel resolution. This does not alter rendering or OS DPI policy.

Vulkan offers Off, FXAA, SMAA, TAA and only the MSAA counts supported by the
device's scene-color/depth formats (2x/4x/8x where supported). TAA also requires
independent attachment blending. Native render-scale choices are **100%, 125%,
150% and 200%**, filtered against framebuffer limits.

AMD **FSR 3.1.4 Native AA, Quality, Balanced and Performance** appear only when
the public FSR build and renderer requirements are
available. Otherwise the AA hint explains the unavailable capability. Selecting
FSR sets independent scale to 100% and disables that control: FSR owns its
internal input resolution, not the HUD/output resolution. Leaving FSR re-enables
independent scale. No dynamic resolution, frame generation or private DLSS is
included. AA/scale changes share the existing deferred renderer rebuild boundary.

GL33 keeps its existing supported MSAA and 100/125/150/175/200% supersampling
implementation. Unsupported Vulkan-only modes are not presented.

## Vulkan water and lighting

Water Quality offers **Standard** (original water), **Enhanced** (existing
refraction, shoreline foam and rain ripples), and **Ray Traced** (Enhanced plus
the corrected hardware reflections, weather-synchronized sky/cloud reflection
fallback and supported dynamic geometry). Ray Traced is an explicit component
choice and makes Overall Quality **Custom**. Water and sunlight RT are independent:
either may retain the shared bounded scene/dynamic resources while the other is
disabled. Returning to Enhanced or Standard disables RT water; Standard restores
the original water path. No ray budget, shader, denoiser or reflection algorithm
is changed by this menu integration.

**Volumetric Lighting: Off / On** uses the corrected existing volumetric sunlight
implementation. It defaults **Off**, retains weather/sun/fog/night-vision
eligibility, and is independent of Overall Quality, like AA/upscaling. Switching
Standard/Enhanced preserves this preference; neither preset automatically turns
it on. No sample-count, intensity or extra quality controls are exposed.

Standard still means Standard shadows/clouds/water and AO Off; Enhanced still
means Enhanced shadows/clouds/water and AO On. Neither selects hardware RT.

## Persistence and capabilities

The existing `graphics.cfg`, `display.cfg`, `game.cfg` and profile handling remain
the only stores. Optional `vulkanAA` and `vulkanRenderScale` fields extend the
existing Vulkan fields; old files default to Off/100%. `vulkanWater=2` remembers
Ray Traced water; `vulkanVolumetricLighting` stores the independent lighting
preference (0 by default). GL33 preserves both without applying them. GL33's `msaaSamples`,
`renderScale`, `shadowQuality` and per-profile caster flags remain independent.
Vulkan AA is requested during the normal graphics boot path before ordinary
world rendering. The UI observes the active renderer after deferred activation,
including FSR fallback, rather than saving a rejected request as successful.

Object detail offers the engine's existing 0.5x/0.75x/1x/2x LOD-size multipliers.
An explicit choice is loaded when each Scene is created. Old configs without
`objectLodConfigured` retain their original 1x scene default; applying unrelated
settings does not opt them into an old auto-detected tier.

Vulkan automatically detects and initializes supported ray-query and FSR device
capabilities at startup. No environment variables are needed. This makes Ray
Traced Shadows/Water and supported FSR modes selectable; it does not enable any
effect or reconstruction until selected or restored from saved preferences.
RT pipelines, acceleration structures and targets remain allocated on demand
only while an RT consumer needs them; disabling one consumer preserves the scene
needed by another. FSR contexts likewise follow the selected AA mode.
Unsupported loaders/devices, builds without FSR and optional initialization
failures retain ordinary rendering and explain the unavailable capability.
Saved but unavailable RT/FSR choices remain remembered while the page shows the
actual fallback. Applying an unrelated gamma/FPS/detail change does not erase
them. This preservation is component-specific: changing lighting, AO or water
does not overwrite an unavailable saved RT-shadow preference, or vice versa.
Explicitly selecting and applying an available alternative replaces that
remembered choice; applying a preset explicitly accepts both raster components.
Browsing and Cancel do not change rendering resources.

Atmosphere, supported-material PBR, classified wet surfaces and underwater optics
remain automatic in Vulkan. Bloom, RTGI, RTAO and screen-space shafts remain
hidden and are not enabled by presets. Developer
commands remain available; clean pages observe external AA/quality/gamma changes
and ordinary window resizing without overwriting staged edits.

## Community UI compatibility

No original PBOs, addon assets or mission UI resources are edited. The existing
`RscOptionsShell`/scroll-list resource classes and notebook styling are reused.
New text uses normal localization keys with an English fallback. Existing
translated labels are reused where their meaning is unchanged.

An addon-owned replacement `RscDisplayOptionsVideo` keeps the original dialog.
Mods replacing their entire bin resource without addon ownership may set
`cwrLegacyVideo = 1;` on `RscDisplayOptionsVideo` to request that fallback. A
missing OptionsShell resource also falls back. This is a resource compatibility
policy, not a player-facing second menu. Existing original-dialog code and mod
control IDs remain intact.

See the [verification record](evidence/graphics-menu/README.md) for build,
regression and direct gameplay checks, and the
[automatic startup checks](evidence/automatic-capabilities/README.md) for RT/FSR
detection, saved preferences and unsupported-feature fallback.

Developer commands remain unchanged. For diagnostics, `CWR_VK_RTAO=0` disables
shared ray-query initialization and `CWR_VK_FSR=0` disables FSR initialization
for that process. The legacy value `1` still requests normal detection. Removing
a disabling override requires a restart: the menu cannot hot-enable device
features omitted at startup. Neither override edits remembered preferences.
