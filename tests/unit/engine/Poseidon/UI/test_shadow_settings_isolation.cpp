#include <Poseidon/UI/Options/VideoPage.hpp>
#include <Poseidon/UI/Options/OptionsShell.hpp>
#include <Poseidon/UI/Settings/GraphicsApply.hpp>
#include <Poseidon/Graphics/Dummy/EngineDummy.hpp>
#include <Poseidon/World/Scene/Scene.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Core/Game/GameLoop.hpp>
#include <Poseidon/Core/resincl.hpp>
#include <Poseidon/Foundation/Common/GamePaths.hpp>
#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <random>

namespace Poseidon { RString GetUserParams(); }
using namespace Poseidon;

namespace
{
struct IsolationEngine : EngineDummy
{
    bool vulkan = true;
    float gamma = 1.2f;
    GraphicsQuality quality;
    ImageQuality image;
    ImageQuality GetImageQuality() const override { return image; }
    RString ImageQualityUnavailable(const ImageQuality&) const override { return ""; }
    RString ApplyImageQuality(const ImageQuality& q) override { image = q; return ""; }
    bool HasVulkanGraphics() const override { return vulkan; }
    float GetGamma() const override { return gamma; }
    void SetGamma(float value) override { gamma = value; }
    GraphicsQuality GetGraphicsQuality() const override { return quality; }
    RString GraphicsQualityRTUnavailable() const override { return ""; }
    RString GraphicsQualityError() const override { return ""; }
    bool ApplyGraphicsQuality(const GraphicsQuality& value) override
    {
        quality = value;
        GScene->SetObjectShadows(true);
        GScene->SetVehicleShadows(true);
        return true;
    }
};

// test_main already isolates GamePaths into a per-process temporary directory.
// Restore the shared graphics file and globals so other cases stay independent.
struct IsolationFixture
{
    Engine* oldEngine = GEngine;
    Scene* oldScene = GScene;
    RString oldPlayer = Glob.header.playerName;
    int oldCap = gUserFpsCap;
    std::string graphicsPath = GamePaths::Instance().UserDir() + "graphics.cfg";
    std::optional<std::string> oldGraphics;
    IsolationEngine engine;
    std::unique_ptr<Scene> scene;

    IsolationFixture()
    {
        std::ifstream existing(graphicsPath, std::ios::binary);
        if (existing) oldGraphics.emplace(std::istreambuf_iterator<char>(existing), std::istreambuf_iterator<char>());
        GEngine = &engine;
        GScene = nullptr;
        Glob.header.playerName = ("shadow-isolation-" + std::to_string(std::random_device{}())).c_str();
        ParamFile profile;
        profile.Add("objectShadows", 0);
        profile.Add("vehicleShadows", 0);
        profile.Add("visualQuality", .01f);
        profile.Add("sentinel", 73);
        profile.Save(GetUserParams());
        GraphicsConfig cfg;
        cfg.shadowQuality = GraphicsConfig::TierOff;
        cfg.Save(graphicsPath);
        scene = std::make_unique<Scene>();
        GScene = scene.get();
    }
    ~IsolationFixture()
    {
        scene.reset(); // Exercise the real Scene::SaveConfig while its renderer is alive.
        GScene = oldScene;
        GEngine = oldEngine;
        Glob.header.playerName = oldPlayer;
        gUserFpsCap = oldCap;
        if (oldGraphics) { std::ofstream out(graphicsPath, std::ios::binary); out << *oldGraphics; }
        else std::filesystem::remove(graphicsPath);
    }
    GraphicsConfig Config() const
    {
        GraphicsConfig cfg;
        cfg.Load(graphicsPath);
        return cfg;
    }
    void CheckProfile(int objects, int vehicles)
    {
        ParamFile profile;
        profile.Parse(GetUserParams());
        CHECK(profile.ReadValue("objectShadows", -1) == objects);
        CHECK(profile.ReadValue("vehicleShadows", -1) == vehicles);
        CHECK(profile.ReadValue("sentinel", -1) == 73);
    }
};

struct DetailPage : VideoPage
{
    OptionsScrollList::Provider& Rows() { return ProviderRef(); }
    int At(Row row) { return FindRow(row); }
};
using QualityPage = DetailPage;
}

