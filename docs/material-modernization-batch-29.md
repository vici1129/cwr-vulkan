# Nogova multistory building family

Base: `89a1ef5822bfbc6407213d103808aa42575a7542`. All **10 models / 38 visual
LODs / 84 original placements / 19 pending texture keys** are assessed. One
whole roof source and 11 exact regions qualify. Eight models gain **214
highest-detail / 592 alternative-LOD prerequisites**; these are not visible
coverage percentages. **Zero models are fully resolved**, and **one whole
texture key is newly qualified**. All ten remain partial in the material ledger.

| Original `o/hous/` model | LODs | Placements | Added near / alternative triangles |
| --- | ---: | ---: | ---: |
| `dum_patr_nizky_prujezd` | 4 | 6 | 61 / 167 |
| `dum_patrovy01` | 4 | 10 | 13 / 41 |
| `dum_patrovy01c` | 4 | 4 | 15 / 49 |
| `dum_patrovy01d` | 4 | 4 | 15 / 55 |
| `dum_patrovy01prujezd` | 4 | 3 | 63 / 173 |
| `dum_patrovy02` | 4 | 11 | 0 / 0 |
| `dum_patrovy03` | 4 | 7 | 0 / 0 |
| `dum_patrovy04` | 3 | 24 | 24 / 32 |
| `dum_patrovy05` | 3 | 7 | 4 / 8 |
| `dum_patrovy06` | 4 | 8 | 19 / 67 |

## Source dispositions

The [source audit](evidence/material-modernization/batch-29/source-audit.json)
records every pending key, original provenance/pixels, shared users, all-LOD
lighting/UV/geometry gates, regional trials and remaining boundaries.

- **Qualified:** `o/hous/tasky_jeste_tmavsi.pac`, enhanced-only identity **207**,
  dedicated overlapping roof tiles. All six shared model users are in this batch.
- **Partially qualified:** exact-model identities **578–580** admit small mineral
  trim/entry faces in `dum_patrovy03.pac`; **581–586** admit separate plaster and
  one lower-LOD tile/mineral face in `dum_patrovy_balkon.pac`; **587–588** admit
  left-half plaster in `interier_chodba01.pac` on the two passage variants.
  Corridor edge overshoot additionally requires original ClampU|ClampV. Glazing,
  doors, corroded joints and neighboring tile/photo regions remain separate.
- **Previously partial:** `orlhot_kom.pac` reuses Batch 13 brick regions 545–554.
  Frame, aperture and border faces remain untyped; no new chimney coverage.
- **Assessed and retained:** the other **14 keys**, including both dark utility
  fills, gutter/coating uncertainty and mixed facade/shopfront/door sources.
  Several facade bands contain only small fragmented trim; plausible blank-side
  bounds split original polygon fans. None becomes a whole-source identity.

The substantial `dum_patrovy01b.pac` masonry trial is explicitly **rejected**.
Mineral pixels and intact original fans did not prevent a triangular lighting
cut on the original `06` gable: compare trial [PBR off](evidence/material-modernization/batch-29/rejected-dum_patrovy06-gable-native-PBR-off.png),
[PBR on / GI off](evidence/material-modernization/batch-29/rejected-dum_patrovy06-gable-enhanced-off.png)
and [GI on](evidence/material-modernization/batch-29/rejected-dum_patrovy06-gable-gi.png).
All new gable-atlas rules 571–577 are withdrawn, including narrow unaccepted
details; reception deferral would not fix the raster failure. The accepted
[gable](evidence/material-modernization/batch-29/accepted-dum_patrovy06-gable-gi.png)
restores original treatment. No shader or lighting workaround is introduced.

All accepted coefficients reuse existing rough dielectric approximations:
roughness .90, F0 .04, metallic zero, not measured optics or recovered albedo.
Original photographs, geometry, UVs, normals, samplers, authored lighting,
alpha/order behavior and immutable/captured-input gates remain intact.

## Original Nogova rendering and cost

Original `R01WarCry.Noe` runs native Windows Vulkan through the existing developer
harness, copied profiles, --nosound and synchronization validation. All 84 family
horizontal placements match the original dump within .051m, including all 11
camera references. Runtime/dump heights differ, so no vertical/full-transform
equality is claimed. Buildings are never moved. These are
developer-camera observations, not physical traversal, Steam or mission completion.
Exterior cameras are terrain-grounded; the main passage is a low .67m floor-height
view and the corrected low passage is approximately 1.79m. Its first below-floor
scout and obstructed rear scouts are excluded.

