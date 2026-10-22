# Helfenburk: exact mineral rows and broken wall edges

Base: `c7b9fed0bf0ddc7fc64f30925831efc3a2a85222`.
Five original models / **20 authored visual levels / six Everon placements**
are assessed: tower, gate, second building, battlement and two wall instances.
Nine source-unknown images are physically viewed among 17 decoded references.
Original cutout composites remain separate from opaque mineral proof, including
the detached wall's reused composite front even in its highest authored level.

## Original pixels, geometry and UV scope

Whole enhanced-only identity **201**, `data/helfenburg_zhrouc_zd.pac`, uses the
existing unmeasured .9 / stone-family-6 approximation. Its original broken
stone/mortar photograph includes inter-stone contact shading, not an independent
window/background/soil patch. Original repeated U/V and authored lighting gates
are preserved. It supplies broken wall edges, not all surrounding masonry.

The other photographs are not whole-source stone. Exact source-pixel,
model/geometry/binding identities support only these mineral-only rectangles:

| Region | Model / source | Original UV box | Ordinary triangles by authored level |
| --- | --- | --- | --- |
| 563 | Tower / `helfenburk_vez_front` | U[-10,10], V[.56,1.005] | 48 / 48 / 48 / 8 / 0 |
| 564 | Battlement / `helfenburg_hradby_in` | U[-10,10], V[.22,1.005] | 8 / 7 / 5 / 4 |
| 566 | Wall / `helfenburg_hradby_in` | U[-10,10], V[.22,1.005] | 10 / 10 |
| 567 | Gate / `helfenburk_brana_ins` | U[.005,.34], V[.39,.98] | 38 / 38 / 0 / 0 |
| 568 | Same gate/source, other side | U[.64,.995], V[.39,.98] | 28 / 28 / 0 / 0 |

All sources in the table are original `data/*.pac`; model keys and exact
pixel fingerprints are in the [source audit](evidence/material-modernization/batch-23/source-audit.json).
The five admitted regions require **zero split original face fans**, evaluated before
independent authored/geometry rejection. Tower V=.54 cuts two fans and is not
used. The original repeated U samples the same mineral-only row. Tiny lower V
overshoot stays at the original clamped mineral edge; sampler flags are unchanged.
The gate side bounds exclude its arch photograph, upper background and green
bottom pixels. Original LODs and their exact resolution bits/bindings are pinned;
no section ordinal is substituted for a geometry identity.

Near-level mineral-region mesh areas are 53.44, 34.32, 56.67, 16.69 and
17.61 square metres respectively. Broken-edge near mesh areas are 62.47 gate,
32.14 building, 55.97 battlement and 22.54 wall. These are geometry measurements,
not exposed area, receiver counts or visible-world coverage percentages.

Concrete retained cases:

- Upper curtain-wall slit photographs remain original. Gate/building lower-row
  trials cut original fans or select tiny/no complete faces; they do not inherit
  the battlement/wall proof. Far-only wall fragments are not a separate adoption.
- Outer battlement region **565 is withdrawn**. Pilot executable
  `3CCF5E781673C0E9BD50D4AF94CABF869FA4EB93FFD2B24CF03410FF40115BC4`
  produces a new diagonal boundary on the continuous wall. It persists with
  GI off and disappears with PBR off in the same runtime/view: a raster adoption
  failure, not merely GI reception. Whole-face and mineral-pixel proofs, passing
  tests, native receiver pixels and even an eligible reflected hit do not make
  this pilot acceptable. No reception-only workaround, ambient exception, shader
  change or relaxed gate is used. A regression test keeps region565 absent.
  Compare its [GI-off](evidence/material-modernization/batch-23/rejected-helfen-wall-CPU-auto-GI-off.png)
  and [PBR-off](evidence/material-modernization/batch-23/rejected-helfen-wall-CPU-PBR-off.png)
  controls with the [reduced native view](evidence/material-modernization/batch-23/final-helfen-wall-gi.png).
- Tower front windows, hardware and the photographed dark band stay outside the
  lower mineral row. An elevated near-only upper strip and the gate's .590-square-
  metre reuse are not independently promoted.
- Tower side contains photographed windows/door/recesses. Three conservative
  bounds, including the original negative U offset, select no complete ordinary
  faces; no generated mask is introduced.
