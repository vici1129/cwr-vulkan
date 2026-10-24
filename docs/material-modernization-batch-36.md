# Batch 36 — Original observation towers and scaffolding

From `origin/main` at `21b2fdd`: four models, 17 visual LODs, 76 original
placements and all seven pending texture keys assessed. Eight exact-model
regions add conservative rough PBR/reflector treatment using existing paths.
Original ambient is preserved; no new GI receiver, whole-source promotion,
material mask, normal reconstruction, shader, infrastructure or gate change.

| Original model | LODs / placements | Regions | Added highest-detail / alternative prerequisites | Retained render-behavior alternatives |
| --- | --- | --- | --- | --- |
| `data3d/hlaska.p3d` | 5 / 17 | 681–682 | 328 / 592 | 324 |
| `data3d/vez.p3d` | 4 / 41 | 683–686 | 246 / 466 | 32 |
| `o/misc/leseni2x.p3d` | 4 / 12 | 687 | 532 / 946 | 1,440 |
| `o/misc/leseni4x.p3d` | 4 / 6 | 688 | 756 / 1,346 | 2,620 |

Original pixels, complete polygon fans, UV repeats/clamps, normals, material
indices and lighting are inspected across all levels, reusing Batches 12,
20–21 and 32–35. The [source audit](evidence/material-modernization/batch-36/source-audit.json)
records source fingerprints, exact geometry/bindings and per-LOD decisions.
Roughness .90 / F0 .04 / metallic zero approximate dull coating/corrosion;
these are not measured coefficients or a declaration of uniform bare metal.

The black `hlaska_sum` structure and small `hlaska_strana` basket edge retain
their original dark fill. Shared ammunition-store/garage users do not inherit
the new exact-model treatment. Vez's central support, enclosure, narrow posts,
roof and platform/frame components use their original continuous coating
sources, including weathering and photographed frame edges. Its support has
interpolated inward-facing normals; they are preserved. The mottled roof is
actually built and tested as a rough coating approximation, resolving the
Batch 12 followup without guessing a wood/tile/metal substrate.

Scaffold tubes, supports and diagonal braces retain the three rust/shading
strips in `lsni_tce`. Original relief/highlight pixels are not converted into
material masks. Deck planks, clamps and ladders use separate `IsTransparent`
sections; all 4,416 scoped render-behavior triangles remain original. Tower
basket/floor cards and ladder/rung cutouts likewise retain their gates. The
last scaffold visual LOD contains no ordinary tube faces and gains no region.
All eight built reflector candidates are accepted; there are no tested rendering
rejections. Receiver-enabled variants were not tested or represented as failures.
`reception=false` retains the original ambient of these dark/photographically
shaded coatings under the existing family-0 structural-coating contract. No
timber or mineral receiver family is assigned to these metal structures.
No source, UV, normal, sampler or authored-lighting repair is made.

Eight original placements are sampled in Nogova `R01WarCry.Noe`, Malden
`01TakeTheCar.ABEL`, Kolgujev `03Bomberman.CAIN` and Everon `05HeavyMetal.Eden`.
Copied profiles, muted direct developer harness, native 1920×1080/TAA100%,
PBR Off / automatic PBR with GI Off / GI On, 09:00 and 17:30 controls,
advancing-simulation camera translation/turns, 0/8/22/52/92-metre retreats,
and independently verified forced-CPU toggles/restoration are retained.
This is not physical traversal, Steam-route acceptance or mission completion,
nor proof of all 76 placements or selection of every runtime LOD.

The [scaffold comparison](evidence/material-modernization/batch-36/noe-leseni4x-front-comparison.jpg),
[tower comparison](evidence/material-modernization/batch-36/abel-vez-front-comparison.jpg)
and corresponding motion/CPU sheets retain dark support treatment, peeling
coating detail, open basket silhouettes and original deck gaps. The visual
change is subtle. Surrounding old terrain, roof, tree and stone GI is not new
target coverage. Initial building/fence-blocked scouts are excluded; far
terrain/occlusion and original silhouette simplifications remain limitations.
The [distance sheet](evidence/material-modernization/batch-36/representative-distance-controls.jpg)
explicitly retains the obstructed two-level scaffold retreats; these do not
establish clear far-LOD appearance acceptance.

New native GI reception: **zero**. New captured CPU-lit reception: **zero**.
All eight regions explicitly retain ambient. Normal-route probes independently
identify new eligible reflector hits; per-island identities and examples are
in the [qualification receipt](evidence/material-modernization/batch-36/qualification.json).
Observed new identities are 681–684 and 686–688; no eligible hit on the tiny
post region 685 was observed in these bounded probes.
These are GPU GI hit observations, not receiver counts, coverage percentages
or proof that the hit geometry used hardware transformation. Native/CPU family
counters include surrounding scenery and are not model masks. Captured CPU-lit
geometry still uses GPU GI tracing, not CPU ray tracing.

RTX 4060 Ti / driver 617.14; automatic PBR, one-bounce GI, RT sun/cloud shadows;
AO/water/volumetrics off. Warm 10 s, GI Off/On/On/Off, settle 4 s/sample 6 s;
means of paired interval medians. No builds/tests/censuses during sampling.
Whole-scene milliseconds, baseline → final:

| Original view | GPU frame GI Off | GPU frame GI On | Trace + filter + temporal |
| --- | --- | --- | --- |
| Nogova two-level scaffold | 5.513 → 5.525 | 6.186 → 6.251 | .718 → .754 |
| Nogova four-level scaffold | 5.759 → 5.822 | 6.431 → 6.699 | 1.103 → 1.187 |
| Malden open tower | 3.354 → 3.536 | 4.804 → 5.330 | .626 → .826 |
| Malden coated tower | 4.609 → 4.611 | 5.554 → 5.147 | .887 → .854 |

Tower intervals vary substantially, including a negative frame delta. These
are scene observations, not isolated material costs, optimization results or
a universal budget. Forced-CPU performance is not separately timed.

Vulkan/tools and GL33 builds pass. Focused Vulkan: 96 cases / 277,885 assertions;
GL33: 95 / 277,845. Road/surface ordering: 17 / 52 each. Vulkan policy: 690
checks; Python inventory/census: 19 tests. Gameplay synchronization validation
reports zero errors/warnings across 112,879 presented frames, including scouting
and idle frames. Live GL33 controls retain zero changed world-ROI
pixels across all five PBR commands, which return `UNSUPPORTED`.

Fresh 726-model censuses retain 1,711 source / 1,708 runtime visual levels and
reproduce historical prerequisites without regression. Geometry, UVs, normals,
flags, lighting, 270 original inputs and 67 settings/fixture hashes are preserved.
Private repositories, SDKs and snapshots are untouched. Batch 35's audit now
records `reception=false` for region 678 in both duplicated accepted-region
records; the qualified renderer behavior and its existing test are unchanged.

All four source assessments close, **not full-model modernization**. Added
prerequisites: **1,862 / 3,350**; all 3,350 scoped source-unknown alternatives
are resolved as ambient-preserving PBR/reflectors. Global totals: **24,166 /
47,596** qualified prerequisites; **368 complete / 116 partial / 31 pending /
211 retained-existing** dispositions. Backlog: **147 models / 402 distinct
pending keys**. `hlaska_sum` remains pending on its other three users, so seven
scoped keys close but the global distinct-key count drops by six. The original
726-model inventory is unchanged. Stop after Batch 36.
