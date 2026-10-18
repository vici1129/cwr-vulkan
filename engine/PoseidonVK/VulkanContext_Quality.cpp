#include <PoseidonVK/VulkanContext.hpp>

namespace Poseidon::vk
{
GraphicsQuality VulkanContext::GetGraphicsQuality() const
{
    GraphicsQuality q;
    const bool enhanced = _softShadowsEnabled && _contactRequested && _localShadowsEnabled;
    const bool standard = !_softShadowsEnabled && !_contactRequested && !_localShadowsEnabled;
    q.shadows = enhanced ? (RTShadowsEnabled() ? (SoftRTShadowsEnabled() && RTDynamicEnabled() ? 2 : -1) : 1)
                         : (standard && !RTShadowsEnabled() ? 0 : -1);
    q.clouds = _cloudsRequested && _cloudShadows && _cloudsFailure.empty() && _weatherFailure.empty() ? 1
               : (!_cloudsRequested && !_cloudShadows ? 0 : -1);
    const bool enhancedWater = _refractionRequested && _shoreFoamRequested && _rainRipplesEnabled &&
                               _refractionFailure.empty();
    q.water = enhancedWater ? (RTReflectionsEnabled() ?
                                  (_skyReflections && RTDynamicEnabled() && _weatherFailure.empty() ? 2 : -1) : 1)
              : (!_refractionRequested && !_shoreFoamRequested && !_rainRipplesEnabled && !RTReflectionsEnabled() ? 0 : -1);
    q.ao = _ssaoEnabled;
    q.volumetricLighting = VolumetricFogEnabled();
    return q;
}

std::string VulkanContext::GraphicsQualityRTUnavailable() const
{
    if (!_rtaoAvailable) return "Ray Traced unavailable: " + _rtaoReason;
    VkPhysicalDeviceFeatures features{};
    if (_physical) vkGetPhysicalDeviceFeatures(_physical, &features);
    if (!features.independentBlend) return "Ray Traced unavailable: independent MRT blending is unsupported.";
    return {};
}

std::string VulkanContext::GraphicsQualityFailure() const
{
    if (_cloudsRequested && !_cloudsFailure.empty()) return _cloudsFailure;
    if (_cloudShadows && !_weatherFailure.empty()) return _weatherFailure;
    if (!_refractionFailure.empty()) return _refractionFailure;
    if (!_rtReflectionFailure.empty()) return _rtReflectionFailure;
    if (_skyReflections && RTReflectionsEnabled() && !_weatherFailure.empty()) return _weatherFailure;
    return {};
}

std::string VulkanContext::SetGraphicsQuality(const GraphicsQuality& q)
{
    // Preflight the entire bundle before changing any renderer state.
    if (!q.Valid()) return "Invalid Vulkan graphics selection.";
    if (q.shadows == 2 || q.water == 2)
        if (auto reason = GraphicsQualityRTUnavailable(); !reason.empty()) return reason;
    if (q.water == 2 && !_rtReflectionFailure.empty()) return _rtReflectionFailure;
    VkPhysicalDeviceFeatures features{};
    if (_physical) vkGetPhysicalDeviceFeatures(_physical, &features);
    if (q.shadows > 0 && _physical && !features.independentBlend)
        return "Enhanced shadows require independent MRT blending.";
    if (q.water)
        if (auto reason = WaterCaptureSupport(); !reason.empty()) return reason;

    const char* shadow = q.shadows > 0 ? "on" : "off";
    SetSoftShadows(shadow);
    SetContactShadows(shadow);
    SetLocalShadows(shadow);
    if (q.shadows != 2) SetSoftRTShadows("off");
    SetRTShadows(q.shadows == 2 ? "on" : "off");
    if (q.shadows == 2) SetSoftRTShadows("on");
    SetRTReflections(q.water == 2 ? "on" : "off");
    SetSkyReflections(q.water == 2 ? "on" : "off");
    // Decide after both player RT consumers have changed. Developer AO/GI
    // consumers also retain their shared bounded scene and dynamic geometry.
    const bool dynamic = q.shadows == 2 || q.water == 2;
    if (dynamic || (!RTAOEnabled() && !RTGIEnabled()))
        if (_rtDynamicEnabled != dynamic) SetRTDynamic(dynamic ? "on" : "off");
    SetVolumetricClouds(q.clouds ? "on" : "off");
    SetCloudShadows(q.clouds ? "on" : "off");
    SetWaterRefraction(q.water ? "on" : "off");
    SetShoreFoam(q.water ? "on" : "off");
    SetRainRipples(q.water ? "on" : "off");
    SetSSAO(q.ao, 1.2f, 1.5f, .03f, 80.f);
    SetVolumetricFog(q.volumetricLighting ? "on" : "off");
    return {};
}
}
