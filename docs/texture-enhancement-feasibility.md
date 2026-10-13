# Faithful stock-texture feasibility experiment (2026-10-13)

## Decision and scope

**Do not expand this resampling experiment into an HD pack.** A three-texture,
local-only 2x/4x prototype improves the continuity of magnified texels, particularly
roof grooves, but does not recover missing artwork. Close rocks remain blurry;
the normal-distance improvement is small. Four times the width/height is not a
proportionate improvement over 2x. This is a finding about the conservative local
methods tested, not proof that licensed higher-quality sources or careful manual
restoration could never help.

Started from public main `62d250b`, whose parent is `8539d7d`. The preceding RTGI
no-fix review was closed and pushed separately in `62d250b`. The experiment used
an isolated worktree and isolated user/cache/temp profiles. No game PBO, model,
mission, user installation, GL33 code or private DLSS/NGX work was changed.
Experimental textures and raw captures remain local; none are distributed here.

## Existing path: source resolution is the main constraint

`TextureVK.cpp` opens the original bank reference and uses
`DecodePAAMipChainBuffer`; level zero is the finest stored source image, not an
arbitrarily reduced residency level. `UseMipmap` keeps that level resident.
`VulkanContext_Texture.cpp` uploads every decoded RGBA level and caches the image.
Both UNORM and sRGB views refer to that allocation. The sampled tail stops before
either dimension reaches four pixels, consistently with the existing GL33 policy.

`TextureSamplerInfo` uses linear magnification/minification and trilinear mip
selection, with supported device anisotropy capped at 16x. Point samplers retain
nearest filtering; wrap/clamp is unchanged. The live RTX 4060 Ti reported **16x**.
The PAA/PAC decoder follows the stored offsets/sequential levels, validates
dimensions, and decodes the original DXT1 blocks. Source inspection and stock-bank
tests found no lost top mip or missing anisotropy to repair. Existing AA-dependent
sampling behavior was left alone.

The source textures really are small relative to their mapped surface area. The
selected rock's enlarged texels are visible with the original finest mip loaded.
The selected roofs retain repeating baked shading and low-resolution edges.
Neither interpolation nor increased texture dimensions restores information absent
from those sources. Existing geometry/UV discontinuities are a separate limit.

## Exact selection and local processing

| Bank texture | Original | 2x / 4x | Verified use |
|---|---:|---:|---|
| `data/skala_piskovec2.pac` | 256x256, 7 stored mips | 512x512 / 1024x1024 | `kamen3_zula.p3d`, Malden object 83921 near X/Z 7219.9/7705.7; `skala_new.p3d` cluster near Everon 9250/1050 |
| `data/strecha_tasky_vln.pac` | 64x256, 5 stored mips | 128x512 / 256x1024 | Stock tiled roofs, including Malden `dum_mesto2l.p3d` object 37880 near 5425.9/2805.8 and Everon `dum_olezlina.p3d` object 32001 near 9434/1580.2 |
| `data/strecha_tasky_vln90.pac` | 256x64, 5 stored mips | 512x128 / 1024x256 | Authored rotated roof variant; kept with its companion to avoid inconsistent treatment of roof orientations |

These are three dedicated texture files across two surface families. Vehicle
atlases, foliage, effects, UI, text and markings were excluded. All selected
sources decode as DXT1 with every alpha value 255. Model references and stock WRP
placements were inspected, then the actual surfaces were visited in gameplay.

The existing local `PoseidonTools` extracted only these named assets and decoded
them to RGBA. Its decoder/encoder sources have no differences from this baseline.
Windows GDI+ `HighQualityBicubic`, high-quality pixel placement and tiled boundary
handling generated independent 2x and 4x candidates from each original. There was
no generative model, new pattern, detail synthesis, color grading, or unsharp-mask
pass. The existing PAA encoder produced DXT1 upper levels. The final local PAC
containers prepend one/two new levels to the **byte-exact original mip payloads**;
the original resolution and every coarser level are retained, rather than
regenerated from an enlarged image. Container assembly checked every dimension
and original payload. These are valid ordinary PAC files, not a new asset format.

Aspect ratios, UV layout, material names and alpha classification are preserved.
After encoding and decoding, top-level mean RGB shifts were below **0.082/255 per
channel** for all six candidates; alpha remained exactly 255. This measures mean
color preservation, not per-pixel identity or recovery of detail. DXT1 recompression
and the cubic kernel can change local contrast despite a stable mean.

## Opt-in path and restoration

No engine feature is needed. The existing `_ENABLE_PATCHING` /
`QFBank::ScanPatchFiles` mechanism allows a loose `data/<exact-name>.pac` to supply
an existing bank entry. Two isolated local game roots contain precisely these
three patch files. Their ordinary stock folders (`DTA`, `AddOns`, `BIN`, `Worlds`,
etc.) are directory junctions to the original installation, used only for reads.
Launching with `-C <experimental-root>` opts in. Launching with the ordinary stock
root restores the original bank textures; no files need to be replaced or deleted.
Profiles, cache and temporary output are outside the stock folders.

