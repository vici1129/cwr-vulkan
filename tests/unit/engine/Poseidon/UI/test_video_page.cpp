#include <Poseidon/UI/Options/VideoPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>
#include <Poseidon/UI/OptionsUICommon.hpp>
#include <Poseidon/UI/Settings/GameSettingsConfig.hpp>
#include <Poseidon/Graphics/Dummy/EngineDummy.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/Game/GameLoop.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/Foundation/Common/GamePaths.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <SDL3/SDL_keycode.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <random>

using namespace Poseidon;
namespace Poseidon
{
RString GetUserParams();
}
namespace
{
struct FileSnapshot
{
    std::string path;
    std::optional<std::string> contents;
    explicit FileSnapshot(const char* name) : path(GamePaths::Instance().UserDir() + name)
    {
        std::ifstream in(path, std::ios::binary);
        if (in)
            contents.emplace(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    ~FileSnapshot()
    {
        if (contents)
        {
            std::ofstream out(path, std::ios::binary);
            out << *contents;
        }
        else
            std::filesystem::remove(path);
    }
};
struct VideoEngine : EngineDummy
{
    bool vulkan = true, rt = false, fsr = false, reject = false, pending = false, failDisplay = false;
    int qualityCalls = 0, imageCalls = 0, displayCalls = 0, vsync = 1, width = 1280, height = 720;
    int samples = 0;
    int refresh = 0, refreshWidth = 0;
    int logicalWidth = 0, logicalHeight = 0;
    float gamma = 1.2f, scale = 1.f;
    WindowMode mode = WindowMode::Windowed;
    GraphicsQuality quality;
    ImageQuality image;
    bool HasVulkanGraphics() const override { return vulkan; }
    GraphicsQuality GetGraphicsQuality() const override { return quality; }
    ImageQuality GetImageQuality() const override { return image; }
    RString GraphicsQualityRTUnavailable() const override { return rt ? "" : "Restart with CWR_VK_RTAO=1"; }
    RString GraphicsQualityError() const override { return reject ? "Request rejected" : ""; }
    RString ImageQualityUnavailable(const ImageQuality& q) const override
    {
        if (q.aa == "msaa8")
            return "Unsupported sample count";
        if (q.IsFSR() && !fsr)
            return "FSR needs CWR_VK_FSR=1 at startup";
        if (q.IsFSR() && q.scale != 100)
            return "FSR owns scale";
        return "";
    }
    bool SupportsMsaaSamples(int n) const override { return n == 0 || n == 2 || n == 4; }
    RString ApplyImageQuality(const ImageQuality& q) override
    {
        ++imageCalls;
        auto reason = ImageQualityUnavailable(q);
        if (reason.GetLength())
            return reason;
        image = q;
        return "";
    }
    bool ApplyGraphicsQuality(const GraphicsQuality& q) override
    {
        ++qualityCalls;
        if (reject)
            return false;
        quality = q;
        return true;
    }
    bool GraphicsQualityPending() const override { return pending; }
    float GetGamma() const override { return gamma; }
    void SetGamma(float v) override { gamma = v; }
    int GetSwapInterval() const override { return vsync; }
    bool SetSwapInterval(int v) override
    {
        vsync = v;
        return true;
    }
    void SetMsaaSamples(int n) override { samples = n; }
    int GetMsaaSamples() const override { return samples; }
    void SetRenderScale(float v) override { scale = v; }
    float GetRenderScale() const override { return scale; }
    int Width() const override { return width; }
    int Height() const override { return height; }
    WindowMode GetCurrentWindowMode() const override { return mode; }
    void GetVideoResolution(int& w, int& h) const override
    {
        w = logicalWidth > 0 && mode == WindowMode::Windowed ? logicalWidth : width;
        h = logicalHeight > 0 && mode == WindowMode::Windowed ? logicalHeight : height;
    }
    bool SetWindowMode(WindowMode v) override
    {
        ++displayCalls;
        if (!failDisplay)
            mode = v;
        return !failDisplay;
    }
    bool SwitchRes(int w, int h, int) override
    {
        ++displayCalls;
        if (!failDisplay)
        {
            width = w;
            height = h;
        }
        return !failDisplay;
    }
    void ListResolutions(FindArray<ResolutionInfo>& out) override
    {
        out.Clear();
        out.Add(ResolutionInfo{800, 600, 32});
        out.Add(ResolutionInfo{1280, 720, 32});
    }
    void ListRefreshRates(FindArray<int>& out) override
    {
        out.Clear();
        out.Add(60);
        out.Add(120);
    }
    int RefreshRate() const override { return refresh; }
    bool SwitchRefreshRate(int hz) override
    {
        if (mode != WindowMode::Fullscreen)
            return false;
        refresh = hz;
        refreshWidth = width;
        return true;
    }
};
class Page : public VideoPage
{
  public:
    auto& Rows() { return ProviderRef(); }
    int At(Row row) const { return FindRow(row); }
    void Set(Row row, int value) { Rows().SetRowValue(At(row), value); }
    int Value(Row row) { return Rows().RowValue(At(row)); }
    void Action(Row row, OptionsShell& shell) { Rows().OnRowAction(At(row), shell); }
    void Tick(OptionsShell& shell)
    {
        for (int n = 0; n < 4; ++n)
            OnSimulate(shell);
    }
    int Choice(Row row, const std::string& text)
    {
        auto def = Rows().RowFor(At(row));
        for (int i = 0; i < def.count; ++i)
            if (text == def.options[i])
                return i;
        return -1;
    }
};
struct Fixture
{
    Engine* previous = GEngine;
    Scene* previousScene = GScene;
    RString previousPlayer = Glob.header.playerName;
    int previousCap = gUserFpsCap;
    float previousView = GetSelectedPreferredViewDistance();
    bool previousMission = GetRespectMissionViewDistance();
    FileSnapshot graphics{"graphics.cfg"}, display{"display.cfg"}, game{"game.cfg"};
    VideoEngine engine;
    Page* page = nullptr;
    std::unique_ptr<OptionsShell> shell;
    Fixture()
    {
        GEngine = &engine;
        GScene = nullptr;
        Glob.header.playerName = ("video-page-" + std::to_string(std::random_device{}())).c_str();
        ParamFile profile;
        profile.Add("objectShadows", 0);
        profile.Add("vehicleShadows", 0);
        REQUIRE(profile.Save(GetUserParams()) == LSOK);
        GraphicsConfig cfg;
        cfg.shadowQuality = GraphicsConfig::TierOff;
        cfg.msaaSamples = 4; // Other renderer's saved choice must survive Vulkan edits.
        cfg.renderScale = 1.75f;
        cfg.Save(graphics.path);
        DisplayConfig d;
        d.windowMode = DisplayConfig::Windowed;
        d.resolutionWidth = 1280;
        d.resolutionHeight = 720;
        d.Save(display.path);
    }
    void Open()
    {
        auto p = std::make_unique<Page>();
        page = p.get();
        shell = std::make_unique<OptionsShell>(nullptr, true, false, std::move(p));
    }
    GraphicsConfig Saved()
    {
        GraphicsConfig cfg;
        REQUIRE(cfg.Load(graphics.path));
        return cfg;
    }
    ~Fixture()
    {
        shell.reset();
        GEngine = previous;
        GScene = previousScene;
        Glob.header.playerName = previousPlayer;
        gUserFpsCap = previousCap;
        SetSelectedPreferredViewDistance(previousView);
        SetRespectMissionViewDistance(previousMission);
    }
};
} // namespace

TEST_CASE("Video page has compact sections and stages without renderer calls", "[UI][VideoPage]")
{
    Fixture f;
    f.Open();
    auto& p = *f.page;
    CHECK(p.Rows().RowKind(p.At(VideoPage::DisplayHeader)) == OptionsScrollList::KindHeader);
    CHECK(p.Rows().RowKind(p.At(VideoPage::ImageHeader)) == OptionsScrollList::KindHeader);
    CHECK(p.At(VideoPage::ObjectShadows) == -1);
    CHECK(p.At(VideoPage::Multitexturing) == -1);
    p.Set(VideoPage::Gamma, 50);
    p.Set(VideoPage::Fps, 2);
    p.Set(VideoPage::Overall, 1);
    CHECK(f.engine.gamma == 1.2f);
    CHECK(f.engine.qualityCalls == 0);
    CHECK(f.engine.imageCalls == 0);
    CHECK(f.engine.displayCalls == 0);
    CHECK(std::string(p.Rows().SliderValueText(p.At(VideoPage::Gamma))) == "1.4");
    CHECK(f.Saved().vulkan == GraphicsQuality{});
}
TEST_CASE("Video presets change exactly the validated groups and derive Custom", "[UI][VideoPage]")
{
    Fixture f;
    f.Open();
    auto& p = *f.page;
    p.Set(VideoPage::Scale, 2);
    p.Set(VideoPage::ViewDistance, 37);
    p.Set(VideoPage::Objects, 2);
    const int view = p.Value(VideoPage::ViewDistance);
    p.Set(VideoPage::Overall, 1);
    for (auto row : {VideoPage::Shadows, VideoPage::Clouds, VideoPage::Water, VideoPage::AO})
    {
        CHECK(p.Value(row) == 1);
        p.Set(row, 0);
        CHECK(p.Value(VideoPage::Overall) == 2);
        p.Set(row, 1);
        CHECK(p.Value(VideoPage::Overall) == 1);
    }
    p.Set(VideoPage::Overall, 0);
    CHECK(p.Value(VideoPage::Scale) == 2);
    CHECK(p.Value(VideoPage::ViewDistance) == view);
    CHECK(p.Value(VideoPage::Objects) == 2);
    p.Set(VideoPage::Overall, 2);
    CHECK(p.Value(VideoPage::Overall) == 1);
    p.Set(VideoPage::Overall, 2);
    CHECK(p.Value(VideoPage::Overall) == 0);
}
TEST_CASE("Video filters device capabilities and explains startup requirements", "[UI][VideoPage]")
{
    Fixture f;
    f.Open();
    auto& p = *f.page;
    CHECK(p.Choice(VideoPage::AA, "MSAA 8x") == -1);
    CHECK(p.Choice(VideoPage::AA, "MSAA 4x") >= 0);
    CHECK(p.Choice(VideoPage::AA, "FSR 3.1.4 Quality") == -1);
    CHECK(std::string(p.Rows().RowDescription(p.At(VideoPage::AA))).find("CWR_VK_FSR") != std::string::npos);
    CHECK(p.Rows().RowFor(p.At(VideoPage::Shadows)).count == 2);
    CHECK(p.Rows().RowFor(p.At(VideoPage::Water)).count == 2);
    p.Set(VideoPage::Water, 2);
    CHECK(p.Value(VideoPage::Water) == 0);
    CHECK(std::string(p.Rows().RowDescription(p.At(VideoPage::Water))).find("CWR_VK_RTAO") != std::string::npos);
    p.Set(VideoPage::Shadows, 2);
    CHECK(p.Value(VideoPage::Shadows) == 0);
    CHECK(std::string(p.Rows().RowDescription(p.At(VideoPage::Shadows))).find("CWR_VK_RTAO") != std::string::npos);
}
TEST_CASE("Video RT water and shadows apply independently and volume does not redefine presets", "[UI][VideoPage]")
{
    Fixture f;
    f.engine.rt = true;
    f.Open();
    auto& p = *f.page;
    REQUIRE(p.Choice(VideoPage::Water, "Ray Traced") == 2);
    CHECK(p.Value(VideoPage::VolumetricLighting) == 0);
    p.Set(VideoPage::VolumetricLighting, 1);
    CHECK(p.Value(VideoPage::Overall) == 0);
    CHECK_FALSE(p.Rows().IsPending(p.At(VideoPage::Overall)));
    p.Set(VideoPage::Overall, 1);
    CHECK(p.Value(VideoPage::VolumetricLighting) == 1);
    CHECK(p.Value(VideoPage::Overall) == 1);
    for (int shadows : {0, 2, 1})
        for (int water : {2, 1, 0})
        {
            const auto before = f.engine.quality;
            p.Set(VideoPage::Shadows, shadows);
            p.Set(VideoPage::Water, water);
            CHECK(f.engine.quality == before);
            CHECK(p.Value(VideoPage::Overall) == (shadows == 1 && water == 1 ? 1 : 2));
            p.Action(VideoPage::Apply, *f.shell);
            p.Tick(*f.shell);
            CHECK(f.engine.quality.shadows == shadows);
            CHECK(f.engine.quality.water == water);
            CHECK(f.engine.quality.volumetricLighting);
            CHECK(f.Saved().vulkan == f.engine.quality);
        }
    p.Set(VideoPage::Overall, 0);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan.Preset() == 0);
    CHECK(f.Saved().vulkan.volumetricLighting);
    p.Set(VideoPage::VolumetricLighting, 0);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan == GraphicsQuality{});
}
TEST_CASE("Video new controls stage Cancel, commit OK, and reject incomplete activation", "[UI][VideoPage]")
{
    Fixture f;
    f.engine.rt = true;
    f.Open();
    f.page->Set(VideoPage::Water, 2);
    f.page->Set(VideoPage::VolumetricLighting, 1);
    f.shell->OnButtonClicked(IDC_CANCEL);
    CHECK(f.engine.qualityCalls == 0);
    CHECK(f.Saved().vulkan == GraphicsQuality{});
    f.Open();
    f.page->Set(VideoPage::Water, 2);
    f.page->Set(VideoPage::VolumetricLighting, 1);
    f.shell->OnButtonClicked(IDC_OK);
    for (int n = 0; n < 4; ++n) f.shell->OnSimulate(nullptr);
    CHECK(f.Saved().vulkan.water == 2);
    CHECK(f.Saved().vulkan.volumetricLighting);
    f.Open();
    f.page->Set(VideoPage::Water, 1);
    f.page->Set(VideoPage::VolumetricLighting, 0);
    f.page->Action(VideoPage::Apply, *f.shell);
    // Simulate a deferred activation fallback rather than a synchronous rejection.
    f.engine.quality.volumetricLighting = true;
    f.page->Tick(*f.shell);
    CHECK(f.Saved().vulkan.water == 2);
    CHECK(f.Saved().vulkan.volumetricLighting);
    CHECK(f.engine.quality.water == 2);
}
TEST_CASE("Video switches FSR and independent scale as one coherent bundle", "[UI][VideoPage]")
{
    Fixture f;
    f.engine.fsr = true;
    f.Open();
    auto& p = *f.page;
    p.Set(VideoPage::Scale, 3);
    CHECK(p.Value(VideoPage::Scale) == 3);
    p.Set(VideoPage::AA, p.Choice(VideoPage::AA, "FSR 3.1.4 Quality"));
    CHECK(p.Value(VideoPage::Scale) == 0);
    CHECK(p.Rows().IsDisabled(p.At(VideoPage::Scale)));
    p.Set(VideoPage::Scale, 3);
    CHECK(p.Value(VideoPage::Scale) == 0);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkanImage == ImageQuality{"fsr-quality", 100});
    p.Set(VideoPage::AA, p.Choice(VideoPage::AA, "TAA"));
    p.Set(VideoPage::Scale, 1);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkanImage == ImageQuality{"taa", 125});
    CHECK(f.Saved().msaaSamples == 4);
    CHECK(f.Saved().renderScale == 1.75f);
}
TEST_CASE("Video Apply waits for actual activation then persists the whole selection",
          "[UI][VideoPage][ShadowIsolation]")
{
    Fixture f;
    f.engine.pending = true;
    f.Open();
    auto& p = *f.page;
    p.Set(VideoPage::Overall, 1);
    p.Set(VideoPage::Gamma, 50);
    p.Set(VideoPage::Fps, 2);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan == GraphicsQuality{});
    CHECK(p.Rows().IsDisabled(p.At(VideoPage::Cancel)));
    p.Set(VideoPage::Overall, 0);
    CHECK(f.engine.qualityCalls == 1);
    f.engine.pending = false;
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan == GraphicsQuality::Enhanced());
    CHECK(f.Saved().gamma == 1.4f);
    CHECK(f.Saved().fpsCap == 60);
    CHECK(f.Saved().shadowQuality == GraphicsConfig::TierOff);
    CHECK_FALSE(p.Rows().IsDisabled(p.At(VideoPage::Cancel)));
}
TEST_CASE("Video Cancel discards staged values and OK commits before closing", "[UI][VideoPage]")
{
    Fixture f;
    f.Open();
    f.page->Set(VideoPage::Overall, 1);
    f.shell->OnButtonClicked(IDC_CANCEL);
    CHECK(f.Saved().vulkan == GraphicsQuality{});
    CHECK(f.engine.qualityCalls == 0);
    f.Open();
    f.page->Set(VideoPage::Overall, 1);
    f.shell->OnButtonClicked(IDC_OK);
    // The third tick completes and destroys the page: drive through its owner.
    for (int n = 0; n < 4; ++n)
        f.shell->OnSimulate(nullptr);
    CHECK(f.Saved().vulkan == GraphicsQuality::Enhanced());
}
TEST_CASE("Video rejected requests restore previous state and never save requested values", "[UI][VideoPage]")
{
    Fixture f;
    f.engine.reject = true;
    f.Open();
    auto& p = *f.page;
    p.Set(VideoPage::Overall, 1);
    p.Set(VideoPage::Gamma, 50);
    p.Action(VideoPage::Apply, *f.shell);
    CHECK(f.Saved().vulkan == GraphicsQuality{});
    CHECK(p.Value(VideoPage::Overall) == 0);
    CHECK(f.engine.gamma == 1.2f);
    CHECK(std::string(p.Rows().RowDescription(p.At(VideoPage::Overall))) == "Request rejected");
}
TEST_CASE("Video display confirmation reverts or keeps without applying staged graphics early", "[UI][VideoPage]")
{
    Fixture f;
    f.Open();
    auto& p = *f.page;
    p.Set(VideoPage::Mode, 0);
    p.Set(VideoPage::Overall, 1);
    p.Action(VideoPage::Apply, *f.shell);
    CHECK(f.engine.mode == WindowMode::Borderless);
    CHECK(f.engine.qualityCalls == 0);
    f.shell->OnButtonClicked(9202); // Revert (Escape and timeout resolve the same callback).
    CHECK(f.engine.mode == WindowMode::Windowed);
    CHECK(f.engine.width == 1280);
    CHECK(f.Saved().vulkan == GraphicsQuality{});
    p.Set(VideoPage::Mode, 0);
    p.Action(VideoPage::Apply, *f.shell);
    f.shell->OnButtonClicked(9201); // Keep.
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan == GraphicsQuality::Enhanced());
    DisplayConfig cfg;
    REQUIRE(cfg.Load(f.display.path));
    CHECK(cfg.windowMode == DisplayConfig::Borderless);
}
TEST_CASE("Video failed display request cannot be saved by Keep", "[UI][VideoPage]")
{
    Fixture f;
    f.engine.failDisplay = true;
    f.Open();
    auto& p = *f.page;
    p.Set(VideoPage::Mode, 0);
    p.Set(VideoPage::Overall, 1);
    p.Action(VideoPage::Apply, *f.shell);
    f.shell->OnButtonClicked(9201);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan == GraphicsQuality{});
    DisplayConfig cfg;
    REQUIRE(cfg.Load(f.display.path));
    CHECK(cfg.windowMode == DisplayConfig::Windowed);
}
TEST_CASE("Video GL33 exposes only meaningful controls and preserves Vulkan preferences", "[UI][VideoPage]")
{
    Fixture f;
    f.engine.vulkan = false;
    auto cfg = f.Saved();
    cfg.vulkan = {2, 1, 2, true, true};
    cfg.vulkanImage = {"fsr-quality", 100};
    cfg.Save(f.graphics.path);
    f.Open();
    auto& p = *f.page;
    CHECK(p.At(VideoPage::Overall) == -1);
    CHECK(p.At(VideoPage::Shadows) == -1);
    CHECK(p.At(VideoPage::VolumetricLighting) == -1);
    CHECK(p.At(VideoPage::ObjectShadows) >= 0);
    CHECK(p.At(VideoPage::Multitexturing) >= 0);
    CHECK(p.Choice(VideoPage::AA, "FXAA") == -1);
    CHECK(p.Choice(VideoPage::AA, "MSAA 4x") >= 0);
    p.Set(VideoPage::AA, p.Choice(VideoPage::AA, "MSAA 4x"));
    p.Set(VideoPage::Scale, 3);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().msaaSamples == 4);
    CHECK(f.Saved().renderScale == 1.75f);
    CHECK(f.Saved().vulkan == cfg.vulkan);
    CHECK(f.Saved().vulkanImage == cfg.vulkanImage);
}
TEST_CASE("Video image config round-trips and old files default safely", "[Settings][VideoPage]")
{
    Fixture f;
    for (const char* mode : {"off", "fxaa", "smaa", "taa", "msaa2", "msaa4", "msaa8", "fsr-native", "fsr-quality",
                             "fsr-balanced", "fsr-performance"})
    {
        auto cfg = f.Saved();
        cfg.vulkanImage = {mode, 100};
        REQUIRE(cfg.Save(f.graphics.path));
        CHECK(f.Saved().vulkanImage == cfg.vulkanImage);
    }
    ParamFile old;
    old.Add("shadowQuality", 0);
    old.Save(f.graphics.path.c_str());
    CHECK(f.Saved().vulkanImage == ImageQuality{});
    CHECK_FALSE(f.Saved().vulkan.volumetricLighting);
    old.Add("vulkanWater", 2);
    old.Add("vulkanVolumetricLighting", 1);
    old.Save(f.graphics.path.c_str());
    CHECK(f.Saved().vulkan.water == 2);
    CHECK(f.Saved().vulkan.volumetricLighting);
    old.Add("vulkanAA", "invalid");
    old.Add("vulkanRenderScale", 175);
    old.Save(f.graphics.path.c_str());
    CHECK(f.Saved().vulkanImage == ImageQuality{});
}
TEST_CASE("Video preserves addon menu resources and explicit legacy fallback", "[UI][VideoPage]")
{
    ParamFile resource;
    CHECK_FALSE(UseLegacyVideoResource(resource, true));
    CHECK(UseLegacyVideoResource(resource, false));
    resource.Add("cwrLegacyVideo", 1);
    CHECK(UseLegacyVideoResource(resource, true));
    resource.Add("cwrLegacyVideo", 0);
    resource.SetOwner("community-ui", true);
    CHECK(UseLegacyVideoResource(resource, true));
}

