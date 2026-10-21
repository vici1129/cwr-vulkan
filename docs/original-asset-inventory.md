# Original asset library and modernization backlog

Source/render-policy revision: `2817c57` (latest public main at scan start).
Snapshot: 2026-10-21, available `game-local/Remastered` installation.
This is read-only source reconnaissance, not a new rendering implementation or
a claim about every historical retail distribution. Archive hashes identify the
exact available stock/remastered inputs, including the local `cwr_logo` addon.

[Complete compact inventory](evidence/original-asset-inventory/inventory.json)
and [30-item prioritized backlog](evidence/original-asset-inventory/backlog.json)
contain paths, provenance and per-island usage. No original asset bytes are
published. The backlog bundles close variants into **30 tasks / 78 models**.

Implementation batches and the current per-model assessment ledger are tracked
in [original-world material modernization](material-modernization.md). The
inventory below remains the original source-policy snapshot.

## Library size and actual world references

| Scope | Unique model paths | Unique PAC/PAA paths |
|---|---:|---:|
| Globally mounted DTA/AddOns archives (43 PBOs) | 1,400 | 7,043 |
| Mission/template containers (36 PBOs; separate namespaces) | 0 | 36 |
| Loose stock files outside PBOs | 0 | 187 |
| Complete available library | **1,400** | **7,266** |
| Directly referenced by the four WRPs | **726** | see below |
| Visual sections of those 726 models, across all visual LODs | — | **1,433** |
| WRP terrain texture-table declarations | — | **1,120** |
| Union of those two texture reference sets | — | **2,553** |

There are **407,848 WRP placement records**. All 726 model paths resolve, and
all 2,553 referenced textures resolve and decode: 1,946 opaque, 337 cutout,
270 blend. These two texture reference sets do not overlap in this snapshot.
There are 674 library models and 4,713 textures outside these direct reference
sets; **that does not mean unused**. Proxies, effects, UI, vehicles, soldiers,
mission-spawned/config-driven objects and other worlds can use them. They were
inventoried as resources, not inspected as placed-world geometry.

| Island / source | Placements | Unique models | Visual-section textures | Terrain declarations |
|---|---:|---:|---:|---:|
| Malden / `Worlds/abel.wrp` | 103,114 | 248 | 643 | 190 |
| Everon / `Worlds/eden.wrp` | 56,740 | 264 | 581 | 241 |
| Kolgujev / `Worlds/cain.wrp` | 70,770 | 152 | 418 | 390 |
| Nogova / `Noe.pbo:noe.wrp` | 177,224 | 345 | 629 | 302 |
| Union / sum | **407,848** | **726** | **1,433** | **1,120** |

Island model counts overlap; only eight model paths occur on all four islands.
Terrain counts are declarations in the WRP table, not measured painted area or
a promise that every table entry is used by a terrain cell.

| Placements per model, four-island sum | Models | Placement records |
|---|---:|---:|
| 1 | 171 | 171 |
| 2–9 | 206 | 792 |
| 10–99 | 189 | 6,315 |
| 100–999 | 94 | 32,624 |
| 1,000+ | 66 | 367,946 |

Frequency is heavily skewed toward vegetation: `o/tree/dd_bush02d.p3d` alone
has 27,602 placements, `dd_bush01c` 27,045 and `smrk_maly` 21,859. A frequency-only
queue would therefore mostly select unsupported canopy transport, not easy
building modernization.

## Source-level PBR/GI qualification

All **726 referenced models / 1,711 visual LODs** were inspected with the legacy
optimized ODOL loader and existing shared material qualifier. Every visual section records
its texture, original flags, material-index histogram, normal checks and
qualification result. Visual means source resolution `<10000`, matching the
existing tool/runtime contract; collision, memory and shadow LODs are not counted
as visual surfaces.

Loader clarification (batch 03): ordinary ODOL gameplay uses ModelCache and
ShapeAdapter, whereas this original snapshot used `LODShape::LoadOptimized`.
The [current runtime-adapter census](evidence/material-modernization/runtime-census.json)
reinspects all 726 / 1,711 and, after explicitly replaying the older source policy,
matches every LOD count, normal/vertex hint and complete section prerequisite
multiset. Section order is not stable between these routes. The original snapshot
is retained unchanged; it does not supply a runtime source fingerprint. Use the
explicit `model inspect --runtime-loader` diagnostic for gameplay UV/fingerprint
work. Neither route substitutes for a live instance/lighting/receiver check.

For an interpretable geometry denominator, the following takes **one
highest-detail visual LOD per distinct model** (minimum resolution, then index),
without multiplying by placements or adding mutually exclusive LOD alternatives:

| Shared automatic-qualification result | Triangles |
|---|---:|
| Eligible source/geometry/render prerequisites | **3,108** |
| Otherwise eligible, but no trustworthy source material identity | 104,662 |
| Unsupported opacity/render behavior | 60,144 |
| Authored lighting / fitted-land vertex hints | 46,894 |
| Invalid geometry prerequisites | 301 |
| Total inspected highest-detail section triangles | **215,109** |

