#pragma once
#include <array>

namespace Poseidon::vk
{
struct ShapeLocalLight
{
    std::array<float, 4> position{}, direction{}, diffuse{}, ambient{};
};
// std140, shared with shape_uniforms.glsl. Outside the 128-byte push limit.
struct ShadowLighting
{
    std::array<float, 64> matrices{};
    std::array<float, 4> splits{}, control{}, forward{}, strength{}; // strength: darkness, texel UV, soft sun radius, reserved
};
struct alignas(16) ShapeLighting
{
    std::array<float, 16> world{};
    std::array<float, 12> normal{};
    std::array<float, 4> sunDirection{};
    std::array<float, 4> ambient{}, diffuse{}, emissive{}, specular{};
    std::array<float, 4> fogParams{}, fogColor{};
    std::array<float, 4> eyeCoef{0, 0, 0, 1}, localCount{};
    std::array<ShapeLocalLight, 8> localLights{};
    std::array<float, 4> shadowReceiver{}; // enabled, software projection X/Y
    ShadowLighting shadow{};
    std::array<float, 16> previousMVP{};
    std::array<float, 4> temporal{}; // jitter NDC xy, history valid, reactive
    std::array<float, 4> temporalExtent{}; // inverse width/height
    std::array<float, 4> outputColor{};
    std::array<float, 4> atmosphereSun{}, atmosphereColor{};
    std::array<float, 4> rtSun{}; // x: 1 sunlight separation, 2 contact capture preserving legacy CSM; yzw: forced sun
    std::array<float, 4> rtWater{}; // actual water draw, animated bump UV offset
    std::array<float, 4> rainRipples{}; // intensity, simulation seconds, camera world X/Z
    std::array<float, 4> wetSurface{}; // darkening, restrained sun sheen, exponent, relative water height
    std::array<float, 32> localShadowMatrices{};
    std::array<float, 4> localShadowControl{}; // rendered slot mask, resolution, per-slot effective ranges
    std::array<float, 4> pbr{}; // perceptual roughness (0=legacy), dielectric F0, reserved
    std::array<float, 4> pbrDiffuse{}; // authored material mode, used for conservative eligibility
};
} // namespace Poseidon::vk
