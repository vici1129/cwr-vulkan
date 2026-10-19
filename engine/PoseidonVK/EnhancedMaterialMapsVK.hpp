#pragma once
#include <Poseidon/Graphics/Textures/PAADecoder.hpp>
#include <algorithm>
#include <span>

namespace Poseidon::vk
{
// Storage packing only, not artwork generation. Independent linear XYZ normal
// and red-channel roughness maps share the existing data sampler/RT buffer.
// Uses available authored mip levels; an absent channel has no effect.
inline std::vector<DecodedImage> PackEnhancedMaps(std::span<const DecodedImage> normals,
                                                  std::span<const DecodedImage> roughness)
{
    std::vector<DecodedImage> result;
    const int width = std::max(normals.empty() ? 0 : normals[0].width, roughness.empty() ? 0 : roughness[0].width);
    const int height = std::max(normals.empty() ? 0 : normals[0].height, roughness.empty() ? 0 : roughness[0].height);
    for (size_t level = 0; level < std::max(normals.size(), roughness.size()); ++level)
    {
        const auto* n = normals.empty() ? nullptr : &normals[std::min(level, normals.size() - 1)];
        const auto* r = roughness.empty() ? nullptr : &roughness[std::min(level, roughness.size() - 1)];
        DecodedImage out;
        out.width = std::max(1, width >> level);
        out.height = std::max(1, height >> level);
        out.rgba.resize(size_t(out.width) * out.height * 4);
        auto pixel = [&](const DecodedImage* src, int x, int y)
        {
            return src->rgba.data() +
                   (size_t(y * src->height / out.height) * src->width + x * src->width / out.width) * 4;
        };
        for (int y = 0; y < out.height; ++y)
            for (int x = 0; x < out.width; ++x)
            {
                auto* dst = out.rgba.data() + (size_t(y) * out.width + x) * 4;
                dst[0] = 128;
                dst[1] = 128;
                dst[2] = 255;
                dst[3] = 255;
                if (n)
                    std::copy_n(pixel(n, x, y), 3, dst);
                if (r)
                    dst[3] = pixel(r, x, y)[0];
            }
        result.push_back(std::move(out));
        if (result.back().width == 1 && result.back().height == 1)
            break;
    }
    return result;
}
} // namespace Poseidon::vk
