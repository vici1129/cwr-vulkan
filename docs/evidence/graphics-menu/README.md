# Vulkan graphics menu verification (2026-10-18)

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

Remaining focused follow-ups: player-friendly RT device startup opt-in, Vulkan
AA/upscaling/render-scale UI modernization, and localization of the new English
labels. The existing RT-water transition and volumetric road-banding visual gates
remain unresolved and neither feature is selected by either preset.
