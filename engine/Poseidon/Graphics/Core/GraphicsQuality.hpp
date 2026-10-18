#pragma once
#include <string>

namespace Poseidon
{
// Renderer state/capability exchange, not a separate configuration store.
struct ImageQuality
{
    std::string aa = "off";
    int scale = 100;
    bool operator==(const ImageQuality& b) const { return aa == b.aa && scale == b.scale; }
    bool operator!=(const ImageQuality& b) const { return !(*this == b); }
    bool IsFSR() const { return aa.compare(0, 4, "fsr-") == 0; }
};
// Four preset components plus independent lighting. -1 is an observation of a developer's
// partial group; it is never a selectable or persisted quality tier.
struct GraphicsQuality
{
    int shadows = 0; // Standard, Enhanced, Ray Traced
    int clouds = 0;  // Standard, Enhanced
    int water = 0;   // Standard, Enhanced, Ray Traced
    bool ao = false;
    bool volumetricLighting = false; // Independent preference, not a preset component.

    bool operator==(const GraphicsQuality& other) const
    {
        return shadows == other.shadows && clouds == other.clouds && water == other.water && ao == other.ao &&
               volumetricLighting == other.volumetricLighting;
    }
    bool operator!=(const GraphicsQuality& other) const { return !(*this == other); }
    bool Valid() const { return shadows >= 0 && shadows <= 2 && clouds >= 0 && clouds <= 1 && water >= 0 && water <= 2; }
    int Preset() const
    {
        if (shadows == 0 && clouds == 0 && water == 0 && !ao) return 0;
        if (shadows == 1 && clouds == 1 && water == 1 && ao) return 1;
        return 2;
    }
    static GraphicsQuality Enhanced() { return {1, 1, 1, true}; }
};
}
