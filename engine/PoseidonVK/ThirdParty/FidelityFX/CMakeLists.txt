# Only the FSR 3.1.4 upscaler and AMD's Vulkan backend. No sample framework,
# frame interpolation, SDK compiler binaries, or unrelated effects are linked.
set(FFX_GPU_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/FidelityFX/gpu")
set(FFX_PASS_SHADER_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${FFX_PASS_SHADER_OUTPUT_PATH}")
set(FFX_SC_EXECUTABLE "${CWR_FSR_SHADER_COMPILER}")
include("${FFX_GPU_PATH}/CMakeCompileShaders.txt")
add_library(CwrFsr3 STATIC
    sdk/src/components/fsr3upscaler/ffx_fsr3upscaler.cpp
    sdk/src/shared/ffx_assert.cpp sdk/src/shared/ffx_message.cpp
    sdk/src/shared/ffx_object_management.cpp sdk/src/shared/ffx_breadcrumbs_list.cpp
    sdk/src/backends/vk/ffx_vk.cpp
    sdk/src/backends/shared/ffx_shader_blobs.cpp
    sdk/src/backends/shared/blob_accessors/ffx_fsr3upscaler_shaderblobs.cpp)
target_include_directories(CwrFsr3 SYSTEM PUBLIC sdk/include)
target_include_directories(CwrFsr3 PRIVATE sdk/src/shared sdk/src/components
    sdk/src/backends/shared "${FFX_PASS_SHADER_OUTPUT_PATH}")
target_compile_definitions(CwrFsr3 PRIVATE FFX_FSR3UPSCALER NOMINMAX)
# ffx_util.h uses std::popcount in C++20 without including <bit>.
target_precompile_headers(CwrFsr3 PRIVATE <bit>)
target_link_libraries(CwrFsr3 PUBLIC Vulkan::Vulkan)
add_subdirectory(sdk/src/backends/vk)
add_dependencies(CwrFsr3 CwrFsr3Shaders)