The accepted `06` street view increases native roof-family samples **10,595 →
15,424**, with bounded eligible reflector hits identifying **207**. The `04`
front increases native facade samples **0 → 1,833**, with a hit identifying **579**.
Compare the accepted [roof](evidence/material-modernization/batch-29/accepted-dum_patrovy06-front-gi.png)
and [small plaster details](evidence/material-modernization/batch-29/accepted-dum_patrovy04-front-gi.png).
The separate [05 entry](evidence/material-modernization/batch-29/accepted-dum_patrovy05-entry-gi.png)
checks its original small recess faces; its family totals include nearby materials.
These are half-resolution family/frame samples, not identity masks or coverage
percentages. Nearby old roofs/stone and reflected-hit observations are distinct.
Both passages and the windowed `01` roof show **no new native reception**;
corridor reflector identity 587 is observed separately. No CPU-route gain is claimed.

Same-runtime PBR/GI controls, advancing-simulation daylight/dusk translation and
turning, close balcony controls and the restored gable views retain the accepted
boundaries. Original photographic shading/LOD artifacts remain. The retained
[dusk Off](evidence/material-modernization/batch-29/accepted-dum_patrovy06-front-live-dusk-off-move16.png)
and [On](evidence/material-modernization/batch-29/accepted-dum_patrovy06-front-live-dusk-on-move16.png)
are representative movement frames; broader materials are not certified by them.

RTX 4060 Ti, native 1080p, auto/TAA100%, one bounce, RT sun/cloud shadows,
AO/water/volumetrics off. The matched `06` roof workload measures GI-on GPU frame
**4.750 → 4.930ms**, GI trace/filter/history **.546 → .592ms**, and CPU recording
**11.641 → 11.932ms**. Warm10, Off/On/On/Off, settle4/sample6, averaging paired
interval medians. No build, census or other game runs during timing. The added
GI stages are about .046ms here; frame variation is not an isolated material
cost or FPS-improvement claim. Raw intervals remain in the qualification receipt.

## Validation and reproduction

Public Vulkan/tools and GL33 builds pass. Focused suites pass **93 cases /
127,492 assertions** on Vulkan and **92 / 127,452** on GL33; road ordering/ZBias
passes **17 / 52** per build. All **690 policy checks** and **18 inventory/census
tests** pass. The existing rectangle test now scales its interior test triangle
to each region; rejection checks and qualification functions are unchanged.

Fresh censuses inspect 726 models / 1,708 retained / 1,711 authored LODs and
verify all 726 historical reports with zero replay differences. Only accepted
family prerequisites change; 21,999 alternative source-unknown triangles retain
specific boundaries. The baseline/accepted native Vulkan runs close after 89,851
presented frames with zero validation errors/warnings; the rejected trial is separate.
Live Nogova GL33 reports all five PBR commands unsupported and zero changed
world-ROI pixels. All 270 original input and 67 settings/fixture hashes match;
private DLSS remains clean and remote-free, with SDKs and preserved snapshots present.

```powershell
dist/vulkan-local/PoseidonTools.exe model inspect `
  build/original-asset-inventory-20261021/library/o/hous/dum_patrovy06.p3d `
  --runtime-loader --all-source-lods --textures --sections --lighting --qualify --uv-triangles `
  --texroot build/original-asset-inventory-20261021/library `
  --material-config game-local/Remastered/BIN/CONFIG.BIN
```

For the Batch 27 census command, extend enhanced replay through **207** and
regional replay through **588**; withdrawn 565 and 571–577 have no active entries.
The accepted Vulkan executable is
`C3CE9835600075C8E04D909E3F5E565BD13512312FA5C590A77453E5D20F12CC`.
The [qualification receipt](evidence/material-modernization/batch-29/qualification.json)
retains loaded hashes, controls, costs, validation and preservation evidence.
Ledger totals remain **228 complete / 119 partial / 168 pending**, plus **211
retained existing contracts**. Batch 29 stops here; retained fallbacks are not
new PBR coverage.