TEST_CASE("Video persists window size in logical units without DPI inflation", "[UI][VideoPage]")
{
    Fixture f;
    f.engine.width = 1600;
    f.engine.height = 900;
    f.engine.logicalWidth = 1280;
    f.engine.logicalHeight = 720;
    f.Open();
    auto& p = *f.page;
    CHECK(p.Value(VideoPage::Resolution) == p.Choice(VideoPage::Resolution, "1280x720"));
    p.Set(VideoPage::Gamma, 50);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    DisplayConfig cfg;
    REQUIRE(cfg.Load(f.display.path));
    CHECK(cfg.resolutionWidth == 1280);
    CHECK(cfg.resolutionHeight == 720);
    CHECK(f.engine.width == 1600);
    CHECK(f.engine.displayCalls == 0);
}

TEST_CASE("Video failed bundle save cannot leave a new gamma on disk after Cancel", "[UI][VideoPage]")
{
    Fixture f;
    f.Open();
    auto& p = *f.page;
    DisplayConfig original;
    REQUIRE(original.Load(f.display.path));
    std::filesystem::remove(f.display.path);
    std::filesystem::create_directory(f.display.path); // Fixture-owned display writer failure.
    p.Set(VideoPage::Gamma, 50);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().gamma == 1.2f);
    CHECK(std::string(p.Rows().RowDescription(p.At(VideoPage::Apply))).find("could not be saved") != std::string::npos);
    f.shell->OnButtonClicked(IDC_CANCEL);
    CHECK(f.engine.gamma == 1.2f);
    CHECK(f.Saved().gamma == 1.2f);
    std::filesystem::remove(f.display.path);
    REQUIRE(original.Save(f.display.path));
}

