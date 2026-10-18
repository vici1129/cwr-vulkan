#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

triCursorScroll -3;
triSimFrames 1;
triCursorScroll -3;
triSimFrames 1;
triCursorScroll -3;
triSimFrames 1;
triAssertEq [(triControlText 500), "Aspect Handling"];
for "_i" from 0 to 2 do { if ((triControlText 501) == "Adaptive") exitWith {}; triInvokeButton 509 };
for "_i" from 0 to 3 do { if ((triControlText 511) == "21:9") exitWith {}; triInvokeButton 519 };
triInvokeButton 509;
triInvokeButton 519;
triAssertEq [(triControlText 501), "Original (stretched HUD)"];
triAssertEq [(triControlText 511), "21:9"];
triEndTest;