- Outer gate photo combines crenellation background, deep arch recess and green
  edge pixels. Lower side bounds select no complete original faces.
- `travahlina` is mixed grass/soil, not uniform mineral. `zbor_dum_total_strop`
  retains its dark green/black-stained board-like treatment without a recoverable
  independent surface/finish approximation. Its eight near ordinary triangles
  occupy local Y=12.1889 / 62.64 square metres, not the ground-level courtyard
  floor or visible vertical wall; they are not claimed as new normal-view coverage.
- Far cutout composites, original authored lighting and unsupported original
  geometry/render/capture cases retain their existing contracts.

No original pixel, geometry, UV, normal, authored hint, clamp flag, shader, light
transport, filter, ray budget or safety function changes. Original v3 and earlier
PBR/RTGI functionality remain. No artwork or material maps are created.

## Actual gameplay and distinct evidence

Original Everon `05HeavyMetal` runs through the existing direct developer harness,
copied profiles, `--nosound` and synchronization validation. Terrain-grounded
standing views inspect the original gate courtyard, lower tower, outer
battlement and corrected inner courtyard. This is not Steam-route, physically walked traversal, mission
completion, all-placement or all-distance acceptance.

Compare gate [baseline](evidence/material-modernization/batch-23/baseline-helfen-gate-gi.png)
and [candidate](evidence/material-modernization/batch-23/final-helfen-gate-gi.png),
tower [baseline](evidence/material-modernization/batch-23/baseline-helfen-tower-gi.png)
and [candidate](evidence/material-modernization/batch-23/final-helfen-tower-gi.png),
and battlement [baseline](evidence/material-modernization/batch-23/baseline-helfen-wall-gi.png)
and [candidate](evidence/material-modernization/batch-23/final-helfen-wall-gi.png).
The corrected inner courtyard also has a fresh [baseline](evidence/material-modernization/batch-23/baseline-helfen-inner-wall-gi.png)
and [candidate](evidence/material-modernization/batch-23/final-helfen-inner-wall-gi.png).
Candidate views also have same-runtime native/GI-off, CPU/GI-on, CPU/PBR-off and
CPU/GI-off controls; selected controls are public and all local capture hashes
are recorded. Baselines include native/GI-off and CPU/GI-on controls.
Normal hardware-T&L-On and deliberately toggled CPU-transform diagnostics are
separate. A normal scene can already contain CPU-fallback draws; a family
counter is not a source-specific receiver identity or coverage percentage.

Stone-family half-resolution samples illustrate the route distinction. Each
normal-route cell is native/CPU-fallback pixels; the CPU diagnostic column
lists CPU pixels after deliberately disabling hardware T&L:

| View | Normal route before / reduced candidate | CPU diagnostic before / reduced candidate |
| --- | ---: | ---: |
| Gate | 43,026/0 → 49,550/0 | 42,991 → 48,151 |
| Tower | 99,112/12,178 → 102,062/12,181 | 111,280 → 114,254 |
| Outer battlement | 943/8,316 → 5,721/8,484 | 3,989 → 8,763 |
| Inner courtyard | 0/0 → 89,258/0 | 0 → 60,267 |

These scene-family observations are not source-specific counts. The near
inner wall and broken edges now visibly participate in the normal route;
the source-specific eligible-hit evidence is separate. Original CPU/native
LOD and draw differences are not a clean whole-scene equivalence claim.

The detached wall's [front](evidence/material-modernization/batch-23/final-helfen-detached-wall-gi.png)
retains its original unsupported composite. Its [reverse mineral face](evidence/material-modernization/batch-23/final-helfen-detached-back-gi.png)
has separate same-runtime native/GI-off and CPU/PBR-off/GI-off/on controls. The
reverse view's normal stone-family footprint is **0/0**, versus **0/56,193** in
the CPU diagnostic. New native reception for detached region566 is **not
demonstrated**; coherent CPU reception and actual eligible reflected-hit566
are established separately. No exact rejecting live gate or all-world zero is
inferred. This reverse-face supplement is visual-only, not another GPU benchmark.

