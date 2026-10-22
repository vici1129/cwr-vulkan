# Crooked birch and apple: source limitations retained

Base: `080e4e1b379dd46851505a5c0f92b99052dd64e1`.
This assessment covers **two models / six authored levels / 1,037 original
placements**: `data3d/str_briza_kriva.p3d` and `data3d/jablon.p3d`.
No material identity, region, artwork, shader or rendering behavior changes.
Assessment completion is not new rendering adoption.

## Concrete source decisions

The original 64x256 `data/briza_kriv_kmen.pac` is a bent-trunk photograph with
backdrop/pale surrounding photographic content, not a dedicated neutral bark
image. Its original near-level UV footprints span that mixed image over
46 source-unknown prerequisites / 6.690 square metres. Three inspected small
bark-looking UV bounds select **zero complete original triangles**. Wider bounds
do not independently isolate bark; no photo mask, polygon surgery or generated
material map is used. The existing mixed-birch regression exclusion remains
unchanged. The dedicated bark contracts from earlier batches are not broadened.

The apple's original 32x32 `data/black_sum.pac` is dark utility noise, not an
independently sourced bark/substrate/treatment image. It textures 58 near-level
opaque trunk prerequisites / 1.183 square metres. Tree-like geometry and dark
pixels do not establish neutral wood reflectance or justify creating detail/
specular response on a silhouette. Other users of this generic image remain
recorded in provenance; they are not automatically assessed by this tree batch.

Both models replace the opaque near trunk with blended photographic tree cards
at later levels. The original `briza`, `briza_rovna` and `jablon_renovace` images
contain bark/branches/leaves together. Existing blend/opacity, alpha traversal,
shadows and LOD behavior remain intact; an opaque single-surface material is not
invented for those cards. The 104 source-unknown and 212 render-behavior
prerequisites across alternatives remain unpromoted. No original normal, hint,
geometry, UV, setting or asset is repaired or rewritten.

All five referenced original images were physically viewed. Exact decoded
fingerprints, original provenance, six native geometry/binding identities and
the three zero-face UV trials are in the
[source receipt](evidence/material-modernization/batch-19/source-audit.json).
Raw decoded originals and full UV/position buffers stay ignored locally.

## Actual gameplay and unchanged-policy checks

Original Malden `01TakeTheCar` runs through the existing direct developer
harness with copied profiles, `--nosound` and synchronization validation.
Two terrain-grounded standing views inspect the
[birch trunk](evidence/material-modernization/batch-19/crooked-birch-gi.png) and
[orchard silhouettes](evidence/material-modernization/batch-19/apple-trunk-gi.png),
with [birch retreat](evidence/material-modernization/batch-19/crooked-birch-retreat-22.png),
[orchard retreat](evidence/material-modernization/batch-19/apple-trunk-retreat-22.png)
and [CPU route](evidence/material-modernization/batch-19/crooked-birch-CPU.png)
controls. Two HWTL Off/On pairs are verified in game stdout; grounded retreat
distances are 0/8/22/52/92 metres. This is not all-placement, Steam-route or
mission-completion acceptance. Existing foliage/LOD and CPU/native differences
are not clean whole-scene foliage acceptance.

The unchanged RTX 4060 Ti scene measurements use 1920x1080, TAA100%, one bounce,
RT sun/cloud shadows and AO/water/volumetrics off. Ten-second warmup precedes
Off/On/On/Off with four-second settling and six-second sampling intervals.
Paired GI-on interval medians average 10.327 ms GPU frame / 1.919 ms
trace+filter+history at the birch and 5.872 / .824 ms in the orchard. All samples,
GI-off costs and CPU-recording medians remain in the receipt. No build, heavy
census or other game runs during timing. These are existing scene costs, **not
before/after improvements or new material costs**; no implementation delta or
second-resolution material benchmark is claimed.

The game closes normally after **16,595 presented frames**, synchronization
validation on, **zero errors and warnings**. Fresh focused tests pass
**85 cases / 96,317 assertions** on Vulkan and **84 / 96,277** on GL33;
690 policy guards and 18 inventory/census tests pass. Fresh live GL33
Off/On/Enhanced/Auto/Off controls have zero changed world-ROI pixels.
All 270 original input hashes and 67 settings/fixture/input hashes match.
Private DLSS is clean and remote-free; SDKs and preserved snapshots remain.

No rebuild is represented as performed: the admitted executable remains
`6F0A14974528E8485EF3A3509398CD78F29F0782D1ADF20A880C5834AB3DBC65`.
Material/qualification/test bytes match the base exactly. Fresh two-model/six-
level inspection reproduces the current global 726-model / 1,711-level source
census exactly. The global census is unchanged, not claimed as rerun here.
Full modes, intervals, artifact identity and preservation are in the
[qualification receipt](evidence/material-modernization/batch-19/qualification.json).

The ledger now records **208 complete source assessments, 126 partial,
181 pending and 211 retained existing contracts**. These two assessments close
with documented original-source limitations, not new PBR/GI rendering coverage.
The original 726-model goal continues.
