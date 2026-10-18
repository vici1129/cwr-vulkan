#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

private _desktop = triGetDesktopDisplayMode;
for "_i" from 0 to 3 do { if ((triControlText 521) == "Exclusive Fullscreen") exitWith {}; triInvokeButton 529 };
triAssertEq [(triControlText 521), "Exclusive Fullscreen"];
private _picked = "";
for "_i" from 0 to 79 do {
    triInvokeButton 539;
    private _candidate = triControlText 531;
    if (_candidate != "Native" && _candidate != triGetWindowSize) exitWith { _picked = _candidate };
};
if (_picked == "") exitWith { "FAIL:no_non_native_mode" };
#include "../../../helpers/video_apply.sqf"
#include "../../../helpers/display_confirm_keep.sqf"
#include "../../../helpers/video_top.sqf"
for "_i" from 0 to 79 do { if ((triControlText 531) == "Native") exitWith {}; triInvokeButton 538 };
triAssertEq [(triControlText 531), "Native"];
#include "../../../helpers/video_apply.sqf"
#include "../../../helpers/display_confirm_keep.sqf"
triAssertEq [triGetCurrentDisplayMode, _desktop];
triEndTest;
