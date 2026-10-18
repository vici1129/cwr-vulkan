# Vulkan graphics menu verification (2026-10-18)

The later [final graphics-controls verification](../graphics-controls/README.md)
adds qualified RT water and corrected volumetric lighting to this same page.
The dated results below describe their respective implementation snapshots.

Implemented from public main `e6fc59c`, using the mappings in
[graphics-quality.md](../../graphics-quality.md). No shader/algorithm changes,
original PBO edits, private SDK changes or performance campaign.

## Automated checks

- Vulkan and GL33 `RelWithDebInfo` builds, using the current local public build
  helper and fresh/current caches. Vulkan includes the existing public FSR build.
- Vulkan focused Catch2: **2,070 assertions / 112 cases**, covering new menu and
  persistence tests, existing graphics UI/configuration, Vulkan shape/AA and
  shadow math. [Output](vulkan-tests.txt).
- GL33 focused Catch2: **469 assertions / 41 cases**. [Output](gl33-tests.txt).
- Vulkan driver-free policy/lifetime: **517 checks**. Its intentional synthetic
  resource/validation failures are separate from live validation results.
  [Output](policy-tests.txt).
- Tests exercise staging without renderer calls, preset/Custom recognition,
  stepper wrapping, AO/group independence, RT capability gating, complete Apply
  requests, rejection/actual-state display, configuration round-trip and failed
  writes. The shared settings writer now checks the actual save result, including
  when an old file already exists.

## Live checks

Direct diagnostic launches, stock GOG data, isolated copied profiles, `--nosound`,
existing developer harness and exact-window input. These are practical mission
smoke checks, not Steam-route acceptance or mission completion. Screenshots are
unretouched window captures.

1. Malden `01TakeTheCar.ABEL`: boot the old profile with legacy Shadows Off;
   verify Standard, four 2048-square CSM cascades and **24,078 caster vertices**
   in a completed frame window. Select Enhanced; independently Apply each shadow,
   cloud, water and AO change, recognize Custom and then exact Enhanced again.
   Stage Standard and Cancel; reopen to confirm Enhanced and unchanged saved
   fields. With RT bootstrap absent, the selector offers raster choices and a
   direct developer RT request returns the restart requirement. Resume Enhanced
   gameplay and verify native keyboard movement and active volumetric clouds.
   [Live assertions/status](malden-menu.json).
2. Normal shutdown and restart using those saved graphics/profile files on
   Everon `05HeavyMetal.Eden`: Enhanced is loaded before gameplay, cloud rendering
   and water capture/composition are active. Repeat grouped Apply/Cancel tests;
   explicitly select Ray Traced, save it, return to raster, and save again.
   [Live assertions/status](everon-menu.json).
3. Resume the mission with Ray Traced selected: soft sunlight reports **four
   samples**, executed queries and dynamic geometry hits. A one-second native
   keyboard movement changes the player position. Return through the menu to
   Enhanced raster shadows and resume the mission again. Both raster and RT
   retain CSM and the supported geometry/environment rules.
4. An additional restart with a saved Ray Traced preference and no startup opt-in
   shows actual Enhanced raster shadows, preserves the saved RT preference until
   explicit OK/Apply, and reports `CWR_VK_RTAO=1` honestly. Standard/Enhanced
   never enable bloom, volumetric sunlight, shafts, RT water, RTGI or RTAO.
5. Developer interoperability: explicitly enable RT water, dynamic geometry and
   bloom, then change RT sunlight back to raster through the menu. RT water,
   its dynamic scene and bloom remain enabled; restore the developer-only
   features afterward. The ordinary preset trials leave all hidden effects Off.
6. The original Video panel is reachable through Display & detail > Video
   options; resolution, refresh, view distance, terrain, gamma/brightness and
   existing mod resource controls remain. GL33 opens its original Graphics page
   and rejects Vulkan effect requests; no Vulkan panel is presented.

Final Vulkan runtime SHA-256:
`44EF86F370362677912A6B2ACCAEF8088831DD7EA01A6DDBE008C23F267529BF`.
Final run logs are retained locally under `build/shadow-live/menu-verified-malden`
and `menu-verified-everon`. Earlier migration/restart/GL33 trials use the
`menu-*` evidence folders alongside them. No user profile is checked into Git.
The final Malden run submitted/presented **4,112 / 4,112** frames; final Everon
**6,828 / 6,828**. Both shut down normally with **zero Vulkan core/synchronization
validation errors and warnings**.

## Screenshots

