#pragma once

#include <string>

namespace Poseidon { class LODShapeWithShadow; }
using Poseidon::LODShapeWithShadow;

namespace Poseidon {
namespace Model {

struct Model;

namespace ShapeAdapter
{
    struct ProxyModelName
    {
        std::string modelName;
        int id = -1;
    };

    ProxyModelName normalizeProxyModelName(const std::string& selectionName);
    // Tool-only source inspection can retain levels normally removed by runtime
    // optimization. Ordinary gameplay callers keep the unchanged default.
    LODShapeWithShadow* convertToLODShape(const Model& model, bool reversed = false, bool preserveSourceLODs = false);
}

} // namespace Model
} // namespace Poseidon
