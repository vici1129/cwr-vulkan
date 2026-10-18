#include "../../../helpers/options_preamble.sqf"
#include "../../../helpers/display_preamble.sqf"

triAssertEq [triGetWindowMode, "windowed"];
private _start = triGetWindowSize;
private _picked = "";
for "_i" from 0 to 12 do {
    triInvokeButton 539;
    private _candidate = triControlText 531;
    if (_candidate != "Native" && _candidate != _start) exitWith { _picked = _candidate };
};
if (_picked == "") exitWith { "FAIL:no_distinct_window_size" };
#include "../../../helpers/video_apply.sqf"
#include "../../../helpers/display_confirm_keep.sqf"
triAssertEq [triGetWindowMode, "windowed"];
triAssertEq [triGetWindowSize, _picked];
triAssertWindowedSmallerThanDesktop;
triEndTest;
