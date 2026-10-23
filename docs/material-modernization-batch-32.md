# Signpost and corrugated-fence rendering reevaluation

Base: `origin/main` at `78e00f6`. **Both candidates were built, run and accepted
as PBR/reflector-only treatments. Zero new GI receivers or source assessments.**

`data/sloup_bily_orez_v.pac` now explicitly reuses existing identity **86**.
Fresh encoded bytes and decoded RGBA exactly match `sloupek_bily_orezly_.pac`;
the original specular-power lobe hint and receiver family zero are preserved.
The [native sign comparison](evidence/material-modernization/batch-32/b32-noe-117557-front-comparison.png)
shows almost no daylight change in this direction and a subtle dusk response.
Worn paint, dark/light variation and board artwork remain recognizable, without
new post-side/cap seams. The unshaded source names remain independent. An original
end-marker using **both** source names also passes matched original/candidate controls.

`o/misc/plot_vlnplech1.p3d` and `plot_vlnplech2.p3d` gain exact-model regions
**589/590** in the lower half of `o/hous/vlnityplot02.pac`. Each LOD selects four
whole panel triangles, **25.12 m² counting both faces**, with no split fans.
Roughness **.90**, F0 **.04**, metallic **zero**, family **zero** and
`reception=false` use the existing ambient-preserving path. Original pixels,
normals, clamps and six geometry/binding fingerprints remain authoritative.
Separate support/hardware triangles, authored material 207 and excluded faces
stay gated; other atlas users are unchanged. Photographed fasteners remain artwork,
without separately invented fastener optics, normal maps or masks.

Compare the fence [front](evidence/material-modernization/batch-32/b32-plot_vlnplech1-159618-front-comparison.png),
[back](evidence/material-modernization/batch-32/b32-plot_vlnplech1-159618-back-comparison.png)
and [second variant](evidence/material-modernization/batch-32/b32-plot_vlnplech2-159252-back-comparison.png).
Daylight changes are mild; the sun-facing dusk panel becomes visibly warmer,
while its back remains cool. No meaningful new lighting cut, material seam,
silhouette change or movement trail was observed in the tested views. Existing
photographic shading and LOD artifacts remain. This is an unmeasured rough
dielectric approximation, not recovered albedo or a galvanized-metal claim.

Original Nogova `R01WarCry.Noe` runs native Windows Vulkan with copied profiles,
sound disabled and synchronization validation. Eight front/back views cover both
post arrangements and both fence variants at 09:00/17:30. Each compares PBR Off,
PBR On/GI Off and PBR On/GI On; **On means `triPBR 'auto'`**, the existing regional
material mode. Advancing-simulation translation/turning covers the sign and both
fences; fence retreats test 0/8/22/52/92 m. These are grounded developer-camera
gameplay observations, not physical traversal or mission completion.

Native bounded probes identify **86, 589 and 590** as eligible reflectors. The
86 hit position matches the tested original post. **None receives GI:** family
zero retains ambient on native and CPU routes; nearby ground/building receiver
counters are separate. The [receipt](evidence/material-modernization/batch-32/qualification.json)
retains actual states, views, hit examples, hashes and timing intervals.

RTX 4060 Ti / 617.14, native 1080p, auto/TAA100%, one bounce, RT sun/cloud shadows,
AO/water/volumetrics off. Warm10, GI Off/On/On/Off, settle4/sample6; paired interval
medians, with no build/census/tests during sampling:

| Matched workload, GI On | Original → accepted GPU frame | GI trace/filter/history total |
| --- | ---: | ---: |
| Signpost | 4.873 → 4.987 ms | .760 → .788 ms |
| Fence panel | 2.363 → 2.436 ms | .311 → .357 ms |

These are representative scene costs including normal variation, not isolated
material costs or FPS improvements. PBR-on/GI-off frame controls are 4.448→4.558 ms
and 2.171→2.208 ms respectively; no new receiver work is introduced.

Public Vulkan/tools and GL33 builds pass. Focused suites pass **95 cases / 132,198
assertions Vulkan**, **94 / 132,158 GL33**; road ordering **17 / 52** each, **690**
policy checks and **19** inventory/census tests. **86,413 presented Vulkan frames,
zero validation errors/warnings.** GL33 preserves both candidate views across five
PBR commands with zero changed world-ROI pixels. All 270 original-input and 67
settings/fixture hashes match; private DLSS, SDKs and snapshots remain preserved.

Fresh regression censuses cover 726 models / 1,708 retained / 1,711 authored LODs,
with zero replay differences. Texture-scoped census replay for
`--replay-texture-identity data/sloup_bily_orez_v.pac 86` preserves canonical users
of 86; extend the existing regional replay through 590. The
[source audit](evidence/material-modernization/batch-32/source-audit.json) records
all 43 affected models / 48 source LODs and unchanged geometry/lighting/UV gates.
The alias adds 416 near / 418 alternative-LOD prerequisites, including the previously
reviewed end-marker's 6 / 8; fences add 8 / 24. Totals become **19,109 / 36,176**.
These are prerequisites, not visible-coverage percentages or fully modernized models.

Assessment states remain **352 complete / 119 partial / 44 pending / 211 retained
existing contracts**; **163 models** remain in the assessment backlog. No additional
assessment progress is counted. Stop after Batch 32.
