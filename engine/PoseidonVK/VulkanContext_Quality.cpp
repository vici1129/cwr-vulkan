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
    q.water = _refractionRequested && _shoreFoamRequested && _rainRipplesEnabled && _refractionFailure.empty() ? 1
              : (!_refractionRequested && !_shoreFoamRequested && !_rainRipplesEnabled ? 0 : -1);
    q.ao = _ssaoEnabled;
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
    return {};
}

std::string VulkanContext::SetGraphicsQuality(const GraphicsQuality& q)
{
    // Preflight the entire bundle before changing any renderer state.
    if (!q.Valid()) return "Invalid Vulkan graphics selection.";
    if (q.shadows == 2)
        if (auto reason = GraphicsQualityRTUnavailable(); !reason.empty()) return reason;
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
    // Other developer RT consumers own the same scene. Preserve it for them.
    if (q.shadows == 2 || (!RTAOEnabled() && !RTReflectionsEnabled() && !RTGIEnabled()))
        if (_rtDynamicEnabled != (q.shadows == 2)) SetRTDynamic(q.shadows == 2 ? "on" : "off");
    SetVolumetricClouds(q.clouds ? "on" : "off");
    SetCloudShadows(q.clouds ? "on" : "off");
    SetWaterRefraction(q.water ? "on" : "off");
    SetShoreFoam(q.water ? "on" : "off");
    SetRainRipples(q.water ? "on" : "off");
    SetSSAO(q.ao, 1.2f, 1.5f, .03f, 80.f);
    return {};
}
}
