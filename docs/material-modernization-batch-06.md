# Batch 06: original barn-atlas boards and masonry

LOD-count erratum: [batch 07](material-modernization-batch-07.md) separates the
1,711 authored levels from 1,708 ordinary-adapter retained levels and corrects
three removed forest slots. This batch's 37 locally verified building LODs and
material decisions are unaffected; the original receipts remain historical.

Base: `e248c40923885b505c291a3c8f17abbdd99873c1`. The existing auto-only regional
path now distinguishes multiple physical materials in one original atlas. It
adds 16 exact-model regions across **10 world-placed models / 37 visual LODs**.
These remain partial source reviews, not ten completed buildings. No whole-atlas,
v3/enhanced-table, default, shader, ray-budget or transport change is made.

## Original evidence and physical boundaries

The two original 512×512 images, `o/hous/stodola01.pac` and `stodola01a.pac`, were
individually reinspected alongside native geometry, UVs, normals, render flags
and authored-lighting hints for every placed user and visual LOD. The first atlas
has plain boards on the left, a hardware-bearing wooden door on the right and
stone/mortar below. The second mixes holed wooden gables, brick/stone and another
hardware-bearing door. Neither is a single physical material.

| Regions | Exact model family | Inspected original region |
| --- | --- | --- |
| 518 | `domek_kovarna` | Plain upper-left wooden cladding, original U/V clamp required |
| 519–522 | `domek_sedy`, `domek_sedy_bez`, `domek_zluty`, `domek_zluty_bez` | Stone foundation row in `stodola01` |
| 523/524/525 | `stodola` | Left boards / stone foundation / stone-and-brick strip |
| 526/527/528 | `stodola2` | Left boards / stone foundation / stone-and-brick strip |
| 529/530/531 | `stodola3` | Left boards / stone foundation / stone-and-brick strip |
| 532/533 | `zluty_statek`, `zluty_statek_in` | Stone foundation row in `stodola01` |

Every model key above is under `o/hous/` and ends in `.p3d`. Wood uses the existing
unmeasured .85 roughness / family 4 approximation; stone/brick .90 / family 6,
F0 .04 and metallic 0. Original color, weathering and photographed shadows remain.
These are bounded artistic coefficients, not recovered optical measurements.

The [source audit](evidence/material-modernization/batch-06/source-audit.json)
records exact rectangles, original source SHA256s, decoded RGBA fingerprints,
sampler requirements, geometry/binding fingerprints and all section qualifications.
Thirty-six exact visual-LOD geometry identities admit a region; the remaining
reviewed LOD does not inherit one. Model, resolution, vertex/section counts,
immutable pixels and ordered original texture bindings are required together.
All corners of the **whole original triangle** must satisfy one inspected convex
rectangle, plus all existing geometry, opacity, ordinary-vertex and static gates.

The stone row spans the entire image width, so its original bounded repeated U
coordinates sample the same physical stone/mortar class. No modulo operation
turns a cross-material triangle into a valid one. Small original wood-edge
overshoots additionally require the inspected ClampV or ClampU|ClampV flags.
The unclamped negative-V wood faces in `stodola2`'s last LOD remain unqualified.
No sampler, original UV, mesh or texture is changed; no map, artwork, per-pixel
mask or replacement geometry is generated.

Hardware, holed gables, cross-material faces and further potentially separable
frame patches remain pending. The whole mixed-source identity is still zero.
The additional highest-detail prerequisite count is 905, without multiplying by
placements or alternative LODs. It describes possible PBR/reflector inputs, **not
receiver pixels, visible coverage or an adoption objective**.

## Shared raster/ray implementation

The previous regional candidate represented one material for a model/texture.
It now anchors a source-proof group; whole-triangle selection returns the actual
unique region. Overlapping approvals reject rather than choose by registry order.
Model, texture, dimensions and pixel fingerprint must agree within the group.
Native raster uses the existing cached material ranges. CPU submission retains
original-index/UV checks and rejects clipping-generated regional triangles; it
adopts the actual selected region. Ray submission independently verifies that
same region's pixels and uses its identity and roughness, not the anchor's wood
parameters for a stone part. Rejected ranges keep traversal geometry. The existing
1024-geometry fallback still declines transport rather than dropping geometry.