These are mutually exclusive **first-failure buckets**, not independent counts
of every defect. Flags/opacity can mask a later normal or identity failure.
The geometry bucket covers the classifier's normal/UV/position/area checks;
it is not evidence that all 301 failures have the same cause. Separately,
132 models have at least one invalid normal in some visual LOD.

The shared qualifier passes every highest-detail triangle in 8 models, some
triangles in 115, and none in 603. The 123 models with any passing triangle
account for 1,937 placements, **not 1,937 fully modernized objects**. Of the
3,108 passing triangles, 3,097 use existing v3 identities and 11 enhanced-only
identities. None gain the new source-declared identity 256. In particular,
1,600 passing signpost triangles use reflector-only identity 86; they do not
establish GI-receiver eligibility.

This is **not a complete census of already functioning render features**.
Legacy lighting, alpha traversal, RT shadows and the hand-verified enhanced
catalog have their own contracts. The existing four catalog entries cover
`data3d/zed_kamenna.p3d` and `data3d/zed2.p3d` (169 and 165 placements respectively),
with model/LOD/section/pixel/geometry fingerprint gates. Their declarations are
retained separately in the inventory; this CLI does not evaluate those
fingerprints. Do not label them unsupported merely because the stricter generic
qualifier rejects their fitted/authored hints, and do not add their declared
triangles to the passing total without checking that separate contract.

Reflection and reception remain independent. Static eligibility is only a
prerequisite: reception needs captured ambient/direct inputs, compatible live
object/material state and the existing opaque-world gates. No gameplay capture
was performed here. This inventory does **not** measure visible coverage,
distance-weighted importance, FPS, or actual receiver pixels. Enhanced features
remain developer-only/default Off, and one bounce remains default.

## Main blockers and next actions

1. **Missing optical identity / mixed opaque atlases.** Finite normals and opaque
   pixels do not distinguish walls from painted glass, doors or metal hardware.
   Preserve existing roofs; seek defensible source/section separation before
   any future material adoption. Do not turn the backlog into a filename allowlist.
2. **Alpha/thin vegetation and authored lighting.** High placement frequency does
   not make canopy geometry an opaque dielectric. Retain cutout traversal and
   original canopy lighting; a correct thin-surface contract is separate work.
3. **Fitted ground and special render behavior.** Roads and some walls carry land
   hints/depth/lighting behavior. Existing hand-verified catalog exceptions are
   not permission to bypass those gates for unrelated models.
4. **Local geometry/LOD defects.** Invalid normals/degenerate or otherwise invalid
   triangles must keep fallback. Check all visual LODs, not only the near mesh.
5. **Receiver provenance.** Authored packed lighting cannot supply an independent
   ambient term. A successful static reflector audit alone cannot enable reception.

The base original `CONFIG.BIN` was supplied to every qualification. An additional
audit of 32 config files in the stock addon archives found no addon `CfgMaterials`
classes, and none of their 21 mapped texture references overlap the placed-model
visual-section set. Thus those addon mappings do not change this snapshot's
placed-model source classification. This is not an assumption about future mods.

## Prioritized modernization backlog

Ordering is a **curated engineering judgment**: narrower opaque/source audits
first (A), mixed-source or section-specific work next (B), new transport/ground
contracts deferred (C). Placement frequency and family reuse inform the order;
it is not a pure frequency ranking or a promise of material identity. Names
describe audit targets, not approved physical classifications. All exact model
paths, per-island counts, triangle buckets and next actions are in `backlog.json`.

