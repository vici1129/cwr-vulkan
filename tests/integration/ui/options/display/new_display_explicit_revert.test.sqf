#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

triAssertEq [triGetWindowMode, "windowed"];
for "_i" from 0 to 3 do { if ((triControlText 521) == "Exclusive Fullscreen") exitWith {}; triInvokeButton 529 };
triAssertEq [(triControlText 521), "Exclusive Fullscreen"];
#include "../../../helpers/video_apply.sqf"
triInvokeButton 9202;
triSimFrames 10;
triAssertEq [triGetWindowMode, "windowed"];
#include "../../../helpers/video_top.sqf"
triAssertEq [(triControlText 521), "Window"];
triEndTest;