Focused tests cover shared-atlas models, multiple regions/families, reversed rule
order, overlaps, missing clamp flags, mixed UVs, wrong pixels/model/LOD/bindings,
authored lighting, render state and missing captured inputs. All previously
approved regions retain their policy and prerequisite counts.

## A receiver trial that did not pass

PBR/reflector source proof is not sufficient proof of coherent ambient replacement.
The first `stodola3` board-receiver trial produced a
[triangle seam in overcast light](evidence/material-modernization/batch-06/rejected-receiver-trial-barnroof-overcast-on.png)
and a [stepped dusk boundary](evidence/material-modernization/batch-06/rejected-receiver-trial-barnroof-dusk-on.png).
Native face diagnostics find five partially classified original faces in its
highest-detail LOD, two in the next and one in the last. Unresolved hardware and
wrapped-UV neighbors cannot simply inherit the boards' material response.

**Region 529 therefore retains its GI-off PBR ambient reference, while keeping
independently verified PBR and ray-reflection participation.** A narrow explicit
receiver gate applies only to this new region, on both native and CPU routes.
All earlier regions and other receiver candidates keep their existing behavior.
The [final overcast](evidence/material-modernization/batch-06/modernization-b06-reception-noe-1080-barnroof-overcast-on.png)
and [final dusk](evidence/material-modernization/batch-06/modernization-b06-reception-noe-1080-barnroof-dusk-on.png)
views have no demonstrated triangle/step seam. No altered filter, sample count,
transport gain or acceptance threshold conceals the rejected result. This is
explicitly not whole-facade GI reception.

## Actual gameplay and its limits

Original `Resistance/R01WarCry.Noe` gameplay uses the direct developer harness,
copied profiles and `--nosound`, with terrain-grounded standing cameras around
original WRP placements. It is not a Steam-launch or mission-completion claim.
Full-size unamplified morning captures:

| Standing view | Previous policy | Accepted runtime |
| --- | --- | --- |
| Smithy cladding and rafters; windowed facade retained | [before](evidence/material-modernization/batch-06/modernization-b06-baseline-noe-1080-smithy.png) | [after](evidence/material-modernization/batch-06/modernization-b06-reception-noe-1080-smithy.png) |
| Large barn boards and exposed masonry; board reception deferred | [before](evidence/material-modernization/batch-06/modernization-b06-baseline-noe-1080-barnroof.png) | [after](evidence/material-modernization/batch-06/modernization-b06-reception-noe-1080-barnroof.png) |
| Other barn's long wall; authored/instance gates retained | [before](evidence/material-modernization/batch-06/modernization-b06-baseline-noe-1080-barn1side.png) | [after](evidence/material-modernization/batch-06/modernization-b06-reception-noe-1080-barn1side.png) |
| Open barn, clear exterior view | [before](evidence/material-modernization/batch-06/modernization-b06-baseline-noe-1080-barn2clear.png) | [after](evidence/material-modernization/batch-06/modernization-b06-reception-noe-1080-barn2clear.png) |

The smithy sample has 10,357 native wood-family receiver pixels, 10,398 on CPU
and 10,379 after native restoration. A same-runtime enhanced/auto control records
6,928 / 10,364 wood pixels; those modes also retain their pre-existing qualification
differences, so these are not exact per-identity pixel counts. The large barn
sample has 3,322 stone-family receiver pixels and **zero board receivers**.
The closed barn views record zero native wood receivers; the source prerequisites
do not override their authored/instance lighting contracts.

Bounded unique grid/phase probes independently record new reflector endpoints:
529 on 33 paths, 530 and 531 on one each at the large barn; 523/524 on 6/1 at
the closed barn; 526 on four at the clear open-barn view; and 518 on one at the
smithy. This proves actual sampled source identities, including reflection-only
529, not just nearby object names. It is not a global transport distribution.
An obstructed exploratory camera under a neighboring porch recorded 194,777 wood
pixels, but its probes predominantly hit the smithy's rafters: **that count is not
used as barn-wall reception evidence**. No unobserved source hit is claimed.