Nested Video return regression fix: the parent now merges overlapping live
settings when its Video child closes, after either OK or Cancel. Vulkan/GL33
builds and **485 focused assertions / 46 cases** pass on both builds. An isolated
live Malden check retains gamma 1.1 and brightness 1.7 after OK, a later FPS-cap
change and parent closure; Cancel discards a gamma 1.2 preview. Restart confirms
gamma 1.1, brightness 1.7 and a 30 FPS cap. Local logs: `video-return-fix` and
`video-return-restart` under `build/shadow-live/`.

- [Standard on Malden](standard.png)
- [Enhanced applied on Everon](enhanced.png)
- [Explicit Ray Traced component on Everon](ray-traced.png)
- [No RT capability and restart hint](no-rt.png)

Remaining focused follow-ups: player-friendly RT/FSR device startup opt-in and
additional translations for the new English-fallback labels. AA/upscaling/scale
UI modernization is complete below. The subsequent RT-water transition and
volumetric road-banding corrections are now qualified through the linked final
integration; neither feature is automatically enabled by either preset.

## Shadow preference isolation regression (2026-10-18)

Vulkan's CSM caster gates are now process-local. Quality Apply/OK leaves the
legacy shadow tier untouched; the detail page, shared live-apply path and Scene profile
save no longer copy Vulkan gates into GL33 preferences. No config schema or
renderer algorithm changed. The nested Video return merge remains in place.

- Vulkan and GL33 `RelWithDebInfo` builds passed. Each focused configuration/UI
  run passed **569 assertions / 49 cases**; Vulkan driver-free policy/lifetime
  guards passed **517 checks**. Existing Vulkan shape/AA and shadow math passed
  **1,601 assertions / 71 cases**. New tests exercise the actual Scene load/save,
  GraphicsApply, detail Mount/child-return/Unmount and quality Apply/OK/Cancel
  paths, including mixed flags, absent old-profile entries and GL33 edits.
- Direct isolated Malden gameplay checks started in GL33 with legacy tier Off
  and both profile gates disabled. Vulkan Standard rendered CSM; Enhanced and
  explicit Ray Traced Apply/OK/Cancel preserved all three legacy preferences.
  Restart restored soft RT sunlight with executed queries, dynamic geometry and
  four 2048-square CSM cascades (25,989 caster vertices in a completed window).
  Returning to Enhanced stopped RT world dispatch and retained CSM. Developer
  shadow-map disable/enable commands still changed the active runtime state.
- In nested Video, accepted gamma 1.1 and temporary caster edits survived later
  FPS-cap changes; Cancel discarded the gamma 1.2/caster preview. Legacy saved
  shadow fields remained Off, and restart retained gamma 1.1 and FPS cap 30.
- Returning to GL33 still showed Shadows Off. Changing its menu to Medium saved
  legacy tier 2 and both profile flags On; another GL33 restart showed Medium.
  These were copied test profiles, not changes to the player's original profile.
- Both Vulkan sessions shut down normally with **zero validation errors and
  warnings**. Local logs/captures are under `build/shadow-live/isolation-*`;
  build/test output is under `build/shadow-isolation-*.log`.

Tested runtime SHA-256: Vulkan
`277841ABFED4559A6B1557E901EB42CD7D1F5D4456E8DB2A3AD953EA27CB0962`;
GL33 `8D7363797D440A0CD5AAFAB254FBCD46BAC1E9BD856BBE10EE95039A22809EFD`.

## Unified Video modernization (2026-10-19)

The [Video guide and consolidation audit](../../video-settings.md) describe the
single DISPLAY / IMAGE QUALITY / GRAPHICS page, renderer-specific controls,
capability hints and community-resource fallback. The redundant Graphics and
Vulkan Quality page implementations and ordinary nested Video navigation are
removed. Existing display enumeration/switching and configuration writers are
reused; no rendering algorithms, artistic parameters, game assets or private
DLSS/NGX sources changed.

### Automated checks

- Public Vulkan and GL33 `RelWithDebInfo` builds passed with
  `build/local-build.ps1`. The pre-existing `FileServerMT.hpp` constructor-order
  warning is unchanged.
- Each build passed **742 assertions / 108 cases** across Video, shadow
  isolation, GraphicsConfig, DisplayConfig/Page, Game settings and notebook UI.
  Tests cover staging, exact presets/Custom, grouped changes, capability-filtered
  AA/RT/FSR, scale ownership, actual-state acceptance, deferred completion,
  Apply/OK/Cancel, failed display requests, persistence, independent renderer
  fields, old profiles and mod resource ownership/explicit fallback.
- Failed-writer tests include a blocked display path and a read-only later
  writer: graphics/profile files are restored using temporary in-memory byte
  snapshots, Cancel restores the previous runtime values, and Retry succeeds.
  No backup settings store or new configuration format is introduced.
