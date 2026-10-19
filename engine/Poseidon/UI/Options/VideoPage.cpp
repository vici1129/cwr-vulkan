#include <Poseidon/UI/Options/VideoPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>
#include <Poseidon/UI/Options/ConfirmRevertPage.hpp>
#include <Poseidon/UI/Settings/GraphicsApply.hpp>
#include <Poseidon/UI/Settings/GameSettingsConfig.hpp>
#include <Poseidon/UI/Options/GamePage.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/Game/GameLoop.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/Foundation/Common/GamePaths.hpp>
#include <Poseidon/Foundation/Platform/AppConfig.hpp>
#include <Poseidon/IO/Filesystem/Utf8Paths.hpp>
#include <SDL3/SDL_keycode.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>

namespace Poseidon
{
void ApplyCurrentMissionViewDistance();
RString GetUserParams();
namespace
{
std::string GraphicsPath()
{
    return GamePaths::Instance().UserDir() + "graphics.cfg";
}
std::string DisplayPath()
{
    return GamePaths::Instance().UserDir() + "display.cfg";
}
const int caps[] = {0, 30, 60, 90, 120, 144, 240};
const char* aaNames[] = {"off",   "fxaa",       "smaa",        "taa",          "msaa2",          "msaa4",
                         "msaa8", "fsr-native", "fsr-quality", "fsr-balanced", "fsr-performance"};
const char* aaLabels[] = {"Off",
                          "FXAA",
                          "SMAA",
                          "TAA",
                          "MSAA 2x",
                          "MSAA 4x",
                          "MSAA 8x",
                          "FSR Native AA",
                          "FSR Quality",
                          "FSR Balanced",
                          "FSR Performance"};
int Samples(const std::string& aa)
{
    return aa == "msaa2" ? 2 : aa == "msaa4" ? 4 : aa == "msaa8" ? 8 : 0;
}
int Slider(float value, float lo, float hi)
{
    return int(std::lround((std::clamp(value, lo, hi) - lo) * 100 / (hi - lo)));
}
float FromSlider(int v, float lo, float hi)
{
    return std::round((lo + std::clamp(v, 0, 100) * (hi - lo) / 100) * 10) / 10;
}
template <class T, size_t N>
int Index(const T (&values)[N], T value)
{
    for (int i = 0; i < int(N); ++i)
        if (values[i] == value)
            return i;
    return 0;
}
const char* Text(const char* key, const char* english)
{
    return LocalizeStringWithFallback(key, english);
}
const char* Unavailable(const std::string& reason, bool fsr)
{
    if (reason.find("=0") != std::string::npos)
        return Text("STR_VIDEO_DISABLED_SESSION", "Disabled by a startup override. Remove it and restart to use this option.");
    if (fsr && (reason.find("build") != std::string::npos || reason.find("compiled") != std::string::npos))
        return Text("STR_VIDEO_FSR_BUILD", "FSR is unavailable in this version of the game.");
    if (reason.find("failed") != std::string::npos || reason.find("initializ") != std::string::npos ||
        reason.find("entry point") != std::string::npos || reason.find("missing vk") != std::string::npos)
        return Text("STR_VIDEO_RESTART", "Unavailable this session. Restart the game to try again.");
    return fsr ? Text("STR_VIDEO_FSR_DEVICE", "FSR is not supported by this device.")
               : Text("STR_VIDEO_RT_DEVICE", "Ray tracing is not supported by this device.");
}
} // namespace

VideoPage::VideoPage()
{
    m_video.page = this;
}
int VideoPage::FindRow(Row row) const
{
    auto it = std::find(m_rows.begin(), m_rows.end(), row);
    return it == m_rows.end() ? -1 : int(it - m_rows.begin());
}
const char* VideoPage::TitleText() const
{
    return Text("STR_VIDEO_TITLE", "Video Settings");
}

void VideoPage::BuildRows()
{
    m_rows = {DisplayHeader, Monitor,     Mode,    Resolution, Refresh,       Vsync, Fps,
              Brightness,    Gamma,       Style,   HudWidth,   ImageHeader,   AA,    Scale,
              ViewDistance,  MissionView, Terrain, Objects,    GraphicsHeader};
    if (m_vulkan)
        m_rows.insert(m_rows.end(), {Overall, Shadows, Clouds, Water, AO, VolumetricLighting});
    else
        m_rows.insert(m_rows.end(), {ObjectShadows, VehicleShadows, Multitexturing});
    m_rows.insert(m_rows.end(), {Particles, Apply, OK, Cancel});
    m_aaModes.clear();
    if (GEngine)
        for (const char* name : aaNames)
        {
            if (m_vulkan
                    ? !GEngine->ImageQualityUnavailable({name, 100}).GetLength()
                    : ((std::string(name) == "off" || Samples(name)) && GEngine->SupportsMsaaSamples(Samples(name))))
                m_aaModes.emplace_back(name);
        }
    if (m_aaModes.empty())
        m_aaModes.emplace_back("off");
}

void VideoPage::Capture()
{
    m_graphics.Load(GraphicsPath());
    if (GEngine)
    {
        m_graphics.brightness = GEngine->GetBrightness();
        m_graphics.gamma = GEngine->GetGamma();
        const int vsync = GEngine->GetSwapInterval();
        m_graphics.vsync = vsync < 0 ? GraphicsConfig::VsyncAdaptive
                           : vsync   ? GraphicsConfig::VsyncOn
                                     : GraphicsConfig::VsyncOff;
        m_graphics.multitexturing = GEngine->IsMultitexturing();
        m_graphics.msaaSamples = GEngine->GetMsaaSamples();
        m_graphics.renderScale = GEngine->GetRenderScale();
        m_quality = m_oldQuality = GEngine->GetGraphicsQuality();
        m_image = m_oldImage =
            m_vulkan ? GEngine->GetImageQuality()
                     : ImageQuality{m_graphics.msaaSamples ? "msaa" + std::to_string(m_graphics.msaaSamples) : "off",
                                    int(std::lround(m_graphics.renderScale * 100))};
        m_rtReason = static_cast<const char*>(GEngine->GraphicsQualityRTUnavailable());
        m_fsrReason = static_cast<const char*>(GEngine->ImageQualityUnavailable({"fsr-quality", 100}));
    }
    m_graphics.fpsCap = gUserFpsCap;
    if (GScene)
    {
        for (int t = 1; t <= 5; ++t)
            if (std::abs(TerrainGridForTier(static_cast<GraphicsConfig::Tier>(t)) - GScene->GetPreferredTerrainGrid()) <
                .001f)
                m_graphics.terrainDetail = static_cast<GraphicsConfig::Tier>(t);
        for (int i = 0; i < 4; ++i)
            if (std::abs(GScene->GetObjectLODBias() - ObjectLodBiasForTier(static_cast<GraphicsConfig::Tier>(i + 1))) <
                .001f)
                m_graphics.objectLod = static_cast<GraphicsConfig::Tier>(i + 1);
        m_graphics.particlesQuality = GScene->GetCloudlets() ? GraphicsConfig::TierHigh : GraphicsConfig::TierOff;
        m_objectShadows = m_oldObjects = GScene->GetObjectShadows();
        m_vehicleShadows = m_oldVehicles = GScene->GetVehicleShadows();
    }
    m_view = m_oldView = GetSelectedPreferredViewDistance();
    m_missionView = m_oldMissionView = GetRespectMissionViewDistance();
    m_saved = m_liveGraphics = m_graphics;
    BuildRows();
}

void VideoPage::CaptureDisplay()
{
    CaptureEngineState();
    // Use the live window size, not a stale display.cfg, for exact rollback.
    if (GEngine && m_applied.windowMode != DisplayConfig::Borderless)
    {
        GEngine->GetVideoResolution(m_applied.resolutionWidth, m_applied.resolutionHeight);
        m_applied.refreshRate = m_applied.windowMode == DisplayConfig::Fullscreen ? GEngine->RefreshRate() : 0;
        m_pending = m_applied;
        RebuildLists();
    }
}
void VideoPage::ObserveWindowSize()
{
    // OS window moves/resizes are live, not failed display requests. Keep
    // explicit mode/monitor selections and any separately staged size intact.
    if (!GEngine || GEngine->GetCurrentWindowMode() != WindowMode::Windowed ||
        m_applied.windowMode != DisplayConfig::Windowed || m_pending.windowMode != m_applied.windowMode ||
        m_pending.monitor != m_applied.monitor)
        return;
    int width = 0, height = 0;
    GEngine->GetVideoResolution(width, height);
    const int monitor = GEngine->GetCurrentMonitor();
    const bool sizeStaged = m_pending.resolutionWidth != m_applied.resolutionWidth ||
                            m_pending.resolutionHeight != m_applied.resolutionHeight;
    if (width > 0 && height > 0 &&
        (monitor != m_applied.monitor ||
         (!sizeStaged && (width != m_applied.resolutionWidth || height != m_applied.resolutionHeight))))
    {
        m_applied.monitor = m_pending.monitor = monitor;
        if (!sizeStaged)
        {
            m_applied.resolutionWidth = m_pending.resolutionWidth = width;
            m_applied.resolutionHeight = m_pending.resolutionHeight = height;
        }
        RebuildLists();
    }
}
void VideoPage::Mount(OptionsShell& shell)
{
    m_vulkan = GEngine && GEngine->HasVulkanGraphics();
    m_display.SetPage(this);
    CaptureDisplay();
    Capture();
    ScrollListPage::Mount(shell);
}
void VideoPage::Unmount(OptionsShell& shell)
{
    ScrollListPage::Unmount(shell);
}
void VideoPage::OnReshown(OptionsShell& shell)
{
    RebuildLists(); // Also refresh Native/Default and monitor labels after a language switch.
    ScrollListPage::OnReshown(shell);
}

int VideoPage::DisplayRow(Row row) const
{
    switch (row)
    {
        case Monitor:
            return 0;
        case Mode:
            return 1;
        case Resolution:
            return 2;
        case Refresh:
            return 3;
        case Style:
            return 4;
        case HudWidth:
            return 5;
        default:
            return -1;
    }
}
std::vector<int> VideoPage::ScaleChoices() const
{
    std::vector<int> result;
    for (int scale : {100, 125, 150, 175, 200})
        if (!m_vulkan ||
            (scale != 175 && GEngine && !GEngine->ImageQualityUnavailable({m_image.aa, scale}).GetLength()))
            result.push_back(scale);
    return result;
}
int VideoPage::VideoProvider::RowCount() const
{
    return int(page->m_rows.size());
}
const char* VideoPage::VideoProvider::RowLabel(int i) const
{
    const Row row = page->m_rows.at(i);
    int d = page->DisplayRow(row);
    if (d >= 0)
        return page->m_display.RowLabel(d);
    static const char* keys[] = {"STR_VIDEO_DISPLAY",
                                 "",
                                 "",
                                 "",
                                 "",
                                 "STR_DISP_MAIN_OPT_GRAPHICS_VSYNC",
                                 "STR_DISP_MAIN_OPT_GRAPHICS_FPS_CAP",
                                 "STR_DISP_MAIN_OPT_GRAPHICS_BRIGHTNESS",
                                 "STR_DISP_MAIN_OPT_GRAPHICS_GAMMA",
                                 "",
                                 "",
                                 "STR_VIDEO_IMAGE",
                                 "STR_VIDEO_AA",
                                 "STR_VIDEO_SCALE",
                                 "STR_VIDEO_VIEW",
                                 "STR_VIDEO_MISSION",
                                 "STR_DISP_MAIN_OPT_GRAPHICS_TERRAIN_DETAIL",
                                 "STR_VIDEO_OBJECTS",
                                 "STR_VIDEO_GRAPHICS",
                                 "STR_VIDEO_OVERALL",
                                 "STR_VIDEO_SHADOWS",
                                 "STR_VIDEO_CLOUDS",
                                 "STR_VIDEO_WATER",
                                 "STR_VIDEO_AO",
                                 "STR_VIDEO_VOLUMETRIC_LIGHTING",
                                 "STR_VIDEO_OBJECT_SHADOWS",
                                 "STR_VIDEO_VEHICLE_SHADOWS",
                                 "STR_VIDEO_PARTICLES",
                                 "STR_VIDEO_DETAIL_TEXTURES",
                                 "STR_VIDEO_APPLY",
                                 "STR_VIDEO_OK",
                                 "STR_VIDEO_CANCEL"};
    static const char* labels[] = {"DISPLAY",
                                   "",
                                   "",
                                   "",
                                   "",
                                   "VSync",
                                   "FPS Cap",
                                   "Brightness",
                                   "Gamma",
                                   "",
                                   "",
                                   "IMAGE QUALITY",
                                   "AA / Upscaling",
                                   "Render Scale",
                                   "View Distance",
                                   "Mission View Limit",
                                   "Terrain Detail",
                                   "Object Detail",
                                   "GRAPHICS",
                                   "Overall Quality",
                                   "Shadow Quality",
                                   "Cloud Quality",
                                   "Water Quality",
                                   "Ambient Occlusion",
                                   "Volumetric Lighting",
                                   "Object Shadows",
                                   "Vehicle Shadows",
                                   "Particles / Smoke",
                                   "Detail Textures",
                                   "Apply",
                                   "OK",
                                   "Cancel"};
    return Text(keys[row], labels[row]);
}
const char* VideoPage::VideoProvider::RowDescription(int i) const
{
    if (page->m_statusKey)
        return Text(page->m_statusKey, page->m_statusFallback);
    Row row = page->m_rows.at(i);
    int d = page->DisplayRow(row);
    if (d >= 0)
        return page->m_display.RowDescription(d);
    switch (row)
    {
        case AA:
            if (!page->m_vulkan)
                return Text("STR_VIDEO_AA_RASTER_HELP", "Smooth jagged edges.");
            if (page->m_vulkan && !page->m_fsrReason.empty())
                return Unavailable(page->m_fsrReason, true);
            return Text("STR_VIDEO_AA_HELP", "Smooth jagged edges. FSR improves performance while keeping the interface sharp.");
        case Scale:
            if (page->m_vulkan && page->m_image.IsFSR())
                return Text("STR_VIDEO_FSR_SCALE_HELP", "FSR controls render resolution. Choose another AA mode to change render scale.");
            return Text("STR_VIDEO_SCALE_HELP", "Higher values improve sharpness but reduce performance. 100% uses the display resolution.");
        case Overall:
            return Text("STR_VIDEO_OVERALL_HELP", "Set shadows, clouds, water and ambient occlusion together. Other settings stay unchanged.");
        case Shadows:
            if (!page->m_rtReason.empty())
                return Unavailable(page->m_rtReason, false);
            return Text("STR_VIDEO_SHADOW_HELP", "Enhanced adds softer, more detailed shadows. Ray Traced adds realistic sunlight shadows.");
        case Clouds:
            return Text("STR_VIDEO_CLOUD_HELP",
                        "Enhanced adds three-dimensional clouds and moving cloud shadows.");
        case Water:
            if (!page->m_rtReason.empty())
                return Unavailable(page->m_rtReason, false);
            return Text("STR_VIDEO_WATER_HELP",
                        "Enhanced adds clear shallows, shore foam and rain ripples. Ray Traced adds detailed reflections.");
        case AO:
            return Text("STR_VIDEO_AO_HELP", "Add subtle shading where surfaces meet.");
        case VolumetricLighting:
            return Text("STR_VIDEO_VOLUMETRIC_LIGHTING_HELP", "Show sunlight through mist and fog. Independent of Overall Quality.");
        case Terrain:
            return Text(
                "STR_VIDEO_TERRAIN_HELP",
                "Smaller values add terrain detail. 3.125 m may cause problems with original missions.");
        case Objects:
            return Text("STR_VIDEO_OBJECT_HELP",
                        "Object detail at the same distance: 0.5x / 0.75x / 1x / 2x. Higher values show more detail.");
        case ViewDistance:
            return Text("STR_VIDEO_VIEW_HELP",
                        "How far you can see. Mission View Limit lets missions use a shorter distance.");
        case MissionView:
            return Text("STR_VIDEO_MISSION_HELP",
                        "Allow missions to limit visibility. Your preferred view distance is remembered.");
        case Particles:
            return Text("STR_VIDEO_PARTICLES_HELP",
                        "Show smoke, dust and muzzle flashes.");
        case Apply:
            return Text("STR_VIDEO_APPLY_HELP", "Apply and save changes. Confirm display changes with Keep.");
        case OK:
            return Text("STR_VIDEO_OK_HELP", "Apply and save changes, then return.");
        case Cancel:
            return Text("STR_VIDEO_CANCEL_HELP", "Discard unapplied changes and return.");
        case Vsync:
            return Text("STR_VIDEO_VSYNC_HELP", "Reduce screen tearing. If Adaptive is unavailable, On is used instead.");
        case Fps:
            return Text("STR_VIDEO_FPS_HELP",
                        "Limit the frame rate to reduce power use and heat.");
        case ObjectShadows:
        case VehicleShadows:
            return Text("STR_VIDEO_LEGACY_SHADOW_HELP", "Show shadows cast by these objects.");
        case Multitexturing:
            return Text("STR_VIDEO_TEXTURE_HELP", "Add fine surface detail to textures.");
        default:
            return Text("STR_VIDEO_STAGE_HELP",
                        "Choose settings, then Apply or OK to save.");
    }
}

OptionsScrollList::RowDef VideoPage::VideoProvider::RowFor(int i) const
{
    Row row = page->m_rows.at(i);
    int d = page->DisplayRow(row);
    if (d >= 0)
        return page->m_display.RowFor(d);
    if (RowKind(i) == OptionsScrollList::KindSlider)
        return {502, nullptr, -1};
    if (row == DisplayHeader || row == ImageHeader || row == GraphicsHeader || row >= Apply)
        return {-1, nullptr, 0};
    auto& choices = page->m_choices;
    choices.clear();
    auto add = [&](const char* key, const char* fallback) { choices.emplace_back(Text(key, fallback)); };
    switch (row)
    {
        case AA:
            for (auto& mode : page->m_aaModes)
                for (size_t n = 0; n < std::size(aaNames); ++n)
                    if (mode == aaNames[n])
                    {
                        const std::string key = "STR_VIDEO_AA_" + mode;
                        add(key.c_str(), aaLabels[n]);
                    }
            break;
        case Scale:
            for (int scale : page->ScaleChoices())
                choices.push_back(std::to_string(scale) + "%");
            break;
        case Overall:
            add("STR_VIDEO_STANDARD", "Standard");
            add("STR_VIDEO_ENHANCED", "Enhanced");
            add("STR_VIDEO_CUSTOM", "Custom");
            break;
        case Shadows:
        case Clouds:
        case Water:
            add("STR_VIDEO_STANDARD", "Standard");
            add("STR_VIDEO_ENHANCED", "Enhanced");
            if ((row == Shadows || row == Water) && page->m_rtReason.empty())
                add("STR_VIDEO_RT", "Ray Traced");
            if ((row == Shadows && page->m_quality.shadows < 0) || (row == Clouds && page->m_quality.clouds < 0) ||
                (row == Water && page->m_quality.water < 0))
                add("STR_VIDEO_CUSTOM", "Custom");
            break;
        case Vsync:
            add("STR_VIDEO_OFF", "Off");
            add("STR_VIDEO_ON", "On");
            add("STR_VIDEO_ADAPTIVE", "Adaptive");
            break;
        case Fps:
            add("STR_VIDEO_UNLIMITED", "Unlimited");
            for (int cap : caps)
                if (cap)
                    choices.push_back(std::to_string(cap));
            break;
        case Terrain:
            choices = {"50 m", "25 m", "12.5 m", "6.25 m", "3.125 m"};
            break;
        case Objects:
            choices = {"0.5x", "0.75x", "1x", "2x"};
            break;
        default:
            add("STR_VIDEO_OFF", "Off");
            add("STR_VIDEO_ON", "On");
            break;
    }
    auto& ptrs = page->m_choicePtrs;
    ptrs.clear();
    for (auto& text : choices)
        ptrs.push_back(text.c_str());
    return {502, ptrs.data(), int(ptrs.size())};
}
OptionsScrollList::Kind VideoPage::VideoProvider::RowKind(int i) const
{
    switch (page->m_rows.at(i))
    {
        case DisplayHeader:
        case ImageHeader:
        case GraphicsHeader:
            return OptionsScrollList::KindHeader;
        case Apply:
        case OK:
        case Cancel:
            return OptionsScrollList::KindAction;
        case Brightness:
        case Gamma:
        case ViewDistance:
            return OptionsScrollList::KindSlider;
        case AO:
        case VolumetricLighting:
        case ObjectShadows:
        case VehicleShadows:
        case Particles:
        case Multitexturing:
        case MissionView:
            return OptionsScrollList::KindBoolean;
        default:
            return OptionsScrollList::KindStepper;
    }
}
int VideoPage::VideoProvider::RowValue(int i) const
{
    Row row = page->m_rows.at(i);
    int d = page->DisplayRow(row);
    if (d >= 0)
        return page->m_display.RowValue(d);
    const auto& c = page->m_graphics;
    const auto& q = page->m_quality;
    switch (row)
    {
        case Vsync:
            return c.vsync;
        case Fps:
            return Index(caps, c.fpsCap);
        case Brightness:
            return Slider(c.brightness, .4f, 1.8f);
        case Gamma:
            return Slider(c.gamma, .5f, 2.3f);
        case ViewDistance:
            return GamePage::ViewDistanceToSlider(page->m_view);
        case MissionView:
            return page->m_missionView;
        case Terrain:
            return int(c.terrainDetail) - 1;
        case Objects:
            return int(c.objectLod) - 1;
        case AA:
            for (int n = 0; n < int(page->m_aaModes.size()); ++n)
                if (page->m_image.aa == page->m_aaModes[n])
                    return n;
            return 0;
        case Scale:
        {
            auto values = page->ScaleChoices();
            for (int n = 0; n < int(values.size()); ++n)
                if (page->m_image.scale == values[n])
                    return n;
            return 0;
        }
        case Overall:
            return q.Preset();
        case Shadows:
            return q.shadows < 0 ? (page->m_rtReason.empty() ? 3 : 2) : q.shadows;
        case Clouds:
            return q.clouds < 0 ? 2 : q.clouds;
        case Water:
            return q.water < 0 ? (page->m_rtReason.empty() ? 3 : 2) : q.water;
        case AO:
            return q.ao;
        case VolumetricLighting:
            return q.volumetricLighting;
        case ObjectShadows:
            return page->m_objectShadows;
        case VehicleShadows:
            return page->m_vehicleShadows;
        case Particles:
            return c.particlesQuality != GraphicsConfig::TierOff;
        case Multitexturing:
            return c.multitexturing;
        default:
            return 0;
    }
}
void VideoPage::VideoProvider::SetRowValue(int i, int v)
{
    if (IsDisabled(i))
        return;
    Row row = page->m_rows.at(i);
    int d = page->DisplayRow(row);
    if (d >= 0)
    {
        page->m_display.SetRowValue(d, v);
        page->m_statusKey = nullptr;
        return;
    }
    auto& c = page->m_graphics;
    auto& q = page->m_quality;
    switch (row)
    {
        case Vsync:
            if (v >= 0 && v <= 2)
                c.vsync = static_cast<GraphicsConfig::VsyncMode>(v);
            break;
        case Fps:
            if (v >= 0 && v < int(std::size(caps)))
                c.fpsCap = caps[v];
            break;
        case Brightness:
            c.brightness = FromSlider(v, .4f, 1.8f);
            break;
        case Gamma:
            c.gamma = FromSlider(v, .5f, 2.3f);
            break;
        case ViewDistance:
            page->m_view = GamePage::SliderToViewDistance(v);
            break;
        case MissionView:
            page->m_missionView = v != 0;
            break;
        case Terrain:
            if (v >= 0 && v < 5 &&
                (!GScene ||
                 TerrainGridForTier(static_cast<GraphicsConfig::Tier>(v + 1)) >= GScene->GetMinimalTerrainGrid()))
                c.terrainDetail = static_cast<GraphicsConfig::Tier>(v + 1);
            break;
        case Objects:
            if (v >= 0 && v < 4)
            {
                c.objectLod = static_cast<GraphicsConfig::Tier>(v + 1);
                c.objectLodConfigured = true;
            }
            break;
        case AA:
            if (v >= 0 && v < int(page->m_aaModes.size()))
            {
                page->m_image.aa = page->m_aaModes[v];
                if (page->m_image.IsFSR())
                    page->m_image.scale = 100;
                page->m_imageTouched = true;
            }
            break;
        case Scale:
        {
            auto values = page->ScaleChoices();
            if (v >= 0 && v < int(values.size()))
            {
                page->m_image.scale = values[v];
                page->m_imageTouched = true;
            }
            break;
        }
        case Overall:
        {
            const bool volume = q.volumetricLighting;
            if (v == 2 && q.Preset() != 2)
                v = 1 - q.Preset();
            if (v == 0)
            {
                q = {};
                page->m_qualityTouched = true;
                page->m_shadowsTouched = page->m_waterTouched = true;
            }
            else if (v == 1)
            {
                q = GraphicsQuality::Enhanced();
                page->m_qualityTouched = true;
                page->m_shadowsTouched = page->m_waterTouched = true;
            }
            q.volumetricLighting = volume;
            break;
        }
        case Shadows:
            if (v >= 0 && v <= (page->m_rtReason.empty() ? 2 : 1))
            {
                q.shadows = v;
                page->m_qualityTouched = true;
                page->m_shadowsTouched = true;
            }
            break;
        case Clouds:
            if (v == 0 || v == 1)
            {
                q.clouds = v;
                page->m_qualityTouched = true;
            }
            break;
        case Water:
            if (v >= 0 && v <= (page->m_rtReason.empty() ? 2 : 1))
            {
                q.water = v;
                page->m_qualityTouched = true;
                page->m_waterTouched = true;
            }
            break;
        case AO:
            q.ao = v != 0;
            page->m_qualityTouched = true;
            break;
        case VolumetricLighting:
            q.volumetricLighting = v != 0;
            break;
        case ObjectShadows:
            page->m_objectShadows = v != 0;
            break;
        case VehicleShadows:
            page->m_vehicleShadows = v != 0;
            break;
        case Particles:
            c.particlesQuality = v ? GraphicsConfig::TierHigh : GraphicsConfig::TierOff;
            break;
        case Multitexturing:
            c.multitexturing = v != 0;
            break;
        default:
            break;
    }
    page->m_statusKey = nullptr;
}
const char* VideoPage::VideoProvider::SliderValueText(int i) const
{
    char value[32];
    switch (page->m_rows.at(i))
    {
        case Brightness:
            std::snprintf(value, sizeof(value), "%.1f", page->m_graphics.brightness);
            break;
        case Gamma:
            std::snprintf(value, sizeof(value), "%.1f", page->m_graphics.gamma);
            break;
        case ViewDistance:
            std::snprintf(value, sizeof(value), "%.0f m", page->m_view);
            break;
        default:
            return nullptr;
    }
    page->m_valueText = value;
    return page->m_valueText.c_str();
}
bool VideoPage::VideoProvider::IsDisabled(int i) const
{
    Row row = page->m_rows.at(i);
    if (page->m_applying || !GEngine)
        return true;
    int d = page->DisplayRow(row);
    if (d >= 0)
        return page->m_display.IsDisabled(d);
    return (row == Scale && page->m_image.IsFSR()) || (row == Apply && !page->Dirty());
}
bool VideoPage::VideoProvider::IsPending(int i) const
{
    const Row row = page->m_rows.at(i);
    int d = page->DisplayRow(row);
    if (d >= 0)
        return page->m_display.IsPending(d);
    const auto& a = page->m_graphics;
    const auto& b = page->m_saved;
    switch (row)
    {
        case Vsync:
            return a.vsync != b.vsync;
        case Fps:
            return a.fpsCap != b.fpsCap;
        case Brightness:
            return a.brightness != b.brightness;
        case Gamma:
            return a.gamma != b.gamma;
        case AA:
            return page->m_image.aa != page->m_oldImage.aa;
        case Scale:
            return page->m_image.scale != page->m_oldImage.scale;
        case ViewDistance:
            return page->m_view != page->m_oldView;
        case MissionView:
            return page->m_missionView != page->m_oldMissionView;
        case Terrain:
            return a.terrainDetail != b.terrainDetail;
        case Objects:
            return a.objectLod != b.objectLod || a.objectLodConfigured != b.objectLodConfigured;
        case Overall:
            return page->m_quality.shadows != page->m_oldQuality.shadows ||
                   page->m_quality.clouds != page->m_oldQuality.clouds ||
                   page->m_quality.water != page->m_oldQuality.water || page->m_quality.ao != page->m_oldQuality.ao;
        case Shadows:
            return page->m_quality.shadows != page->m_oldQuality.shadows;
        case Clouds:
            return page->m_quality.clouds != page->m_oldQuality.clouds;
        case Water:
            return page->m_quality.water != page->m_oldQuality.water;
        case AO:
            return page->m_quality.ao != page->m_oldQuality.ao;
        case VolumetricLighting:
            return page->m_quality.volumetricLighting != page->m_oldQuality.volumetricLighting;
        case ObjectShadows:
            return page->m_objectShadows != page->m_oldObjects;
        case VehicleShadows:
            return page->m_vehicleShadows != page->m_oldVehicles;
        case Particles:
            return a.particlesQuality != b.particlesQuality;
        case Multitexturing:
            return a.multitexturing != b.multitexturing;
        default:
            return false;
    }
}
void VideoPage::VideoProvider::OnRowAction(int i, Display& host)
{
    if (IsDisabled(i))
        return;
    auto& shell = static_cast<OptionsShell&>(host);
    Row row = page->m_rows.at(i);
    if (row == Cancel)
        shell.OnButtonClicked(IDC_CANCEL);
    else if (row == Apply || row == OK)
        page->Commit(shell, row == OK);
}

bool VideoPage::Dirty() const
{
    const auto& a = m_graphics;
    const auto& b = m_saved;
    return m_retrySave || m_qualityTouched || m_imageTouched || HasPendingChanges(m_pending, m_applied) ||
           m_image != m_oldImage || m_quality != m_oldQuality || m_view != m_oldView ||
           m_missionView != m_oldMissionView || m_objectShadows != m_oldObjects || m_vehicleShadows != m_oldVehicles ||
           a.vsync != b.vsync || a.fpsCap != b.fpsCap || a.brightness != b.brightness || a.gamma != b.gamma ||
           a.terrainDetail != b.terrainDetail || a.objectLod != b.objectLod ||
           a.objectLodConfigured != b.objectLodConfigured || a.particlesQuality != b.particlesQuality ||
           a.multitexturing != b.multitexturing;
}
void VideoPage::Commit(OptionsShell& shell, bool close)
{
    if (!GEngine || m_applying)
        return;
    ObserveWindowSize();
    if (!Dirty())
    {
        if (close)
            shell.PopPage();
        return;
    }
    if (m_vulkan)
    {
        if (GEngine->ImageQualityUnavailable(m_image).GetLength())
        {
            Status("STR_VIDEO_IMAGE_ERROR", "These image settings are unavailable. Choose another option.");
            return;
        }
        if (m_quality != m_oldQuality && (!m_quality.Valid() ||
            ((m_quality.shadows == 2 || m_quality.water == 2) && !m_rtReason.empty())))
        {
            Status("STR_VIDEO_GROUP_ERROR", "Choose available graphics options before applying.");
            return;
        }
    }
    m_close = close;
    if (HasPendingChanges(m_pending, m_applied))
    {
        ApplyToEngine(m_pending);
        const float timeout = AppConfig::Instance().ConfirmRevertTimeoutSeconds();
        shell.PushPage(std::make_unique<ConfirmRevertPage>([this]() { ApplySelection(); },
                                                           [this]()
                                                           {
                                                               ApplyToEngine(m_applied);
                                                               m_pending = m_applied;
                                                               m_close = false;
                                                               RebuildLists();
                                                           },
                                                           timeout > 0 ? timeout : 15.f));
    }
    else
        ApplySelection();
}
void VideoPage::ApplySelection()
{
    if (m_vulkan)
    {
        if (GEngine->ApplyImageQuality(m_image).GetLength())
        {
            Status("STR_VIDEO_RENDER_ERROR", "Could not apply these settings. Previous values restored.");
            Restore();
            return;
        }
        if ((m_quality != m_oldQuality || m_retrySave) && m_quality != GEngine->GetGraphicsQuality() &&
            !GEngine->ApplyGraphicsQuality(m_quality))
        {
            Status("STR_VIDEO_RENDER_ERROR", "Could not apply these settings. Previous values restored.");
            Restore();
            return;
        }
    }
    else
    {
        m_graphics.msaaSamples = Samples(m_image.aa);
        m_graphics.renderScale = m_image.scale / 100.f;
        // One meaningful On/Off control per legacy gate, not fake shadow tiers.
        if (m_objectShadows != m_oldObjects || m_vehicleShadows != m_oldVehicles || m_retrySave)
            m_graphics.shadowQuality =
                (m_objectShadows || m_vehicleShadows) ? GraphicsConfig::TierHigh : GraphicsConfig::TierOff;
    }
    auto common = m_graphics;
    common.shadowQuality = m_saved.shadowQuality; // Individual GL33 gates below own this UI transaction.
    ApplyGraphicsConfigToEngine(common, &m_liveGraphics);
    m_liveGraphics = common;
    if (!m_vulkan && GScene)
    {
        if ((m_objectShadows != m_oldObjects || m_retrySave) && m_objectShadows != GScene->GetObjectShadows())
            GScene->SetObjectShadows(m_objectShadows);
        if ((m_vehicleShadows != m_oldVehicles || m_retrySave) && m_vehicleShadows != GScene->GetVehicleShadows())
            GScene->SetVehicleShadows(m_vehicleShadows);
    }
    if ((m_view != m_oldView || m_missionView != m_oldMissionView || m_retrySave) &&
        (m_view != GetSelectedPreferredViewDistance() || m_missionView != GetRespectMissionViewDistance()))
    {
        SetSelectedPreferredViewDistance(m_view);
        SetRespectMissionViewDistance(m_missionView);
        ApplyCurrentMissionViewDistance();
    }
    m_applying = true;
    m_waitFrames = 0;
    Status("STR_VIDEO_APPLYING", "Applying video settings...");
}
void VideoPage::Restore()
{
    ApplyToEngine(m_applied);
    auto common = m_saved;
    common.shadowQuality = m_liveGraphics.shadowQuality;
    ApplyGraphicsConfigToEngine(common, &m_liveGraphics);
    m_liveGraphics = common;
    if (m_vulkan)
    {
        GEngine->ApplyImageQuality(m_oldImage);
        if (m_oldQuality.Valid() && GEngine->GetGraphicsQuality() != m_oldQuality)
            GEngine->ApplyGraphicsQuality(m_oldQuality);
    }
    if (GScene)
    {
        if (GScene->GetObjectShadows() != m_oldObjects)
            GScene->SetObjectShadows(m_oldObjects);
        if (GScene->GetVehicleShadows() != m_oldVehicles)
            GScene->SetVehicleShadows(m_oldVehicles);
    }
    if (GetSelectedPreferredViewDistance() != m_oldView || GetRespectMissionViewDistance() != m_oldMissionView)
    {
        SetSelectedPreferredViewDistance(m_oldView);
        SetRespectMissionViewDistance(m_oldMissionView);
        ApplyCurrentMissionViewDistance();
    }
    m_applying = false;
    m_close = false;
    m_qualityTouched = m_imageTouched = false;
    m_shadowsTouched = m_waterTouched = false;
    m_pending = m_applied;
    m_graphics = m_saved;
    m_image = m_oldImage;
    m_quality = m_oldQuality;
    m_view = m_oldView;
    m_missionView = m_oldMissionView;
    m_objectShadows = m_oldObjects;
    m_vehicleShadows = m_oldVehicles;
}
bool VideoPage::Save()
{
    // The existing writers span four existing files. Keep only in-memory byte
    // snapshots for this Apply, so an I/O failure cannot leave a partial bundle
    // that survives Cancel. No backup file or second settings store is created.
    const std::string paths[] = {GraphicsPath(), DisplayPath(), GamePaths::Instance().UserDir() + "game.cfg",
                                 static_cast<const char*>(GetUserParams())};
    std::optional<std::string> before[4];
    for (int i = 0; i < 4; ++i)
    {
        const auto path = FilesystemPathFromUtf8(paths[i]);
        std::error_code error;
        if (!std::filesystem::exists(path, error))
        {
            // A first-run profile may not have a parent directory yet. The
            // existing profile writer creates it; absence is not an I/O error.
            if (error && error != std::errc::no_such_file_or_directory)
                return false;
            continue;
        }
        if (!std::filesystem::is_regular_file(path, error) || error)
            return false;
        std::ifstream in(path, std::ios::binary);
        if (!in)
            return false;
        before[i].emplace(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        if (in.bad())
            return false;
    }
    auto failedWrite = [&]()
    {
        for (int i = 0; i < 4; ++i)
        {
            const auto path = FilesystemPathFromUtf8(paths[i]);
            if (before[i])
            {
                std::ofstream out(path, std::ios::binary);
                out << *before[i];
            }
            else
            {
                std::error_code error;
                if (std::filesystem::is_regular_file(path, error))
                    std::filesystem::remove(path, error);
            }
        }
        return false;
    };
    GraphicsConfig cfg;
    cfg.Load(GraphicsPath()); // Preserve the other renderer's independently saved preferences.
    cfg.vsync = m_graphics.vsync;
    cfg.fpsCap = m_graphics.fpsCap;
    cfg.brightness = m_graphics.brightness;
    cfg.gamma = m_graphics.gamma;
    cfg.terrainDetail = m_graphics.terrainDetail;
    cfg.objectLod = m_graphics.objectLod;
    cfg.objectLodConfigured = m_graphics.objectLodConfigured;
    cfg.particlesQuality = m_graphics.particlesQuality;
    if (m_vulkan)
    {
        // A missing startup capability may have produced a temporary fallback.
        // Editing gamma/FPS must not erase the saved RT/FSR preference.
        if (m_imageTouched || m_image != m_oldImage)
            cfg.vulkanImage = m_image;
        if ((m_qualityTouched || m_quality != m_oldQuality) && m_quality.Valid())
        {
            // Each RT preference can fall back independently. Choosing water,
            // AO or lighting must not erase an unavailable saved shadow choice
            // (or vice versa); an explicit preset accepts both raster choices.
            if (m_shadowsTouched || m_quality.shadows != m_oldQuality.shadows)
                cfg.vulkan.shadows = m_quality.shadows;
            if (m_waterTouched || m_quality.water != m_oldQuality.water)
                cfg.vulkan.water = m_quality.water;
            if (m_qualityTouched || m_quality.clouds != m_oldQuality.clouds)
                cfg.vulkan.clouds = m_quality.clouds;
            if (m_qualityTouched || m_quality.ao != m_oldQuality.ao)
                cfg.vulkan.ao = m_quality.ao;
            if (m_quality.volumetricLighting != m_oldQuality.volumetricLighting)
                cfg.vulkan.volumetricLighting = m_quality.volumetricLighting;
        }
    }
    else
    {
        cfg.msaaSamples = m_graphics.msaaSamples;
        cfg.renderScale = m_graphics.renderScale;
        cfg.shadowQuality = m_graphics.shadowQuality;
        cfg.multitexturing = m_graphics.multitexturing;
    }
    cfg.qualityPreset = cfg.DerivePresetFromTiers(); // Legacy compatibility only; not a player effect preset.
    if (!cfg.Save(GraphicsPath()) || !m_pending.Save(DisplayPath()))
        return failedWrite();
    if (!SaveGameSettings())
        return failedWrite();
    // Vulkan's temporary caster gates remain excluded by Scene's writer.
    if (GScene && !GScene->SaveConfig())
        return failedWrite();
    GEngine->SaveConfig();
    return true;
}
bool VideoPage::Finish(OptionsShell& shell)
{
    int modeWidth = 0, modeHeight = 0;
    GEngine->GetVideoResolution(modeWidth, modeHeight);
    if (m_vulkan && (GEngine->ImageQualityUnavailable(m_image).GetLength() || GEngine->GetImageQuality() != m_image ||
                     ((m_quality != m_oldQuality || m_retrySave) &&
                      (GEngine->GraphicsQualityError().GetLength() || GEngine->GetGraphicsQuality() != m_quality))))
    {
        Status("STR_VIDEO_RENDER_ERROR", "Could not apply these settings. Previous values restored.");
        Restore();
        return false;
    }
    if (!m_vulkan && (GEngine->GetMsaaSamples() != Samples(m_image.aa) ||
                      std::abs(GEngine->GetRenderScale() * 100 - m_image.scale) > .1f))
    {
        Status("STR_VIDEO_RENDER_ERROR",
                        "Could not apply these settings. Previous values restored.");
        Restore();
        return false;
    }
    if (GEngine->GetCurrentMonitor() != m_pending.monitor ||
        (m_pending.windowMode == DisplayConfig::Fullscreen && m_pending.refreshRate > 0 &&
         GEngine->RefreshRate() != m_pending.refreshRate) ||
        GEngine->GetCurrentWindowMode() != static_cast<WindowMode>(m_pending.windowMode) ||
        (m_pending.windowMode != DisplayConfig::Borderless && m_pending.resolutionWidth > 0 &&
         (modeWidth != m_pending.resolutionWidth || modeHeight != m_pending.resolutionHeight)))
    {
        Status("STR_VIDEO_DISPLAY_ERROR", "Could not change the display. Previous settings restored.");
        Restore();
        return false;
    }
    // SDL may reject adaptive VSync; persist/report the effective interval.
    const int interval = GEngine->GetSwapInterval();
    m_graphics.vsync = interval < 0 ? GraphicsConfig::VsyncAdaptive
                       : interval   ? GraphicsConfig::VsyncOn
                                    : GraphicsConfig::VsyncOff;
    m_liveGraphics.vsync = m_graphics.vsync;
    m_applying = false;
    if (!Save())
    {
        m_retrySave = true;
        m_close = false;
        Status("STR_VIDEO_SAVE_ERROR", "Settings are active, but could not be saved. Try Apply again.");
        return false;
    }
    m_retrySave = false;
    m_qualityTouched = m_imageTouched = false;
    m_shadowsTouched = m_waterTouched = false;
    m_applied = m_pending;
    m_saved = m_graphics;
    m_oldImage = m_image;
    m_oldQuality = m_quality;
    m_oldView = m_view;
    m_oldMissionView = m_missionView;
    m_oldObjects = m_objectShadows;
    m_oldVehicles = m_vehicleShadows;
    if (m_vulkan && GScene)
    {
        m_objectShadows = m_oldObjects = GScene->GetObjectShadows();
        m_vehicleShadows = m_oldVehicles = GScene->GetVehicleShadows();
    }
    Status("STR_VIDEO_SAVED", "Video settings applied and saved.");
    if (m_close)
    {
        shell.PopPage();
        return true;
    }
    return false;
}
void VideoPage::OnSimulate(OptionsShell& shell)
{
    if (!m_applying)
        ObserveWindowSize();
    if (m_applying && m_waitFrames++ >= 2 && GEngine && !GEngine->ImageQualityPending() &&
        !GEngine->GraphicsQualityPending())
        if (Finish(shell))
            return; // Finish may pop and destroy this page.
    if (!m_applying && !Dirty() && GEngine)
    {
        int modeWidth = 0, modeHeight = 0;
        GEngine->GetVideoResolution(modeWidth, modeHeight);
        // Observe developer commands and ordinary window resizing while clean.
        // Never replace a player's staged edits with an external snapshot.
        if (GEngine->GetCurrentWindowMode() != static_cast<WindowMode>(m_applied.windowMode) ||
            GEngine->GetCurrentMonitor() != m_applied.monitor ||
            (m_applied.windowMode != DisplayConfig::Borderless && m_applied.resolutionWidth > 0 &&
             (modeWidth != m_applied.resolutionWidth || modeHeight != m_applied.resolutionHeight)))
            CaptureDisplay();
        if ((m_vulkan && (m_oldImage != GEngine->GetImageQuality() || m_oldQuality != GEngine->GetGraphicsQuality())) ||
            m_saved.gamma != GEngine->GetGamma() || m_saved.brightness != GEngine->GetBrightness() ||
            m_saved.fpsCap != gUserFpsCap)
            Capture();
    }
    if (List())
        List()->RenderPage();
    ScrollListPage::OnSimulate(shell);
}
bool VideoPage::OnButtonClicked(OptionsShell& shell, int idc)
{
    if (idc == IDC_CANCEL && m_applying)
        return true;
    if (idc == IDC_CANCEL && m_retrySave)
        Restore();
    if (idc == IDC_OK)
    {
        Commit(shell, true);
        return true;
    }
    return ScrollListPage::OnButtonClicked(shell, idc);
}
bool VideoPage::OnKeyDown(OptionsShell& shell, unsigned key)
{
    if (key == SDLK_ESCAPE && m_applying)
        return true;
    if (key == SDLK_ESCAPE && m_retrySave)
        Restore();
    return ScrollListPage::OnKeyDown(shell, key);
}
} // namespace Poseidon
