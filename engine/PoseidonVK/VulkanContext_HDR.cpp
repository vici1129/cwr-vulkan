#include <PoseidonVK/VulkanContext.hpp>
#include <cmath>
#include <cstdlib>

namespace Poseidon::vk
{
std::string VulkanContext::SetHDR(std::string_view mode)
{
    if (mode == "status")
        return std::string("internal=linear-RGBA16F exposure=1 requested=") + (_hdrRequested ? "HDR10" : "SDR") +
            " active=" + (_hdrActive ? "HDR10-PQ-BT2020" : "SDR-sRGB") +
            " format=" + std::to_string(_outputFormat.format) + " colorSpace=" + std::to_string(_outputFormat.colorSpace) +
            " peakNits=" + std::to_string(_hdrPeak) + " whiteNits=" + std::to_string(_hdrWhite) +
            " requestedPeak=" + std::to_string(_requestedHDRPeak) + " requestedWhite=" + std::to_string(_requestedHDRWhite) +
            " metadata=" + (_hdrActive && _setHdrMetadata ? "submitted" : "none") +
            " physicalDisplay=unverified reason=" + _hdrReason;
    if (mode == "on" || mode == "off")
    {
        _hdrRequested = mode == "on";
        _recreate = true; // Re-query surface availability even if request is unchanged.
        return "OK: output requested for next frame; use status for negotiated output";
    }
    const bool peak = mode.starts_with("peak "), white = mode.starts_with("white ");
    if (peak || white)
    {
        const std::string number(mode.substr(peak ? 5 : 6));
        char* end = nullptr;
        const float value = std::strtof(number.c_str(), &end);
        if (end == number.c_str() || *end || !std::isfinite(value) ||
            (peak && (value < _requestedHDRWhite || value < 100 || value > 10000)) ||
            (white && (value < 80 || value > 500 || value > _requestedHDRPeak)))
            return "INVALID: peak 100..10000 nits, white 80..500 nits, white <= peak";
        if (peak) _requestedHDRPeak = value;
        else _requestedHDRWhite = value;
        _recreate = true; // Keep metadata and pixel encoding changes at the same idle boundary.
        return "OK: HDR luminance updated for next frame";
    }
    return "INVALID: on, off, status, peak <nits>, white <nits>";
}

void VulkanContext::UpdateHDRMetadata()
{
    if (!_hdrActive || !_setHdrMetadata || !_swapchain) return;
    VkHdrMetadataEXT metadata{VK_STRUCTURE_TYPE_HDR_METADATA_EXT};
    metadata.displayPrimaryRed = {.708f, .292f};
    metadata.displayPrimaryGreen = {.170f, .797f};
    metadata.displayPrimaryBlue = {.131f, .046f};
    metadata.whitePoint = {.3127f, .3290f};
    metadata.maxLuminance = _hdrPeak;
    metadata.minLuminance = 0;
    // The display-mapped signal is bounded by peak. Frame-average content light
    // has not been measured; zero means unknown, not an invented scene statistic.
    metadata.maxContentLightLevel = _hdrPeak;
    metadata.maxFrameAverageLightLevel = 0;
    _setHdrMetadata(_device, 1, &_swapchain, &metadata);
}
} // namespace Poseidon::vk
