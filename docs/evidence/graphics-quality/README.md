# Graphics-quality evidence

See [the design report](../../graphics-quality.md) for decisions and limitations.
Source is public main `cdf53e1`; timings use the unchanged baseline executable,
not the stronger candidate. Raw data is from actual stock Malden/Everon missions,
with developer-controlled fixed cameras plus separate real-input checks.

## Images

All PNGs are original window captures, copied byte-for-byte. No crop, retouch,
brightness adjustment or difference amplification. Window chrome and original
HUD remain. [screenshots.json](screenshots.json) records source-run identity and
SHA-256 for each published image. Rejected trials are not accepted improvements.

| Comparison | Baseline / Off | Alternative / On |
|---|---|---|
| Road banding isolation: only volumetric toggle differs | [Volume Off](road-volume-off.png) | [Volume On](road-volume-on.png) |
| Everon coast: Enhanced versus RT stress | [Enhanced](everon-coast-enhanced.png) | [RT](everon-coast-rt.png) |
| Malden village: hardware sunlight, otherwise Standard | [Off](malden-sun-off.png) | [On](malden-sun-on.png) |
| Malden sunset: stock versus volumetric cloud group | [Stock](sunset-clouds-stock.png) | [Volumetric](sunset-clouds-volume.png) |
| Stronger bloom, same frozen session | [Current](bloom-current.png) | [Stronger](bloom-stronger.png) |
| Rejected compiled trial: dawn forest, separate launches | [Baseline](tuning-forest-baseline.png) | [Candidate](tuning-forest-candidate.png) |
| Rejected compiled trial: Everon sunset, separate launches | [Baseline](tuning-sunset-baseline.png) | [Candidate](tuning-sunset-candidate.png) |

[Malden RT water](malden-coast-rt.png) independently shows the reflection
transition seen on Everon. Previous committed shallow-water, local-shadow,
soft-CSM, PBR and wet-surface pairs remain linked from the renderer record;
they are prior evidence, not new screenshots from this pass.

## Data and labels

- `quality-*.json`: complete sampled status strings, profiling intervals and
  GPU clock/power snapshots. Paths in session receipts use checkout placeholders.
- [summary.json](summary.json): median/min/max/count for GPU, CPU wall frame,
  CPU command recording and allocations; available isolated-pass medians too.
  Groups pool the profiler windows of the same configuration; this is not a
  distribution of individual frame latencies or a p95 traversal benchmark.
- `*-pixels.json`: full-window current/stronger/restored numerical comparisons.
- `quality-tuning-*-status.jsonl` and [tuning-state.json](tuning-state.json):
  compiled-trial camera-state receipts and image hashes. Cross-launch differences
  in weather/LOD prevent claiming an exact matched tuning improvement.
- [runtime-receipts.json](runtime-receipts.json): shutdown receipts, build hashes
  and validation requests; the three `quality-validate-*` entries include actual
  commands and before/after player positions. Timing runs deliberately disable
  validation; their zero counters alone are not validation evidence. The
  interrupted Everon 1440p run lacks normal shutdown and was fully repeated.

Final combined sources are `quality-malden-1080-final`,
`quality-malden-1440-final`, `quality-everon-1080-final` and
`quality-everon-1440-resumed`. `quality-aa-1440` supplies the additional AA table.
Isolated effects use `quality-malden-1080b` and `quality-malden-1440-final`.
The former's `volume` sunset experiment has scattering inactive; use its later
`volume-active` rows, never that inactive result, for volumetric cost.
The initial Malden `village` label actually means road; `town` is the village.

`standard` = four automatic features + CSM, AO Off.
`recommended` = Standard + soft/contact/local shadows, volumetric clouds/cloud
shadows, refraction/foam/rain ripples, SSAO On.
`recommended-sun` adds soft RT sunlight and bounded dynamic geometry.
`recommended-ray` additionally adds RT water and weather-sky misses; this is a
stress candidate, **not** an accepted preset. Historical exploratory `enhanced`
and `ray` also enable bloom/volumetrics and are not the final mappings.

## Reproduction recipe

Use an isolated profile and stock game data, current developer harness, Vulkan
with public FSR built, `CWR_VK_PROFILE=1`, `CWR_VK_FSR=1`; add `CWR_VK_RTAO=1`
only for a device initialized with RT capability. Timing startup excludes
`--vk-validation`; separate validation startup includes it. Use a verified
1920x1080 or 2560x1440 drawable, SDR, AA Off, scale 100%, VSync Off, view distance
1400. Keep the existing cap/pacing and automatic GPU clocks unchanged.

Load `01TakeTheCar.ABEL` (Malden) or `05HeavyMetal.Eden` (Everon), not an empty
renderer scene. Use `setAccTime 0`, `setDate [1985,7,5,hour,minute]`,
`triCheatWeather .45`, `0 setFog 0`, `0 setRain 0`, and `triSetView` below.
Camera arrays are engine X/up/Z followed by direction; script `camCreate` uses
X/Z/AGL instead. Do not teleport the player for the fixed-view sweep: mission
triggers can end the first mission if simulation subsequently advances.

| Island / view | Camera | Time / weather |
|---|---|---|
| Malden road/rain/night | `[7116,78,6100,0,-.12,1]` | noon / rainy noon / midnight |
| Malden sunset | `[3520,8,3240,1,.12,.2]` | 19:30 |
| Malden coast | `[3500,5,3240,1,-.08,0]` | noon |
| Malden village | script camera `[3102,6332,3]`, target `[3079,6351,3]` | noon |
| Malden active forest | `[6800,79,5500,1,.05,0]` | 06:00, weather 0, fog .6 |
| Everon village/rain/night | `[9448,28,1580,-1,.02,0]` | noon / rainy noon / midnight |
| Everon sunset | `[4800,40,3600,0,.12,-1]` | 19:30 |
| Everon coast | `[5100,5,3750,0,-.08,1]` | noon |

Rain uses `triCheatWeather 1; 0 setRain 1`, simulation at 4x for about seven
seconds, then frozen. Log actual rain/fog/cloud position: this is not a seeded
weather replay. Keep that resulting state fixed across each configuration pair.
The compiled-trial `forest` request uses the Malden coordinates on both islands;
only the Malden forest is the active-scattering performance/visual reference.

Set each feature explicitly via existing commands, then `triEnableShadowMaps`.
Use fixed `triSSAO [enabled,1.2,1.5,.03,80]`; CSM has four 2048 cascades and
the existing profile coverage, darkness .35, bias .00002, fade 40. Set the four
automatic controls `triAtmosphere`, `triWetSurfaces`, `triUnderwater`, `triPBR`
On in each proposed configuration. Keep `triRTGI`, `triRTAO`, `triBloom`,
`triVolumetricFog`, `triSunShafts` Off except an explicitly isolated test.

Each measured block waits three seconds, then gathers 6.2 seconds of the existing
two-second logs. Individual tests are Off/On/On/Off; final combined tests are
Standard/Enhanced/RT with reverse repeats on coast and Malden village. Capture
after sampling. Do not build, encode images or run another game during sampling.
Interpret requested versus active status, not merely the command return value.
Retain whole-frame CPU pacing separately from GPU timestamps and CPU recording.

No automation framework, test scene assets, derivative artwork, private SDK,
menu code or persistent settings are part of this evidence package.