TEST_CASE("Vulkan Scene load and save preserve legacy profile shadow gates", "[ShadowIsolation]")
{
    IsolationFixture f;
    const int vehiclePreference = GENERATE(0, 1);
    ParamFile profile;
    profile.Parse(GetUserParams());
    profile.Add("vehicleShadows", vehiclePreference);
    profile.Save(GetUserParams());
    f.scene->LoadConfig();
    CHECK(f.scene->GetObjectShadows());
    CHECK(f.scene->GetVehicleShadows());
    f.scene->SaveConfig();
    f.CheckProfile(0, vehiclePreference);

    // Developer/live changes also remain process-local under Vulkan.
    f.scene->SetObjectShadows(false);
    f.scene->SaveConfig();
    f.CheckProfile(0, vehiclePreference);
    f.engine.vulkan = false;
    f.scene->LoadConfig();
    CHECK_FALSE(f.scene->GetObjectShadows());
    CHECK(f.scene->GetVehicleShadows() == (vehiclePreference != 0));
    f.scene->SetObjectShadows(true);
    f.scene->SaveConfig();
    f.CheckProfile(1, vehiclePreference); // GL33 can still save independently selected gates.
}

TEST_CASE("Vulkan Scene save preserves missing old-profile shadow entries", "[ShadowIsolation]")
{
    IsolationFixture f;
    ParamFile oldProfile;
    oldProfile.Add("visualQuality", .01f);
    oldProfile.Save(GetUserParams());
    f.scene->LoadConfig();
    f.scene->SaveConfig();
    ParamFile saved;
    saved.Parse(GetUserParams());
    CHECK(saved.FindEntry("objectShadows") == nullptr);
    CHECK(saved.FindEntry("vehicleShadows") == nullptr);
}

TEST_CASE("Graphics apply leaves Vulkan runtime caster gates independent of the legacy tier", "[ShadowIsolation]")
{
    IsolationFixture f;
    auto cfg = f.Config();
    ApplyGraphicsConfigToEngine(cfg);
    CHECK(f.scene->GetObjectShadows());
    CHECK(f.scene->GetVehicleShadows());
    f.scene->SetObjectShadows(false);
    ApplyGraphicsConfigToEngine(cfg);
    CHECK_FALSE(f.scene->GetObjectShadows()); // Do not undo a developer override on FPS edits.
    CHECK(f.scene->GetVehicleShadows());
    f.engine.vulkan = false;
    ApplyGraphicsConfigToEngine(cfg);
    CHECK_FALSE(f.scene->GetObjectShadows());
    CHECK_FALSE(f.scene->GetVehicleShadows());
    cfg.shadowQuality = GraphicsConfig::TierMedium;
    ApplyGraphicsConfigToEngine(cfg);
    CHECK(f.scene->GetObjectShadows());
    CHECK(f.scene->GetVehicleShadows());
}

TEST_CASE("Vulkan quality Apply and OK save only the Vulkan shadow choice", "[ShadowIsolation]")
{
    IsolationFixture f;
    auto cfg = f.Config();
    cfg.shadowQuality = GENERATE(GraphicsConfig::TierOff, GraphicsConfig::TierMedium, GraphicsConfig::TierHigh);
    REQUIRE(cfg.Save(f.graphicsPath));
    auto page = std::make_unique<QualityPage>();
    auto* raw = page.get();
    OptionsShell shell(nullptr, true, false, std::move(page));
    for (int shadows : {1, 2, 0})
    {
        raw->Rows().SetRowValue(raw->At(VideoPage::Shadows), shadows);
        raw->Rows().OnRowAction(raw->At(VideoPage::Apply), shell);
        for (int i = 0; i < 4; ++i) raw->OnSimulate(shell);
        CHECK(f.Config().vulkan.shadows == shadows);
        CHECK(f.Config().shadowQuality == cfg.shadowQuality);
    }
    raw->Rows().SetRowValue(raw->At(VideoPage::Shadows), 2);
    shell.OnButtonClicked(IDC_CANCEL);
    CHECK(f.Config().vulkan.shadows == 0); // Cancel never saves the staged RT choice.
    auto okPage = std::make_unique<QualityPage>();
    raw = okPage.get();
    shell.PushPage(std::move(okPage));
    raw->Rows().SetRowValue(raw->At(VideoPage::Shadows), 1);
    raw->Rows().OnRowAction(raw->At(VideoPage::OK), shell);
    for (int i = 0; i < 3; ++i) raw->OnSimulate(shell);
    CHECK(f.Config().vulkan.shadows == 1);
    CHECK(f.Config().shadowQuality == cfg.shadowQuality);
}

