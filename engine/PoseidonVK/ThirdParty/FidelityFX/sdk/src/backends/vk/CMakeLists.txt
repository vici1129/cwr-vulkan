# CWR build selection; shader compilation rules are unmodified AMD SDK rules.
include(CMakeShadersFSR3Upscaler.txt)
# AMD's helper accidentally includes its output-variable name in the list.
list(REMOVE_ITEM FFX_SC_PERMUTATION_OUTPUTS FSR3UPSCALER_PERMUTATION_OUTPUTS)
list(REMOVE_DUPLICATES FFX_SC_PERMUTATION_OUTPUTS)
add_custom_target(CwrFsr3Shaders DEPENDS ${FFX_SC_PERMUTATION_OUTPUTS})
set(FFX_SC_PERMUTATION_OUTPUTS ${FFX_SC_PERMUTATION_OUTPUTS} PARENT_SCOPE)
