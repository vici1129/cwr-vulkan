# Original quay paving, with mixed wall and green seawall fallback

Base: `4e3520f17a97437a27db1dc6e06211191df34e9d`.
Two original Nogova models / seven authored visual levels / four placements
are assessed. Five source-unknown images are physically inspected among seven
decoded references. The alpha railing and distant composite references are not
additional optical approvals.

## Source, geometry and retained regions

Enhanced-only identity 205, `o/hous/nabrezi01a.pac`, is the original continuous
aged stone/mortar photograph, 256x256 opaque RGBA fingerprint
`6836735905726451377`. Existing stone family 6, roughness .90, F0 .04 and metallic
zero are unmeasured rough-dielectric approximations. Original pixels/contact
shading, repeated UVs, sampler state and independent per-face/live gates remain.
No physical wetness map or replacement artwork is inferred.

| Model | New ordinary prerequisites by authored level | Near source mesh / upward area |
| --- | --- | ---: |
| `o/hous/nabrezi.p3d` | 159 / 64 / 23 / 23 | 1526.85 / 707.06 square metres |
| `o/hous/nabrezi_najezd.p3d` | 183 / 93 / 53 | 1718.07 / 964.48 square metres |

These source mesh areas include deep vertical/buried geometry; they are not
exposed paving or world-rendering coverage. Upward means geometric normal Y>.8,
not proof that a face is visible. The main source extends down to local Y-27.95.
342 highest-detail / 598 alternative-level additional prerequisites are diagnostic,
not the adoption target or placement-multiplied coverage.

- `o/hous/nabrezi01.pac` mixes grey cap, stone/mortar, rusted blue metal door,
  hardware/recess and green lower finish. Conservative left stone-only boxes
  contain no complete original ordinary faces. Broad cap bounds include negative
  V on NoClamp faces that wrap into green pixels. The ramp splits an original
  polygon fan at level 2 / section 1 / face 73 even in narrower positive-V trials.
  The main-only safe positive-V fragments are two elevated paired plate/cap
  constructions: 24 near triangles, but just 2.33 square metres upward and 2.33
  downward at local Y25.6704..26.1108. They are not broad walkway/facade coverage;
  no meaningful independent normal-view gain is established. No tiny regional
  identity is added merely for counts, and no unsafe broader bounds are admitted.
- `o/hous/nabrezi01b.pac` is green-film stone on vertical seawall faces, not a
  hidden horizontal floor. Its photograph cannot distinguish persistent biofilm,
  coating or wet finish well enough to recover waterline optics. Some original
  geometry is below sea level, but visible green portions remain above water.
  No all-submerged or bare dry-stone declaration; original appearance remains.
- `o/hous/budka01.pac` combines corrugated shed coating, oxide/exposed metal,
  door/hardware and recesses. No uniform conductor/painted-sheet assumption.
  Its other shared shed model is outside this two-model assessment.
- `data/blck_sum.pac` is flat near-black utility on small paired caps/hardware,
  with no independent substrate/finish proof. It is distinct from `black_sum`.

## Selected gameplay evidence

Original `R01WarCry.Noe` runs through the existing direct developer harness,
copied profiles, --nosound and Vulkan synchronization validation. All four
original object transforms are queried without moving them. Upper main/ramp,
southern upper and main lower cameras stand 1.8m above inspected original stone
floors, not terrain underneath the quay. These are selected developer views,
not physically walked traversal, all-placement acceptance, Steam or mission completion.
Each frozen action explicitly matches existing simulation clock 0.

Native stone reception stays **0 before / 0 after** in the main upper, ramp
and lower views. The southern view's **3965 / 3967** is pre-existing surrounding
stone, not new paving reception. New native paving PBR/reception is not proved.
CPU-transform examples change from **0 / 0 / 3933 / 0** to
**194363 / 196874 / 207493 / 130529**, respectively. These are retained
half-resolution family/frame footprints, not source-specific pixel masks.

Same-runtime CPU PBR-off, PBR-on/GI-off, sky-only and GI-on controls retain a
continuous paving response without a new hard lighting boundary. The mixed
wall/door and green seawall sources do not inherit the new material. Route/LOD,
water, foliage and terrain differences are not clean whole-scene equivalence.