TEST_CASE("Unified Vulkan Video stages and saves without copying GL33 shadow gates", "[ShadowIsolation]")
{
    IsolationFixture f;
    auto page = std::make_unique<DetailPage>();
    auto* raw = page.get();
    OptionsShell shell(nullptr, true, false, std::move(page));
    CHECK(raw->At(VideoPage::ObjectShadows) == -1);
    raw->Rows().SetRowValue(raw->At(VideoPage::Gamma), 50);
    raw->Rows().SetRowValue(raw->At(VideoPage::Fps), 2);
    raw->Rows().OnRowAction(raw->At(VideoPage::Apply), shell);
    for (int i = 0; i < 4; ++i) raw->OnSimulate(shell);
    CHECK(f.engine.GetGamma() == 1.4f);
    CHECK(f.scene->GetObjectShadows());
    CHECK(f.scene->GetVehicleShadows());
    shell.PopPage();
    CHECK(f.Config().shadowQuality == GraphicsConfig::TierOff);
    CHECK(f.Config().gamma == 1.4f);
    f.scene->SaveConfig();
    f.CheckProfile(0, 0);
}

TEST_CASE("Unified GL33 Video still applies and persists legacy shadow changes", "[ShadowIsolation]")
{
    IsolationFixture f;
    f.engine.vulkan = false;
    f.scene->LoadConfig();
    for (int enabled : {1, 0})
    {
        auto page = std::make_unique<DetailPage>();
        auto* raw = page.get();
        OptionsShell shell(nullptr, true, false, std::move(page));
        raw->Rows().SetRowValue(raw->At(VideoPage::ObjectShadows), enabled);
        raw->Rows().SetRowValue(raw->At(VideoPage::VehicleShadows), enabled);
        raw->Rows().OnRowAction(raw->At(VideoPage::Apply), shell);
        for (int i = 0; i < 4; ++i) raw->OnSimulate(shell);
        CHECK(f.scene->GetObjectShadows() == (enabled != 0));
        CHECK(f.scene->GetVehicleShadows() == (enabled != 0));
        shell.PopPage();
        CHECK(f.Config().shadowQuality == (enabled ? GraphicsConfig::TierHigh : GraphicsConfig::TierOff));
        f.scene->SaveConfig();
        f.CheckProfile(enabled, enabled);
    }
}

TEST_CASE("Vulkan presets do not reset unrelated mod or developer scene detail", "[ShadowIsolation]")
{
    IsolationFixture f;
    f.scene->SetPreferredTerrainGrid(17.f);
    f.scene->SetObjectLODBias(1.3f);
    auto page = std::make_unique<DetailPage>();
    auto* raw = page.get();
    OptionsShell shell(nullptr, true, false, std::move(page));
    raw->Rows().SetRowValue(raw->At(VideoPage::Overall), 1);
    raw->Rows().OnRowAction(raw->At(VideoPage::Apply), shell);
    for (int i = 0; i < 4; ++i) raw->OnSimulate(shell);
    CHECK(f.scene->GetPreferredTerrainGrid() == 17.f);
    CHECK(f.scene->GetObjectLODBias() == 1.3f);
}

TEST_CASE("Explicit unified object detail survives Scene creation without migrating old defaults", "[VideoPage][ShadowIsolation]")
{
    IsolationFixture f;
    f.engine.vulkan = GENERATE(false, true);
    CHECK_FALSE(f.Config().objectLodConfigured);
    f.scene->LoadConfig();
    CHECK(f.scene->GetObjectLODBias() == 1.f); // Old auto-detected tier is not a new player choice.
    auto page = std::make_unique<DetailPage>();
    auto* raw = page.get();
    OptionsShell shell(nullptr, true, false, std::move(page));
    const auto choices = raw->Rows().RowFor(raw->At(VideoPage::Objects));
    CHECK(std::string(choices.options[1]) == "0.75x");
    CHECK(std::string(choices.options[3]) == "2x");
    CHECK(raw->Rows().RowValue(raw->At(VideoPage::Objects)) == 2);
    raw->Rows().SetRowValue(raw->At(VideoPage::Objects), 3);
    raw->Rows().OnRowAction(raw->At(VideoPage::Apply), shell);
    for (int i = 0; i < 4; ++i) raw->OnSimulate(shell);
    CHECK(f.scene->GetObjectLODBias() == 2.f);
    CHECK(f.Config().objectLodConfigured);
    CHECK(f.Config().objectLod == GraphicsConfig::TierUltra);
    // GScene is null while the real Scene constructor runs at startup.
    GScene = nullptr;
    auto restarted = std::make_unique<Scene>();
    CHECK(restarted->GetObjectLODBias() == 2.f);
    restarted.reset();
    GScene = f.scene.get();
    // Each offered value uses the established engine mapping, not invented tiers.
    for (int i = 0; i < 4; ++i)
    {
        raw->Rows().SetRowValue(raw->At(VideoPage::Objects), i);
        raw->Rows().OnRowAction(raw->At(VideoPage::Apply), shell);
        for (int n = 0; n < 4; ++n) raw->OnSimulate(shell);
        CHECK(f.scene->GetObjectLODBias() == ObjectLodBiasForTier(static_cast<GraphicsConfig::Tier>(i + 1)));
    }
}
