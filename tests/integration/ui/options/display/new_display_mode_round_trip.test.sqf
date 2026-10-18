#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

#include "../../../helpers/video_top.sqf"
for "_i" from 0 to 3 do { if ((triControlText 521) == "Borderless") exitWith {}; triInvokeButton 529 };
triAssertEq [(triControlText 521), "Borderless"];
#include "../../../helpers/video_apply.sqf"
#include "../../../helpers/display_confirm_keep.sqf"
triAssertEq [triGetWindowMode, "borderless"];
#include "../../../helpers/video_top.sqf"
for "_i" from 0 to 3 do { if ((triControlText 521) == "Exclusive Fullscreen") exitWith {}; triInvokeButton 529 };
triAssertEq [(triControlText 521), "Exclusive Fullscreen"];
#include "../../../helpers/video_apply.sqf"
#include "../../../helpers/display_confirm_keep.sqf"
triAssertEq [triGetWindowMode, "fullscreen"];
#include "../../../helpers/video_top.sqf"
for "_i" from 0 to 3 do { if ((triControlText 521) == "Window") exitWith {}; triInvokeButton 529 };
triAssertEq [(triControlText 521), "Window"];
#include "../../../helpers/video_apply.sqf"
#include "../../../helpers/display_confirm_keep.sqf"
triAssertEq [triGetWindowMode, "windowed"];
triEndTest;