- Logical-window versus drawable-pixel tests prevent DPI-scaled sizes from
  inflating persistence/rollback. Physical OS DPI settings were not changed.
  An additional regression preserves mod/developer scene detail when only a
  Vulkan effect preset changes.
- Real Scene-construction tests cover explicit object-detail persistence in both
  renderers and preserve the 1x default of old auto-detected configs. The selector
  uses the existing 0.5x/0.75x/1x/2x engine mapping.
- Vulkan shape/AA/shadow math: **1,601 assertions / 71 cases**. Driver-free
  policy/lifetime guards: **517 checks**. Their intentional synthetic failures
  are not live validation errors.
- Existing SQF display/graphics fixtures now target the unified layout. Their
  shared top-level Apply helper was replayed through the existing live harness;
  no new test framework was added. The full CLI/Pester jobs were not run here.

### Direct gameplay and persistence

Stock GOG data, isolated copied profiles, `--nosound`, exact-window input and the
existing harness; these are diagnostic mission checks, not Steam-route acceptance
or mission completion. Malden `01TakeTheCar.ABEL` and Everon `05HeavyMetal.Eden`
both rendered gameplay with the new menu and normal keyboard movement.

- Live AA selections covered Off, FXAA, SMAA, TAA, MSAA 2x/4x/8x and FSR 3.1.4
  Native AA/Quality/Balanced/Performance. Each saved selection matched active AA.
  Native scales 100/125/150/200% worked; selecting FSR restored 100% independent
  scale and disabled that row. FSR dispatches executed during gameplay.
- Standard/Enhanced/Custom, each shadow/cloud/water/AO group, particles, terrain,
  object bias, visibility/mission limits, brightness/gamma and VSync/frame cap
  were exercised. Mouse chevrons, keyboard scrolling, controller left/right and
  controller Back worked. Staging/Cancel did not commit the preview.
- Windowed 1024x768 and restoration to 1280x720 passed on Vulkan and GL33.
  Vulkan exclusive **1280x720 at 120 Hz** matched both requested and active
  display modes. Explicit Revert and the 15-second timeout restored the previous
  mode. Aspect handling/HUD width choices retained their existing behavior.
  Vulkan refresh is now requested after entering exclusive mode at the selected
  size; monitor mode lists use the selected output. The machine had one monitor,
  so no physical multi-monitor move was qualified.
- A saved FSR Quality / Ray Traced configuration restarted on Everon with FSR
  dispatches, soft sunlight queries and dynamic casters active. Without startup
  opt-ins, the page instead showed AA Off / Enhanced raster, excluded unavailable
  choices and rejected developer FSR/RT requests honestly. Applying unrelated FPS
  retained the saved unavailable RT/FSR preferences.
- Vulkan -> GL33 -> Vulkan -> GL33 preserved separate choices. GL33 first retained
  its original shadows Off, then saved **MSAA 4x, scale 175%, objects On and
  vehicles Off**. Vulkan restored FSR independently, retained nonempty CSM,
  applied Ray Traced and returned to raster without overwriting those GL33
  fields. The next GL33 restart restored all four preferences. GL33 detail-texture
  toggles also worked. Developer shadow-map disable/enable remained functional.
- Final exact-binary runs applied and saved successfully, retained the mixed
  GL33 caster preferences, and closed cleanly. Final Vulkan shutdown reported
  **zero validation errors and warnings**. Experimental features were not enabled
  by presets; the existing RTAO status remained Off alongside sunlight RT.
- The final object-detail follow-up saved 2x in Vulkan, restored it after a
  restart and into GL33, then saved 0.75x in GL33 and restored that exact runtime
  bias after GL33 and Vulkan restarts. These last checks used live harness/UI
  state; a new capture attempt was refused on foreground-focus loss, so no
  additional visual/input result is claimed for that attempt.

Local build/test output: `build/video-modernization-*.log`. Local mission logs,
status assertions and captures: `build/shadow-live/video-modernization-*`.
Early raw-script driver attempts (comment parsing / nested SDL-event pumping)
were corrected and excluded from acceptance; one stalled isolated test process
was stopped without an active settings write. Original player profiles, PBOs,
gameplay assets, private repositories/SDKs and persistent power settings were
not modified.

Final Vulkan runtime SHA-256:
`83E3C2F813A7256881134D5D9A28891282ACB602F500D3375EE99D57759BD2FD`.
Final GL33 runtime SHA-256:
`C84A24AFF2AC56A45992C13B208E1BE53201F149AC2E56A0AC690E439D349DAC`.
Final clean run labels: `video-modernization-object-vk`,
`video-modernization-object-vk-restart`, `video-modernization-object-gl33`,
`video-modernization-object-gl33-restart` and
`video-modernization-object-return-vk`.
