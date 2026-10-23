# Common building facades: classic brick houses

Base: `8fa245e0c0df1727dd2571f929d819ae3f3ec7fb`. Assessment only: **no rendering
change qualifies**. The three `data3d/cihlovej_dum{,_in,_mini}.p3d` variants cover
75 original Malden placements and 15 authored visual LODs. Their substantial
walls and partially assessed sources made this a useful bounded family review.

## Source and geometry findings

Fresh native inspection reproduces the current source census for all three
models, including positions, normals, UVs, samplers, lighting and geometry/binding
fingerprints. Four original images were decoded and inspected; original assets
are preserved. The [source audit](evidence/material-modernization/batch-28/source-audit.json)
records provenance, pixels, all-LOD gates and 27 conservative regional trials.

- `data/cihldum.pac` and `cihldum_back.pac` combine brick, glazing, shutters,
  doors, sills, drainpipes and photographed shadows. Tested brick bands above
  and below the upper windows and narrow lower piers select **zero complete
  ordinary triangles** across all variants/LODs. Larger wall faces cross mixed
  regions; opacity and a brick majority do not justify a whole-source identity.
- `data/cihldum_side.pac` contains broad brick walls with photographed pipes and
  edge fittings. Large original faces span those features. In the main model,
  major near side faces also retain authored material 201. The safe central
  U[.17,.84], V[0,1] trial selects four small chimney triangles per participating
  LOD: 1.686 / 1.686 / 1.243 square metres at highest detail for main / enterable /
  mini. These are elevated chimney faces, not useful broad facade coverage;
  main's last LOD and mini's last LOD select none. No identity is added for them.
- `data/cihldum_sum.pac` retains its prior unidentified dark roof/fill treatment.
  Neighboring brick does not establish its optics. Existing roof, interior,
  alpha, authored-lighting and invalid-geometry contracts remain independent.

The three facade sources account for 1,872 ordinary source-unknown triangles,
464 authored-lighting rejections and four geometry rejections across alternative
LODs. These counts are inspection buckets, not visible coverage or a reason to
adopt. More detailed physical separation remains unresolved; the three models
stay partial in the ledger. Podhradi and ruin-plaster rejections remain intact.

## Original gameplay and native reception

The unchanged admitted public Vulkan executable runs original Malden
`01TakeTheCar.ABEL` through the existing developer harness, copied profile,
`--nosound` and synchronization validation. Standing cameras inspect original
objects 14282, 17057 and 1369 without moving buildings. Main front, side and rear,
mini front and enterable front receive v3/GI-off, auto/GI-off, sky-only and GI-on
controls. The initial western enterable scout shows a neighboring plaster house
and is excluded; the southern view shows the intended brick front.

Compare the retained main [front](evidence/material-modernization/batch-28/brick28-main-gi.png),
[side GI-off](evidence/material-modernization/batch-28/brick28-side-enhanced-off.png)
and [side GI-on](evidence/material-modernization/batch-28/brick28-side-gi.png).
Facade-family native/CPU samples are zero in all five daylight GI-on views.
Other family/terrain counters belong to existing scenery and paths; they do not
establish brick-wall reception. **No new native receiver or reflected identity
is introduced or claimed.** CPU-only participation would not justify adoption.

Overcast/dusk GI controls, 0/8/22/52/92m grounded camera retreats and TAA/FSR
Quality/MSAA4 controls use existing tools. Advancing-simulation daylight/dusk
GI Off/On trials also translate and turn the front/side cameras, with intermediate
and settled captures. The [dusk front Off](evidence/material-modernization/batch-28/brick28-main-live-dusk-off-move16.png)
and [On](evidence/material-modernization/batch-28/brick28-main-live-dusk-on-move16.png)
retain the photographic wall treatment. Original shading/LOD differences and
spatial-MSAA limitations remain; no new material seam is introduced. These are
developer-camera observations, not physical traversal, Steam-route or mission
completion acceptance. There is no candidate before/after improvement to claim.

## Cost and validation

There is **no incremental GPU cost**, because only assessment documentation and
evidence change. The qualification receipt records one unchanged 1080p side-view
Off/On/On/Off workload (warm10, settle4, sample6), with raw GPU frame and GI-stage
samples: GPU frame **5.316 / 5.723 ms Off/On**, GI trace/filter/history **.785 ms On**.
This describes existing GI cost, not a before/after material benchmark. No build,
census or other game runs during those intervals.

Existing focused tests pass **92 cases / 115,638 assertions** on Vulkan and
**91 / 115,598** on GL33; road ordering/ZBias passes **17 / 52** per build.
All **690 policy checks** and **18 inventory/census tests** pass. The unchanged
batch-27 binaries are hash-verified; no rebuild or global census rerun is claimed.
The family-only fresh inspection has zero census differences.

The Vulkan run shuts down normally after **25,407 presented frames**, with **zero
validation errors/warnings**. Live Malden GL33 reports all five PBR commands
unsupported and **zero changed world-ROI pixels**. All **270 original input hashes**
and **67 settings/fixture/input hashes** match; private DLSS remains clean and
remote-free, with SDKs and preserved snapshots present. The
[qualification receipt](evidence/material-modernization/batch-28/qualification.json)
records these checks, loaded binaries and timing limits.
The ledger remains **228 complete / 119 partial / 168 pending**, plus **211
retained existing contracts**. Batch 28 stops with this family and its original
rendering retained.
