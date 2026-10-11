# AMD FidelityFX FSR 3.1.4

Selected source files from the official FidelityFX SDK **v1.1.4**, commit
`c6efa6bf7f2027b3ec94f28578bb5965eabb9e55`:
https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/tree/v1.1.4

The SDK source and shaders retain their MIT copyright/permission headers and
the complete [SDK license](sdk/LICENSE.txt). These permissive terms allow use
in this GPL-3.0-or-later application; distribution must retain both notices.
The runtime subset consists of the FSR3 upscaler component, AMD Vulkan backend,
shader blob accessor, shared support, common GPU headers, SPD and FSR1 helper
headers used by the upscaler. There is no Cauldron, Frame Generation, DX12
backend, NGX, proprietary runtime, or unrelated effect implementation.

The two local CMakeLists.txt files select this subset. The sole backend source
change guards the unused Frame Generation swapchain callback with its effect
build flags, avoiding a link dependency on Frame Generation. One GLSL binding
format is corrected: `rw_luma_history` uses `rgba16f` to match the reference
host's RGBA16F allocation, instead of the upstream `rgba8` declaration which
produces undefined storage-image access (confirmed by Vulkan validation).
The reference algorithm is unchanged. Other imported files are unchanged
upstream text (apart from repository line-ending normalization); upstream
whitespace is retained. The local build glue supplies the missing
C++20 `<bit>` include and removes an erroneous output
variable name from the SDK shader compilation helper's generated dependency
list; it does not change the shaders or algorithm.

Enable with `CWR_FSR3=ON` and `CWR_FSR_SHADER_COMPILER` pointing to the official
SDK v1.1.4 `sdk/tools/binary_store/FidelityFX_SC.exe` (keep its accompanying
compiler files adjacent). The tool runs only at build time and is not copied
or distributed by this repository. Vulkan SDK >= 1.3.250 is required. All
reference shader permutations are compiled using AMD's own compilation rules.
Missing tools disable FSR with an explicit CMake warning. Default builds omit
the SDK and runtime integration.

The common core HLSL header is retained because AMD's compiler includes it in
the GLSL dependency files even though its code is conditionally excluded. The
HLSL callback header is also required by AMD's host C++ implementation for its
shared constant-buffer declarations. No HLSL entrypoints are imported.
