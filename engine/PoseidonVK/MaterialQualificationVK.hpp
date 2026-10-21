#pragma once
#include <PoseidonVK/ShapeGeometryVK.hpp>
#include <PoseidonVK/PBRMaterialVK.hpp>
#include <PoseidonVK/OriginalMaterialRegionsVK.hpp>
#include <cmath>

namespace Poseidon::vk
{
// Source evidence and transport prerequisites are deliberately separate. A
// default diffuse render state is NOT evidence that an opaque atlas is one
// dielectric. Only an explicit source material or an existing verified identity
// supplies that evidence. Neither UV extent nor color statistics invent it.
constexpr unsigned SourceDiffuseIdentity = 256;
inline bool SourceDiffuseMaterial(const TexMaterial* material)
{
    if (!material || !material->SourceDefined()) return false;
    const auto white = [](ColorVal c) { return c.R()==1 && c.G()==1 && c.B()==1 && c.A()==1; };
    const auto black = [](ColorVal c) { return c.R()==0 && c.G()==0 && c.B()==0; };
    return white(material->_ambient) && white(material->_diffuse) &&
           black(material->_emmisive) && black(material->_forcedDiffuse) && black(material->_specular) &&
           material->_specularPower==0 && !material->_bumpmap && !material->_detailmap;
}
inline QualifiedMaterial OriginalMaterialSource(std::string_view texture, const TexMaterial* material = nullptr)
{
    if (const auto id = EnhancedPBRIdentity(texture))
        return {id, PBRReceiverFamily(id), PBRMaterial(texture, 0, true)};
    if (SourceDiffuseMaterial(material)) return {SourceDiffuseIdentity, 1, .9f};
    return {}; // Includes all untyped opaque mixed atlases, even with perfect normals.
}
inline float OriginalRasterRoughness(QualifiedMaterial source, std::string_view texture, int legacyPower)
{
    // Source evidence must not replace the verified signpost's authored lobe hint.
    return source.identity==SourceDiffuseIdentity || OriginalRegion(source.identity) ?
        source.roughness : PBRMaterial(texture,legacyPower,true);
}
enum class MaterialRejection { None, Source, RenderBehavior, AuthoredLighting, Geometry };
inline bool OriginalDiffuseFlags(int flags)
{
    return SupportedShapeSpec(render::SplitLegacy(flags)) &&
        !(flags & (IsAlpha | IsTransparent | IsAlphaFog | IsColored | IsLight | IsShadow | DisableSun |
                   NoDropdown | NoZBuf | NoZWrite | IsAnimated | IsHidden | IsHiddenProxy | OnSurface |
                   IsOnSurface | SpecularTexture | DetailTexture | ZBiasMask));
}
inline bool OriginalOrdinaryVertex(ClipFlags clip)
{
    // 201, 205, 207 (and every other authored material/light mode) remain
    // legacy. A neighboring ordinary triangle never inherits their rejection.
    return !(clip & (ClipUserMask | ClipLightMask | ClipLandMask));
}
inline QualifiedMaterial OriginalSectionSource(std::string_view texture, const ShapeSectionInfo* section)
{
    // A different eligible section must not lend its source proof to this draw.
    // Missing section evidence (including standalone polygons) stays legacy.
    if (!section || !OriginalDiffuseFlags(section->properties.Special())) return {};
    return OriginalMaterialSource(texture,section->surfMat);
}
inline MaterialRejection OriginalTriangle(const Shape& shape, const std::array<VertexIndex, 3>& indices)
{
    if (shape.NNorm()!=shape.NVertex()) return MaterialRejection::Geometry;
    for (auto i : indices)
    {
        if (unsigned(i)>=unsigned(shape.NVertex())) return MaterialRejection::Geometry;
        if (!OriginalOrdinaryVertex(shape.Clip(i))) return MaterialRejection::AuthoredLighting;
        const auto& n=shape.Norm(i);
        const auto& p=shape.Pos(i);
        const auto& uv=shape.UV(i);
        if (!std::isfinite(n.X()) || !std::isfinite(n.Y()) || !std::isfinite(n.Z()) ||
            !std::isfinite(n.SquareSize()) || n.SquareSize()<=1e-12f ||
            !std::isfinite(p.X()) || !std::isfinite(p.Y()) || !std::isfinite(p.Z()) ||
            !std::isfinite(uv.u) || !std::isfinite(uv.v)) return MaterialRejection::Geometry;
    }
    const auto area=(shape.Pos(indices[1])-shape.Pos(indices[0])).CrossProduct(
                    shape.Pos(indices[2])-shape.Pos(indices[0]));
    if (!std::isfinite(area.SquareSize()) || area.SquareSize()<=1e-12f) return MaterialRejection::Geometry;
    return MaterialRejection::None;
}
struct MaterialRange
{
    uint32_t begin, end, section;
    MaterialRejection rejection;
    unsigned regionalIdentity=0; // Geometry/UV candidate only; exact immutable pixels still required.
};
inline unsigned OriginalRegionCandidate(const Shape& shape, std::string_view texture)
{
    if (shape.IsAnimated() || (shape.GetOrHints() & ClipLandMask)) return 0;
    const auto candidate=OriginalRegionCandidate(static_cast<const char*>(shape.SourceModel()),shape.SourceResolution(),
        shape.NVertex(),shape.NSections(),shape.SourceFingerprint(),texture);
    if (!candidate) return 0;
    std::vector<std::string_view> names;
    for (int i=0;i<shape.NTextures();++i)
    {
        const auto* image=shape.GetTexture(i);
        names.emplace_back(image?static_cast<const char*>(image->Name()):"");
    }
    return OriginalRegionBindingsCompatible(shape.SourceFingerprint(),OriginalRegionTextureBindings(names))?candidate:0;
}
inline unsigned OriginalTriangleRegion(const Shape& shape, unsigned candidate,
                                       const std::array<VertexIndex,3>& indices)
{
    const auto* region=OriginalRegion(candidate);
    if (!region || OriginalRegionCandidate(shape,region->texture)!=candidate ||
        OriginalTriangle(shape,indices)!=MaterialRejection::None) return 0;
    std::array<std::array<float,2>,3> uv;
    for (size_t i=0;i<3;++i) uv[i]={shape.UV(indices[i]).u,shape.UV(indices[i]).v};
    return OriginalRegionUV(candidate,uv)?candidate:0;
}
inline std::vector<MaterialRange> OriginalMaterialRanges(const Shape& shape)
{
    std::vector<MaterialRange> ranges;
    uint32_t index=0;
    for (int s=0; s<shape.NSections(); ++s)
    {
        const auto& section=shape.GetSection(s);
        const auto* texture=section.properties.GetTexture();
        const auto candidate=texture?OriginalRegionCandidate(shape,MaterialIdentity(static_cast<const char*>(texture->Name()))):0;
        for (Offset at=section.beg; at<section.end; shape.NextFace(at))
        {
            const auto& face=shape.Face(at);
            for (int v=2; v<face.N(); ++v)
            {
                const std::array<VertexIndex,3> indices{face.GetVertex(0),face.GetVertex(v-1),face.GetVertex(v)};
                auto rejection=OriginalTriangle(shape,indices);
                if (!OriginalDiffuseFlags(section.properties.Special())) rejection=MaterialRejection::RenderBehavior;
                const auto region=rejection==MaterialRejection::None?OriginalTriangleRegion(shape,candidate,indices):0;
                if (!ranges.empty() && ranges.back().section==unsigned(s) && ranges.back().rejection==rejection &&
                    ranges.back().regionalIdentity==region)
                    ranges.back().end=index+3;
                else ranges.push_back({index,index+3,unsigned(s),rejection,region});
                index+=3;
            }
        }
    }
    return ranges;
}
// Reflection does not require captured ambient. Reception always does.
inline MaterialRejection QualifyOriginalMaterial(QualifiedMaterial source, MaterialRejection triangle,
                                                 int flags, bool opaque, bool immutable)
{
    if (!opaque || !immutable || !OriginalDiffuseFlags(flags)) return MaterialRejection::RenderBehavior;
    if (triangle!=MaterialRejection::None) return triangle;
    return source ? MaterialRejection::None : MaterialRejection::Source;
}
inline bool OriginalMaterialReception(MaterialRejection result, bool capturedAmbient, bool capturedDirect)
{
    return result==MaterialRejection::None && capturedAmbient && capturedDirect;
}
} // namespace Poseidon::vk
