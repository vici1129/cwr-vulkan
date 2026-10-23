# Batch 34 — Original Nogova industrial buildings

From `origin/main` at `e411876`: four source assessments completed across
`o/hous/tovarna1.p3d`, `hangar_2.p3d`, `vysilac_fm.p3d` and `benzina.p3d`:
19 authored visual LODs, 22 original placements, all 16 pending texture keys.
Twenty exact-model regions use the existing mechanism; no whole mixed atlas,
renderer infrastructure, texture map, geometry, transparency or safety gate changes.

## Source decisions

Original encoded hashes and decoded pixels, complete original triangle fans,
UVs, normals, material indices, authored lighting and sampler boundaries were
reviewed across every LOD. The shared floor is the same source in all four
models; `doravnik` and `tovarna1_dopravnik` are demonstrably different sources.
The corrugated sheet fingerprint matches Batch 32. Original weathering, baked
shading and tiny painted/fastener details remain artwork. Accepted approximations
use roughness .90, F0 .04, metallic zero; these are not measured coefficients.

All paths below are under `o/hous/`. “Accepted” describes bounded source
qualification, not proof of visible coverage or GI reception. Detailed per-model,
per-LOD dispositions, gates and fingerprints are in the [source audit](evidence/material-modernization/batch-34/source-audit.json).

| Pending original key | Disposition |
| --- | --- |
| `benzina01.pac` | Accept intact pale kerb 616 and reflector-only fascia 617. Broad mixed-apron trial lost dark paving detail at dusk; narrow to kerb, retain displays/signage/dark apron. |
| `benzina_asfalt.pac` | Built/tested 618 rejected: broad nearly flat gold asphalt with PBR on, already GI off. Original asphalt retained. |
| `benzina_strop.pac` | Reflector-only 619 on five ordinary triangles each in LOD2/3; near authored canopy remains excluded. |
| `pumpa_barak.pac` | Accept lower-half rough plaster 620; retain upper shutter/door mixed faces. |
| `doravnik.pac` | Reflector-only upper/lower coated panels 624/625; retain mixed central truss/belt/aperture fans. |
| `tovarna1_dopravnik.pac` | Built beam 622 blocked by eight near hangar land-hint vertices; later hangar and all factory uses are authored. No gate bypass or accepted entry. |
| `tovarna1_podklad.pac` | Accept ordinary concrete supports 626; authored siblings excluded. |
| `tovarna1_podlaha1.pac` | Accept exact factory floor/roof 627, transmitter base 636 and alternative station roof 621. Authored/land-gated hangar floor and authored station near roof retained. |
| `tovarna1_stenainter.pac` | Accept upper plain plaster 628; lower doorway, metal door, pipe and electrical-fitting atlas retained. |
| `tovarna1_stenalux.pac` | Accept bottom blank strip 629: only two triangles each at LOD1/2, none near. Built near inter-window 630/631/632 rejected for conspicuous CPU PBR-on/GI-off bright blocks. Window atlas retained. |
| `tovarna1_stenaspod.pac` | Accept dedicated distressed lower plaster 633, including baked dirt/shading. |
| `tovarna1_strop1.pac` | Accept ordinary coarse ceiling 634; broad authored interior ceiling remains excluded. |
| `tovarna1_vrata.pac` | Reflector-only top rail 635 and intact right door 640. Broadened trial to complete original fans; isolated left box selects none; central dark hatch retained. |
| `vlnityplot02.pac` | Accept rough coated sheets 623 only at LOD2/3/4: 79 alternative prerequisites. Both nearest LODs remain whole-shape land-gated; other model users unchanged. |
| `vysfm_2.pac` | Reflector-only mast/coating 637; 16 invalid last-LOD triangles retain geometry fallback. |
| `vysfm_main.pac` | Reflector-only cabinet side/top and painted door 638/639; retain other marking/mechanical/ladder and authored faces. |

Fresh source inspection adds **1,752 highest-detail / 4,925 alternative-LOD
prerequisites**, not simultaneous triangles or visible coverage. Of the original
8,198 source-unknown alternatives, 3,273 retain explicit fallback reasons.
The unchanged scoped buckets are 9,758 authored-lighting, 1,562 render-behavior
and 16 geometry triangles. Original normals and UVs are unchanged.

## Actual rendering and limits

Original `R01WarCry.Noe` gameplay controls use copied profiles and `--nosound`:
native PBR Off / auto-GI Off / auto-GI On at 09:00 and 17:30, four exterior
headings per model plus six interior/roof/support views, advancing-simulation
camera translations/turns and retreats. These are developer-camera observations,
not physical traversal, Steam-route acceptance or mission completion.

