// Reach the bottom. Apply and OK are the last two non-pinned rows in both
// renderers, so Apply occupies slot 6 and Cancel remains pinned in slot 8.
triCursorScroll -3
triSimFrames 1
triCursorScroll -3
triSimFrames 1
triCursorScroll -3
triSimFrames 1
triCursorScroll -3
triSimFrames 1
triCursorScroll -3
triSimFrames 1
triCursorScroll -3
triSimFrames 1
triCursorScroll -3
triSimFrames 1
triAssertEq [(triControlText 561), "Apply"]
triInvokeButton 563
triSimFrames 8
