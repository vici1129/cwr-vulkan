#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace Poseidon::vk
{
constexpr float LocalShadowMaxRange = 160.f;
constexpr int LocalShadowResolution = 1024;
// Positive-Z perspective, column-major, depth 0..1. The shared depth shader
// flips Y; receiver projection uses the same flip. No camera-grid snapping:
// the map stays attached to the moving light, never to the observer.
inline std::array<float, 16> LocalShadowMatrix(const std::array<float, 3>& p, const std::array<float, 3>& direction,
                                               float halfAngle, float farPlane)
{
    auto dot = [](auto a, auto b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };
    auto cross = [](auto a, auto b)
    { return std::array<float, 3>{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; };
    auto normalize = [&](auto a)
    {
        const float s = std::sqrt(dot(a, a));
        for (auto& v : a)
            v /= s;
        return a;
    };
    const auto f = normalize(direction);
    const auto r =
        normalize(cross(std::abs(f[1]) > .95f ? std::array<float, 3>{1, 0, 0} : std::array<float, 3>{0, 1, 0}, f));
    const auto u = cross(f, r);
    const float scale = 1.f / std::tan(halfAngle), nearPlane = .1f;
    const float a = farPlane / (farPlane - nearPlane), b = -nearPlane * a;
    std::array<float, 16> m{};
    for (int c = 0; c < 3; ++c)
    {
        m[c * 4] = r[c] * scale;
        m[c * 4 + 1] = u[c] * scale;
        m[c * 4 + 2] = f[c] * a;
        m[c * 4 + 3] = f[c];
    }
    m[12] = -dot(r, p) * scale;
    m[13] = -dot(u, p) * scale;
    m[14] = -dot(f, p) * a + b;
    m[15] = -dot(f, p);
    return m;
}
} // namespace Poseidon::vk
