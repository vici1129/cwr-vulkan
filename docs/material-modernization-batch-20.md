# Guard booths, pallets and watchtower: original finish limits

Base: `641b4cf201d139388e33c75d07cc9c1a4b819185`.
Six original models / 20 authored visual levels / 197 world placements are
inspected. Twelve source-unknown images are physically viewed; 41 decoded
references are not 41 optical approvals. No material identity, exact region,
artwork, shader, geometry, normal, UV, lighting hint or setting changes.

## Physical regions and concrete decisions

The guard booth's `hlidbud_prvni` atlas mixes white panels, dark timber/rust trim
and corrugated metal. Upper paint-only bounds split ten original face fans and
select only .269 square metres at the nearest level. Lower-left bounds select
nothing. There is no whole-atlas facade declaration or generated isolation mask.
The separately textured `hlidbud_panel` is the lower plinth, not the white wall;
its wood/formwork/concrete interpretation remains uncertain. Its 26.66 square
metres of mesh are mostly buried at the inspected original placement, not exposed
wall coverage. The plinth top is only approximately .1 metres above terrain.

Crucially, ordinary `hlidbud_podlaha` triangles are the upward **roof** at local
Y=1.89013. The visible interior floor at Y=-.723834 has authored lighting and
downward normals. Those original normals/hints are preserved; neither the
filename nor the dark plank-like photo establishes an ordinary floor improvement.
The [exterior](evidence/material-modernization/batch-20/guard-base-gi.png) and
[interior CPU diagnostic](evidence/material-modernization/batch-20/guard-window-east-CPU.png)
show the retained appearance. The interior camera is grounded, not a physically
walked door or mission-completion test.

`drevo_dirkate` contains a dark photographed split/void as well as weathered
timber. Tested gap-free left/right/top bounds isolate just .209/.063/.070 square
metres on the pallet. Watchtower right bounds select .230 square metres. These
are tiny fragments, not full pallet or timber coverage. Wider original faces
sample the mixed photo; no UV clipping, artwork or material map is introduced.
The [coastal pallet](evidence/material-modernization/batch-20/pallet-coast-gi.png),
[CPU route](evidence/material-modernization/batch-20/pallet-coast-CPU.png) and
[retreat](evidence/material-modernization/batch-20/pallet-coast-retreat-22.png)
retain the original gaps. Shared fortress, provisional fence and crawl-obstacle
uses remain behind their existing authored/land-hint contracts.

The [watchtower](evidence/material-modernization/batch-20/watchtower-ground-gi.png)
has separately modeled structural members, corrugated enclosure, door, tread and
underside. Original photos mix exposed metal/rust-like treatment with worn paint,
hardware or inward-corner/relief lighting. They do not support a uniform rough
dielectric declaration or an invented metallicity/occlusion map. Broad original
UV footprints do not establish paint-only isolation. Dark brown tabletop and
mottled roof sources likewise lack independently established substrate/treatment.
The [CPU view](evidence/material-modernization/batch-20/watchtower-ground-CPU.png)
and [retreat](evidence/material-modernization/batch-20/watchtower-ground-retreat-22.png)
are retained controls, not new reception evidence.

The watchtower's painted-board source `podlaha_prkna_h` is a useful shared-family
followup. Here, however, its only two ordinary near triangles total .0417 square
metres of thin vertical platform edge with -X normals. The broad visible platform
is authored-lit/downward-normal, not an ordinary floor receiver. Other building
users must be inspected before global source adoption; the watchtower review
therefore stays **partial**, with that explicit actionable source still pending.

Original provenance, decoded pixel fingerprints, geometry/binding identities,
18 UV trials, floor/roof normal proof and per-source reasons are in the
[source receipt](evidence/material-modernization/batch-20/source-audit.json).
Raw decoded originals/full UV buffers stay ignored locally. Other users of the
additional shared board/roof sources are recorded, not automatically assessed.

## Gameplay, cost and preservation

Unchanged admitted batch-18 executable
`6F0A14974528E8485EF3A3509398CD78F29F0782D1ADF20A880C5834AB3DBC65`
runs original Nogova `Resistance/R01WarCry` and Malden `01TakeTheCar` through the
existing direct developer harness, copied profiles, `--nosound`, synchronization
validation and terrain-grounded standing cameras. This is not Steam-route or
mission-completion acceptance. Three HWTL Off/On pairs are verified in game stdout.
Malden adds grounded 0/8/22/52/92-metre distance controls. Native/CPU differences
remain existing behavior, not a clean whole-scene rendering acceptance claim.

Two obstructed pallet scouts are excluded: the Nogova castle terrain view and
the first Malden airfield view blocked by a door. The corrected coastal view
above is used for appearance and timing. The initial Nogova harness request
raced startup; a retry succeeded after the healthy listener appeared. Neither
orchestration correction changes a product gate or setting.

RTX 4060 Ti, 1920x1080, TAA100%, one bounce, RT sun/cloud shadows, AO/water/
volumetrics off. Each scene warms ten seconds, then runs Off/On/On/Off with
four-second settling and six-second sampling. GI-on paired interval medians:

| Existing scene | GPU frame | Trace + filter + history |
| --- | ---: | ---: |
| Booth exterior | 2.449 ms | .329 ms |
| Coastal pallet | 4.830 ms | .979 ms |
| Watchtower | 3.865 ms | .693 ms |

All intervals, GI-off and CPU-recording costs remain in the
[qualification receipt](evidence/material-modernization/batch-20/qualification.json).
No build, heavy census or other game runs during timing. These are unchanged
scene observations, not before/after improvements or added material costs.
No second-resolution benchmark or universal performance budget is claimed.

Both Vulkan runs close normally: 177,450 Nogova and 24,790 Malden presented
frames, **zero validation errors/warnings**. Nogova lifecycle frames include
initial unconfigured mission/scouting idle time; they are not 177,450 appearance
acceptance samples. Fresh focused tests pass **85 / 96,317** on Vulkan and
**84 / 96,277** on GL33, plus 690 policy guards and 18 inventory/census tests.
Fresh live GL33 Off/On/Enhanced/Auto/Off controls change zero world-ROI pixels.
All 270 original inputs and 67 settings/fixture/input hashes match. Private DLSS
stays clean and remote-free; both SDKs and preserved snapshots remain present.

No rebuild/global census rerun is represented as performed. Material/qualification/
test bytes match the base; fresh six-model/20-level native inspection reproduces
the current 726-model/1,711-level source census. No triangle count is promoted.
Booth/pallet assessments close (39 placements); watchtower remains partial and
three existing retained-contract models are rechecked. The ledger records
**210 complete source assessments, 126 partial, 179 pending, 211 retained existing
contracts**. The full goal continues; these are not rendering-coverage percentages.
