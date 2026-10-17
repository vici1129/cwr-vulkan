#pragma once

namespace Poseidon
{
// The four player choices. -1 is a read-only observation of a developer's
// partial group; it is never a selectable or persisted quality tier.
struct GraphicsQuality
{
    int shadows = 0; // Standard, Enhanced, Ray Traced
    int clouds = 0;  // Standard, Enhanced
    int water = 0;   // Standard, Enhanced
    bool ao = false;

    bool operator==(const GraphicsQuality& other) const
    {
        return shadows == other.shadows && clouds == other.clouds && water == other.water && ao == other.ao;
    }
    bool operator!=(const GraphicsQuality& other) const { return !(*this == other); }
    bool Valid() const { return shadows >= 0 && shadows <= 2 && clouds >= 0 && clouds <= 1 && water >= 0 && water <= 1; }
    int Preset() const
    {
        if (*this == GraphicsQuality{}) return 0;
        if (*this == Enhanced()) return 1;
        return 2;
    }
    static GraphicsQuality Enhanced() { return {1, 1, 1, true}; }
};
}