V3, auto/GI-off, sky-only, GI-on and enhanced/auto controls separate the effects.
Morning, overcast/dusk, moving/settled TAA and FSR Quality, two-bounce operation
and original lower-LOD retreats were inspected. Photographic shadows and original
LOD/texture changes remain. MSAA4 executes and validates, but its existing
spatial-only GI grain is not certified as a clean visual result.

## Cost, validation and preservation

RTX 4060 Ti; auto, TAA, 100% world resolution, one bounce, RT sun/cloud shadows;
AO/water optics/volumetrics off. Each runtime/view warms ten seconds, then runs
Off/On/On/Off with four-second settling and six-second samples. Values average
paired interval medians; probes are off and no build/other game runs during timing.

| View | GI-on GPU frame, before / after | GI trace, before / after | Trace+filter+history, before / after |
| --- | ---: | ---: | ---: |
| Smithy, 1920×1080 | 5.711 / 5.761 ms | .735 / .770 ms | 1.184 / 1.222 ms |
| Smithy, 2560×1440 | 5.827 / 5.830 ms | .706 / .739 ms | 1.178 / 1.211 ms |
| Large barn, 1920×1080 | 2.550 / 2.388 ms | .284 / .315 ms | .447 / .464 ms |
| Large barn, 2560×1440 | 3.824 / 3.949 ms | .409 / .509 ms | .690 / .796 ms |

Measured GI scopes add about .038/.034 ms at the smithy and .017/.106 ms at the
large barn. Total-frame deltas are +.050/+.004 and −.162/+.125 ms. The negative
1080p result is **not a speedup claim**: raster changes 1.200→1.085 ms there, and
Off-frame/LOD/clock variation is material. CPU command recording is around 14.3–14.5
ms at the smithy; at the large barn it rises 4.482→5.148 ms at 1080p and
4.508→5.066 ms at 1440p. More material ranges have a CPU cost. These samples do
not establish a universal GPU/CPU budget or isolated total-frame GI cost.

The four accepted-runtime runs present **103,631 frames**, synchronization validation
on, zero errors and warnings, all with verified shutdown. The rejected receiver
trial's 36,756 frames are separate. The accepted Vulkan executable SHA256 is
`3136502AD8AD2B7B2D25513BE5D23933B9B9FF2A529E98B5BAD89DB1E8775FAD`.
Public Vulkan game/tools/tests and GL33 game/tests build. Focused Vulkan tests
pass 71 cases / 58,296 assertions; GL33 70 / 58,256; Python inventory/census 15;
driver-free Vulkan policy checks 690. The intentional driver-free teardown error
is not a gameplay validation finding. Live GL33 Off/On/Enhanced/Auto/Off are
unsupported no-ops with zero changed original-world ROI pixels.

All 270 original inventory input hashes and 67 retained settings/fixture/input
hashes match. Private DLSS remains clean and remote-free, with SDKs and preserved
snapshots present. Original assets, settings, GL33, authored gates and previous
PBR/RTGI functionality are preserved. Full states, runtime identities, interval
samples, probes, source decisions and preservation checks are in the
[qualification receipt](evidence/material-modernization/batch-06/qualification.json).

## Current census and remaining work

The fresh [726-model / 1,711-LOD census](evidence/material-modernization/runtime-census.json)
reproduces all previous complete section prerequisite multisets after replaying
only regions 518–533. It also reproduces the immutable original inventory with
zero differences after all post-inventory identities are explicitly replayed:

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --cache build/material-modernization-20261021/batch-06/runtime-census-final `
  --output build/material-modernization-20261021/batch-06/runtime-census-final/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 165 `
  --replay-regional-identities 512 513 514 515 516 517 518 519 520 521 522 523 524 525 526 527 528 529 530 531 532 533
```

The ledger remains **32 complete source reviews, 75 partial, 408 awaiting source
review and 211 retained prerequisite dispositions**. This batch adds verified
regions to ten partial reviews; it does not close their remaining mixed atlas
cases or turn prerequisite counts into rendering coverage. Podhradi's previously
rejected wall coverage remains deferred. Related frame, aperture and wall-region
assessment continues through the full inventory.
