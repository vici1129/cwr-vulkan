# Batch 08: dedicated original bark families

Base: `b2506c16972375c98899583ba7f637dd8e9cb350`. Twelve dedicated original bark
sources join the existing enhanced/auto material table as identities **166-177**,
using the existing wood-family approximation: roughness .85, F0 .04, metallic 0.
These are unmeasured rough-dielectric coefficients, not recovered optical data.
The 23-entry v3 table, original assets, settings, shaders, transport/ray budgets
and every opacity, authored-lighting, geometry and captured-input gate are unchanged.

## Original-source assessment

| Original source | Identity | Shared model family |
| --- | ---: | --- |
| `data/kmen2.pac` | 166 | Classic poplars |
| `data/kmen_borovice.pac` | 167 | Classic pine stumps and authored forest clumps |
| `data/kura3.pac` | 168 | Classic hornbeam |
| `data/kura_jablon_asi_ne.pac` | 169 | Classic beech/apple trees |
| `data/lesni_kmen_smrku.pac` | 170 | Classic fir/sparse conifer variants |
| `data/n_strom_01_kura.pac` | 171 | Classic tall scrub and authored forest clumps |
| `data/n_strom_lipa_kmen.pac` | 172 | Classic linden |
| `o/tree/akat01_krd2.pac` | 173 | Nogova acacias and authored forest clumps |
| `o/tree/borovice02c.pac` | 174 | Nogova pine variants |
| `o/tree/dd_les_kura03.pac` | 175 | Nogova conifer clumps/single-tree variant |
| `o/tree/dub_kura_fx.pac` | 176 | Nogova oak |
| `o/tree/smrkur.pac` | 177 | Nogova spruce variants |

Each original image was individually viewed; each is dedicated bark rather than
leaf/branch cutouts, hardware or a mixed building atlas. Original color, weathering,
lichen, photographic highlights and baked shadows remain. Repeated UVs sample the
same physical class; no wrapping/cropping rule, inferred normal, new map, artwork
or replacement mesh is introduced.

All **35 world-placed shared models / 108 authored visual LODs** were reinspected
through the original ODOL adapter with native triangle geometry, face/UV placement,
normals, section state and authored-lighting diagnostics. The ordinary adapter
retains 105 of these levels under inspection defaults; the three excluded forest
levels remain separately inspected source-only levels, as corrected in batch 07.
All prior normal/hint/UV/section prerequisites reproduce. Only source-identity
rejections for the twelve reviewed keys become eligible; all authored, render and
geometry rejections remain. Ten shared models have no eligible ordinary bark at
all and retain their existing prerequisite disposition.

The [source audit](evidence/material-modernization/batch-08/source-audit.json)
records original SHA256 provenance, exact shared users/LODs, geometry/binding
fingerprints and qualifications. The additional highest-detail prerequisite count
is 2,144 across distinct models, without multiplying by placements or adding
alternative LODs. It is not a visible-coverage percentage or an adoption objective.
Original decoded images and per-vertex geometry/UV dumps stay in ignored local
evidence, not public reconstructable assets.

Two nearby ambiguous sources are deliberately not whole-image approvals:
`data/briza_kriv_kmen.pac` photographs a bent birch against substantial flat
background; `o/tree/kmenletokruh.pac` places a circular cut end on a plain canvas.
The latter's three clump users have land-hinted shapes; the existing exact-regional
path rejects that condition, and some original cap UVs exceed V=1 without ClampV.
No mask, repaired UV, inferred physical canvas material or gate bypass resolves
that evidence. Their remaining regional/source questions stay explicit, not
silently inherited from neighboring bark. Leaves, whole-tree impostors and authored
202/201 trunks likewise retain the original lighting contracts.

## Actual gameplay and transport limits

Original Nogova `Resistance/R01WarCry.Noe` and Everon `05HeavyMetal.Eden` gameplay
use the direct developer harness, copied profiles and `--nosound`. Cameras stand
at terrain height near original WRP placements; this is not a Steam-launch or
mission-completion claim. Native auto, GI-off/sky-only/GI-on, original v3, morning,
overcast/dusk, moving/settled TAA and FSR, two-bounce, CPU/native restoration and
original lower-LOD retreats are checked separately.

Native auto views preserve their existing tree alpha-route/captured-input gate;
**they do not establish new native bark GI reception**. Qualified bark still
contributes as reflected GI endpoints. Unique grid/phase probe samples in the
Nogova views hit identity 173 on 25 paths, 174 on 19, and 176 on eight. These are
bounded source-identity observations, not a global transport distribution.
CPU submission independently captures ordinary bark reception: acacia and pine
samples have 27,318 and 7,907 wood-family receiver pixels, returning to zero after
native restoration. These are route-specific frame samples, not all-tree coverage.
MSAA's existing spatial-only GI grain is not certified as a clean visual result.

The Everon probes also identify poplar 166 on seven paths and linden 172 on seven.
The poplar CPU sample has 9,856 wood-family receiver pixels; the old-policy CPU
control has zero there and at the acacia view. Linden's native view already has
about 240 wood-family pixels from neighboring geometry before this change; its
similar new count is **not attributed to new bark reception**. Global family
counters cannot establish a particular source's receiver identity.

Full-size, unamplified controls distinguish the working paths:

