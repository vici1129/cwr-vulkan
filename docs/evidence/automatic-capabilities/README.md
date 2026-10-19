# Automatic Vulkan RT/FSR capabilities (2026-10-19)

Based on current public main `3c28381`. Normal Vulkan startup now detects the
existing ray-query and public FSR 3.1.4 capabilities. Device features are enabled
before device creation; effects and reconstruction still require a player
selection or an existing saved preference. No settings store, launcher, shader,
rendering algorithm or GL33 implementation was added or changed.

`CWR_VK_RTAO=0` and `CWR_VK_FSR=0` remain process-local diagnostic disables.
Legacy `=1` still works. Removing a disable requires restart. Optional device
creation retries supported combinations and then the original raster features;
failure to initialize RT entry points/limits cannot abort ordinary rendering.
Existing lazy scene/pipeline/target creation and shared consumer ownership remain.

## Builds and focused checks

Both `RelWithDebInfo` public builds passed through `build/local-build.ps1` with
current caches in `build/vulkan-local` and `build/gl33-local`. Vulkan includes
the existing public FidelityFX SDK integration; GL33 does not link it.

- Vulkan: **1,718 assertions / 105 cases**. [Output](vulkan-tests.txt).
- GL33: **1,678 assertions / 104 cases**. [Output](gl33-tests.txt).
- Driver-free Vulkan policy/lifetime: **624 checks**. [Output](policy-tests.txt).
  New startup checks cover absent variables, legacy `=1`, independent `=0`
  overrides, old loaders, missing RT extensions, missing FSR features, FSR on
  non-RT hardware, and device-creation failures affecting RT, FSR or both. They
  inspect actual feature chains, including device-address structure uniqueness,
  and verify no effects/reconstruction are selected by capability detection.
- The same driver-free sources built separately with FSR compiled out:
  **566 checks passed**. [Output](no-fsr-policy-tests.txt). This links only the
  Vulkan loader, without the FidelityFX library.
- Video/config tests cover unsupported-choice explanations, saved preferences,
  deferred activation, RT-consumer independence, raster presets, independent
  volumetric lighting, failed writes and GL33 settings isolation.

Focused Catch2 filter:
`[VideoPage],[vulkan-shape],[ShadowIsolation],[GraphicsConfig],[vulkan-aa],[WindowMetrics],[WindowPlacement]`.
The policy suite deliberately injects a teardown validation error and resource
failures; these synthetic test messages are not live Vulkan findings.

## Real mission checks

Windows, RTX 4060 Ti, driver 617.14, 1280x720 windowed SDR. Direct diagnostic
launches used stock `01TakeTheCar.ABEL` (Malden) and `05HeavyMetal.Eden` (Everon),
isolated copied profiles, `--nosound`, the existing developer harness and exact
window keyboard input. These are short gameplay checks, not mission completion
or Steam-route qualification. Screenshots are unretouched window captures.

Both startup variables were explicitly absent from each normal child process.
No profile or original game asset was edited in place.

| Check | Observed result |
|---|---|
| Fresh normal Vulkan launch | RT and FSR available, all RT effects and AA Off; zero BLAS/TLAS builds, rays, FSR dispatches and GI history allocations before and after raster gameplay. |
| Video Settings selection | Ray Traced Shadows and Water selectable; FSR Quality saved and active at 853x480 input / 1280x720 output, with completed FSR dispatches. |
| Malden coast | Actual RT sunlight queries and textured water hits with FSR Quality; both effects also active after native player movement. |
| Shared scene | Shadows Off leaves water dispatching; water Off leaves sunlight dispatching. Dynamic/shared scene remains active for the other consumer. |
| Presets and lighting | Standard/Enhanced retain their mappings and preserve independent Volumetric Lighting On. Neither preset selects RT or changes AA. |
| Restart on Everon | Saved RT shadows/water and FSR Quality activate before opening Video Settings; coast rendering and native player movement succeed. |
| Both diagnostic overrides `=0` | Enhanced raster and AA Off; explicit disable/restart reasons; unrelated FPS Apply preserves saved `vulkanShadows=2`, `vulkanWater=2`, `vulkanAA="fsr-quality"`. |
| GL33 with that copied profile | GL33 menu and native Malden movement work; Vulkan preferences remain remembered. |
| Remove overrides and restart after GL33 | All three saved Vulkan choices reactivate, including FSR dispatch and RT water hits on Everon. |

Accepted renderer status samples and validation summaries are in
[states.json](states.json). Physical unsupported-GPU coverage is represented by
driver-free capability/failure tests and the explicit live disable overrides;
no second, non-RT GPU was available for a hardware run.

| Vulkan run | Submitted / presented | Validation errors / warnings |
|---|---:|---:|
| Malden menu/consumer/preset/movement checks | 13,053 / 13,053 | 0 / 0 |
| Everon saved-settings restart | 7,686 / 7,686 | 0 / 0 |
| Diagnostic disables, saved-choice preservation | 2,361 / 2,361 | 0 / 0 |
| Final runtime, restore after GL33 | 2,725 / 2,725 | 0 / 0 |
| Final runtime, fresh raster gameplay | 3,111 / 3,111 | 0 / 0 |

Khronos validation and synchronization validation were enabled for all Vulkan
runs. Existing host loader messages about a stale EOS overlay manifest and
deliberately filtered implicit layers remain; they are distinct from the zero
renderer validation counts. No system layer configuration was changed.

Final Vulkan runtime SHA256:
`B14898A1BB0EE32552468E988C325D3D7601D98798E88B7A25299936B71B001E`.
The final rebuild additionally removes obsolete developer status wording; the
startup negotiation and selection paths are the same ones exercised throughout.
Full local receipts/logs/captures remain under `build/shadow-live/startup-*-20261019`.

## Captures

- [Ray Traced shadows and water on a normal launch](menu-rt.png).
- [FSR Quality restored with both variables unset](menu-fsr.png).
- [Everon RT water with FSR Quality](everon-coast.png).
- [Independent GL33 Video Settings](menu-gl33.png).