Actual raw bounded eligible-hit examples identify **201, 563, 564, 566, 567 and
568** on the reduced build. These are participation examples, not distributions,
source-specific receiver pixels or all-world coverage. Withdrawn565 is absent.
Morning/overcast/dusk, enhanced/auto, one/two bounce, moving/settled TAA and FSR
Quality, MSAA4 and grounded distance trials use existing controls. Movement
images are settled captures, not in-motion frames. MSAA spatial-only grain is
a limit, not clean MSAA acceptance. At the outer-wall22-metre retreat,
hill/foliage hides much of the battlement; that retained image documents the
limit, not complete wall LOD visibility.

## Cost, census and preservation

RTX 4060 Ti; TAA100%, one bounce, RT sun/cloud shadows, AO/water/volumetrics off.
Native-route runs warm ten seconds, then use Off/On/On/Off with four-second
settling and six-second samples at 1920x1080 and 2560x1440 for four main views.
A paired detached-front comparison additionally runs at 1080p. Builds, heavy censuses
and other games are idle during timing. Scene costs are not isolated material
costs, optimization claims or a universal GPU budget. The slow CPU route is a
visual diagnostic here, not separately benchmarked or recommended for play.

| Scene / resolution | GI-on GPU frame, before / reduced | Trace + filter + history, before / reduced |
| --- | ---: | ---: |
| Gate / 1080p | 4.138 / 4.292 ms | .656 / .714 ms |
| Tower / 1080p | 2.374 / 2.216 ms | .357 / .334 ms |
| Outer battlement / 1080p | 5.451 / 5.496 ms | .945 / .973 ms |
| Inner courtyard / 1080p | 2.602 / 2.770 ms | .496 / .653 ms |
| Detached front / 1080p | 5.341 / 5.432 ms | 1.077 / 1.096 ms |
| Gate / 1440p | 4.193 / 4.453 ms | .656 / .729 ms |
| Tower / 1440p | 3.423 / 3.368 ms | .488 / .500 ms |
| Outer battlement / 1440p | 5.488 / 5.806 ms | 1.004 / 1.076 ms |
| Inner courtyard / 1440p | 3.009 / 3.216 ms | .339 / .598 ms |

Values average the two GI-on interval medians. Raw intervals retain individual
stage variation, including filter/history spikes; negative scene deltas are not
an optimization claim. CPU recording/world costs and all Off intervals remain
in the [qualification receipt](evidence/material-modernization/batch-23/qualification.json).

Eight paired runs normally shut down after **162,126 presented frames**, with
**zero validation errors and zero warnings**. Fifteen hardware-T&L toggle pairs
restore the normal route. A separate untimed reverse-face check normally closes
after **4,361 frames**, zero errors/warnings and one restored toggle pair. The
wider raster-failure pilot and scouting/invalid camera trials are excluded from
these totals and admitted timing. Fresh GL33 controls remain unsupported for all
five PBR commands with zero changed world-ROI pixels. All 270 original input and
67 settings/fixture/input hashes are unchanged; private DLSS stays clean and
remote-free, with its SDKs and preserved snapshots present.

Fresh ordinary-adapter and source-only censuses inspect 726 models / **1,708
retained** and **1,711 authored levels** respectively. All 726 historical reports
verify with zero replay differences. Replaying only source 201 and regions
563/564/566/567/568 reproduces the preceding complete prerequisites. All 20 family geometry,
binding, UV, normal, hint and flag identities remain original. The same three
forest source-only levels lack historical retained comparison.

The reduced admission has **313 highest-detail / 681 alternative-level** additional prerequisites, which are
not an adoption target, actual receivers or rendering coverage. Focused tests
pass **88 cases / 110,333 assertions** on Vulkan and **87 / 110,293** on GL33;
690 policy checks and 18 inventory/census tests pass. Existing qualification
functions and the old enhanced-material test file remain byte-identical to base.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-23/accepted-runtime-census `
  --output build/material-modernization-20261021/batch-23/accepted-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 201 `
  --replay-regional-identities @(512..568)
```

Add `--all-source-lods` with a separate `batch-23/accepted-source-census` cache/output.
The admitted Vulkan executable is
`0211734B8F50236DECA50AD864965F0AD86736DEE751656992EBEDAA0CFC29BC`.
The ledger now has **219 complete source assessments, 121 partial, 175 pending
and 211 retained existing contracts**. All five castle source reviews close with
the explicit pixel, geometry, raster-failure and runtime limits above.
The wider original-world goal continues; complete source assessment is not
complete rendering coverage.
