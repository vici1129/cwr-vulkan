#include <PoseidonVK/EngineVK.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>
#include <Poseidon/World/Terrain/Landscape.hpp>
#include <Poseidon/Graphics/Rendering/Lighting/Lights.hpp>
#include <cmath>

namespace Poseidon
{
void EngineVK::UpdateEnvironment()
{
    const auto* camera = GScene->GetCamera();
    const auto* land = GScene->GetLandscape();
    if (!land || !camera) return;
    const auto position = camera->Position();
    const auto& p = camera->ProjectionNormal();
    const float ground = land->SurfaceY(position.X(), position.Z());
    const float depth = ground < land->GetSeaLevel() && position.Y() >= ground
        ? std::max(land->GetSeaLevel() - position.Y(), 0.f) : 0.f;
    _vk.SetEnvironmentFrame(land->GetRainDensity(), Glob.time.toFloat(),
        {depth, p(2,2), p.Position().Z(), 0,
         1.f/p(0,0), 1.f/p(1,1), camera->DirectionAside().Y(), camera->DirectionUp().Y(),
         camera->Direction().Y(), _fogColor.R(), _fogColor.G(), _fogColor.B()});
}

void EngineVK::UpdateAtmosphere()
{
    const auto* camera = GScene->GetCamera();
    const auto* light = GScene->MainLight();
    const auto* land = GScene->GetLandscape();
    vk::ShapeLighting volume{};
    _vk.SetVolumetricFrame(volume);
    if (!light || !land || _nightVision)
    {
        _vk.SetAtmosphereFrame({}, {}, {});
        return;
    }
    const auto sun = -light->SunDirection();
    const auto color = light->SunColor();
    const float daylight = std::clamp(1.f - light->NightEffect(), 0.f, 1.f);
    const float clear = std::clamp(land->SkyThrough(), 0.f, 1.f);
    // Camera-relative CSM matrices consume world-oriented metres. No translation.
    const auto right = camera->DirectionAside(), up = camera->DirectionUp(), forward = camera->Direction();
    volume.world = {right.X(),   right.Y(),   right.Z(),   0, up.X(), up.Y(), up.Z(), 0,
                    forward.X(), forward.Y(), forward.Z(), 0, 0,      0,      0,      1};
    volume.atmosphereSun = {sun.X(), sun.Y(), sun.Z(), daylight * clear * clear * std::clamp(sun.Y() / .08f, 0.f, 1.f)};
    const auto energy = light->Diffuse() * GetAccomodateEye();
    volume.atmosphereColor = {energy.R(), energy.G(), energy.B(), 0};
    volume.fogColor = {_fogColor.R(), _fogColor.G(), _fogColor.B(), 0};
    volume.fogParams = {GScene->GetFogMinRange(), GScene->GetFogMaxRange(), 0, 0};
    _vk.SetVolumetricFrame(volume);
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
