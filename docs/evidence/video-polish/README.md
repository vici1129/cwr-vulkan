# Video Settings localization and polish

2026-10-19, public main based on `c217568`. Windows, RTX 4060 Ti, stock local
Remastered missions; no original assets, ordinary profiles or private DLSS
files changed. Both public renderers built with `build/local-build.ps1`.

## Changes checked

- New Video strings are executable-owned UTF-8 CSV resources, loaded by the
  existing stringtable parser after mission/campaign/global tables. Eight
  supported languages, missing-key/empty-translation/unknown-language English
  fallback and mod string priority are covered by focused tests.
- Concise localized help/status/errors; Object Detail describes exactly
  0.5x / 0.75x / 1x / 2x. FSR choices omit the integration version number.
  Unsupported-device, missing-build, initialization and diagnostic-disable
  explanations are distinct. No player instructions to set environment variables.
- Video-only font/geometry-aware cell fitting and UTF-8 marquee preserve the
  original notebook layout, resource ownership and other pages' sizing policy.
  Returning from confirmation refreshes localized Native/Default choices.

## Focused tests

Both executables pass **1,604 assertions in 139 cases**:

```text
[VideoPage],[DisplayPage],[ConfirmRevertPage],[optionsUI],[GraphicsConfig],
[DisplayConfig],[ShadowIsolation],[stringtable],[controller-ui],
[WindowMetrics],[WindowPlacement]
```

See [Vulkan output](vulkan-tests.txt) and [GL33 output](gl33-tests.txt).
Coverage includes staged/active values, renderer-specific controls, independent
presets/lighting, unavailable preference retention, failed activation/save,
display confirmation/timeout and controller layout policy.

## Direct real-mission checks

Exact executable/PID targeting, muted `--nosound` launches, isolated copied
profiles, no splash, local test missions and the existing debug harness were
used. Normal Vulkan/GL33 runs explicitly unset both optional startup variables.
Menu selections, persistence and renderer completion counters were observed;
physical scan-code movement changed the real mission player's position.

| Run under `build/shadow-live/` | Result | Submitted / presented | Validation errors / warnings |
|---|---|---:|---:|
| `video-polish-release-malden-20261019` | Eight languages via Game Settings, input, RT/FSR, display/save flows, player movement | 23,375 / 23,375 | 0 / 0 |
| `video-polish-release-everon-20261019` | Saved FSR Quality + RT shadows/water restored, player movement | 1,893 / 1,893 | 0 / 0 |
| `video-polish-release-gl33-20261019` | GL33-only controls, saved Vulkan choices preserved, player movement | n/a | n/a |
| `video-polish-release-unavailable-20261019` | Diagnostic disables, raster fallback, unrelated save preserves unavailable choices, player movement | 3,505 / 3,505 | 0 / 0 |

Malden's FSR counter reached 149 completed dispatches at the coast; sunlight RT
reported completed ray queries and water reported completed textured hit shading.
The unavailable run deliberately set both overrides to zero; the other runs
required neither override. No second physical unsupported GPU was available;
driver-free unavailable/build/failure tests supplement that diagnostic run.
Khronos and synchronization validation were enabled. Existing host loader
notices about the stale EOS overlay manifest/filtered implicit layers are not
renderer validation errors; system layer configuration was not changed.

Live interaction checks included:

- English/French/Italian/Spanish/German/Czech/Polish/Russian through the ordinary
  Game Settings language picker, returning to Video after each selection.
  Cyrillic/accented text, scrolling and long focused labels were inspected.
- Native keyboard Down, controller-adapter D-pad value changes/Back, pinned
  Cancel, and native mouse-button change of FPS Cap with a geometry-targeted
  in-game cursor. No physical controller was connected; adapter injection and
  focused controller tests are not a physical hardware check.
- Apply, OK/save-and-return, Cancel/discard; exclusive fullscreen on monitors
  0/1/0, Keep, windowed restoration, explicit Revert and countdown timeout.
- A native resize with staged gamma/FPS, and a deliberately blocked isolated
  display writer: accurate error, successful retry and correct Cancel rollback.
  The first save-flow attempt was interrupted by overlapping test input and
  rejected; the complete save-flow rerun passed without concurrent input.
- 1920x1080 and 1280x720 notebook views. No independent notebook UI-scale setting
  exists; alternate OS DPI percentages were not tested or changed.

Developer-only limitation: switching language through the harness while Video
is already open can leave some unchanged values visually stale on Vulkan until
the page is reopened. Semantic values remain correct. The actual player picker
is on another page, and all eight normal close/change/reopen routes passed.
No renderer changes were made to address this diagnostic-only path.

## Runtime identity and captures

Vulkan SHA256:
`FED137062AB418EE88151E463A5662608E1F5799307F528E04CFB58D330BB884`

GL33 SHA256:
`40F83204C8788B333A18E40EE00F127CE8013D93E0D3A28D66B4ED792BE6F7D9`

Raw session receipts, action logs, profiles and additional captures remain in
the ignored run directories above. Published unedited captures:

- [Russian Video Settings at 1080p](menu-russian.png)
- [FSR Quality at 1080p](menu-fsr.png)
- [GL33 controls at 1080p](menu-gl33.png)
- [Display confirmation](display-confirmation.png)
- [Real Everon gameplay](everon-gameplay.png)

No shaders, rendering algorithms, preset mappings or GL33 renderer behavior were
changed. These are focused UI checks, not a new renderer performance campaign
or a claim of full release qualification.
