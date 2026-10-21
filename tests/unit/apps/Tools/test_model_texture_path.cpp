#include <catch2/catch_test_macros.hpp>
#include "../../../../apps/tools/Tools/commands/ModelTexturePath.hpp"

TEST_CASE("Qualification texture paths default to the model directory, not the working directory",
          "[Tools][material-qualification]")
{
    namespace fs = std::filesystem;
    const auto model = fs::temp_directory_path() / "qualification-model" / "shape.p3d";
    const auto texture = fs::path("data") / "wall.pac";
    const auto path = PoseidonTools::QualificationTexturePath(model, {}, "data\\wall.pac");
    REQUIRE(path == model.parent_path() / texture);
    REQUIRE(path != fs::current_path() / texture);
    const auto root = fs::temp_directory_path() / "explicit-texture-root";
    REQUIRE(PoseidonTools::QualificationTexturePath(model, root, "data\\wall.pac") == root / texture);
    REQUIRE(PoseidonTools::QualificationTexturePath("shape.p3d", {}, "data/wall.pac") == texture);
    REQUIRE(PoseidonTools::QualificationTexturePath("models/shape.p3d", {}, "data/wall.pac") ==
            fs::path("models") / texture);
}
