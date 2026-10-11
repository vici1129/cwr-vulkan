#include <PoseidonVK/EngineVK.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Lights.hpp>
#include <cmath>

namespace Poseidon
{
void EngineVK::UpdateAtmosphere()
{
    const auto* camera = GScene->GetCamera();
    const auto* light = GScene->MainLight();
    const auto* land = GScene->GetLandscape();
    if (!light || !land || _nightVision)
    {
        _vk.SetAtmosphereFrame({}, {}, {});
        return;
    }
    const auto sun = -light->SunDirection();
    const auto color = light->SunColor();
    const float daylight = std::clamp(1.f - light->NightEffect(), 0.f, 1.f);
    const float clear = std::clamp(land->SkyThrough(), 0.f, 1.f);
    const float range = GScene->GetFogMaxRange();
    // Mission visibility is read, never extended or shortened. Dense weather
    // gets the original fog color; it must not become a bright shaft source.
    const float air = std::clamp((range - 150.f) / 650.f, 0.f, 1.f);
    const float strength = daylight * clear * air;
    const auto viewSun = camera->GetInvTransform().Rotate(sun);
    const auto& p = camera->ProjectionNormal();
    std::array<float, 4> screen{};
    if (viewSun.Z() > .01f && sun.Y() > 0)
    {
        screen = {.5f + .5f * p(0, 0) * viewSun.X() / viewSun.Z(), .5f - .5f * p(1, 1) * viewSun.Y() / viewSun.Z(),
                  strength * clear * std::clamp(sun.Y() / .08f, 0.f, 1.f), 0};
    }
    _vk.SetAtmosphereFrame({sun.X(), sun.Y(), sun.Z(), strength},
                           {color.R(), color.G(), color.B(), std::exp(-std::max(camera->Position().Y(), 0.f) / 8000.f)},
                           screen);
}
} // namespace Poseidon
