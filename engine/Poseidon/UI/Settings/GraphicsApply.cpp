#include <Poseidon/UI/Settings/GraphicsApply.hpp>

#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp> // pulls Engine + extern GEngine
#include <Poseidon/World/Scene/Scene.hpp>

namespace Poseidon
{

} // namespace Poseidon
#include <Poseidon/Core/Game/GameLoop.hpp>
namespace Poseidon
{
using Poseidon::gUserFpsCap;

float TerrainGridForTier(GraphicsConfig::Tier tier)
{
    switch (tier)
    {
        case GraphicsConfig::TierLow:
            return 50.0f;
        case GraphicsConfig::TierMedium:
            return 25.0f;
        case GraphicsConfig::TierHigh:
            return 12.5f;
        case GraphicsConfig::TierUltra:
            return 6.25f;
        case GraphicsConfig::TierExtreme:
            return 3.125f;
        default:
            return 6.25f;
    }
}

namespace
{
// Tier → projected-screen-size multiplier for LOD selection.  Low
// halves the apparent size (picks coarser LOD); Ultra doubles
// (picks finer LOD).  Range matches Scene::SetObjectLODBias clamp
// of [0.25, 4.0].
float ObjectLodTierToBias(GraphicsConfig::Tier t)
{
    switch (t)
    {
        case GraphicsConfig::TierLow:
            return 0.5f;
        case GraphicsConfig::TierMedium:
            return 0.75f;
        case GraphicsConfig::TierHigh:
            return 1.0f;
        case GraphicsConfig::TierUltra:
            return 2.0f;
        default:
            return 1.0f;
    }
}

// VsyncMode enum → SDL swap interval ints (0 / 1 / -1).
int VsyncToInterval(GraphicsConfig::VsyncMode v)
{
    switch (v)
    {
        case GraphicsConfig::VsyncOff:
            return 0;
        case GraphicsConfig::VsyncOn:
            return 1;
        case GraphicsConfig::VsyncAdaptive:
            return -1;
    }
    return 1;
}
} // namespace

float ObjectLodBiasForTier(GraphicsConfig::Tier tier)
{
    return ObjectLodTierToBias(tier);
}

void ApplyGraphicsConfigToEngine(const GraphicsConfig& cfg, const GraphicsConfig* baseline)
{
    if (GScene)
    {
        if (!baseline || cfg.terrainDetail != baseline->terrainDetail)
            GScene->SetPreferredTerrainGrid(TerrainGridForTier(cfg.terrainDetail));
        if (!baseline || cfg.objectLod != baseline->objectLod || cfg.objectLodConfigured != baseline->objectLodConfigured)
            GScene->SetObjectLODBias(ObjectLodTierToBias(cfg.objectLod));
        // Shadow tier — Off → both off; Low+ → both on.  Low/Med/High
        // discrimination is UI-only until a shadow-distance bias hook
        // lands.
        // Vulkan's quality group owns temporary CSM caster gates. The shared
        // legacy preference still belongs to GL33, including when editing FPS.
        if ((!GEngine || !GEngine->HasVulkanGraphics()) && (!baseline || cfg.shadowQuality != baseline->shadowQuality))
        {
            const bool shadowsOn = cfg.shadowQuality != GraphicsConfig::TierOff;
            GScene->SetObjectShadows(shadowsOn);
            GScene->SetVehicleShadows(shadowsOn);
        }
        // Particles tier — Off → cloudlets off; Low/High → on.  Low vs
        // High is UI-only until the cloudlet system grows tiered density.
        if (!baseline || cfg.particlesQuality != baseline->particlesQuality)
            GScene->SetCloudlets(cfg.particlesQuality != GraphicsConfig::TierOff);
    }
    if (GEngine)
    {
        if (!baseline || cfg.vsync != baseline->vsync)
        {
            const int interval = VsyncToInterval(cfg.vsync);
            if (!GEngine->SetSwapInterval(interval) && interval < 0) GEngine->SetSwapInterval(1);
        }
        if (!baseline || cfg.brightness != baseline->brightness) GEngine->SetBrightness(cfg.brightness);
        if (!baseline || cfg.gamma != baseline->gamma) GEngine->SetGamma(cfg.gamma);
        if (!baseline || cfg.alphaToCoverage != baseline->alphaToCoverage) GEngine->SetAlphaToCoverage(cfg.alphaToCoverage);
        if (!baseline || cfg.renderScale != baseline->renderScale) GEngine->SetRenderScale(cfg.renderScale);
        if (!baseline || cfg.msaaSamples != baseline->msaaSamples) GEngine->SetMsaaSamples(cfg.msaaSamples);
        if (!baseline || cfg.multitexturing != baseline->multitexturing) GEngine->SetMultitexturing(cfg.multitexturing);
    }
    if (!baseline || cfg.fpsCap != baseline->fpsCap) gUserFpsCap = cfg.fpsCap;
}

} // namespace Poseidon
