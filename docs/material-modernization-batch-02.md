# Batch 02: original brick pillars, bark and stacked logs

Base: `b8bf2a1b4abb25916ecee00d5fa044aca6652cd3`. This continues the
[original-world material assessment](material-modernization.md) through related
masonry and rough wood families. Seven exact original sources join the enhanced
table, without changing v3, defaults, shader transport, ray budgets or filters.

| Original source | Existing material class | Enhanced identity |
| --- | --- | ---: |
| `data/2_sloup_cihl.pac`, `data/2_sloup_cihl_n.pac` | Rough brick/mortar, r=.90 | 134–135 |
| `data/kmen_borovice_04.pac` | Dedicated pine bark, r=.85 | 136 |
| `o/tree/javor01_kr3.pac` | Dedicated maple bark, r=.85 | 137 |
| `o/hous/hrmd_kr.pac` | Bark on modeled wooden members, r=.85 | 138 |
| `o/hous/hrmd_top.pac`, `o/hous/hrmd_zbk.pac` | Stacked logs/bark, r=.85 | 139–140 |

Original pixels and mesh geometry were viewed locally. All shared **11 placed
models / 36 visual LODs** were inspected with original config, native normals,
sections, UV bounds, lighting indices and existing triangle qualification.
Before/after geometry, UVs, flags and every opacity/render, authored-lighting and
invalid-geometry rejection bucket agree. Only the reviewed source-identity
failures become eligible. The [source audit](evidence/material-modernization/batch-02/source-audit.json)
retains that evidence. Its 1,609 newly eligible highest-detail triangles are a
source audit count, not the objective or a visible-coverage denominator.

The original pillar models occur 262 times; the dedicated pine family occurs
14,622 times and the two main maple models 14,563 times. Shared source inspection
also includes `afstr borovice`, `javor01posed`, and two rust-gate variants.
These frequencies guided practical scene selection; they do not imply that
all of each model or all of its LODs gain rendering coverage.

Three wood-pile sources were explicitly reviewed and retained:

| Original source | Concrete reason for retaining legacy shading |
| --- | --- |
| `hrmd_pl` | Weathered corrugated/rusted metal; not established as one of the existing wood/stone dielectric classes |
| `hrmd_vrt` | Wooden door includes metal hinges/hardware; no clean whole-source wood identity |
| `hrmd_zp` | Photographic log ends share supports and vegetation; geometry/UVs do not establish a clean all-LOD physical boundary |

All foliage/cutout/impostor sections, authored vegetation lighting and invalid
leaf normals retain their previous behavior. Near opaque trunks do not promote
their lower LOD cards. Existing model/section, normal, material-lighting,
captured ambient/direct, opacity, immutable-object and dynamic-texture gates are
unchanged. No maps, artwork, geometry or original assets are generated or edited.

## Observed gameplay behavior

The existing direct developer harness runs original Nogova Resistance and Malden
missions with copied profiles and `--nosound`. Standing cameras view original
WRP placements. These checks are not a Steam-launch or mission-completion claim.
Full-size unamplified comparisons cover fence pillars and the wood pile outside
a Nogova house, a maple grove, and a Malden pine slope:

| Scene | Before | After |
| --- | --- | --- |
| Brick pillars | [before](evidence/material-modernization/batch-02/modernization-b02-baseline-noe-1080-pillar.png) | [after](evidence/material-modernization/batch-02/modernization-b02-new-noe-1080-pillar.png) |
| Wood-pile side | [before](evidence/material-modernization/batch-02/modernization-b02-baseline-noe-1080-woodpile.png) | [after](evidence/material-modernization/batch-02/modernization-b02-new-noe-1080-woodpile.png) |
| Maple grove | [before](evidence/material-modernization/batch-02/modernization-b02-baseline-noe-1080-maple.png) | [after](evidence/material-modernization/batch-02/modernization-b02-new-noe-1080-maple.png) |
| Pine slope | [before](evidence/material-modernization/batch-02/modernization-b02-baseline-abel-1080-pine.png) | [after](evidence/material-modernization/batch-02/modernization-b02-new-abel-1080-pine.png) |