| Rank / tier | Candidate (close variants bundled) | Placements | Next boundary to establish |
|---|---|---:|---|
| 1 A | Nogova rock variants (15 models) | 2,353 | Verify original surface/source consistency |
| 2 A | Nogova `zidka01`–`04` walls | 1,145 | Verify narrow opaque sections; preserve namespaces |
| 3 A | Classic green/rust brick fence pillars | 262 | Pillar-only source proof, not wire/gate adoption |
| 4 A | Nogova `drevo_hromada` wood pile | 48 | Audit all sections; opacity/name is not proof |
| 5 A | Classic `statek_kulna` farm shed | 54 | Preserve supported wood; inspect remaining sections |
| 6 B | Classic green wire fence | 1,048 | Separate hardware and retain cutouts |
| 7 B | Classic rust wire fence | 518 | Do not mistake opaque metal for dielectric |
| 8 B | `jezek kov` anti-tank obstacle | 554 | Establish appropriate metal/paint response |
| 9 B | Classic farm main building | 48 | Keep roof; resolve wall/window identity |
| 10 B | `budova1` / `budova2` barracks | 75 | Mixed opaque atlas; windows/doors stay legacy |
| 11 B | `dum_mesto3` city-house pilot | 29 | Keep four roof triangles; facade remains unresolved |
| 12 B | Classic `dum_rasovna` variants | 68 | Exterior/interior and authored-lighting separation |
| 13 B | Classic brick-house variants | 75 | Roof, authored indices and geometry fallback |
| 14 B | Nogova `stodola` barn | 73 | Separate opaque, cutout and authored ranges |
| 15 B | Nogova `stodola3` barn | 35 | Verify opaque atlas identity and openings |
| 16 B | Nogova grey house | 53 | Window/interior/cutout separation |
| 17 B | Nogova ruined house | 50 | Preserve openings and authored shading |
| 18 B | Nogova Podhradi house | 45 | Opaque identity, authored ranges stay legacy |
| 19 B | Nogova yellow farm | 43 | Mixed facade and cutout atlas |
| 20 B | Nogova yellow house | 39 | Windows, interiors and authored indices |
| 21 B | Nogova smithy house | 33 | Source separation across LODs |
| 22 B | Nogova multi-storey house 02 | 11 | Opaque mixed sections versus authored shading |
| 23 B | `panelak` / `panelak2` apartments | 33 | Keep 201/205/207, cutouts, interiors and invalid geometry legacy |
| 24 B | Classic chapel | 56 | Verify source identity, not opacity alone |
| 25 B | Nogova rubble wall | 74 | Original source/geometry consistency |
| 26 B | Classic pine trunk candidates | 14,622 | Trunks only; foliage stays legacy |
| 27 B | Nogova maple trunk candidates | 14,563 | Verify trunk/leaf separation across LODs |
| 28 C | Nogova small spruce canopy | 21,859 | Requires thin/cutout vegetation contract |
| 29 C | Nogova common bush canopy | 27,602 | No qualifying diffuse region; retain original response |
| 30 C | Classic fitted roads (24 models) | 13,098 | Requires fitted-ground lighting contract |

For the prior pilots, the complete WRP counts are: barracks 52 `budova1` and
23 `budova2` across the classic islands; apartments 26 `panelak` and 7 `panelak2`
on Nogova; `dum_mesto3` 3 on Malden and 26 on Everon. They remain verification
cases, not privileged qualification names.

## Method, reproducibility and limits

The single [orchestration helper](../tools/materials/inventory_original_assets.py)
uses existing `PoseidonTools pbo list/extract`, `terrain inspect/objects/textures`,
`model inspect --textures --sections --qualify`, `image inspect`, and config JSON
tools. No new decoder, renderer feature, asset-management service or pipeline was
introduced. Python 3.11+ is sufficient; workers launch bounded independent CLI
inspections. Existing tool output is retained locally for auditing.

```powershell
python tools/materials/inventory_original_assets.py `
  --game game-local/Remastered --tool dist/vulkan-local/PoseidonTools.exe `
  --cache build/original-asset-inventory-20261021 `
  --output docs/evidence/original-asset-inventory/inventory.json
python -m unittest discover -s tools/materials -p test_inventory_original_assets.py
```

Use a fresh cache for different inputs/tool versions; `--phase inspect` reuses
the archive scan and raw inspection outputs. DTA/AddOns paths use the engine's
bank-name namespace. Other PBOs retain `@<container-path>/` scope. Normalize case,
slashes and redundant `.` only; never deduplicate by basename, extension swapping,
or identical pixels. No normalized-path collisions occurred in this snapshot.

The inventory schema keeps complete library `models`/`textures` lists; `sources`
identify archive index plus original entry or loose path and SHA256. Placed models
add `placements` and `inspection.visualLODs`; each section retains source identity,
flags, material-index counts, normals and triangle buckets. Texture `worldModels`
is a reverse index for nonempty visual sections, `terrainWorlds` is a separate
WRP-table index, and `decoded` records dimensions/alpha. Empty/default texture
references are not counted as PAC/PAA assets. `summary` and `coverageContract`
define aggregation units. Per-section vertex histograms can overlap and must not
be summed as distinct mesh vertices. Nonvisual and mission/proxy-only geometry
are outside the inspection denominator.

Two existing-tool limits are retained, not repaired in this read-only task:
preview-section `tris=` values are not the native fan-count metric and are not
used for coverage arithmetic. Also, LOD 3 of `o/tree/les_nw_jehl_t1`, `t2` and
`trojuhelnik` reports preview faces but zero native section triangles. These three
lower-LOD discrepancies are explicitly recorded; their highest-detail LODs are
inspected normally. Native section indices describe this loader inspection and
are not a new stable asset-ID system or a proposed hardcoded allowlist.

Checks: all WRP parsed placement rows equal the existing reader's object totals;
all model/texture paths resolve; 726 model inspections and 2,553 decodes succeed;
all eight parser/namespace/dataset-integrity tests pass, as do the four existing
material-qualification cases (92 assertions). No game/GPU run is claimed for this
read-only source inventory. All **270 original inputs**
(79 PBOs, 187 loose textures, three loose WRPs and base config) retain their hashes.
Original settings were not opened for writing. Private DLSS remains clean and
remote-free, and both private SDK directories remain present. No renderer,
game settings, original assets, replacement artwork or generated maps changed.
