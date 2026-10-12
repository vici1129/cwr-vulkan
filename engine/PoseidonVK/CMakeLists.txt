# Vulkan is opt-in. GL33-only builds never search for, or link, the SDK.
find_package(Vulkan REQUIRED)
option(CWR_FSR3 "Build AMD FidelityFX FSR 3.1.4 Vulkan upscaling" OFF)
set(CWR_FSR_SHADER_COMPILER "" CACHE FILEPATH "Official SDK v1.1.4 FidelityFX_SC executable")
if(CWR_FSR3)
    if(EXISTS "${CWR_FSR_SHADER_COMPILER}" AND Vulkan_VERSION VERSION_GREATER_EQUAL "1.3.250")
        add_subdirectory(ThirdParty/FidelityFX)
    else()
        message(WARNING "FSR disabled: requires SDK v1.1.4 FidelityFX_SC and Vulkan SDK >= 1.3.250")
    endif()
endif()
if(NOT Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    message(FATAL_ERROR "Vulkan shaders require glslangValidator from the installed Vulkan SDK")
endif()
set(VK_SHADER_HEADERS)
foreach(kind triangle shape screen shadow gamma ssao csm)
    foreach(stage vert frag)
        if(kind STREQUAL "screen" AND stage STREQUAL "frag")
            continue() # Screen vertices reuse the diffuse fragment shader.
        endif()
        if(kind STREQUAL "shadow" AND stage STREQUAL "vert")
            continue() # Native/screen vertices retain the engine's projection.
        endif()
        if(kind STREQUAL "ssao" AND stage STREQUAL "vert")
            continue() # Reuses the final-composition fullscreen triangle.
        endif()
        set(header "${CMAKE_CURRENT_BINARY_DIR}/generated/PoseidonVK/Shaders/${kind}.${stage}.hpp")
        add_custom_command(OUTPUT "${header}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/PoseidonVK/Shaders"
            COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" -V --target-env vulkan1.0
                --vn "Cwr${kind}_${stage}" -o "${header}" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/${kind}.${stage}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/${kind}.${stage}"
                "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/shape_uniforms.glsl"
                "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/shadow_sample.glsl"
                "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/soft_shadow_math.glsl"
                "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/color_space.glsl"
                "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/weather.glsl" VERBATIM)
        list(APPEND VK_SHADER_HEADERS "${header}")
    endforeach()
endforeach()
foreach(kind fxaa smaa_edge smaa_weight smaa_blend aa_composite aa_composite_ms ssao_ms taa shape_temporal shape_sun shape_sun_temporal shape_water shape_water_temporal shadow_sun fsr_masks fsr_composite bloom sun_shafts sun_shafts_ms volumetric_fog volumetric_fog_ms water_capture water_capture_ms water_refraction water_refraction_ms)
    set(source "${kind}")
    set(defines)
    if(kind STREQUAL "shape_temporal")
        set(source "shape")
        set(defines -DTEMPORAL=1)
    endif()
    if(kind MATCHES "^shape_sun")
        set(source "shape")
        set(defines -DRT_SUN=1)
        if(kind MATCHES "temporal$")
            list(APPEND defines -DTEMPORAL=1)
        endif()
    endif()
    if(kind STREQUAL "shadow_sun")
        set(source "shadow")
        set(defines -DRT_SUN=1)
    endif()
    if(kind MATCHES "^shape_water")
        set(source "shape")
        set(defines -DRT_SUN=1 -DRT_WATER=1)
        if(kind MATCHES "temporal$")
            list(APPEND defines -DTEMPORAL=1)
        endif()
    endif()
    if(kind MATCHES "_ms$")
        string(REGEX REPLACE "_ms$" "" source "${kind}")
        set(defines -DMULTISAMPLE_DEPTH=1)
    endif()
    set(header "${CMAKE_CURRENT_BINARY_DIR}/generated/PoseidonVK/Shaders/${kind}.frag.hpp")
    add_custom_command(OUTPUT "${header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/PoseidonVK/Shaders"
        COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" -V --target-env vulkan1.0 ${defines}
            --vn "Cwr${kind}_frag" -o "${header}" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/${source}.frag"
        DEPENDS "Shaders/${source}.frag" Shaders/smaa_common.glsl
            Shaders/shape_uniforms.glsl Shaders/shadow_sample.glsl Shaders/soft_shadow_math.glsl Shaders/color_space.glsl Shaders/weather.glsl
            ThirdParty/SMAA/SMAA.hlsl ThirdParty/FXAA/Fxaa3_11.h VERBATIM)
    list(APPEND VK_SHADER_HEADERS "${header}")
endforeach()
foreach(kind rtao rtao_ms rtao_filter rtao_filter_ms rtshadow rtshadow_ms rtshadow_filter rtshadow_filter_ms rtwater rtwater_ms rtwater_filter rtwater_filter_ms)
    set(stage comp)
    if(kind MATCHES "filter")
        set(stage frag)
    endif()
    string(REGEX REPLACE "_ms$" "" source "${kind}")
    set(defines)
    if(kind MATCHES "_ms$")
        set(defines -DMULTISAMPLE_DEPTH=1)
    endif()
    if(kind MATCHES "^rtshadow")
        string(REPLACE "rtshadow" "rtao" source "${source}")
        list(APPEND defines -DSUN_SHADOW=1)
    endif()
    if(kind MATCHES "^rtwater")
        string(REPLACE "rtwater" "rtao" source "${source}")
        list(APPEND defines -DWATER_REFLECTION=1)
    endif()
    set(header "${CMAKE_CURRENT_BINARY_DIR}/generated/PoseidonVK/Shaders/${kind}.${stage}.hpp")
    add_custom_command(OUTPUT "${header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/PoseidonVK/Shaders"
        COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" -V --target-env vulkan1.2 ${defines}
            --vn "Cwr${kind}_${stage}" -o "${header}" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/${source}.${stage}"
        DEPENDS "Shaders/${source}.${stage}" Shaders/rtao_depth.glsl Shaders/rtao_coverage.glsl Shaders/sunlight_depth.glsl Shaders/rtao_water.glsl Shaders/color_space.glsl Shaders/weather.glsl VERBATIM)
    list(APPEND VK_SHADER_HEADERS "${header}")
endforeach()
foreach(kind contact_shadows contact_shadows_ms contact_shadows_rt contact_shadows_rt_ms)
    set(defines)
    if(kind MATCHES "_ms$")
        list(APPEND defines -DMULTISAMPLE_DEPTH=1)
    endif()
    if(kind MATCHES "_rt")
        list(APPEND defines -DCONTACT_RT=1)
    endif()
    set(header "${CMAKE_CURRENT_BINARY_DIR}/generated/PoseidonVK/Shaders/${kind}.frag.hpp")
    add_custom_command(OUTPUT "${header}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/PoseidonVK/Shaders"
        COMMAND "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}" -V --target-env vulkan1.0 ${defines}
            --vn "Cwr${kind}_frag" -o "${header}" "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/contact_shadows.frag"
        DEPENDS Shaders/contact_shadows.frag Shaders/rtao_depth.glsl Shaders/sunlight_depth.glsl
            Shaders/rtao_coverage.glsl VERBATIM)
    list(APPEND VK_SHADER_HEADERS "${header}")
endforeach()
add_custom_target(PoseidonVKShaders DEPENDS ${VK_SHADER_HEADERS})

add_library(PoseidonVK STATIC VulkanContext.cpp VulkanContext_Gamma.cpp VulkanContext_SSAO.cpp VulkanContext_Triangle.cpp VulkanContext_Mesh.cpp VulkanContext_Texture.cpp TextureVK.cpp BufferVK.cpp VertexBufferVK.cpp
    EngineVK.cpp EngineVK_Shadow.cpp VulkanContext_Shadow.cpp EngineVK_Mesh.cpp EngineVK_2D.cpp EngineVK_Software.cpp EngineVK_Unsupported.cpp GraphicsBackendVK.cpp)
add_dependencies(PoseidonVK PoseidonVKShaders)
target_sources(PoseidonVK PRIVATE VulkanContext_AA.cpp EngineVK_Temporal.cpp EngineVK_Atmosphere.cpp EngineVK_RTAO.cpp)
target_sources(PoseidonVK PRIVATE EngineVK_Weather.cpp)
target_include_directories(PoseidonVK PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(PoseidonVK PUBLIC Poseidon Vulkan::Vulkan)
if(WIN32)
    target_compile_definitions(PoseidonVK PRIVATE __MSC__=1100 _WINDOWS)
endif()
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(PoseidonVK PRIVATE /GR /permissive
        $<$<CXX_COMPILER_ID:Clang>:-Wno-error=non-pod-varargs>
        $<$<CXX_COMPILER_ID:Clang>:-Wno-error=c++11-narrowing>)
endif()

# Driver-free policy test: also builds directly with MSVC and SDK headers when
# the full engine dependency toolchain is unavailable.
add_executable(PoseidonVKPolicyTests VulkanContext.cpp VulkanContext_Shadow.cpp VulkanContext_Gamma.cpp VulkanContext_SSAO.cpp VulkanContext_Triangle.cpp VulkanContext_Mesh.cpp VulkanContext_Texture.cpp BufferVK.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit/engine/Poseidon/Graphics/test_vulkan_buffers.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit/engine/Poseidon/Graphics/test_vulkan_commands.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit/engine/Poseidon/Graphics/test_vulkan_policy.cpp)
target_include_directories(PoseidonVKPolicyTests PRIVATE ${CMAKE_SOURCE_DIR}/engine)
target_sources(PoseidonVKPolicyTests PRIVATE VulkanContext_AA.cpp)
add_dependencies(PoseidonVKPolicyTests PoseidonVKShaders)
target_include_directories(PoseidonVKPolicyTests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(PoseidonVKPolicyTests PRIVATE Vulkan::Vulkan)
add_test(NAME PoseidonVKPolicyTests COMMAND PoseidonVKPolicyTests)
foreach(target PoseidonVK PoseidonVKPolicyTests)
    target_sources(${target} PRIVATE VulkanContext_Weather.cpp)
    target_sources(${target} PRIVATE VulkanContext_RTAO.cpp)
    target_sources(${target} PRIVATE VulkanContext_Contact.cpp)
    target_sources(${target} PRIVATE VulkanContext_Water.cpp)
    target_sources(${target} PRIVATE VulkanContext_FSR.cpp VulkanContext_HDR.cpp VulkanContext_Bloom.cpp VulkanContext_Atmosphere.cpp VulkanContext_Volumetric.cpp)
    if(TARGET CwrFsr3)
        target_link_libraries(${target} PRIVATE CwrFsr3)
        target_compile_definitions(${target} PRIVATE CWR_FSR3=1)
    endif()
endforeach()