| Standing view / route | Previous policy | Accepted policy |
| --- | --- | --- |
| Acacia, native Enhanced material response | [before](evidence/material-modernization/batch-08/modernization-b08-modes-baseline-noe-1080-acacia-material-enhanced.png) | [after](evidence/material-modernization/batch-08/modernization-b08-modes-new-noe-1080-acacia-material-enhanced.png) |
| Poplar, native Enhanced material response | [before](evidence/material-modernization/batch-08/modernization-b08-modes-baseline-eden-1080-poplar-material-enhanced.png) | [after](evidence/material-modernization/batch-08/modernization-b08-modes-new-eden-1080-poplar-material-enhanced.png) |
| Acacia, captured CPU auto reception | [before](evidence/material-modernization/batch-08/modernization-b08-modes-baseline-noe-1080-acacia-CPU.png) | [after](evidence/material-modernization/batch-08/modernization-b08-new-noe-1080-acacia-CPU.png) |
| Poplar, captured CPU auto reception | [before](evidence/material-modernization/batch-08/modernization-b08-modes-baseline-eden-1080-poplar-CPU.png) | [after](evidence/material-modernization/batch-08/modernization-b08-new-eden-1080-poplar-CPU.png) |

Native Enhanced shows a modest rough-bark response; native auto keeps its stricter
captured/alpha gate. Its standing [acacia](evidence/material-modernization/batch-08/modernization-b08-new-noe-1080-acacia-gi.png)
and [poplar](evidence/material-modernization/batch-08/modernization-b08-new-eden-1080-poplar-gi.png)
views therefore mainly demonstrate preserved appearance and qualified reflected
contribution to existing receivers, not newly relit trunks. Original leaf cards,
texture detail, photographic shading and LOD/impostor transitions remain. Cross-run
foliage/LOD and TAA/shadow differences prevent a bit-exact image comparison.

The [qualification receipt](evidence/material-modernization/batch-08/qualification.json)
records actual runtime identities, full states, measurements, probes, validation
shutdowns and preservation checks. No artificial brightness multiplier or altered
acceptance/filtering gate is used to present a larger improvement.

## Measured cost and preservation

RTX 4060 Ti; native auto settings described above. Each view/runtime warms ten
seconds, then runs Off/On/On/Off with four-second settling and six-second samples;
the table averages the paired interval medians. Probes are off and no build or
other game runs during timing.

| View | GI-on GPU frame, before / after | GI trace, before / after | Trace/filter/history sum, before / after |
| --- | ---: | ---: | ---: |
| Acacia, 1920x1080 | 3.369 / 3.421 ms | .334 / .384 ms | .502 / .551 ms |
| Acacia, 2560x1440 | 5.543 / 5.901 ms | .540 / .972 ms | .867 / 1.300 ms |
| Poplar, 1920x1080 | 2.993 / 3.021 ms | .459 / .637 ms | .650 / .831 ms |

The acacia GI scopes add about .049 / .433 ms; qualified bark now executes existing
bounded sunlight/sky visibility queries. CPU command medians change 7.536 to 7.608
ms and 7.678 to 7.981 ms. This is a measured cost, not free coverage. Total-frame
deltas are +.052 / +.359 ms; the 1440p Off-frame also shifts +.118 ms, so these runs
do not isolate every clock, raster or recording effect.

Poplar's trace samples are multimodal around .30 and .62-.64 ms, including within
the old policy. Its +.181 ms GI-scope difference of interval medians is **not a
precise causal per-frame cost estimate**; total-frame change is only +.028 ms and
CPU command medians are 7.304 / 7.280 ms. Raw intervals and their spread are retained.
No speedup, universal GPU/CPU budget or additive same-frame timing is inferred.
Enhanced and CPU routes have visual/functional controls, not separate cost claims.

Ten Vulkan before/after and mode-control runs present **92,305 frames**,
synchronization validation on, zero errors and warnings, all with verified shutdown.
The accepted Vulkan executable SHA256 is
`9DE9D296060F2247411D985B5DBAE3BB01F421E43E485C415BA8CEFED3C7941E`.
All 270 original input hashes and 67 retained settings/fixture/input hashes match.
Live GL33 Off/On/Enhanced/Auto/Off remain unsupported no-ops with zero changed
original-world ROI pixels. Private DLSS remains clean and remote-free; private SDKs,
preserved build/runtime snapshots, original assets and settings are untouched.

## Tests, census and remaining work

Public Vulkan game/tools/tests and GL33 game/tests build. Focused Vulkan tests pass
77 cases / 58,679 assertions, GL33 76 / 58,639, Python inventory/census 18 and
driver-free Vulkan policy checks 690. The intentional driver-free teardown error
is not a gameplay validation finding. New tests exercise all twelve identities,
unchanged v3 behavior, absent captured ambient/direct inputs, authored/render/geometry
failures, canopy/mixed-source exclusions and wrong namespaces/extensions.

Fresh censuses retain the corrected scopes: all 726 models, 1,708 ordinary-adapter
retained visual levels and all 1,711 authored source levels. Complete prerequisites
match the immutable original inventory after explicit post-inventory source-policy
replay; the three source-only levels remain outside retained historical comparison.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-08/runtime-census `
  --output build/material-modernization-20261021/batch-08/runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 177 `
  --replay-regional-identities 512 513 514 515 516 517 518 519 520 521 522 523 524 525 526 527 528 529 530 531 532 533
```

For the separate source census, add `--all-source-lods` and use a separate cache/output
under `batch-08/source-census`. Neither census proves live reception or optical
coverage of every original model. Remaining mixed-source and building-region work
continues through the inventory; this goal is not complete.

The ledger now has **54 complete source reviews, 78 partial, 383 awaiting source
review and 211 retained prerequisite dispositions**. Twenty-two models finish
their remaining source-identity review; three forest clumps remain partial because
their cut-end/canvas source is unresolved. Ten already-retained authored models
do not become coverage successes merely because their shared bark image is known.
All 726 paths and the original 1,711 authored-LOD inventory remain accounted for.
