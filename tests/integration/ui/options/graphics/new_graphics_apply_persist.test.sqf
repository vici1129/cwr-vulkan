#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

// The common image section starts at logical row 11 in either renderer.
triCursorScroll -3;
triSimFrames 1;
triCursorScroll -3;
triSimFrames 1;
triCursorScroll -3;
triSimFrames 1;
triCursorScroll -3;
triSimFrames 1;
triAssertEq [(triControlText 540), "Terrain Detail"];
for "_i" from 0 to 5 do { if ((triControlText 541) == "12.5 m") exitWith {}; triInvokeButton 549 };
triAssertEq [(triControlText 541), "12.5 m"];
#include "../../../helpers/video_apply.sqf"
triAssertEq [triControlText 590, "Video settings applied and saved."];
triEndTest;
