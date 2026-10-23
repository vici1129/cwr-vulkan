# Original road direction-sign family

Base: `f3bc6b2a201fdf32605e202ce5a38c45745f1dd2`. All **113 models / 113 visual
LODs / 119 original placements** now have complete source assessments and explicit
retained-original reasons. **New rendering coverage: zero models, identities,
qualified triangles or GI receivers.** No renderer or asset changes qualify.

The [source audit](evidence/material-modernization/batch-30/source-audit.json)
records every model, exact source provenance, original UVs/normals, flags,
lighting, shared users and retained artwork contracts. Four face arrangements
cover the family; 71 exact diagnostic groups retain offsets and UV/normal rounding.

| Original post arrangement | Models | Source-unknown triangles per model |
| --- | ---: | ---: |
| Black upper cap, four light-source sides | 40 | 10 |
| Black upper cap, five light-source faces | 33 | 12 |
| Shaded source on all six faces | 33 | 12 |
| Black upper cap, four light sides, shaded bottom cap | 7 | 12 |

All 1,276 untyped triangles have ordinary material/light mode 0, finite UVs,
valid normals and original ClampU/ClampV. Another **1,998 triangles** retain
transparent/alpha-ordered artwork behavior across 68 board, backing, letter and
distance sources. Artwork, original normals, shading and all safety gates remain.

- `data/blck_sum.pac`: 32x32 opaque black utility source; **160 triangles** on
  tiny upper caps. Neither black pixels nor cap geometry establishes paint,
  metal or recovered albedo. Original downward-facing cap normals are retained.
- `data/sloup_bily_orez.pac`: 64x256 opaque worn paint, exposed/oxidized substrate
  and photographed shading; **706 triangles**. Full-height sides cross these
  features. A half-height UV bound contains no complete original triangles.
- `data/sloup_bily_orez_v.pac`: different 64x256 shaded/worn source;
  **410 triangles**. The same bound selects only horizontal end caps: four
  triangles on each of 33 six-face posts, two on each of seven mixed posts.
  It isolates no broad coating-only side. Tiny caps do not establish coating or
  conductor optics; no region is adopted.

Fresh original SHA256 **and** decoded-pixel fingerprints correct Batch 14's
non-alias wording: `sloup_bily_orez` equals `sloupek_bily_orezly`, while `_v`
equals `sloupek_bily_orezly_`. The two variants themselves differ. These are
distinct exact-name material keys. Existing identity 86 remains its inherited
PBR/reflector-only exception with the original specular-power hint and receiver
family zero. Pixel equality does not establish uniform paint/substrate optics
or authorize extending that exception to untyped sections. Batch 14's retained
mixed-source conclusions and current exclusion tests remain applicable.

Original Malden, Everon and Nogova missions run through the existing muted
developer harness with copied profiles. All **38 / 40 / 41** horizontal placements
match original dumps within .051m; height conventions differ, so full-transform
equality is not claimed. Grounded front/back views cover all four arrangements,
with PBR-off, auto/GI-off and auto/GI-on controls. Representative originals:
[Malden](evidence/material-modernization/batch-30/sign30-abel-40826-front-native-auto-GI-on.png),
[Everon](evidence/material-modernization/batch-30/sign30-eden-27285-front-native-auto-GI-on.png),
[Nogova light post](evidence/material-modernization/batch-30/sign30-noe-117547-front-native-auto-GI-on.png)
and [Nogova shaded post](evidence/material-modernization/batch-30/sign30-noe-117557-front-native-auto-GI-on.png).
These are developer-camera observations, not physical traversal or mission
completion. Surrounding GI counters are not new sign reception. There is no
implementation cost delta or material before/after benchmark to claim.

The [qualification receipt](evidence/material-modernization/batch-30/qualification.json)
records **11,854 presented Vulkan frames, zero validation errors/warnings**, and
GL33's five unsupported PBR controls with zero changed world-ROI pixels. Existing
focused suites pass **93 cases / 127,492 assertions Vulkan**, **92 / 127,452 GL33**;
road ordering passes **17 / 52** per build, plus **690 policy checks** and **18
inventory/census tests**. Hash-verified accepted Batch 29 binaries are reused;
no rebuild or global census rerun is claimed. Fresh family inspection reproduces
the current census exactly. All 270 original-input and 67 settings/fixture hashes
match; private DLSS, SDKs and snapshots remain preserved.

Ledger: **341 complete / 119 partial / 55 pending**, plus **211 retained existing
contracts**. This family has **zero pending assessment keys**; its three untyped
rendering keys and 1,276 source-unknown triangles retain explicit reasons.
Global remaining assessment keys number **471**; existing qualified prerequisites
remain **18,685 highest-detail / 35,734 across alternative LODs**, with **zero added**.
These are not visible-coverage percentages. Stop after Batch 30.