#ifdef _WIN32
TEST_CASE("Video rolls back a partial bundle when the display file is read-only", "[UI][VideoPage]")
{
    Fixture f;
    f.Open();
    auto& p = *f.page;
    const auto originalPermissions = std::filesystem::status(f.display.path).permissions();
    std::filesystem::permissions(f.display.path, std::filesystem::perms::owner_read);
    p.Set(VideoPage::Gamma, 50);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    std::filesystem::permissions(f.display.path, originalPermissions);
    CHECK(f.Saved().gamma == 1.2f); // Graphics wrote first, then the later writer failed.
    CHECK(std::string(p.Rows().RowDescription(p.At(VideoPage::Apply))).find("could not be saved") != std::string::npos);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().gamma == 1.4f);
}
#endif

TEST_CASE("Video applies Vulkan refresh only after entering exclusive mode at the selected size", "[UI][VideoPage]")
{
    Fixture f;
    f.Open();
    auto& p = *f.page;
    p.Set(VideoPage::Mode, 1);
    p.Set(VideoPage::Resolution, 1);
    p.Set(VideoPage::Refresh, 2);
    p.Action(VideoPage::Apply, *f.shell);
    CHECK(f.engine.mode == WindowMode::Fullscreen);
    CHECK(f.engine.width == 800);
    CHECK(f.engine.refreshWidth == 800);
    CHECK(f.engine.refresh == 120);
    f.shell->OnButtonClicked(9201);
    p.Tick(*f.shell);
    DisplayConfig cfg;
    REQUIRE(cfg.Load(f.display.path));
    CHECK(cfg.refreshRate == 120);
}

