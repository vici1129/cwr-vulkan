# Fountains: limited CPU-route mineral coverage

Base: `7cf3d50261f5cf2621c06d0dcec5d36cb6f747bf`.
This survey covers **two models / seven authored visual levels / 16 placements**:
the classic fountain (six Malden, five Everon) and Nogova fountain (five).
Eight original source-unknown images are physically viewed; 14 decoded
references are not 14 optical approvals. The demonstrated new GI reception is
on the **legacy CPU-transform route of the Vulkan renderer**, not new native
reception. GPU RTGI still runs on the GPU. This is not a recommendation to
disable hardware T&L or alter stored rendering settings.

## Physical sources and exact regions

Three separate original classic sources use the existing enhanced-only rough
mineral approximation (.9, stone family 6), not measured reflectance:

| Identity | Original source | Supported physical region |
| --- | --- | --- |
| 198 | `data/kasna_rantl.pac` | Weathered mineral rim/cornice |
| 199 | `data/kasna_schod.pac` | Mineral slab/step and exposed trim |
| 200 | `data/kasna_sloupekbtr.pac` | Capped mineral post without the sibling's central dark strip |

The classic step has 130.67 square metres of near-level mesh area, but about
**124.93 square metres are paired full slab planes**, not an exposed plaza.
The original local planes lie at Y=-1.0546 and -.958295. They are not a claim
of new visible walkway coverage. Authored inner-rim, cap, basin and water
triangles retain their original gates; photographic border shading stays in
the original pixels. The visible material scope is the exposed rim/trim/post.

Exact region **562** supports the lower mineral panel/relief row of original
`o/hous/kasna01.pac` on `o/hous/kasna_new.p3d`, excluding the upper lion/spout
photograph. It uses .9 / family 6, exact 256x256 original pixels with RGBA
fingerprint `16872738925428872060`, four exact original geometry identities
and their ordered texture bindings. The U[-.2,2] / V[.5,1.006] bounds contain
594/286/182/94 ordinary original triangles across four source levels, with
**zero split original face fans**. Broader earlier bounds split fans and are
not used. This is whole-face selection, not polygon surgery or a shader mask.

Original repeated U samples the same mineral row. Tiny original V edge
overshoot samples inspected gray mineral edge pixels. Original source levels
0/1 retain `NoClamp=8192`; levels 2/3 retain `ClampV=32768`. No sampler override,
UV repair or new clamp mode is introduced. Exact source resolution bits pin
the last level to **6.541876792907715**, not the rounded diagnostic 6.54188.
The regression test explicitly rejects that rounded alias.

Concrete retained cases remain:

- `kasna_sidegif`: upper dark aperture/hardware patch plus lower vegetation/soil;
  four inspected clean-panel bounds select no complete ordinary faces.
- `kasna_sloupekgif`: dark central pipe/strip photograph; inspected clean side
  bounds select nothing, and the near ordinary area is only .176 square metres.
- `kasna_nesmysl`: small dark blue/green ornament image without independently
  established mineral/coating/metal reflectance.
- `kasna01_dno`: green watery/dirty basin photo containing coins/debris, not
  uniform stone; near ordinary geometry is only .117 square metres.
- The upper Nogova lion/spout band, animated water/bottom relatives, original
  authored triangles and invalid geometry retain their independent contracts.

No artwork, material maps, texture cleanup, geometry, normal, UV, vertex-lighting
hint or original asset changes. Original provenance, pixel identities, four
geometry/binding identities, 14 UV trials, slab geometry and sampler flags are
in the [source receipt](evidence/material-modernization/batch-22/source-audit.json).
Raw decoded originals and full UV buffers stay ignored locally.

## Actual gameplay and explicit route limits

Original Malden `01TakeTheCar` and Nogova `Resistance/R01WarCry` run through the
existing direct developer harness, copied profiles, `--nosound` and synchronization
validation. Terrain-grounded standing views inspect original WRP placements.
This is not Steam-route, all-placement or mission-completion acceptance.

Compare the classic [baseline native](evidence/material-modernization/batch-22/baseline-fountain-malden-gi.png),
[candidate native](evidence/material-modernization/batch-22/final-fountain-malden-gi.png)
and [candidate CPU](evidence/material-modernization/batch-22/final-fountain-malden-CPU.png),
and the Nogova [baseline native](evidence/material-modernization/batch-22/baseline-fountain-nogova-gi.png),
[candidate native](evidence/material-modernization/batch-22/final-fountain-nogova-gi.png)
and [candidate CPU](evidence/material-modernization/batch-22/final-fountain-nogova-CPU.png).
Same-runtime CPU PBR-off and auto/GI-off captures separate raster appearance
from GI reception. Original water/debris and the mixed wall photographs are
preserved; no broad wall or full fountain relighting is claimed.

Stone-family diagnostic footprints at 1080p illustrate why a nonzero counter
alone is not new material reception:

| Scene | Native before / candidate | CPU before / candidate |
| --- | ---: | ---: |
| Classic | 0 / 0 | 0 / 7,567 |
| Nogova | 9,657 / 9,663 | 10,763 / 31,351 |