An actual eligible reflected hit on identity **205** is observed in the normal
main-upper view. No new eligible hit is observed in the other three views'
bounded 160 phase/grid polls each. This proves limited reflector execution,
not a distribution, an all-world zero or native paving reception. Normal-view
benefit is correspondingly limited; no broad native-floor relighting is claimed.

Overcast/dusk, TAA/FSR Quality/MSAA4 settled movement, two bounces and original
water-feature on/off controls execute. Spatial-only MSAA grain is not clean
MSAA equivalence, and settled captures do not establish in-motion acceptance.
No ungrounded retreat is used to manufacture exposed floor area.

The separate advancing-simulation southern CPU weather/AA/dusk controls retain
aligned geometry in PBR-off, GI-off and GI-on captures. A sharp L-shaped dark
band at dusk triggers an additional hold and same-byte isolation: it disappears
with existing RT sunlight shadows off, returns when restored, and moves between
15:00 / 17:30 / 18:00 sun times. It is an RT-sun-shadow response, not a fixed
source-face material seam or a GI patch. The original elevated railing/top-bar
geometry is a consistent possible occluder; its exact identity is not independently
traced. Shadows are restored on, with no shader/gate/source workaround.
The preceding batch's frozen post-AA view mismatch remains unresolved and is
not fixed or broadly recertified here.

Compare [main native baseline](evidence/material-modernization/batch-26/baseline-quay-main-gi.png)
and [candidate](evidence/material-modernization/batch-26/final-quay-main-gi.png),
then the separate
[CPU baseline](evidence/material-modernization/batch-26/baseline-quay-main-CPU.png),
[CPU PBR-off](evidence/material-modernization/batch-26/final-quay-main-CPU-PBR-off.png)
and [CPU candidate](evidence/material-modernization/batch-26/final-quay-main-CPU.png).
The advancing dusk
[PBR-off](evidence/material-modernization/batch-26/cpu-weather-CPU-dusk-PBR-off.png),
[GI-off](evidence/material-modernization/batch-26/cpu-weather-CPU-dusk-auto-GI-off.png)
and [GI-on](evidence/material-modernization/batch-26/cpu-weather-CPU-dusk-auto-GI-on.png)
show the shadow response. The dedicated
[RT-shadow-on](evidence/material-modernization/batch-26/shadow-control-dusk-RT-on.png),
[off](evidence/material-modernization/batch-26/shadow-control-dusk-RT-off.png),
[restored](evidence/material-modernization/batch-26/shadow-control-dusk-restored.png),
[15:00](evidence/material-modernization/batch-26/shadow-control-afternoon-restored.png)
and [18:00](evidence/material-modernization/batch-26/shadow-control-evening-restored.png)
controls retain the isolation evidence.

Original authored/geometry/opacity/static-instance and captured-light gates remain.
No shader, material map, normal/UV/sampler repair, transport/filter/ray budget,
settings, original v3, existing PBR/RTGI, GL33 or private DLSS changes.

## Cost, census and preservation

| View / resolution | GI-on GPU frame, before / candidate | Trace + filter + history, before / candidate |
| --- | ---: | ---: |
| Main upper / 1080p | 8.271 / 7.907 ms | .775 / .597 ms |
| Ramp upper / 1080p | 2.464 / 2.505 ms | .199 / .197 ms |
| Southern upper / 1080p | 2.384 / 2.281 ms | .186 / .174 ms |
| Main lower / 1080p | 6.463 / 6.657 ms | .464 / .492 ms |
| Main upper / 1440p | 9.047 / 9.082 ms | .655 / .661 ms |
| Ramp upper / 1440p | 3.319 / 3.322 ms | .208 / .210 ms |
| Southern upper / 1440p | 3.403 / 3.390 ms | .212 / .211 ms |
| Main lower / 1440p | 6.571 / 6.657 ms | .379 / .390 ms |
| Separate forced-CPU southern / 1080p | 38.629 / 46.437 ms | 2.120 / 7.768 ms |

Values average the two GI-on interval medians. Main-upper 1080p has appreciable
interval variation: GPU frame 7.706 / 8.835 before and 7.175 / 8.640 after.
Its lower average is not an optimization or isolated material-cost claim;
CPU command recording remains about 22 ms. All raw Off/On intervals and
individual stages remain in the qualification receipt.