Pillars and the separated wood-pile side participate as both reflectors and
receivers: sample frames contain 4,349 stone and 7,260 wood receiver pixels in
the half-resolution diagnostic footprint. Their change is local, with original
brick/bark detail retained. The wood-pile end photo, roof and door remain legacy.

Bark has a deliberate runtime limitation. Native tree rendering demonstrates
new PBR draws and qualified GI reflector hits, but **zero native bark receiver
pixels** in both test scenes. Existing native tree alpha-route/receiver gates
are preserved. Exact-window Pause toggles demonstrate 16,447 CPU wood receiver
pixels in the maple view and 4,178 in the pine view; restoring the native route
restores zero native reception. No ambient is invented to force parity.
The sparse path samples separately identify new bark identities **137 and 136**
(13 and 12 unique eligible grid/phase paths), rather than inferring materials
from nearby object names. These samples are not global transport statistics.

The native bark visual change is subtle rough-material response, with localized
transport into already supported receivers. It is not broad canopy GI, nor a
large whole-frame lighting improvement. Existing canopy/card lighting, baked
texture shading, texture resolution and geometry LOD changes remain visible.
Retreats of 0/8/22/52/92 m check the original lower-LOD fallback; no seamless
all-distance PBR claim is made. Moving and settled TAA/FSR Quality, overcast,
dusk and two-bounce operation were inspected. MSAA retains its existing noisy
spatial-only GI; it is not certified as a clean visual result.

## Cost and regression evidence

RTX 4060 Ti, one bounce, TAA, 100% world resolution, RT sun/cloud shadows on;
AO/water optics/volumetrics off. After ten seconds of warmup, Off/On/On/Off
intervals settle four seconds and sample six. Values average the paired interval
medians. No build or other game runs during timing; probes are off.

| Scene | GI-on GPU frame, before / after | GI trace, before / after |
| --- | ---: | ---: |
| Nogova maple, 1080p | 8.711 / 8.716 ms | .789 / .865 ms |
| Nogova maple, 1440p | 8.539 / 8.530 ms | .751 / .820 ms |
| Malden pine, 1080p | 7.659 / 7.673 ms | .762 / .819 ms |
| Malden pine, 1440p | 7.741 / 7.837 ms | .759 / .836 ms |

Tracing adds approximately .056–.077 ms in these scenes. Total frame deltas
range from -.009 to +.096 ms and do not establish a speedup: CPU command recording
is around 19–21.5 ms, and raster/clock variation dominates these small total-frame
differences. The Nogova 1440p baseline filter intervals also vary; raw scope
samples are retained, without claiming a scaling law or universal GPU budget.

Eight Vulkan runs presented **46,849 frames**, synchronization validation on,
zero errors and warnings. Public Vulkan game/tools/tests and GL33 game/tests
builds succeed. Focused Vulkan tests pass 69 cases / 49,459 assertions; GL33
68 / 49,419; Vulkan driver-free policy checks 690. The live rebuilt GL33
Off/On/Enhanced/Off world control again has zero changed pixels. The identity
boundary regression now uses the actual end of the enhanced table rather than
mistaking the newly valid bark identity 136 for an invalid identity.

The 270 original inventory-input hashes and 67 retained settings/fixture/input
hashes still agree. Private DLSS is clean and remote-free; both SDKs and preserved
build/runtime snapshots remain present. Runtime hashes, counters, paired GPU
intervals, native/CPU reception evidence, source-specific probes and preservation
checks are in the [qualification receipt](evidence/material-modernization/batch-02/qualification.json).
The per-model ledger records the native bark reception limit and explicitly
retained wood-pile sources. Source assessment remains ongoing for the rest of
the inventory; no rendering-coverage percentage is inferred.
