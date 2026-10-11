#pragma once
#include <Poseidon/Graphics/Core/MatrixConversion.hpp>
#include <vulkan/vulkan.h>

namespace Poseidon::vk
{
// Vulkan-only packing; keep the shared CPU projection helpers SDK-independent.
inline VkTransformMatrixKHR RayInstanceTransform(Matrix4Val modelToWorld)
{
    VkTransformMatrixKHR result{};
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 3; ++col) result.matrix[row][col] = modelToWorld(row, col);
        result.matrix[row][3] = modelToWorld.Position()[row];
    }
    return result;
}
} // namespace Poseidon::vk