TEST_CASE("Video gamma Apply preserves unavailable saved RT and FSR preferences", "[UI][VideoPage]")
{
    Fixture f;
    auto cfg = f.Saved();
    cfg.vulkan = {2, 1, 2, true};
    cfg.vulkanImage = {"fsr-quality", 100};
    cfg.Save(f.graphics.path);
    f.engine.quality = GraphicsQuality::Enhanced(); // Startup RT fallback; no FSR startup opt-in.
    f.Open();
    auto& p = *f.page;
    p.Set(VideoPage::Gamma, 50);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().gamma == 1.4f);
    CHECK(f.Saved().vulkan == cfg.vulkan);
    CHECK(f.Saved().vulkanImage == cfg.vulkanImage);
    p.Set(VideoPage::VolumetricLighting, 1);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan.shadows == 2);
    CHECK(f.Saved().vulkan.water == 2);
    CHECK(f.Saved().vulkan.volumetricLighting);
    // Explicitly accepting a raster/Off choice replaces the remembered preference.
    p.Set(VideoPage::Shadows, 1);
    p.Set(VideoPage::AA, p.Choice(VideoPage::AA, "Off"));
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan.shadows == 1);
    CHECK(f.Saved().vulkan.water == 2);
    CHECK(f.Saved().vulkanImage.aa == "off");
    p.Set(VideoPage::Water, 1);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan.water == 1);
}

TEST_CASE("Video reports write failure and allows retry without claiming a successful save", "[UI][VideoPage]")
{
    Fixture f;
    f.Open();
    auto& p = *f.page;
    auto cfg = f.Saved();
    // The fixture owns this process-isolated path. A directory is unwritable as a config file.
    std::filesystem::remove(f.graphics.path);
    std::filesystem::create_directory(f.graphics.path);
    p.Set(VideoPage::Overall, 1);
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(std::string(p.Rows().RowDescription(p.At(VideoPage::Apply))).find("could not be saved") != std::string::npos);
    CHECK_FALSE(p.Rows().IsDisabled(p.At(VideoPage::Apply)));
    std::filesystem::remove(f.graphics.path);
    REQUIRE(cfg.Save(f.graphics.path));
    p.Action(VideoPage::Apply, *f.shell);
    p.Tick(*f.shell);
    CHECK(f.Saved().vulkan == GraphicsQuality::Enhanced());
    CHECK(f.Saved().shadowQuality == GraphicsConfig::TierOff);
}