An initial exact-name environment-variable loader was used to explore the
experiment, then removed after the existing bank-patch route was verified. The
final route uses ordinary stock loading. Temporary allocation logging was used
only to measure Vulkan image requirements and is not a shipping change. No engine
change, texture manager, settings entry, new sampler or mipmap implementation is
proposed.

## Gameplay observations and evidence limits

Stock `01TakeTheCar.ABEL` and `06Ninjas.EDEN` ran through the established local
diagnostic launcher. These are actual Malden/Everon missions, not renderer fixtures,
Steam-route acceptance, or mission-completion tests. Developer positioning and
daylight setup established repeatable views. Close, medium and oblique captures
used identical explicit camera positions/directions, noon, clear weather and AA
Off, TAA, or FSR Quality. Some close roof views deliberately elevate the camera
to inspect texel magnification; ground-level movement supplies ordinary gameplay
context. Separate stills enabled existing PBR, RT sunlight and RTGI together.
RT reflections were enabled too, but the target views reported zero water rays.
A coastal rock inspection also did not place the selected surface in a visible
water reflection. **Reflected appearance of these replacements is unverified**;
an enabled setting is not evidence of actual reflection hit shading.

| Observation | Result |
|---|---|
| Close rock | 2x/4x make the enlarged texel lattice less square. Cracks and weathering remain recognizable, but fine areas still look soft; 4x adds no convincing new geology. |
| Close roof | Tile/groove contours become more continuous. Some dark grooves become stronger and more rounded, so apparent sharpness is not necessarily more faithful detail. |
| Medium/oblique views | Much less benefit; retained authored mips dominate as projected size falls. Thin roof repetition and AA-Off edge aliasing remain. |
| Movement / temporal AA | Short keyboard strafes were captured in all three AA modes on both islands. No obvious new persistent trail or flashing mip boundary was seen in the retained target-visible frames. This sparse capture is not a high-frame-rate flicker measurement or proof of shimmer-free motion. |
| Seams / materials | Existing rock geometry and UV discontinuities remain visible. No new patch-edge seam or alpha hole was identified in the compared views. PBR still selects the same rough dielectric materials; RTGI remains subtle and does not turn the enlarged artwork into added geometry or a new material. |

The raw PNGs are unretouched game-window captures, including window chrome.
Camera framing is matched; separate launches do **not** freeze every mission
event, HUD message, weapon pose, cloud phase, or temporal sample to identical
pixels. Do not interpret whole-frame image differences as a texture-quality
metric. Some initial movement attempts looked downward after leaving the pinned
camera; those are excluded from surface-quality conclusions. Fresh runs performed
the target-visible movement before pinning the camera. Actual player positions
advanced roughly 4.8 metres per short path; repetitions are qualitative comparisons.

Local evidence lives in the original checkout's ignored
`build/shadow-live/tex-*` directories. Useful matched filenames include
`rock-close-{off,taa,fsr-quality}.png`, `roof-close-*.png`, `roof-medium-*.png`,
and `everon-roof-{close,medium,oblique}-*.png`. Source/restoration runs are
`tex-stock-malden`, `tex-stock-return`, and `tex-stock-everon`; candidates include
`tex-2x-malden`, `tex-4x-malden`, `tex-patch4x-malden`, and `tex-patch4x-everon`.
Target-visible movement is retained in `tex-stock-return`, `tex-2x-malden`,
`tex-4x-malden`, `tex-stock-everon`, and `tex-patch4x-everon-move`.
The final unchanged-engine binary also produced the matched rock-close,
roof-close and roof-medium series in `tex-coast-stock` and `tex-final4x`.
The former's extra coastal inspection images are not reflection-quality proof.

The experiment worktree's ignored `build/assets` holds the original extracts,
candidates, and mip manifest; `build/patches` holds the two opt-in roots.
`build/prepare-assets.ps1`, `assemble-patches.py`, `prepare-roots.ps1`,
`launch-patch.ps1`, `compare.py`, `performance.ps1` and `measurements.json`
record local reproduction and measurements. No raw textures or modified texture
derivatives are committed. Distribution would require appropriate permission.
`build/capture-manifest.json` records SHA-256 and sizes for 235 raw local captures,
including exploratory frames whose evidence limitations are noted above.