- **Station:** modest rough plaster/kerb response; facade reception rises from
  zero to 7,650 native samples in the full run and 7,661 in the final guard.
  CPU front records 7,671 versus zero baseline. The final close shutter-facing
  CPU view has zero facade reception. Original dark asphalt stays intact;
  the accepted kerb has a thin warm dusk highlight, not the rejected gold apron.
- **Factory:** native eligible reflector hits independently identify coated
  conveyor panels 624/625, concrete supports 626, lower plaster 633 and door 640.
  Native facade/concrete reception remains zero in tested views. Final CPU front
  demonstrates **56,215 facade samples versus zero baseline**, with coherent
  full-face lower-wall brightening and the rejected window-strip blocks removed.
  Tested roof/interior/support views still have zero new concrete/facade reception;
  source qualification is not a claim of broad floor or roof GI.
- **Transmitter:** native hits 637/638/639 demonstrate the ambient-preserving
  rough mast/cabinet reflectors. No new base reception on either route; surrounding
  rock, timber and stone samples are not credited to the transmitter.
- **Hangar:** no new near reception or eligible 623 hit demonstrated. Yard hits
  625/626 belong to the factory. The accepted farther sheet prerequisites do not
  establish visible hangar adoption; authored interior/floor and near land gates remain.

Counters are half-resolution family samples, not target masks or coverage
percentages. Matched [factory CPU](evidence/material-modernization/batch-34/noe-tovarna1-front-CPU-comparison.jpg),
[native factory](evidence/material-modernization/batch-34/noe-tovarna1-front-comparison.jpg),
[station](evidence/material-modernization/batch-34/noe-benzina-interior-comparison.jpg)
and four motion sheets retain the appearance evidence. Bounded motion showed no
new gross material-boundary failure. Long factory-yard retreats can frame neighboring
hangars; they are not isolated target-LOD transition proofs.

The full pre-final run contained the three subsequently rejected factory strips.
The final binary differs by exactly deleting those entries, checked mechanically.
All seven factory native view matrices, seven factory/close CPU controls, three
other-model native guards, missing close probes and factory/yard timings were
completed on final bytes. Earlier focus-lost absent controls are excluded, not
counted as passes. Unchanged other-model full controls retain their separate hashes.

## GPU cost and validation

RTX 4060 Ti / NVIDIA 617.14, native 1920×1080 TAA100%, auto materials,
one-bounce GI, RT sunlight/cloud shadows; AO/water/volumetrics off. Warm 10 s;
GI Off/On/On/Off, settle 4 s, sample 6 s; means of paired interval medians.
No build/tests/census ran during timing. Units below are ms.

| Original camera workload | GPU frame GI Off, baseline → accepted | GPU frame GI On, baseline → accepted | Trace+filter+temporal, baseline → accepted |
| --- | --- | --- | --- |
| Factory, final binary | 2.188 → 2.323 | 2.434 → 2.368 | .283 → .382 |
| Hangar yard, final binary | 2.170 → 2.158 | 2.412 → 2.345 | .328 → .316 |
| Transmitter, unchanged full run | 3.148 → 3.174 | 3.468 → 3.566 | .600 → .603 |
| Station, unchanged full run | 4.352 → 4.442 | 4.825 → 4.990 | .652 → .784 |

These are scene workloads with ordinary variation and scenery, not isolated
material costs or FPS gains. Added GI-stage work is about .100 ms factory and
.132 ms station. CPU-lit-route performance was not separately timed; native
CPU-command recording is not a substitute. Raw intervals, binary hashes,
probes/counters, rejected trials and capture provenance are in the [qualification receipt](evidence/material-modernization/batch-34/qualification.json).

Public Vulkan/tools and GL33 builds pass. Final focused suites: **95 cases /
204,728 assertions Vulkan**, **94 / 204,688 GL33**; road-order suites **17 / 52**
each, plus **690** policy checks and **19** inventory/census tests. Both fresh
726-model census regressions have zero replay differences: 1,708 runtime /
1,711 authored LODs. Vulkan runs present 438,068 frames including explicitly
separated rejected trials, with zero synchronization-validation errors/warnings.
Live GL33 keeps all four view ROIs pixel-identical across five PBR commands.
All 270 original-input and 67 settings-fixture hashes remain unchanged; private
DLSS stays clean/remote-free with preserved SDKs/snapshots. No shader, road/surface,
alpha, authored-lighting, captured-ambient or qualification gate is changed.

The ledger now records **360 complete / 119 partial / 36 pending** source
assessments and 211 retained existing contracts. Remaining assessment backlog:
155 models / 437 distinct pending keys. Global prerequisite totals are 21,978
highest-detail / 43,556 alternatives. Complete assessment is not rendering adoption.
The original inventory remains immutable. Stop after Batch 34.