These are separate half-resolution frame samples, not source-specific pixel
counts or world percentages. Nogova's native count is essentially unchanged
surrounding scenery; it does **not** demonstrate new fountain reception. The
large CPU change and actual CPU fountain appearance are the demonstrated gain.
The earlier native-positive interpretation was corrected against the baseline.
No new identity appears as an eligible hit in the bounded probes, so **new
reflected-hit participation is not demonstrated** either. Probe absence does
not prove zero all-world participation or identify an exact rejecting gate.
Existing live object/shape/section/texture animation and capture gates remain
unchanged; source prerequisites are not substituted for live capture evidence.

Morning, overcast, dusk, grounded 0/8/22/52/92-metre camera trials, enhanced/auto,
one/two bounce, moving/settled TAA and FSR Quality controls are exercised.
The classic 22-metre retreat moves behind intervening architecture/vegetation
and hides the fountain; that retained diagnostic image is not fountain LOD
acceptance. Nogova's 22-metre view retains the fountain in the town scene.
Movement captures are settled views, not in-motion screenshots. Spatial-only
MSAA GI grain remains a limit, not clean MSAA or whole-scene acceptance.
Existing foliage, water, LOD and CPU/native differences are separate.

## Cost and validation

RTX 4060 Ti, TAA100%, one bounce, RT sun/cloud shadows, AO/water/volumetrics off.
No build, heavy census or other game runs during timing. Native-route runs warm
ten seconds, then use Off/On/On/Off with four-second settling and six-second
sampling at 1920x1080 and 2560x1440:

| Native-route scene | GI-on GPU frame, before / candidate | Trace + filter + history, before / candidate |
| --- | ---: | ---: |
| Classic / 1080p | 4.957 / 5.042 ms | .915 / .942 ms |
| Classic / 1440p | 5.167 / 5.175 ms | .938 / .930 ms |
| Nogova / 1080p | 6.087 / 6.285 ms | 1.601 / 1.771 ms |
| Nogova / 1440p | 6.284 / 6.593 ms | 1.843 / 1.811 ms |

These native-route costs do not establish the cost of the demonstrated CPU
receiver gain. Four separate 1080p comparisons hold hardware T&L **Off** through
all intervals, with actual Off/On restoration verified in game stdout and
CPU-only terrain reception checked in both GI-on states. They warm 20 seconds,
settle eight, then sample at least 45 seconds and until three fresh GPU-stage
reports are collected for each GI-on interval (120-second limit). The renderer
logs those stages every 120 completions; the initial short CPU windows miss
some reports and are explicitly excluded. No logger cadence, graphics setting
or acceptance gate is weakened to manufacture a benchmark.

| Held-CPU scene / 1080p | GI-on GPU frame, before / candidate | Trace + filter + history, before / candidate | CPU recording, before / candidate |
| --- | ---: | ---: | ---: |
| Classic | 27.787 / 34.115 ms | 8.941 / 8.518 ms | 108.380 / 108.018 ms |
| Nogova | 40.870 / 36.222 ms | 8.418 / 11.470 ms | 124.809 / 131.643 ms |

All intervals, durations, sample counts, frame/stage/world costs, CPU recording,
receiver states, exact loaded binaries and excluded short CPU diagnostics are
in the [qualification receipt](evidence/material-modernization/batch-22/qualification.json).
The CPU-transform route is slow here and is not recommended by this assessment.
Scene deltas, including negative stage differences, are not an optimization,
an isolated material cost or a universal performance budget.

The twelve paired native/CPU runs normally shut down after **88,198 presented
frames** (80,974 native-route and 7,224 held-CPU), with **zero validation errors
and zero warnings**. Ten hardware-T&L toggle pairs restore the native route.
Long preflight idle frames are lifecycle evidence, not extra timing samples.
The four superseded short CPU diagnostics and two scouts are excluded from
these totals and admitted timing. A fresh GL33 control reports all five PBR
commands unsupported and zero changed world-ROI pixels. All 270 original input
hashes and 67 settings/fixture/input hashes are unchanged; private DLSS remains
clean and remote-free, with its SDKs and preserved snapshots present.

Fresh censuses inspect 726 models / **1,708 ordinary-adapter retained levels**
and **1,711 authored levels**; all 726 historical reports verify. Replaying only
sources 198-200 and region 562 reproduces the prior complete prerequisites with
zero differences. All seven family geometry/binding identities, UVs, normals,
flags and authored hints are unchanged. The same three forest source-only
levels lack retained historical native comparison. The immutable inventory is
not rewritten. The 642 highest-detail / 1,240 alternative-level prerequisites
are not visible coverage, an adoption target, receivers or proof of reflected hits.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-22/final-runtime-census `
  --output build/material-modernization-20261021/batch-22/final-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 200 `
  --replay-regional-identities @(512..562)
```

Add `--all-source-lods` with separate cache/output under
`batch-22/final-source-census`.

Focused tests pass **87 cases / 100,684 assertions** on Vulkan and
**86 / 100,644** on GL33; 690 policy guards and 18 inventory/census tests pass.
The admitted executable is
`9FD4235FABF4C4B5B98730E836C2126C99C69A6ADFA60FE5800229085539D929`.
No shader, transport, filter, ray budget, reserved identity range or safety
function is changed. Original v3 identities and earlier functionality remain.

The ledger records **214 complete source assessments, 124 partial, 177 pending
and 211 retained existing contracts**. Both fountain source reviews close with
specific original-source and live-route limits, not full rendering coverage.
The wider original-world goal continues.