In the retained experiment worktree, a new unique evidence label can replay the
local opt-in launch (the helper does not accept an existing evidence directory):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File build/launch-patch.ps1 -Label tex-replay-2x -Variant bicubic2x
```

After closing that instance, use a different label and `-Variant stock` for the
original path, or `-Variant bicubic4x` for the larger candidate. These are local
developer helpers, not newly shipped game options. No persistent override is set.

## Memory, upload behavior and verification

Measured Vulkan image allocation requirements for one resident set of the three
textures, including driver padding, are:

| Variant | Decoded chain / upload bytes | Device image allocation bytes |
|---|---:|---:|
| Stock | 524,096 | 531,968 |
| 2x | 2,096,960 | 2,104,832 |
| 4x | 8,388,416 | 8,396,288 |

The chain is also retained on the CPU; a staging buffer is transient during upload.
The existing RT albedo path may additionally retain a shared host-visible RGBA
base-level buffer: 393,216 bytes stock, 1,572,864 at 2x, or 6,291,456 at 4x if all
three are needed. Those RT figures are calculated from the existing allocation
formula, not an operating-system measurement of total resident VRAM. Descriptor
counts and sampler counts do not increase. At fixed texture content these costs
are independent of 1080p/1440p output size. Vulkan currently uploads decoded RGBA,
so small on-disk DXT1 files do not mean compressed GPU texture residency.

Focused decoder/encoder and stock-bank checks passed **1,257 assertions / 38
cases**, and the driver-free Vulkan policy/lifetime suite passed **504 checks**.
The encoder tests required their existing `C:/tmp` output directory; the initial
missing-directory failures were resolved by providing that directory, without
changing encoder code. The policy suite's deliberate teardown error injection is
expected. Live validation results and final timing measurements are recorded below.

### Focused 1080p / 1440p timing

RTX 4060 Ti, native 1920x1080 / 2560x1440 swapchains, windowed, VSync off,
PBR + RT sunlight + RTGI + RT reflections enabled, RTAO off. Validation was
disabled for timing and enabled in separate visual runs. The same binary and
fixed rock/roof cameras were used for stock, 2x and 4x in separate launches.
Each view/mode settled for 2.5 seconds, then sampled 6.2 seconds of the existing
GPU profiler (three or four two-second reports). Values are medians in ms, not
an FPS estimate or a statistically controlled benchmark.

| Output / surface | AA | Stock | 2x | 4x |
|---|---|---:|---:|---:|
| 1080p rock | Off | 2.123 | 2.188 | 2.183 |
| 1080p rock | TAA | 2.212 | 2.218 | 2.216 |
| 1080p rock | FSR Quality | 2.049 | 2.037 | 2.027 |
| 1080p roof | Off | 2.220 | 2.227 | 2.221 |
| 1080p roof | TAA | 2.361 | 2.364 | 2.372 |
| 1080p roof | FSR Quality | 2.033 | 2.039 | 2.040 |
| 1440p rock | Off | 3.108 | 3.086 | 3.115 |
| 1440p rock | TAA | 3.510 | 3.522 | 3.532 |
| 1440p rock | FSR Quality | 2.617 | 2.626 | 2.641 |
| 1440p roof | Off | 3.595 | 3.604 | 3.615 |
| 1440p roof | TAA | 4.058 | 4.071 | 4.079 |
| 1440p roof | FSR Quality | 2.895 | 2.901 | 2.914 |

The largest positive paired median difference was about **0.065 ms**. Most were
under 0.025 ms; run order, GPU clocks and short sampling limit attribution.
Some 1080p FSR samples varied by about 0.22 ms within a window. The CPU frame
interval was roughly 6.17 ms and externally paced, so these are GPU-pass timings,
not evidence of an equivalent end-to-end frame-rate improvement or slowdown.
Memory growth is the much clearer cost. Every recorded settled window reported
**zero texture uploads and zero transient allocations per frame**. Initial loads
upload the larger chains once; travel can load new texture objects as usual.

Validation-enabled visual runs closed with zero errors/warnings: stock Malden
(44,145 frames), 2x Malden (13,412), 4x Malden (16,179), final-bank-patch Malden
(9,250), final-bank-patch Everon (45,914), its fresh movement run (2,369), and
stock Everon (6,855). Timing-run zero counters are not counted as validation
coverage because the layer was disabled there. No large compatibility campaign
was performed and GL33 was not changed or newly gameplay-qualified by this test.
The final unchanged-engine stock/4x runs also closed cleanly (8,056 / 3,710
frames), and the focused unit and policy tests passed again after removing the
temporary instrumentation.

All exploratory loader and allocation-logging edits were removed. The final
clean-engine rebuild succeeded; its executable SHA-256 is
`30e9580ad48f13feb6db6a4b82465ee4d0a82623a352647fa12a3eab0274eecc`.
The original `Dta/Data.pbo` SHA-256 remained
`1587f9c052ff38967463d851ca85a03efd2db09d063da183046a9b16181d5e2f`.
The original checkout's unrelated private edits were preserved. Only documentation
is proposed for the public branch; both local candidate roots remain opt-in.

## Practical next step

Keep the normal stock path. Do not batch-convert the library on this evidence.
If future authorized work has better source art or a convincingly faithful
restoration method, start again with a few dedicated surfaces at 2x, inspect
tile boundaries and local contrast, retain authored lower mips, and compare at
normal gameplay distances before expanding. The existing bank-patch path is
sufficient for that local evaluation; an asset framework or graphics option
would be premature.
