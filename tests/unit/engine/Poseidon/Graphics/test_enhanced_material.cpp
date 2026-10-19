#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <PoseidonVK/EnhancedMaterialVK.hpp>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <Poseidon/Graphics/Textures/PAADecoder.hpp>

TEST_CASE("Authored supplemental maps decode as bounded albedo and linear unit normals",
          "[Graphics][enhanced-material]")
{
    const auto root = std::filesystem::path(TESTS_ROOT_DIR).parent_path();
    const auto color = Poseidon::DecodePAAFile((root / Poseidon::vk::StoneWall.albedo).string());
    const auto normal = Poseidon::DecodePAAFile((root / Poseidon::vk::StoneWall.normalRoughness).string());
    REQUIRE(color.width == 1024);
    REQUIRE(normal.width == color.width);
    REQUIRE(normal.height == color.height);
    REQUIRE(color.rgba.size() == normal.rgba.size());
    float error = 0, minZ = 1, minRoughness = 1;
    bool opaque = true;
    for (size_t i = 0; i < normal.rgba.size(); i += 4)
    {
        const float x = normal.rgba[i] * (2.f / 255) - 1, y = normal.rgba[i + 1] * (2.f / 255) - 1,
                    z = normal.rgba[i + 2] * (2.f / 255) - 1;
        error = std::max(error, std::abs(x * x + y * y + z * z - 1));
        minZ = std::min(minZ, z);
        minRoughness = std::min(minRoughness, normal.rgba[i + 3] / 255.f * Poseidon::vk::StoneWall.roughness);
        opaque &= color.rgba[i + 3] == 255;
    }
    REQUIRE(opaque);
    REQUIRE(error < .02f);
    REQUIRE(minZ > .1f);
    REQUIRE(minRoughness > .8f);
}

namespace EnhancedHistory
{
using std::abs;
using std::max;
using std::min;
#include <PoseidonVK/Shaders/rtgi_history.glsl>
} // namespace EnhancedHistory

TEST_CASE("Enhanced one-sample GI history remains bounded", "[Graphics][enhanced-material]")
{
    float length = 1;
    for (int i = 0; i < 64; ++i)
        length = EnhancedHistory::giHistoryLength(length, 16.f);
    REQUIRE(length == 16);
    REQUIRE(EnhancedHistory::giHistoryWeight(length) == Catch::Approx(15.f / 16));
    REQUIRE_FALSE(EnhancedHistory::giHistorySurface(10, 11, 1));
}

namespace MaterialNormalMath
{
// Compile the actual GLSL frame reconstruction with a small CPU vector adapter.
struct vec2
{
    float x, y;
};
struct vec3
{
    float x, y, z;
    vec3 operator*(float k) const { return {x * k, y * k, z * k}; }
    vec3 operator/(float k) const { return {x / k, y / k, z / k}; }
    vec3 operator+(vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    vec3 operator-(vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    void operator-=(vec3 b) { *this = *this - b; }
};
float dot(vec3 a, vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
vec3 cross(vec3 a, vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
vec3 normalize(vec3 a)
{
    return a / std::sqrt(dot(a, a));
}
using std::abs;
#include <PoseidonVK/Shaders/material_normal.glsl>
} // namespace MaterialNormalMath

TEST_CASE("Enhanced materials require an explicit model visual LOD and section", "[Graphics][enhanced-material]")
{
    using namespace Poseidon::vk;
    REQUIRE(FindEnhancedMaterial("Data3D/ZED_KAMENNA.p3d", .5f, "data\\kamennazed.pac", 48, 1) == &StoneWall);
    REQUIRE(FindEnhancedMaterial(StoneWall.model, 1, StoneWall.section, 24, 1) == &StoneWall);
    REQUIRE_FALSE(FindEnhancedMaterial("addon\\zed_kamenna.p3d", .5f, StoneWall.section, 48, 1));
    REQUIRE_FALSE(FindEnhancedMaterial(StoneWall.model, 1e13f, StoneWall.section, 24, 1));
    REQUIRE_FALSE(FindEnhancedMaterial(StoneWall.model, .5f, "addon\\wall.pac", 48, 1));
    REQUIRE_FALSE(FindEnhancedMaterial(StoneWall.model, .5f, StoneWall.section, 49, 1));
    REQUIRE_FALSE(FindEnhancedMaterial(StoneWall.model, .5f, StoneWall.section, 48, 2));
}

TEST_CASE("Material frames preserve mirrored UV orientation and degenerate fallback", "[Graphics][enhanced-material]")
{
    using namespace MaterialNormalMath;
    const vec3 n{0, 0, 1}, x{1, 0, 0}, y{0, 1, 0};
    const auto flat = materialNormal(n, x, y, {1, 0}, {0, 1}, n);
    REQUIRE(flat.z == Catch::Approx(1));
    const auto mirrored = materialNormal(n, x, y, {-1, 0}, {0, 1}, {.6f, 0, .8f});
    REQUIRE(mirrored.x == Catch::Approx(-.6f));
    REQUIRE(mirrored.z == Catch::Approx(.8f));
    const auto mirroredV = materialNormal(n, x, y, {1, 0}, {0, -1}, {0, .6f, .8f});
    REQUIRE(mirroredV.y == Catch::Approx(-.6f));
    REQUIRE(materialNormal(n, x, y, {0, 0}, {0, 0}, x).z == Catch::Approx(1));
    // Raster first reconstructs object derivatives under nonuniform scaling.
    // Ray hits already have object edges; both inverse-transpose the final n.
    const vec3 worldX{2, 0, 0}, worldY{0, .5f, 0};
    const vec3 objectX{worldX.x / 2, worldX.y / .5f, worldX.z / 3};
    const vec3 objectY{worldY.x / 2, worldY.y / .5f, worldY.z / 3};
    const auto raster = materialNormal(n, objectX, objectY, {1, 0}, {0, 1}, {.6f, 0, .8f});
    const auto ray = materialNormal(n, x, y, {1, 0}, {0, 1}, {.6f, 0, .8f});
    const auto transformed = normalize(vec3{raster.x / 2, raster.y / .5f, raster.z / 3});
    const auto expected = normalize(vec3{.3f, 0, .8f / 3});
    REQUIRE(dot(raster, ray) == Catch::Approx(1));
    REQUIRE(dot(transformed, expected) == Catch::Approx(1));
    REQUIRE(transformed.x > raster.x);
}

TEST_CASE("Ambient replacement preserves all other radiance and fog", "[Graphics][enhanced-material]")
{
    for (float ambient : {0.f, .1f, .5f})
        for (float direct : {0.f, .3f, 2.f})
            for (float local : {0.f, .7f})
                for (float emission : {0.f, .4f})
                    for (float fog : {0.f, .2f, 1.f})
                        for (float fade : {0.f, .5f, 1.f})
                        {
                            const float bounce = .12f, scatter = .2f * (1 - fog);
                            const float raster = (ambient + direct + local + emission) * fog + scatter;
                            const float delta = (bounce - ambient) * fog * fade;
                            const float expected =
                                (ambient * (1 - fade) + bounce * fade + direct + local + emission) * fog + scatter;
                            REQUIRE(raster + delta == Catch::Approx(expected).margin(1e-6));
                            REQUIRE(raster + delta >= 0);
                        }
}
