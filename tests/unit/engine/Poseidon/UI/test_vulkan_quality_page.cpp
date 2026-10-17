#include <Poseidon/UI/Options/VulkanQualityPage.hpp>
#include <Poseidon/Graphics/Dummy/EngineDummy.hpp>
#include <Poseidon/UI/Settings/GraphicsConfig.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <random>

using namespace Poseidon;
namespace
{
std::filesystem::path TempQualityPath(const char* leaf)
{
    const auto directory = std::filesystem::temp_directory_path() /
                           ("cwrr-vulkan-quality-" + std::to_string(std::random_device{}()));
    std::filesystem::create_directory(directory);
    return directory / leaf;
}
struct QualityEngine : EngineDummy
{
    GraphicsQuality active;
    int applies = 0;
    bool pending = false, rt = false, reject = false;
    bool HasVulkanGraphics() const override { return true; }
    GraphicsQuality GetGraphicsQuality() const override { return active; }
    RString GraphicsQualityRTUnavailable() const override { return rt ? "" : "Restart with CWR_VK_RTAO=1"; }
    bool ApplyGraphicsQuality(const GraphicsQuality& q) override
    {
        ++applies;
        if (reject || !q.Valid() || (q.shadows == 2 && !rt)) return false;
        active = q;
        pending = true;
        return true;
    }
    bool GraphicsQualityPending() const override { return pending; }
    RString GraphicsQualityError() const override { return reject ? "Unsupported request" : ""; }
};
struct EngineScope
{
    Engine* previous = GEngine;
    QualityEngine engine;
    EngineScope() { GEngine = &engine; }
    ~EngineScope() { GEngine = previous; }
};
class Page : public VulkanQualityPage
{
public:
    OptionsScrollList::Provider& Rows() { return ProviderRef(); }
};
}

TEST_CASE("Vulkan menu stages complete presets and derives exact matches", "[UI][VulkanQuality]")
{
    EngineScope scope;
    Page page;
    auto& rows = page.Rows();
    REQUIRE(rows.RowCount() == 9);
    CHECK(rows.RowValue(0) == 0);
    rows.SetRowValue(0, 1);
    for (int row = 0; row < 5; ++row) CHECK(rows.RowValue(row) == 1);
    CHECK(scope.engine.applies == 0);
    CHECK(scope.engine.active == GraphicsQuality{});
    for (int row = 1; row < 5; ++row)
    {
        rows.SetRowValue(row, 0);
        CHECK(rows.RowValue(0) == 2);
        rows.SetRowValue(row, 1);
        CHECK(rows.RowValue(0) == 1);
    }
    rows.SetRowValue(0, 0);
    for (int row = 0; row < 5; ++row) CHECK(rows.RowValue(row) == 0);
    rows.SetRowValue(0, 2); // Left from Standard skips the derived Custom sentinel
    CHECK(rows.RowValue(0) == 1);
    rows.SetRowValue(0, 2); // Right from Enhanced wraps to Standard
    CHECK(rows.RowValue(0) == 0);
    CHECK(scope.engine.applies == 0); // Cancel/destruction needs no renderer rollback
}

TEST_CASE("Vulkan menu gates RT by initialized capability", "[UI][VulkanQuality]")
{
    EngineScope scope;
    {
        Page page;
        CHECK(page.Rows().RowFor(1).count == 2);
        page.Rows().SetRowValue(1, 2);
        CHECK(page.Rows().RowValue(1) == 0);
        CHECK(std::string(page.Rows().RowDescription(1)).find("CWR_VK_RTAO") != std::string::npos);
    }
    scope.engine.rt = true;
    Page page;
    CHECK(page.Rows().RowFor(1).count == 3);
    page.Rows().SetRowValue(1, 2);
    CHECK(page.Rows().RowValue(1) == 2);
    CHECK(page.Rows().RowValue(0) == 2);
}

TEST_CASE("Vulkan Apply sends one bundle and disables navigation during activation", "[UI][VulkanQuality]")
{
    EngineScope scope;
    Page page;
    Display host(nullptr);
    auto& rows = page.Rows();
    rows.SetRowValue(0, 1);
    rows.OnRowAction(6, host);
    CHECK(scope.engine.applies == 1);
    CHECK(scope.engine.active == GraphicsQuality::Enhanced());
    for (int row = 0; row < rows.RowCount(); ++row) CHECK(rows.IsDisabled(row));
    rows.SetRowValue(0, 0);
    rows.OnRowAction(6, host);
    CHECK(scope.engine.applies == 1);
}

TEST_CASE("Vulkan rejected requests redisplay actual state", "[UI][VulkanQuality]")
{
    EngineScope scope;
    scope.engine.reject = true;
    Page page;
    Display host(nullptr);
    page.Rows().SetRowValue(0, 1);
    page.Rows().OnRowAction(6, host);
    CHECK(scope.engine.applies == 1);
    CHECK(page.Rows().RowValue(0) == 0);
    CHECK(std::string(page.Rows().RowDescription(0)) == "Unsupported request");
}

TEST_CASE("Vulkan groups persist alongside existing video settings", "[Settings][VulkanQuality]")
{
    const auto path = TempQualityPath("graphics.cfg");
    GraphicsConfig cfg;
    cfg.vulkan = {2, 0, 1, true};
    cfg.brightness = 1.3f;
    cfg.terrainDetail = GraphicsConfig::TierMedium;
    REQUIRE(cfg.Save(path.string()));
    GraphicsConfig loaded;
    REQUIRE(loaded.Load(path.string()));
    CHECK(loaded.vulkan == cfg.vulkan);
    CHECK(loaded.brightness == cfg.brightness);
    CHECK(loaded.terrainDetail == cfg.terrainDetail);
    CHECK(loaded.vulkan.Preset() == 2);
    std::filesystem::remove(path);
    std::filesystem::remove(path.parent_path());
}

TEST_CASE("Vulkan graphics reports a failed settings write", "[Settings][VulkanQuality]")
{
    const auto path = TempQualityPath("directory");
    std::filesystem::create_directory(path);
    GraphicsConfig cfg;
    CHECK_FALSE(cfg.Save(path.string()));
    std::filesystem::remove(path);
    std::filesystem::remove(path.parent_path());
}
