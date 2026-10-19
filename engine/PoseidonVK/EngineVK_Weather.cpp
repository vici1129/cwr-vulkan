#include <PoseidonVK/EngineVK.hpp>
#include <PoseidonVK/ShapeGeometryVK.hpp>
#include <Poseidon/World/Terrain/LandscapeShared.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <cmath>
#include <limits>

namespace Poseidon
{
namespace
{
constexpr int MapSize = 1152, Cells = 12; // integer 96 texels/cell: recentering preserves the sample grid
constexpr float Grid = 6000, Span = Cells * Grid;
constexpr int SkyWidth = 256, SkyHeight = 128;
using Pixel = std::array<float, 4>;
Pixel Sample(const DecodedImage& image, float u, float v, unsigned sampler)
{
    const float x = u * image.width - .5f, y = v * image.height - .5f;
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const float fx = x - ix, fy = y - iy;
    Pixel out{};
    for (int j = 0; j < 2; ++j)
        for (int i = 0; i < 2; ++i)
        {
            int px = ix + i, py = iy + j;
            px = sampler & 1 ? std::clamp(px, 0, image.width - 1) : (px % image.width + image.width) % image.width;
            py = sampler & 2 ? std::clamp(py, 0, image.height - 1) : (py % image.height + image.height) % image.height;
            const float w = (i ? fx : 1 - fx) * (j ? fy : 1 - fy) / 255.f;
            for (int c = 0; c < 4; ++c)
                out[c] += image.rgba[(py * image.width + px) * 4 + c] * w;
        }
    return out;
}
std::shared_ptr<vk::TextureImage> UploadMap(vk::VulkanContext& context, const std::vector<Pixel>& pixels)
{
    // Store straight color after alpha compositing; hardware decodes RGB before filtering.
    std::vector<uint8_t> bytes(pixels.size() * 4);
    for (size_t p = 0; p < pixels.size(); ++p)
        for (int c = 0; c < 4; ++c)
            bytes[p * 4 + c] = uint8_t(std::clamp(
                std::lround(255 * (c == 3 ? pixels[p][c] : pixels[p][c] / std::max(pixels[p][3], .00001f))), 0l, 255l));
    // Coarse ray intervals need a filtered footprint, not point samples of
    // distant cloud detail. Base level stays byte-identical for cloud shadows
    // and the disabled-volume reflection path. Average premultiplied coverage
    // before unpremultiplication so empty texels cannot darken cloud edges.
    std::vector<std::vector<uint8_t>> levels;
    levels.push_back(std::move(bytes));
    auto previous = pixels;
    int size = MapSize;
    while (size > 36)
    {
        const int next = size / 2;
        std::vector<Pixel> filtered(next * next);
        levels.emplace_back(next * next * 4);
        for (int y = 0; y < next; ++y)
            for (int x = 0; x < next; ++x)
            {
                auto& p = filtered[y * next + x];
                for (int j = 0; j < 2; ++j)
                    for (int i = 0; i < 2; ++i)
                        for (int c = 0; c < 4; ++c)
                            p[c] += previous[(y * 2 + j) * size + x * 2 + i][c] * .25f;
                for (int c = 0; c < 4; ++c)
                    levels.back()[(y * next + x) * 4 + c] = uint8_t(std::clamp(
                        std::lround(255 * (c == 3 ? p[c] : p[c] / std::max(p[3], .00001f))), 0l, 255l));
            }
        previous = std::move(filtered);
        size = next;
    }
    std::vector<vk::TextureMip> mips;
    size = MapSize;
    for (const auto& level : levels)
    {
        mips.push_back({uint32_t(size), uint32_t(size), level.data()});
        size /= 2;
    }
    return context.UploadTexture(mips);
}
float Edge(float ax, float ay, float bx, float by, float px, float py)
{
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}
} // namespace
struct EngineVK::WeatherCache
{
    const Landscape* land = nullptr;
    Ref<Object> skyObject;
    std::array<Ref<LODShapeWithShadow>, 4> clouds;
    std::array<vk::ShapeGeometry, 4> geometry;
    std::array<std::array<int, 2>, 3> origins{};
    std::array<std::shared_ptr<vk::TextureImage>, 4> images;
    std::shared_ptr<vk::TextureImage> sourceSky;
    std::vector<std::array<float, 3>> skyUV;
};

void EngineVK::UpdateWeather()
{
    if (!_vk.WeatherNeeded())
    {
        _vk.SetWeatherFrame({}, {});
        return;
    }
    auto* land = GScene->GetLandscape();
    const auto* camera = GScene->GetCamera();
    const auto* sun = GScene->MainLight();
    if (!land || !sun || !camera || _nightVision)
    {
        _vk.SetWeatherFrame({}, {});
        return;
    }
    try
    {
        if (!_weatherCache || _weatherCache->land != land || _weatherCache->skyObject.GetRef() != land->SkyObject())
        {
            _weatherCache = std::make_shared<WeatherCache>();
            auto& cache = *_weatherCache;
            cache.land = land;
            cache.skyObject = land->SkyObject();
            for (int i = 0; i < 4; ++i)
            {
                cache.clouds[i] = GScene->Preloaded(CloudShapes[i]);
                if (cache.clouds[i] && cache.clouds[i]->LevelOpaque(0))
                    cache.geometry[i] = vk::ExtractShapeGeometry(*cache.clouds[i]->LevelOpaque(0));
            }
        }
        auto& cache = *_weatherCache;
        vk::WeatherLighting lighting;
        const auto position = camera->Position();
        lighting.camera = {position.X(), position.Y(), position.Z(), CloudScale};
        const auto direction = -sun->SunDirection();
        // Overcast already removes the engine's direct sunlight. Keep residual
        // modulation modest and taper it out as the sky becomes fully opaque.
        lighting.sun = {direction.X(), direction.Y(), direction.Z(),
                        .55f * std::clamp(land->SkyThrough(), 0.f, 1.f) *
                            std::clamp(1.f - sun->NightEffect(), 0.f, 1.f) *
                            std::clamp((direction.Y() - .04f) / .14f, 0.f, 1.f)};
        const auto eye = GetAccomodateEye();
        const auto cloud = (sun->Ambient() + sun->Diffuse()) * eye * eye * land->CloudsBrightness();
        const auto sky = sun->SkyColor() * eye, sunSky = sun->SunSkyColor() * eye;
        lighting.cloudColor = {cloud.R(), cloud.G(), cloud.B(), land->CloudsAlpha()};
        lighting.skyColor = {sky.R(), sky.G(), sky.B(), 0};
        lighting.sunSky = {sunSky.R(), sunSky.G(), sunSky.B(), 0};
        lighting.options = {0, 0, land->CloudsPosition(), 0};
        const auto ambient = sun->Ambient() * eye * eye * land->CloudsBrightness();
        const auto direct = sun->Diffuse() * eye * eye * land->CloudsBrightness();
        lighting.volumeAmbient = {ambient.R(), ambient.G(), ambient.B(), 0};
        lighting.volumeSun = {direct.R(), direct.G(), direct.B(), 1.f - sun->NightEffect()};
        for (int layer = 0; layer < 3; ++layer)
        {
            const auto move = land->GetCloudLayer(layer);
            lighting.drift[layer] = {move.xCell * Grid + move.x, move.zCell * Grid + move.z, 0, 0};
            int cx = int(std::floor(position.X() / Grid)) - move.xCell - Cells / 2;
            int cz = int(std::floor(position.Z() / Grid)) - move.zCell - Cells / 2;
            const bool reuse = cache.images[layer] && std::abs(cx - cache.origins[layer][0]) <= 2 &&
                               std::abs(cz - cache.origins[layer][1]) <= 2;
            if (reuse)
            {
                cx = cache.origins[layer][0];
                cz = cache.origins[layer][1];
            }
            lighting.layers[layer] = {(cx + move.xCell) * Grid + move.x, (cz + move.zCell) * Grid + move.z, 1.f / Span,
                                      move.height};
            if (reuse)
                continue;
            std::vector<Pixel> pixels(MapSize * MapSize);
            // Include neighboring cell geometry crossing the map boundary.
            for (int z = cz - 2; z < cz + Cells + 2; ++z)
                for (int x = cx - 2; x < cx + Cells + 2; ++x)
                {
                    const auto cell = land->GetCloudCell(layer, x, z);
                    if (cell.type >= 4 || !cache.clouds[cell.type])
                        continue;
                    const auto& geometry = cache.geometry[cell.type];
                    const auto* shape = cache.clouds[cell.type]->LevelOpaque(0);
                    Matrix3 rotation(MRotationY, cell.azimuth);
                    std::vector<std::array<float, 2>> points;
                    for (const auto& v : geometry.vertices)
                    {
                        auto p = rotation * Vector3(v.position[0], v.position[1], v.position[2]);
                        points.push_back({(p.X() + (x - cx) * Grid + cell.x) * MapSize / Span,
                                          (p.Z() + (z - cz) * Grid + cell.z) * MapSize / Span});
                    }
                    for (size_t s = 0; s < geometry.sections.size(); ++s)
                    {
                        const auto& section = shape->GetSection(int(s));
                        auto* texture = dynamic_cast<TextureVK*>(section.properties.GetTexture());
                        if (!texture)
                            continue;
                        const unsigned sampler = vk::ShapeSampler(render::SplitLegacy(section.properties.Special()));
                        const auto& range = geometry.sections[s];
                        for (unsigned t = range.begin; t < range.end; t += 3)
                        {
                            const unsigned a = geometry.indices[t], b = geometry.indices[t + 1],
                                           c = geometry.indices[t + 2];
                            const auto& p = points[a];
                            const auto& q = points[b];
                            const auto& r = points[c];
                            const float area = Edge(p[0], p[1], q[0], q[1], r[0], r[1]);
                            if (std::abs(area) < .0001f)
                                continue;
                            const int x0 = std::max(0, int(std::floor(std::min({p[0], q[0], r[0]}))));
                            const int y0 = std::max(0, int(std::floor(std::min({p[1], q[1], r[1]}))));
                            const int x1 = std::min(MapSize - 1, int(std::ceil(std::max({p[0], q[0], r[0]}))));
                            const int y1 = std::min(MapSize - 1, int(std::ceil(std::max({p[1], q[1], r[1]}))));
                            for (int py = y0; py <= y1; ++py)
                                for (int px = x0; px <= x1; ++px)
                                {
                                    const float u = Edge(q[0], q[1], r[0], r[1], px + .5f, py + .5f) / area;
                                    const float v = Edge(r[0], r[1], p[0], p[1], px + .5f, py + .5f) / area;
                                    const float w = 1 - u - v;
                                    if (u < 0 || v < 0 || w < 0)
                                        continue;
                                    const auto& av = geometry.vertices[a];
                                    const auto& bv = geometry.vertices[b];
                                    const auto& cv = geometry.vertices[c];
                                    const auto color =
                                        Sample(texture->Pixels(), av.uv[0] * u + bv.uv[0] * v + cv.uv[0] * w,
                                               av.uv[1] * u + bv.uv[1] * v + cv.uv[1] * w, sampler);
                                    auto& out = pixels[py * MapSize + px];
                                    for (int k = 0; k < 3; ++k)
                                        out[k] = color[k] * color[3] + out[k] * (1 - color[3]);
                                    out[3] = color[3] + out[3] * (1 - color[3]);
                                }
                        }
                    }
                }
            cache.images[layer] = UploadMap(_vk, pixels);
            cache.origins[layer] = {cx, cz};
        }
        // Resolve the stock skydome's actual directional UV mapping once. This
        // is a small sky-only lookup, not a scene/environment capture system.
        auto* skyTexture = dynamic_cast<TextureVK*>(land->SkyTexture());
        if (skyTexture && cache.skyObject && cache.skyObject->GetShape()->LevelOpaque(0))
        {
            if (cache.skyUV.empty())
            {
                const auto* lod = cache.skyObject->GetShape();
                const auto geometry = vk::ExtractShapeGeometry(*lod->LevelOpaque(0));
                cache.skyUV.resize(SkyWidth * SkyHeight);
                for (int y = 0; y < SkyHeight; ++y)
                    for (int x = 0; x < SkyWidth; ++x)
                    {
                        const float phi = (x + .5f) * 2 * H_PI / SkyWidth - H_PI,
                                    theta = (y + .5f) * (.5f * H_PI) / SkyHeight;
                        const Vector3 ray(std::cos(theta) * std::cos(phi), std::sin(theta),
                                          std::cos(theta) * std::sin(phi));
                        float nearest = std::numeric_limits<float>::max();
                        auto& uv = cache.skyUV[y * SkyWidth + x];
                        for (size_t t = 0; t < geometry.indices.size(); t += 3)
                        {
                            const auto& a = geometry.vertices[geometry.indices[t]];
                            const auto& b = geometry.vertices[geometry.indices[t + 1]];
                            const auto& c = geometry.vertices[geometry.indices[t + 2]];
                            const Vector3 p =
                                Vector3(a.position[0], a.position[1], a.position[2]) + lod->BoundingCenter();
                            const Vector3 e1 = Vector3(b.position[0] - a.position[0], b.position[1] - a.position[1],
                                                       b.position[2] - a.position[2]);
                            const Vector3 e2 = Vector3(c.position[0] - a.position[0], c.position[1] - a.position[1],
                                                       c.position[2] - a.position[2]);
                            const auto h = ray.CrossProduct(e2);
                            const float det = e1 * h;
                            if (std::abs(det) < 1e-8f)
                                continue;
                            const float u = (-p * h) / det;
                            const auto q = (-p).CrossProduct(e1);
                            const float v = (ray * q) / det, distance = (e2 * q) / det;
                            if (u < 0 || v < 0 || u + v > 1 || distance <= 0 || distance >= nearest)
                                continue;
                            nearest = distance;
                            uv = {a.uv[0] * (1 - u - v) + b.uv[0] * u + c.uv[0] * v,
                                  a.uv[1] * (1 - u - v) + b.uv[1] * u + c.uv[1] * v, 1};
                        }
                    }
            }
            const auto source = skyTexture->Image(_vk); // replaced by the weather texture blender, even at same address
            if (cache.sourceSky != source)
            {
                std::vector<uint8_t> bytes(SkyWidth * SkyHeight * 4);
                for (size_t i = 0; i < cache.skyUV.size(); ++i)
                {
                    const auto& uv = cache.skyUV[i];
                    const auto color = Sample(skyTexture->Pixels(), uv[0], uv[1], 2);
                    for (int c = 0; c < 3; ++c)
                        bytes[i * 4 + c] = uint8_t(std::clamp(std::lround(color[c] * 255), 0l, 255l));
                    bytes[i * 4 + 3] = uv[2] > 0 ? 255 : 0;
                }
                cache.images[3] = _vk.UploadTexture(SkyWidth, SkyHeight, bytes.data());
                cache.sourceSky = source;
            }
            lighting.options[0] = 1;
            lighting.options[1] = 1; // stock sky ready, independent of water reflection controls
        }
        _vk.SetWeatherFrame(lighting, cache.images);
    }
    catch (const std::exception& error)
    {
        _vk.FailWeather(error.what());
    }
}

bool EngineVK::DrawVolumetricClouds()
{
    if (!_worldEffectsPending || _nightVision || !GScene || !GScene->GetCamera())
        return false;
    return _vk.DrawVolumetricClouds();
}
} // namespace Poseidon