Four matched-clock normal-route runs normally close after **134,755 presented
frames**, zero Vulkan validation errors/warnings and 12 restored hardware-T&L
toggle pairs. The untimed pilot closes after 5154 frames, zero errors/warnings,
with two restored pairs; the original transform/camera scout closes after 21093
frames, zero errors/warnings. Both are excluded from paired costs/frame totals.

The forced-CPU route has a material measured cost increase, not a free coverage
gain or recommended gameplay setting. CPU command recording remains about
214.48 / 213.25 ms. Its existing adaptive method uses warm20, settle8, minimum
45-second intervals and at least three fresh frame/GI-stage timestamps, bounded
at 120 seconds. It is one representative southern 1080p view, not dense main-quay,
all-instance/all-resolution performance or isolated material cost. No build,
heavy census or other game runs during cost sampling.

The two CPU cost runs close after **2462 frames**, zero validation errors/warnings
and two restored hardware-T&L pairs. Advancing CPU weather controls close after
318 frames, and the separate dusk shadow diagnostic after 9729; both have zero
errors/warnings and one restored pair each. These untimed auxiliary runs are
not pooled into paired cost/frame totals.

Fresh censuses inspect 726 models / 1708 retained visual levels and all 1711
authored levels. All 726 historical retained reports verify with zero replay
differences. Exact new-source replay reproduces preceding complete prerequisites;
all seven family source levels and original geometry/UVs/normals/bindings/hints/
flags remain. The same three original forest source-only levels remain outside
the historical native comparison.

Focused Vulkan tests pass **91 cases / 113176 assertions**; GL33 passes
**90 / 113136**. Existing 690 Vulkan policy checks and 18 Python inventory/census
tests pass. Authored/geometry/alpha/opacity/dynamic-source rejection, original
v3 exclusion, wrong namespace/extension and mixed source negatives remain.
Protected qualification functions, existing regional policy and the old
enhanced-material safety test file are byte-identical to the public base.

Fresh GL33 controls report all five PBR commands unsupported and zero changed
world-ROI pixels. All **270 original input hashes** and **67 settings/fixture/input
hashes** match; private DLSS remains clean and remote-free, with both SDKs and
preserved snapshots present. The
[qualification receipt](evidence/material-modernization/batch-26/qualification.json)
retains measurements, loaded hashes, controls, limits and preservation checks.
The [source audit](evidence/material-modernization/batch-26/source-audit.json)
retains original pixels' identities, geometry/UV/lighting and fallback reasons.

Costs use auto/TAA100%, one bounce, RT sun/cloud shadows, AO/water/volumetrics off.
Warm10 seconds, then Off/On/On/Off with settle4/sample6 at 1080p and 1440p.
Raw individual stages/world/CPU recording remain in the receipt. Scene costs are
not an isolated material benchmark or an optimization claim. Water controls are
separate and restore the sampled off state; no new waterline optics is invented.

After admission, source assessments are **227 complete / 120 partial / 168 pending**,
with another **211 existing classifier contracts retained**. The 30-task backlog
dispositions remain 11 complete / 16 partial / 3 retained. Source assessment,
prerequisites, actual reflected hits and reception are distinct; no full-world
or 100-percent rendering coverage is claimed.

```powershell
python tools/materials/census_runtime_assets.py `
  --inventory docs/evidence/original-asset-inventory/inventory.json `
  --library build/original-asset-inventory-20261021/library `
  --config game-local/Remastered/BIN/CONFIG.BIN `
  --tool dist/vulkan-local/PoseidonTools.exe `
  --legacy-report-cache build/original-asset-inventory-20261021/model-reports `
  --cache build/material-modernization-20261021/batch-26/final-runtime-census `
  --output build/material-modernization-20261021/batch-26/final-runtime-census/census.json `
  --replay-enhanced-from 130 --replay-enhanced-through 205 `
  --replay-regional-identities @(512..569)
```

Add `--all-source-lods` with a separate `batch-26/final-source-census` cache/output.
Accepted scoped Vulkan executable:
`90D3CAA0C97221B83F6A44BE15D0709440E1B6967CDADF5FD4CBBF402EB53ADA`.
