#include <catch2/catch_test_macros.hpp>
#include <BuildConfig.h>
#if CWR_HAS_VULKAN
#include <PoseidonVK/RayTransformVK.hpp>
#endif
#include <PoseidonVK/ShapeGeometryVK.hpp>
#include <PoseidonVK/TerrainRayVK.hpp>
#include <PoseidonVK/ShapeTransformVK.hpp>
#include <PoseidonVK/ScreenGeometryVK.hpp>
#include <PoseidonVK/ScreenPipelineVK.hpp>
#include <PoseidonVK/ShapeLightingVK.hpp>
#include <PoseidonVK/TextureInterpolationVK.hpp>
#include <catch2/catch_approx.hpp>
#include <limits>
#include <bit>
#include <PoseidonVK/LocalShadowVK.hpp>
#include <PoseidonVK/WetSurfaceVK.hpp>
#include <PoseidonVK/PBRMaterialVK.hpp>
#include <PoseidonVK/RaySurfaceVK.hpp>
#include <Poseidon/Graphics/Core/TLVertex.hpp>
#include <Poseidon/World/Scene/Camera/Camera.hpp>

using namespace Poseidon;

TEST_CASE("Original atlas regions require exact geometry, pixels and entire original triangles",
          "[Graphics][vulkan-shape][material-qualification]")
{
    for (const auto& region : vk::OriginalMaterialRegions)
    {
        REQUIRE_FALSE(vk::OriginalMaterialSource(region.texture));
        REQUIRE(vk::OriginalRegionSource(region.identity,region.texture,region.width,region.height,region.pixels).family==region.family);
        REQUIRE_FALSE(vk::OriginalRegionSource(region.identity,region.texture,region.width,region.height,region.pixels^1));
        REQUIRE_FALSE(vk::OriginalRegionSource(region.identity,region.texture,region.width/2,region.height,region.pixels));
        REQUIRE_FALSE(vk::OriginalRegionSource(region.identity,"other\\budova2_s.pac",region.width,region.height,region.pixels));
        for (const auto& geometry : vk::OriginalRegionGeometries)
        {
            REQUIRE(vk::OriginalRegionBindingsCompatible(geometry.fingerprint,geometry.textureBindings));
            REQUIRE_FALSE(vk::OriginalRegionBindingsCompatible(geometry.fingerprint,geometry.textureBindings^1));
            const auto candidate=vk::OriginalRegionCandidate(geometry.model,geometry.resolution,geometry.vertices,
                geometry.sections,geometry.fingerprint,region.texture);
            const bool known=std::any_of(std::begin(vk::OriginalMaterialRegions),std::end(vk::OriginalMaterialRegions),
                [&](const auto& r){return r.model==geometry.model && r.texture==region.texture;});
            REQUIRE(bool(candidate)==known);
            if (candidate)
            {
                REQUIRE(vk::OriginalRegion(candidate)->model==geometry.model);
                REQUIRE(vk::OriginalRegion(candidate)->texture==region.texture);
                REQUIRE(vk::SameOriginalRegionSource(candidate,region.identity)==(geometry.model==region.model));
            }
            REQUIRE_FALSE(vk::OriginalRegionCandidate(geometry.model,geometry.resolution,geometry.vertices,
                geometry.sections,geometry.fingerprint^1,region.texture));
            REQUIRE_FALSE(vk::OriginalRegionCandidate("other\\statek_kulna.p3d",geometry.resolution,geometry.vertices,
                geometry.sections,geometry.fingerprint,region.texture));
            REQUIRE_FALSE(vk::OriginalRegionCandidate(geometry.model,10000,geometry.vertices,
                geometry.sections,geometry.fingerprint,region.texture));
        }
        const float u=region.uMin+.2f*(region.uMax-region.uMin),v=region.vMin+.2f*(region.vMax-region.vMin);
        const std::array<std::array<float,2>,3> lower{{{u,v},{u+.1f,v},{u,v+.1f}}};
        REQUIRE(vk::OriginalRegionUV(region.identity,lower));
        REQUIRE(vk::SelectOriginalRegion(region,lower,ClampU|ClampV)==region.identity);
        if (region.requiredClamp) REQUIRE_FALSE(vk::SelectOriginalRegion(region,lower,0));
        for (const auto bad : {std::array<float,2>{u,region.vMin-.01f},{region.uMax+.01f,v},{region.uMin-.01f,v},
                              {u,region.vMax+.01f},{u,std::numeric_limits<float>::quiet_NaN()}})
        {
            auto mixed=lower; mixed[1]=bad;
            REQUIRE_FALSE(vk::OriginalRegionUV(region.identity,mixed));
        }
        auto source=vk::OriginalRegionSource(region.identity,region.texture,region.width,region.height,region.pixels);
        for (auto rejection : {vk::MaterialRejection::Geometry,vk::MaterialRejection::AuthoredLighting})
            REQUIRE(vk::QualifyOriginalMaterial(source,rejection,ClampU|ClampV,true,true)==rejection);
        for (auto flag : {IsShadow,IsTransparent,IsAlpha,NoZWrite,IsAnimated,DisableSun,IsColored})
            REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,flag,true,true)==vk::MaterialRejection::RenderBehavior);
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,false,true));
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,true,false));
        for (bool software : {false,true})
        {
            vk::ShapeLighting lighting;
            lighting.pbr={source.roughness,.04f,0,float(source.identity)};
            lighting.pbrDiffuse[3]=float(source.family); lighting.rtWater[3]=1;
            vk::QualifyGIReceiver(lighting,software);
            REQUIRE(lighting.rtWater[3]==(region.reception?2:1));
            REQUIRE(lighting.pbrDiffuse[3]==float(region.family+(software && region.reception?8:0)));
            lighting.pbrDiffuse[3]=region.family==5?6:5; lighting.rtWater[3]=1;
            vk::QualifyGIReceiver(lighting,software);
            REQUIRE(lighting.rtWater[3]==1);
        }
    }
    // Keep an actually unallocated boundary as the explicit registry grows.
    REQUIRE_FALSE(vk::OriginalRegion(vk::OriginalMaterialRegions[std::size(vk::OriginalMaterialRegions)-1].identity+1));
    REQUIRE_FALSE(vk::OriginalRegionUV(0,{}));
    REQUIRE(vk::OriginalRegionPixelFingerprint(std::array<uint8_t,4>{1,2,3,255})==9228046714267810950ull);
    const std::array<std::string_view,2> bindings{"data\\budova2_s.pac","data\\budova2_s2.pac"};
    const std::array<std::string_view,2> swapped{bindings[1],bindings[0]};
    REQUIRE(vk::OriginalRegionTextureBindings(bindings)!=vk::OriginalRegionTextureBindings(swapped));
}

TEST_CASE("Atlas region selection is unique, source-bound and sampler-aware",
          "[Graphics][vulkan-shape][material-qualification]")
{
    auto wood=vk::OriginalMaterialRegions[0];
    wood.identity=900;wood.family=4;wood.roughness=.85f;
    wood.uMin=0;wood.uMax=.5f;wood.vMin=-.005f;wood.vMax=.49f;wood.requiredClamp=ClampV;
    auto stone=wood;stone.identity=901;stone.family=6;stone.roughness=.9f;
    stone.uMin=-3;stone.uMax=3;stone.vMin=.5f;stone.vMax=1;stone.requiredClamp=0;
    const std::array<vk::OriginalMaterialRegion,2> rules{wood,stone},reversed{stone,wood};
    const std::array<std::array<float,2>,3> boards{{{.01f,-.001f},{.49f,.01f},{.49f,.48f}}};
    const std::array<std::array<float,2>,3> masonry{{{-2.f,.51f},{2.f,.51f},{2.f,.99f}}};
    const std::array<std::array<float,2>,3> mixed{{{.1f,.2f},{.4f,.2f},{.4f,.8f}}};
    for (const auto& set : {rules,reversed})
    {
        REQUIRE(vk::SelectOriginalRegion(wood,boards,ClampV,set)==wood.identity);
        REQUIRE(vk::SelectOriginalRegion(stone,boards,ClampV,set)==wood.identity);
        REQUIRE_FALSE(vk::SelectOriginalRegion(wood,boards,0,set));
        REQUIRE(vk::SelectOriginalRegion(wood,masonry,0,set)==stone.identity);
        REQUIRE_FALSE(vk::SelectOriginalRegion(wood,mixed,ClampV,set));
    }
    auto overlap=wood;overlap.identity=902;
    const std::array duplicates{wood,overlap};
    REQUIRE_FALSE(vk::SelectOriginalRegion(wood,boards,ClampV,duplicates));
    for (int field=0;field<5;++field)
    {
        auto wrong=wood;
        if(field==0) wrong.model="other\\model.p3d";
        if(field==1) wrong.texture="other\\texture.pac";
        if(field==2) ++wrong.width;
        if(field==3) ++wrong.height;
        if(field==4) wrong.pixels^=1;
        REQUIRE_FALSE(vk::SameOriginalRegionSource(wood,wrong));
        REQUIRE_FALSE(vk::SelectOriginalRegion(wrong,boards,ClampV,rules));
    }
    REQUIRE_FALSE(vk::SameOriginalRegionSource(0,wood.identity));
}

TEST_CASE("Automatic material qualification needs authored evidence, never opaque default state",
          "[Graphics][vulkan-shape][material-qualification]")
{
    TexMaterial untyped;
    REQUIRE_FALSE(untyped.SourceDefined());
    REQUIRE_FALSE(vk::SourceDiffuseMaterial(&untyped));
    REQUIRE_FALSE(vk::OriginalMaterialSource("arbitrary/opaque-atlas.pac",&untyped));
    TexMaterial declared(TESTS_ROOT_DIR "/fixtures/rendering/source-diffuse.cpp");
    REQUIRE(declared.SourceDefined());
    REQUIRE(vk::SourceDiffuseMaterial(&declared));
    const auto source=vk::OriginalMaterialSource("any/original-texture.pac",&declared);
    REQUIRE(source.identity==vk::SourceDiffuseIdentity);
    REQUIRE(source.roughness==.9f);
    REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,ClampU,true,true)==vk::MaterialRejection::None);
    REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,false,true));
    REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,true,false));
    REQUIRE(vk::OriginalMaterialReception(vk::MaterialRejection::None,true,true));
    for (auto flag : {IsShadow,DisableSun,IsColored,IsLight,IsAlpha,IsTransparent,NoZWrite,SpecularTexture,IsAnimated})
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,flag,true,true)==vk::MaterialRejection::RenderBehavior);
    REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,0,false,true)==vk::MaterialRejection::RenderBehavior);
    REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,0,true,false)==vk::MaterialRejection::RenderBehavior);
    declared._specular=Color(.2f,.2f,.2f);
    REQUIRE_FALSE(vk::SourceDiffuseMaterial(&declared));
    declared._specular=HBlack; declared._forcedDiffuse=Color(.1f,0,0);
    REQUIRE_FALSE(vk::SourceDiffuseMaterial(&declared));
    // Hand-verified material identity has precedence over source declarations.
    REQUIRE(vk::OriginalMaterialSource("data\\tasky_tmavsi.pac",&declared).identity==vk::StockPBRIdentity("data\\tasky_tmavsi.pac"));
    for (int power : {0,8,128})
        REQUIRE(vk::OriginalRasterRoughness(vk::OriginalMaterialSource("data\\sloupek_bily_orezly_.pac"),
            "data\\sloupek_bily_orezly_.pac",power)==vk::PBRMaterial("data\\sloupek_bily_orezly_.pac",power,true));
    for (const auto key : {"data\\blok_a_side.pac","data\\blok_a_side_02.pac","data\\domek1_side.pac","o\\hous\\panel02.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
    // Inspected blank plaster is distinct from its window-bearing neighbors.
    // Source proof does not excuse authored lighting or alpha render behavior.
    for (const auto key : {"data\\blok_a_back.pac","data\\ujezd_up.pac"})
    {
        const auto verified=vk::OriginalMaterialSource(key);
        REQUIRE(verified.identity==vk::EnhancedPBRIdentity(key));
        REQUIRE(verified.family==5);
        REQUIRE(vk::QualifyOriginalMaterial(verified,vk::MaterialRejection::AuthoredLighting,0,true,true)==vk::MaterialRejection::AuthoredLighting);
        REQUIRE(vk::QualifyOriginalMaterial(verified,vk::MaterialRejection::None,IsTransparent,true,true)==vk::MaterialRejection::RenderBehavior);
    }
}

TEST_CASE("Dedicated bark identities do not promote foliage, authored trunks or mixed source atlases",
          "[Graphics][vulkan-shape][material-qualification]")
{
    const std::array keys{"data\\kmen2.pac", "data\\kmen_borovice.pac", "data\\kura3.pac",
        "data\\kura_jablon_asi_ne.pac", "data\\lesni_kmen_smrku.pac", "data\\n_strom_01_kura.pac",
        "data\\n_strom_lipa_kmen.pac", "o\\tree\\akat01_krd2.pac", "o\\tree\\borovice02c.pac",
        "o\\tree\\dd_les_kura03.pac", "o\\tree\\dub_kura_fx.pac", "o\\tree\\smrkur.pac"};
    for (unsigned i=0;i<keys.size();++i)
    {
        const auto source=vk::OriginalMaterialSource(keys[i]);
        REQUIRE(source.identity==166+i);
        REQUIRE(source.family==4);
        REQUIRE(source.roughness==.85f);
        REQUIRE(vk::PBRMaterial(keys[i],128)==0); // v3 is unchanged
        REQUIRE(vk::PBRMaterial(keys[i],128,true)==.85f);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,NoClamp,true,true)==vk::MaterialRejection::None);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::AuthoredLighting,NoClamp,true,true)==vk::MaterialRejection::AuthoredLighting);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::Geometry,NoClamp,true,true)==vk::MaterialRejection::Geometry);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,IsTransparent,true,true)==vk::MaterialRejection::RenderBehavior);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,NoClamp,false,true)==vk::MaterialRejection::RenderBehavior);
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,false,true));
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,true,false));
    }
    for (const auto key : {"data\\briza_kriv_kmen.pac", "o\\tree\\kmenletokruh.pac",
        "o\\tree\\smrkvtvmal.pac", "o\\tree\\smrkvtvvel.pac", "o\\tree\\smrk_m_lod.pac",
        "o\\tree\\dd_bush02.pac", "o\\tree\\dd_bush05.pac", "data\\akat01_krd2.pac",
        "other\\tree\\smrkur.pac", "o\\tree\\smrkur.paa"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
}

TEST_CASE("Inspected stone sources distinguish natural rock, masonry and mixed memorial atlases",
          "[Graphics][vulkan-shape][material-qualification]")
{
    struct Source { const char* key; float roughness; unsigned family; };
    const std::array sources{Source{"data\\piskovec.pac",.85f,2},
        Source{"data\\skala_piskovec.pac",.9f,6}, Source{"data\\skala_piskovec3.pac",.85f,2},
        Source{"data\\bozi_muka_sokl.pac",.9f,6}, Source{"data\\hrobecek2_side.pac",.9f,6},
        Source{"data\\hrobecek2_side2.pac",.9f,6}, Source{"data\\hrobecek2_side3.pac",.9f,6}};
    for (unsigned i=0;i<sources.size();++i)
    {
        const auto& expected=sources[i];const auto source=vk::OriginalMaterialSource(expected.key);
        REQUIRE(source.identity==178+i);
        REQUIRE(source.family==expected.family);
        REQUIRE(source.roughness==expected.roughness);
        REQUIRE(vk::PBRMaterial(expected.key,128)==0);
        REQUIRE(vk::PBRMaterial(expected.key,128,true)==expected.roughness);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::AuthoredLighting,0,true,true)==vk::MaterialRejection::AuthoredLighting);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::Geometry,0,true,true)==vk::MaterialRejection::Geometry);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,IsTransparent,true,true)==vk::MaterialRejection::RenderBehavior);
    }
    for (const auto key : {"o\\hous\\nahrobky01.pac","data\\hrobecek2.pac","data\\cross_opak.pac",
        "other\\piskovec.pac","data\\piskovec.paa","data\\helfenburk_vez_front.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
    const std::array<std::array<float,2>,3> stone{{{.6f,.6f},{.72f,.6f},{.6f,.72f}}};
    auto soil=stone;soil[0][0]=.49f;
    auto plaque=stone;plaque[0][1]=.49f;
    for (unsigned id=534;id<=539;++id)
    {
        const auto* region=vk::OriginalRegion(id);REQUIRE(region);
        REQUIRE(vk::SelectOriginalRegion(*region,stone,ClampU|ClampV)==id);
        REQUIRE_FALSE(vk::SelectOriginalRegion(*region,stone,ClampU));
        REQUIRE_FALSE(vk::SelectOriginalRegion(*region,stone,ClampV));
        REQUIRE_FALSE(vk::SelectOriginalRegion(*region,soil,ClampU|ClampV));
        REQUIRE_FALSE(vk::SelectOriginalRegion(*region,plaque,ClampU|ClampV));
    }
    const auto found=std::find_if(std::begin(vk::OriginalRegionGeometries),std::end(vk::OriginalRegionGeometries),
        [](const auto& geometry){return geometry.model=="o\\hous\\nahrobek5.p3d" && geometry.resolution==1.31150222f;});
    REQUIRE(found!=std::end(vk::OriginalRegionGeometries));
    const auto& fractional=*found;
    REQUIRE(vk::OriginalRegionCandidate(fractional.model,1.31150222f,fractional.vertices,
        fractional.sections,fractional.fingerprint,"o\\hous\\nahrobky01.pac")==539);
    REQUIRE_FALSE(vk::OriginalRegionCandidate(fractional.model,1.3115f,fractional.vertices,
        fractional.sections,fractional.fingerprint,"o\\hous\\nahrobky01.pac"));
}

TEST_CASE("Ruin regions retain lintel, debris, window and exact fractional LOD boundaries",
          "[Graphics][vulkan-shape][material-qualification]")
{
    for (const auto key : {"o\\hous\\zborenina1.pac","o\\hous\\zborenina1a.pac",
                          "o\\hous\\domek_ruina1.pac","o\\hous\\domek_ruina2.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
    REQUIRE_FALSE(std::any_of(std::begin(vk::OriginalMaterialRegions),std::end(vk::OriginalMaterialRegions),
        [](const auto& r){return r.texture=="o\\hous\\domek_ruina1.pac" || r.texture=="o\\hous\\domek_ruina2.pac";}));
    for (unsigned id=540;id<=543;++id)
    {
        const auto* r=vk::OriginalRegion(id);REQUIRE(r);
        REQUIRE(r->family==6);REQUIRE(r->roughness==.9f);
        const float u=(r->uMin+r->uMax)*.5f,v=(r->vMin+r->vMax)*.5f;
        const std::array<std::array<float,2>,3> inside{{{u,v},{u+.01f,v},{u,v+.01f}}};
        REQUIRE(vk::SelectOriginalRegion(*r,inside,ClampV)==id);
        auto mixed=inside;
        if(id==542)mixed[0][0]=-.001f; // No wrapping into the right-hand lintel tile.
        else mixed[0][1]=r->vMin-.01f;
        REQUIRE_FALSE(vk::SelectOriginalRegion(*r,mixed,ClampV));
        REQUIRE(vk::QualifyOriginalMaterial({id,r->family,r->roughness},
            vk::MaterialRejection::AuthoredLighting,ClampV,true,true)==vk::MaterialRejection::AuthoredLighting);
    }
    struct Exact { const char* model; float exact, rounded; uint32_t bits; };
    for (const auto& e : {Exact{"o\\hous\\ruiny_3_dvere.p3d",6.90305948f,6.90306f,1088218589u},
        Exact{"o\\hous\\ruiny_3_prasklina.p3d",6.86704254f,6.86704f,1088143056u},
        Exact{"o\\hous\\ruiny_3_roh.p3d",7.31359243f,7.31359f,1089079539u}})
    {
        REQUIRE(std::bit_cast<uint32_t>(e.exact)==e.bits);
        const auto g=std::find_if(std::begin(vk::OriginalRegionGeometries),std::end(vk::OriginalRegionGeometries),
            [&](const auto& g){return g.model==e.model && g.resolution==e.exact;});
        REQUIRE(g!=std::end(vk::OriginalRegionGeometries));
        REQUIRE(vk::OriginalRegionCandidate(g->model,e.exact,g->vertices,g->sections,g->fingerprint,"o\\hous\\zborenina1.pac"));
        REQUIRE_FALSE(vk::OriginalRegionCandidate(g->model,e.rounded,g->vertices,g->sections,g->fingerprint,"o\\hous\\zborenina1.pac"));
    }
}

TEST_CASE("Inspected roof tiles do not promote chimney openings or ambiguous roof sheets",
          "[Graphics][vulkan-shape][material-qualification]")
{
    const std::array keys{"data\\kostel_strecha.pac", "data\\strecha_tasky.pac", "o\\hous\\hasicakstrecha.pac"};
    for (unsigned i=0;i<keys.size();++i)
    {
        const auto source=vk::OriginalMaterialSource(keys[i]);
        REQUIRE(source.identity==185+i);
        REQUIRE(source.family==3);
        REQUIRE(source.roughness==.9f);
        REQUIRE(vk::PBRMaterial(keys[i],128)==0); // v3 is unchanged
        REQUIRE(vk::PBRMaterial(keys[i],128,true)==.9f);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,NoClamp,true,true)==vk::MaterialRejection::None);
        for (auto rejection : {vk::MaterialRejection::AuthoredLighting,vk::MaterialRejection::Geometry})
            REQUIRE(vk::QualifyOriginalMaterial(source,rejection,NoClamp,true,true)==rejection);
        for (auto flag : {IsTransparent,IsAlpha,NoZWrite,IsOnSurface,IsAnimated,DisableSun})
            REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,flag,true,true)==vk::MaterialRejection::RenderBehavior);
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,false,true));
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,true,false));
    }
    // Every other source in the original 31-image roof/chimney survey remains
    // untyped as a whole image. Potential brick-only original faces are a
    // separate unfinished region review, never a license for the hole/frame.
    for (const auto key : {"data\\blok_a_strecha.pac","data\\blok_a_strecha_02.pac",
        "data\\dum_05_strecha_01.pac","data\\garaz_strecha.pac","data\\hang_strecha.pac",
        "data\\hang_strecha_kraj.pac","data\\hruzdum_strecha.pac","data\\kdomek_komin.pac",
        "data\\kdomek_podkomin.pac","data\\komin.pac","data\\komin_maly_side_01.pac",
        "data\\komin_roura_side.pac","data\\komin_roura_top.pac","data\\komin_side_af.pac",
        "data\\komin_top_01.pac","data\\kostel2_strecha2.pac","data\\kostel2_strecha3.pac",
        "data\\kostel2_strecha_veze.pac","data\\kostel2_strecha_vezi.pac","data\\kostel2_strecha_zakr.pac",
        "data\\kostel_t_strecha.pac","data\\strazni_vez_strecha_.pac","o\\hous\\chata6strecha.pac",
        "o\\hous\\kostel02_strechal.pac","o\\hous\\orlhot_kom.pac","o\\hous\\panel_strecha.pac",
        "o\\hous\\posed_strecha.pac","o\\hous\\radnice_strecha.pac",
        "data\\kostel_strecha.paa","other\\strecha_tasky.pac","data\\hasicakstrecha.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
}

TEST_CASE("Castle masonry and slit-free faces do not classify roots or photographed cavities",
          "[Graphics][vulkan-shape][material-qualification]")
{
    REQUIRE(128+std::size(vk::EnhancedPBRMaterials)<=vk::SourceDiffuseIdentity);
    const std::array keys{std::string_view("o\\divk\\zed.pac")};
    for (unsigned i=0;i<keys.size();++i)
    {
        const auto source=vk::OriginalMaterialSource(keys[i]);
        REQUIRE(source.identity==193+i);REQUIRE(source.family==6);REQUIRE(source.roughness==.9f);
        REQUIRE_FALSE(vk::StockPBRIdentity(keys[i]));REQUIRE(vk::PBRMaterial(keys[i],128)==0);
        REQUIRE(vk::PBRMaterial(keys[i],128,true)==.9f);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::AuthoredLighting,NoClamp,true,true)==vk::MaterialRejection::AuthoredLighting);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::Geometry,NoClamp,true,true)==vk::MaterialRejection::Geometry);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,IsAlpha,true,true)==vk::MaterialRejection::RenderBehavior);
    }
    for (const auto key : {"o\\divk\\r_sm_km.pac","o\\divk\\zed_lv03.pac","o\\divk\\zed_lv05.pac",
        "o\\divk\\zed_pr01.pac","o\\divk\\zed_pr04.pac","o\\divk\\zed_pr05.pac","o\\divk\\zed_diry.pac",
        "o\\divk\\hrad_zedop.pac","other\\divk\\zed.pac","o\\divk\\zed.paa","other\\divk\\hrad_zedop.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
    const auto* region=vk::OriginalRegion(560);REQUIRE(region);REQUIRE(region->reception);
    REQUIRE(region->model=="o\\divk\\zedlv.p3d");REQUIRE(region->texture=="o\\divk\\zed_lv03.pac");
    REQUIRE(region->family==6);REQUIRE(region->roughness==.9f);
    REQUIRE(vk::OriginalRegionSource(560,region->texture,512,512,1822267094740399805ull).identity==560);
    const std::array<std::array<float,2>,3> mineral{{{0,.18f},{1,.18f},{0,1}}};
    const std::array<std::array<float,2>,3> slit{{{0,.10f},{.4f,.10f},{0,.16f}}};
    REQUIRE(vk::SelectOriginalRegion(*region,mineral,NoClamp)==560);
    REQUIRE(vk::SelectOriginalRegion(*region,slit,NoClamp)==0);
    REQUIRE(std::count_if(std::begin(vk::OriginalRegionGeometries),std::end(vk::OriginalRegionGeometries),
        [&](const auto& g){return g.model==region->model;})==3);
    REQUIRE_FALSE(vk::OriginalRegion(529)->reception);
    for (unsigned id=557;id<=559;++id) REQUIRE_FALSE(vk::OriginalRegion(id)->reception);
}

TEST_CASE("Dedicated timber sources preserve original floor lighting and mixed photo exclusions",
          "[Graphics][vulkan-shape][material-qualification]")
{
    const std::array keys{std::string_view("o\\hous\\posedbok.pac"),std::string_view("o\\hous\\posedbokin.pac"),
        std::string_view("o\\hous\\posedfloor.pac"),
        std::string_view("o\\hous\\posed01a.pac")};
    REQUIRE(128+std::size(vk::EnhancedPBRMaterials)<=vk::SourceDiffuseIdentity);
    for (unsigned i=0;i<keys.size();++i)
    {
        const auto source=vk::OriginalMaterialSource(keys[i]);
        REQUIRE(source.identity==194+i);REQUIRE(source.family==4);REQUIRE(source.roughness==.85f);
        REQUIRE(vk::PBRReceiverFamily(source.identity)==4);
        REQUIRE_FALSE(vk::StockPBRIdentity(keys[i]));REQUIRE(vk::PBRMaterial(keys[i],128)==0);
        REQUIRE(vk::PBRMaterial(keys[i],128,true)==.85f);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::AuthoredLighting,NoClamp,true,true)==vk::MaterialRejection::AuthoredLighting);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::Geometry,NoClamp,true,true)==vk::MaterialRejection::Geometry);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,IsAlpha,true,true)==vk::MaterialRejection::RenderBehavior);
    }
    for (const auto key : {"o\\hous\\prkno_str.pac","data\\k1_stara_prkna.pac","data\\podlaha_prkna_h.pac","o\\hous\\posed6.pac","o\\hous\\posed8.pac",
        "o\\hous\\posed_strecha.pac","o\\hous\\letokruhy.pac","data\\drevo_dirkate.pac",
        "o\\hous\\posedfloor.paa","data\\posedbok.pac","other\\hous\\posed01a.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
    const auto* underside=vk::OriginalRegion(561);REQUIRE(underside);REQUIRE_FALSE(underside->reception);
    REQUIRE(underside->model=="o\\hous\\posed.p3d");REQUIRE(underside->texture=="o\\hous\\prkno_str.pac");
    REQUIRE(underside->family==4);REQUIRE(underside->roughness==.85f);
    REQUIRE(vk::OriginalRegionSource(561,underside->texture,256,256,11743484817861242001ull).identity==561);
    REQUIRE_FALSE(vk::OriginalRegionSource(561,underside->texture,256,256,11743484817861242000ull));
    const std::array<std::array<float,2>,3> boards{{{.002f,-.0017f},{1.0007f,0},{0,1.0042f}}};
    REQUIRE(vk::SelectOriginalRegion(*underside,boards,NoClamp)==561);
    REQUIRE(std::count_if(std::begin(vk::OriginalRegionGeometries),std::end(vk::OriginalRegionGeometries),
        [&](const auto& g){return g.model==underside->model;})==4);
}

TEST_CASE("Original fountain mineral sources do not classify photographed spouts vegetation or water",
          "[Graphics][vulkan-shape][material-qualification]")
{
    const std::array keys{std::string_view("data\\kasna_rantl.pac"),std::string_view("data\\kasna_schod.pac"),
        std::string_view("data\\kasna_sloupekbtr.pac")};
    REQUIRE(128+std::size(vk::EnhancedPBRMaterials)<=vk::SourceDiffuseIdentity);
    for (unsigned i=0;i<keys.size();++i)
    {
        const auto source=vk::OriginalMaterialSource(keys[i]);
        REQUIRE(source.identity==198+i);REQUIRE(source.family==6);REQUIRE(source.roughness==.9f);
        REQUIRE_FALSE(vk::StockPBRIdentity(keys[i]));REQUIRE(vk::PBRMaterial(keys[i],128)==0);
        REQUIRE(vk::PBRMaterial(keys[i],128,true)==.9f);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::AuthoredLighting,NoClamp,true,true)==vk::MaterialRejection::AuthoredLighting);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::Geometry,NoClamp,true,true)==vk::MaterialRejection::Geometry);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,IsAlpha,true,true)==vk::MaterialRejection::RenderBehavior);
    }
    for (const auto key : {"data\\kasna_nesmysl.pac","data\\kasna_sidegif.pac","data\\kasna_sloupekgif.pac",
        "data\\kasna_dno.pac","data\\kasna_in.pac","data\\kasna_voda.01.paa",
        "o\\hous\\kasna01.pac","o\\hous\\kasna01_dno.pac","o\\hous\\kasna01_dno.01.pac",
        "o\\hous\\kasna01_hladina.paa","o\\hous\\kasna01_hladina.01.paa",
        "data\\kasna_rantl.paa","other\\kasna_schod.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
    const auto* mineral=vk::OriginalRegion(562);REQUIRE(mineral);REQUIRE(mineral->reception);
    REQUIRE(mineral->model=="o\\hous\\kasna_new.p3d");REQUIRE(mineral->texture=="o\\hous\\kasna01.pac");
    REQUIRE(mineral->family==6);REQUIRE(mineral->roughness==.9f);
    REQUIRE(vk::OriginalRegionSource(562,mineral->texture,256,256,16872738925428872060ull).identity==562);
    REQUIRE_FALSE(vk::OriginalRegionSource(562,mineral->texture,256,256,16872738925428872061ull));
    const std::array<std::array<float,2>,3> panel{{{-.18f,.51f},{1.9f,.51f},{.3f,1.0052f}}};
    const std::array<std::array<float,2>,3> spout{{{.1f,.1f},{.9f,.1f},{.1f,.2f}}};
    const std::array<std::array<float,2>,3> mixed{{{0,.49f},{1,.51f},{0,.8f}}};
    REQUIRE(vk::SelectOriginalRegion(*mineral,panel,NoClamp)==562);
    REQUIRE(vk::SelectOriginalRegion(*mineral,spout,NoClamp)==0);
    REQUIRE(vk::SelectOriginalRegion(*mineral,mixed,NoClamp)==0);
    REQUIRE(vk::OriginalRegionCandidate(mineral->model,6.54187679f,190,5,5126656687611659204ull,mineral->texture)==562);
    REQUIRE(vk::OriginalRegionCandidate(mineral->model,6.54188f,190,5,5126656687611659204ull,mineral->texture)==0);
    REQUIRE(vk::OriginalRegionBindingsCompatible(5126656687611659204ull,5978338520408643504ull));
    REQUIRE_FALSE(vk::OriginalRegionBindingsCompatible(5126656687611659204ull,5978338520408643505ull));
    REQUIRE(std::count_if(std::begin(vk::OriginalRegionGeometries),std::end(vk::OriginalRegionGeometries),
        [&](const auto& g){return g.model==mineral->model;})==4);
}

TEST_CASE("Original concrete structure images do not classify buried siblings metal or dark support optics",
          "[Graphics][vulkan-shape][material-qualification]")
{
    const std::array keys{std::string_view("data\\beton_universal.pac"),
        std::string_view("data\\beton_preklad_opak.pac"),std::string_view("data\\panel_maskac_opak.pac")};
    for (unsigned i=0;i<keys.size();++i)
    {
        const auto key=keys[i];const auto source=vk::OriginalMaterialSource(key);
        REQUIRE(source.identity==190+i);REQUIRE(source.family==7);REQUIRE(source.roughness==.9f);
        REQUIRE_FALSE(vk::StockPBRIdentity(key));REQUIRE(vk::PBRMaterial(key,128)==0);
        REQUIRE(vk::PBRMaterial(key,128,true)==.9f);
        for (int flags : {int(NoClamp),int(ClampU),int(ClampV),int(ClampU|ClampV)})
        {
            REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,flags,true,true)==vk::MaterialRejection::None);
            REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::AuthoredLighting,flags,true,true)==vk::MaterialRejection::AuthoredLighting);
            REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::Geometry,flags,true,true)==vk::MaterialRejection::Geometry);
        }
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,IsAlpha,true,true)==vk::MaterialRejection::RenderBehavior);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,NoClamp,false,true)==vk::MaterialRejection::RenderBehavior);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,NoClamp,true,false)==vk::MaterialRejection::RenderBehavior);
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,false,true));
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,true,false));
    }
    for (const auto key : {"data\\beton_top.pac","data\\beton_preklad_opakd.pac","data\\beton_cesta.pac",
        "data\\lampa_old_01_betonsl.pac","data\\lampa_old_01_klobouk.pac","data\\lampa_old_01_tyc_sid.pac",
        "data\\lampa_old_01_vnitr.pac","data\\vrata_01.pac","other\\beton_universal.pac",
        "data\\beton_universal.paa","other\\panel_maskac_opak.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
}

TEST_CASE("Village bell masonry and tile variants do not classify bells or mixed facades",
          "[Graphics][vulkan-shape][material-qualification]")
{
    for (const auto& key : {std::string_view("data\\zvonicka_side.pac")})
    {
        const auto source=vk::OriginalMaterialSource(key);
        REQUIRE(source.identity==189);REQUIRE(source.family==6);REQUIRE(source.roughness==.9f);
        REQUIRE_FALSE(vk::StockPBRIdentity(key));REQUIRE(vk::PBRMaterial(key,128)==0);
        REQUIRE(vk::PBRMaterial(key,128,true)==.9f);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,ClampV,true,true)==vk::MaterialRejection::None);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::AuthoredLighting,ClampV,true,true)==vk::MaterialRejection::AuthoredLighting);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::Geometry,ClampV,true,true)==vk::MaterialRejection::Geometry);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,IsAlpha,true,true)==vk::MaterialRejection::RenderBehavior);
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,false,true));
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,true,false));
    }
    for (const auto key : {"data\\zvonicka_bell.paa","data\\zvonicka_side.paa","other\\zvonicka_side.pac",
        "data\\tasky_tmavsi90.paa","other\\tasky_tmavsi90.pac","data\\deutsche_up.pac",
        "data\\deutsche.pac","data\\deutsche_tram.pac","o\\divk\\zed_diry.pac","o\\divk\\drv_trm.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
    REQUIRE(vk::StockPBRIdentity("data\\tasky_tmavsi.pac")==70);
    REQUIRE(vk::PBRMaterial("data\\tasky_tmavsi.pac",128)==.9f);
    REQUIRE_FALSE(vk::OriginalMaterialSource("data\\tasky_tmavsi90.pac"));
    unsigned geometries=0;
    for (unsigned id=557;id<=559;++id)
    {
        const auto* region=vk::OriginalRegion(id);REQUIRE(region);
        REQUIRE(region->texture=="data\\tasky_tmavsi90.pac");REQUIRE(region->family==3);
        REQUIRE(region->roughness==.9f);REQUIRE_FALSE(region->reception);
        const auto source=vk::OriginalRegionSource(id,region->texture,256,256,13546172877813697002ull);
        REQUIRE(source.identity==id);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,NoClamp,true,true)==vk::MaterialRejection::None);
        for (bool software : {false,true})
        {
            vk::ShapeLighting lighting;
            lighting.pbr={.9f,.04f,0,float(id)};lighting.pbrDiffuse[3]=3;lighting.rtWater[3]=1;
            vk::QualifyGIReceiver(lighting,software);
            REQUIRE(lighting.rtWater[3]==1);REQUIRE(lighting.pbrDiffuse[3]==3);
        }
        geometries+=unsigned(std::count_if(std::begin(vk::OriginalRegionGeometries),std::end(vk::OriginalRegionGeometries),
            [&](const auto& g){return g.model==region->model;}));
    }
    REQUIRE(geometries==14);
}

TEST_CASE("Concrete roadside blocks do not classify rails rusty pipes or sign coatings",
          "[Graphics][vulkan-shape][material-qualification]")
{
    const auto source=vk::OriginalMaterialSource("data\\patnik_new.pac");
    REQUIRE(source.identity==188);REQUIRE(source.family==7);REQUIRE(source.roughness==.9f);
    REQUIRE(vk::PBRMaterial("data\\patnik_new.pac",128)==0);
    REQUIRE(vk::PBRMaterial("data\\patnik_new.pac",128,true)==.9f);
    for (const auto key : {"data\\patnik_new.paa","other\\patnik_new.pac",
        "data\\sloupek_bily_orezly.pac","data\\sloup_bily_orez_v.pac","data\\rez_trubka.pac",
        "data\\nemocnice_zn.pac","data\\nemocnice_zn_bck.pac","data\\pumpa.pac",
        "data\\pumpa_bck.pac","data\\slepa.pac","data\\sterk.pac"})
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
    // Keep the previously supported rail/post identity and authored lobe hint.
    REQUIRE(vk::OriginalMaterialSource("data\\sloupek_bily_orezly_.pac").identity==86);
    REQUIRE(vk::PBRReceiverFamily(86)==0);
    REQUIRE(vk::PBRMaterial("data\\sloupek_bily_orezly_.pac",128,true)==.5f);
    REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,ClampU|ClampV,true,true)==vk::MaterialRejection::None);
    REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::AuthoredLighting,ClampU|ClampV,true,true)==vk::MaterialRejection::AuthoredLighting);
    REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::Geometry,ClampU|ClampV,true,true)==vk::MaterialRejection::Geometry);
    REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::None,IsTransparent,true,true)==vk::MaterialRejection::RenderBehavior);
    REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,false,true));
    REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::None,true,false));
}

TEST_CASE("Chimney brick regions preserve frame aperture and original-face boundaries",
          "[Graphics][vulkan-shape][material-qualification]")
{
    REQUIRE_FALSE(vk::OriginalMaterialSource("o\\hous\\orlhot_kom.pac"));
    REQUIRE(vk::PBRMaterial("o\\hous\\orlhot_kom.pac",128,true)==0);
    unsigned geometries=0;
    for (unsigned id=544;id<=556;++id)
    {
        const auto* r=vk::OriginalRegion(id);REQUIRE(r);
        REQUIRE(r->texture=="o\\hous\\orlhot_kom.pac");
        REQUIRE(r->family==6);REQUIRE(r->roughness==.9f);
        REQUIRE(r->uMin==.22f);REQUIRE(r->uMax==.79f);
        REQUIRE(r->vMin==0);REQUIRE(r->vMax==1);
        const std::array<std::array<float,2>,3> brick{{{.44f,.01f},{.66f,.01f},{.66f,.98f}}};
        REQUIRE(vk::SelectOriginalRegion(*r,brick,NoClamp)==id);
        for (auto outside : {std::array<float,2>{.21f,.5f},{.81f,.5f},{.5f,-.001f},{.5f,1.001f}})
        {
            auto mixed=brick;mixed[0]=outside;
            REQUIRE_FALSE(vk::SelectOriginalRegion(*r,mixed,NoClamp));
        }
        const auto source=vk::OriginalRegionSource(id,r->texture,256,128,13663165639993867772ull);
        REQUIRE(source.identity==id);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::AuthoredLighting,NoClamp,true,true)==vk::MaterialRejection::AuthoredLighting);
        REQUIRE(vk::QualifyOriginalMaterial(source,vk::MaterialRejection::Geometry,NoClamp,true,true)==vk::MaterialRejection::Geometry);
        geometries+=unsigned(std::count_if(std::begin(vk::OriginalRegionGeometries),std::end(vk::OriginalRegionGeometries),
            [&](const auto& g){return g.model==r->model;}));
    }
    REQUIRE(geometries==50);
}

TEST_CASE("Original blended roads retain surface depth and fitted-ground lighting gates",
          "[Graphics][vulkan-shape][material-qualification]")
{
    // Exact legacy adapter flags in the 75-model road-source audit, including
    // bridge overlay LODs and the two near cobble LODs without source land hints.
    for (const int flags : {147712,147714,180480,180482})
    {
        REQUIRE(flags & IsAlpha);
        REQUIRE_FALSE(vk::OriginalDiffuseFlags(flags));
        const vk::QualifiedMaterial hypotheticalStone{130,2,.85f};
        REQUIRE(vk::QualifyOriginalMaterial(hypotheticalStone,vk::MaterialRejection::None,
            flags,true,true)==vk::MaterialRejection::RenderBehavior);
        REQUIRE_FALSE(vk::OriginalMaterialReception(vk::MaterialRejection::RenderBehavior,true,true));
    }
    // Removing only alpha or guessing an opaque center does not make a fitted
    // overlay ordinary geometry; none of these independent gates is optional.
    for (const int flags : {OnSurface,IsOnSurface,NoZWrite,ZBiasStep})
        REQUIRE_FALSE(vk::OriginalDiffuseFlags(flags));
    REQUIRE_FALSE(vk::OriginalOrdinaryVertex(ClipLandOn));
    REQUIRE_FALSE(vk::OriginalOrdinaryVertex(ClipLandKeep));
    REQUIRE(vk::OriginalOrdinaryVertex(ClipNone));
    for (const auto key : {"data\\asfaltka.paa","data\\asfaltka_kr.paa","data\\cesta.paa",
        "data\\cesta_kr.paa","data\\silnice.paa","data\\silnice_kr.paa",
        "o\\road\\asf_new.paa","o\\road\\asf_konec.paa","o\\road\\ces_hned.paa",
        "o\\road\\ces_hned_konec.paa","o\\road\\sil_new.paa","o\\road\\sil_konec.paa",
        "o\\road\\sil_konecupl.paa","o\\road\\sil_kos.paa","o\\road\\sil_kos_k.paa",
        "o\\road\\sil_kos_cur.paa","o\\road\\sil_kos_cur2.paa"})
    {
        REQUIRE_FALSE(vk::OriginalMaterialSource(key));
        REQUIRE(vk::PBRMaterial(key,128,true)==0);
    }
}

TEST_CASE("Automatic triangle ranges preserve ordinary neighbors of authored shadow lighting",
          "[Graphics][vulkan-shape][material-qualification]")
{
    Shape shape;
    for (int mode : {0,201,0,205,0,207,0})
    {
        const auto first=shape.NVertex();
        shape.AddVertexFast(Vector3(0,0,float(first)),Vector3(0,0,-1),mode*ClipUserStep,0,0);
        shape.AddVertexFast(Vector3(1,0,float(first)),Vector3(0,0,-1),mode*ClipUserStep,1,0);
        shape.AddVertexFast(Vector3(0,1,float(first)),Vector3(0,0,-1),mode*ClipUserStep,0,1);
        Poly face; face.Init(); face.SetN(3);
        for (int v=0;v<3;++v) face.Set(v,first+v);
        shape.AddFace(face);
    }
    ShapeSection section; section.properties.Init(); section.material=0;
    section.beg=shape.BeginFaces(); section.end=shape.EndFaces(); shape.AddSection(section);
    auto ranges=vk::OriginalMaterialRanges(shape);
    REQUIRE(ranges.size()==7);
    for (unsigned r=0;r<ranges.size();++r)
    {
        REQUIRE(ranges[r].begin==r*3); REQUIRE(ranges[r].end==(r+1)*3);
        REQUIRE(ranges[r].rejection==(r%2?vk::MaterialRejection::AuthoredLighting:vk::MaterialRejection::None));
    }
    REQUIRE_FALSE(vk::OriginalOrdinaryVertex(ClipLightSky));
    REQUIRE_FALSE(vk::OriginalOrdinaryVertex(ClipLandOn));
    shape.SetNorm(0)=VZero;
    REQUIRE(vk::OriginalTriangle(shape,{0,1,2})==vk::MaterialRejection::Geometry);
    shape.SetNorm(0)=Vector3(0,0,-1);
    shape.SetUV(0,std::numeric_limits<float>::infinity(),0);
    REQUIRE(vk::OriginalTriangle(shape,{0,1,2})==vk::MaterialRejection::Geometry);
}

TEST_CASE("CPU submission retains mixed section source evidence after clipping",
          "[Graphics][vulkan-shape][material-qualification]")
{
    const std::string key="arbitrary/original-wall.pac";
    Ref<TexMaterial> declared=new TexMaterial(TESTS_ROOT_DIR "/fixtures/rendering/source-diffuse.cpp");
    Ref<TexMaterial> unsupported=new TexMaterial(TESTS_ROOT_DIR "/fixtures/rendering/source-diffuse.cpp");
    unsupported->_specularPower=8;
    // The texture default alone would qualify even the unsupported section.
    REQUIRE(vk::OriginalMaterialSource(key,declared).identity==vk::SourceDiffuseIdentity);
    Shape shape;
    for (float z : {2.f,.5f})
    {
        shape.AddVertexFast(Vector3(0,0,z),Vector3(0,0,-1),0,0,0);
        shape.AddVertexFast(Vector3(1,0,z),Vector3(0,0,-1),0,1,0);
        shape.AddVertexFast(Vector3(0,1,z),Vector3(0,0,-1),0,0,1);
    }
    std::array<ShapeSection,5> sections;
    for (int s=0;s<int(sections.size());++s)
    {
        auto& section=sections[s];
        section.properties.Init(); section.material=0; section.surfMat=declared;
        if (s==1) section.surfMat=unsupported;
        if (s==3) section.surfMat=nullptr;
        if (s==4) section.properties.SetSpecial(IsShadow);
        section.beg=shape.EndFaces();
        Poly face; face.Init(); face.SetN(3);
        // First section is entirely clipped away. All others share vertices,
        // so a vertex lighting slot cannot encode their different eligibility.
        for (int v=0;v<3;++v) face.Set(v,(s==0?3:0)+v);
        shape.AddFace(face);
        section.end=shape.EndFaces(); shape.AddSection(section);
    }
    TLVertexTable table;
    for (int v=0;v<shape.NVertex();++v)
    {
        table.AddPos(); table.SetTransPos(v)=shape.Pos(v);
        table.SetClip(v,v<3?ClipNone:ClipFront);
    }
    table.pbrNormal.resize(table.NVertex(),{0,0,1,1});
    Camera camera; camera.SetPerspective(1,100,1,1);
    FaceArray clipped;
    clipped.Clip(shape.Faces(),table,camera,ClipFront,false);
    REQUIRE(clipped.Begin()<clipped.End());
    REQUIRE_FALSE(clipped.FindSection(clipped.Begin(),clipped.Begin()));
    const auto ranges=vk::OriginalMaterialRanges(shape);
    REQUIRE(ranges.size()==sections.size());
    Offset at=clipped.Begin();
    for (int s=1;s<int(sections.size());++s)
    {
        const auto begin=at; clipped.Next(at);
        const auto* submitted=clipped.FindSection(begin,at);
        REQUIRE(submitted);
        REQUIRE(submitted->surfMat==sections[s].surfMat);
        REQUIRE(submitted->properties.Special()==sections[s].properties.Special());
        const auto cpu=vk::OriginalSectionSource(key,submitted);
        const auto nativeRay=vk::OriginalMaterialSource(key,sections[s].surfMat);
        const bool accepted=vk::QualifyOriginalMaterial(nativeRay,ranges[s].rejection,
            sections[s].properties.Special(),true,true)==vk::MaterialRejection::None;
        REQUIRE(bool(cpu)==accepted);
        REQUIRE(bool(cpu)==(s==2));
        if (cpu) REQUIRE(cpu.identity==vk::SourceDiffuseIdentity);
    }
    REQUIRE(at==clipped.End());
    REQUIRE_FALSE(vk::OriginalSectionSource(key,nullptr));
    REQUIRE_FALSE(vk::OriginalSectionSource("data\\tasky_tmavsi.pac",nullptr));
    REQUIRE(vk::OriginalSectionSource("data\\tasky_tmavsi.pac",&sections[1]).identity==
            vk::EnhancedPBRIdentity("data\\tasky_tmavsi.pac"));
}

TEST_CASE("Terrain receivers use independent original normals and palette, rejecting potential water", "[Graphics][vulkan-shape][ray-surface]")
{
    const auto valid=vk::TerrainReceiverMaterial(20,5.25f,{0,2,0},{1.1f,.9f,1});
    REQUIRE(valid[1]==1); REQUIRE(valid[3]==1); REQUIRE(valid[4]==1.1f); REQUIRE(valid[5]==.9f);
    for(float height : {5.25f,0.f,-10.f,std::numeric_limits<float>::quiet_NaN()})
        REQUIRE(vk::TerrainReceiverMaterial(height,5.25f,{0,1,0},{1,1,1})[3]==0);
    for (const auto& normal : {std::array<float,3>{0,0,0},{0,-1,0},
                             {std::numeric_limits<float>::quiet_NaN(),1,0}})
        REQUIRE(vk::TerrainReceiverMaterial(20,5.25f,normal,{1,1,1})[3]==0);
    REQUIRE(vk::TerrainReceiverMaterial(20,5.25f,{0,1,0},{-1,1,1})[3]==0);
    REQUIRE(vk::TerrainReceiverMaterial(20,5.25f,{0,1,0},{3,1,1})[3]==0);
}

TEST_CASE("Native terrain splits only dry triangle eligibility, preserving original index order", "[Graphics][vulkan-shape][ray-surface]")
{
    vk::ShapeGeometry geometry;
    geometry.vertices={{{0,20,0},{0,1,0},{0,0}},{{1,20,0},{0,1,0},{1,0}},
                       {{0,20,1},{0,1,0},{0,1}},{{1,5.25f,1},{0,1,0},{1,1}}};
    geometry.indices={0,1,2,1,3,2,0,1,2,0,1,2};
    const auto ranges=vk::TerrainReceiverRanges(geometry,5.25f);
    REQUIRE(ranges.size()==3);
    REQUIRE(ranges[0].dry); REQUIRE(ranges[0].begin==0); REQUIRE(ranges[0].end==3);
    REQUIRE_FALSE(ranges[1].dry); REQUIRE(ranges[1].begin==3); REQUIRE(ranges[1].end==6);
    REQUIRE(ranges[2].dry); REQUIRE(ranges[2].begin==6); REQUIRE(ranges[2].end==12);
    geometry.vertices[0].normal[1]=0;
    const auto invalid=vk::TerrainReceiverRanges(geometry,5.25f);
    REQUIRE(invalid.size()==1); REQUIRE_FALSE(invalid[0].dry); REQUIRE(invalid[0].end==12);
}

TEST_CASE("Stock GI receivers require qualified identity and independent opaque lighting gates",
          "[Graphics][vulkan-shape][enhanced-material]")
{
    for (unsigned i=0;i<std::size(vk::StockPBRMaterials)+std::size(vk::EnhancedPBRMaterials);++i)
        for (bool software : {false,true})
        {
            vk::ShapeLighting l;
            const bool original=i<std::size(vk::StockPBRMaterials);
            const auto extra=i-std::size(vk::StockPBRMaterials);
            l.pbr={.9f,.04f,0,float(original?64+i:128+extra)};
            const auto family=original?vk::StockPBRMaterials[i].receiverFamily:vk::EnhancedPBRMaterials[extra].receiverFamily;
            l.pbrDiffuse[3]=float(family);
            // No opaque-world prerequisite: identity alone cannot reserve ambient.
            vk::QualifyGIReceiver(l,software);
            REQUIRE(l.rtWater[3]==0);
            l.rtWater[3]=1;
            vk::QualifyGIReceiver(l,software);
            REQUIRE(l.rtWater[3]==(family?2.f:1.f));
            REQUIRE(l.pbrDiffuse[3]==float(family+(family && software?8:0)));
            l.rtWater[3]=1;l.pbr[0]=0;
            vk::QualifyGIReceiver(l,software);
            REQUIRE(l.rtWater[3]==1);
        }
    for (float identity : {0.f,1.5f,63.f,87.f,127.f,128.5f,float(128+std::size(vk::EnhancedPBRMaterials)),1000.f,-1.f,std::numeric_limits<float>::quiet_NaN()})
    {
        vk::ShapeLighting l;l.rtWater[3]=1;l.pbr={.9f,.04f,0,identity};l.pbrDiffuse[3]=1;
        vk::QualifyGIReceiver(l,true);
        REQUIRE(l.rtWater[3]==1); // unknown identities and terrain remain legacy
    }
    for (const auto* material : vk::EnhancedCatalog)
    {
        vk::ShapeLighting l;l.rtWater[3]=1;l.pbr={.9f,.04f,0,float(material->id)};l.pbrDiffuse[3]=1;
        vk::QualifyGIReceiver(l,false);REQUIRE(l.rtWater[3]==2);
    }
}

namespace ReceiverRecordMath {
#define CWR_MATERIAL_CLASS_ONLY
#include <PoseidonVK/Shaders/material_record.glsl>
#undef CWR_MATERIAL_CLASS_ONLY
#undef CWR_MATERIAL_RECORD
}
TEST_CASE("GI family records preserve every original normal bit in exact float integers",
          "[Graphics][vulkan-shape][enhanced-material]")
{
    for(int family=0;family<=15;++family)
        for(unsigned normal : {0u,1u,511u,1023u,524287u,1048575u})
        {
            const float code=ReceiverRecordMath::materialReceiverCode(float(normal),family);
            REQUIRE(ReceiverRecordMath::materialReceiverClass(code)==family);
            REQUIRE((unsigned(code)&1048575u)==normal);
            REQUIRE(code<=16777215.f);
        }
}

TEST_CASE("Ray diffuse metadata rejects unsupported lighting without changing sampler bits",
          "[Graphics][vulkan-shape][ray-surface]")
{
    REQUIRE((vk::RaySurfaceFlags(0) & vk::RayDiffuse) != 0);
    REQUIRE((vk::RaySurfaceFlags(IsTransparent) & vk::RayDiffuse) != 0); // traversal handles alpha; bounce excludes it
    for (int flags : {DisableSun, IsColored, IsLight, IsAlpha, IsAlphaFog, SpecularTexture, IsWater})
        REQUIRE((vk::RaySurfaceFlags(flags) & vk::RayDiffuse) == 0);
    for (int flags : std::array<int, 6>{0, ClampU, ClampV, NoClamp, PointSampling, IsTransparent})
    {
        REQUIRE((vk::RaySurfaceFlags(flags) & 7) == 0);
        REQUIRE((vk::RaySurfaceFlags(flags) & vk::RayVertexNormals) != 0);
    }
}

TEST_CASE("PBR only selects inspected stock dielectric textures", "[Graphics][vulkan-shape]")
{
    REQUIRE(vk::PBRMaterial("data\\skala_piskovec2.pac", 0) == .85f);
    REQUIRE(vk::PBRMaterial("data\\sloupek_bily_orezly_.pac", 0) == .6f);
    REQUIRE(vk::PBRMaterial("data\\sloupek_bily_orezly_.pac", 5) == .65f);
    REQUIRE(vk::PBRMaterial("data\\sloupek_bily_orezly_.pac", 100000) == .5f);
    REQUIRE(vk::PBRMaterial("data\\sloupek_bily_orezly_.pac", -1) == .6f);
    for (int power : {-1, 0, 5, 100000})
    {
        REQUIRE(vk::PBRMaterial("data\\strecha_tasky_vln.pac", power) == .90f);
        REQUIRE(vk::PBRMaterial("data\\strecha_tasky_vln90.pac", power) == .90f);
        REQUIRE(vk::PBRMaterial("data\\prkna_dark.pac", power) == .85f);
    }
    for (auto name : {"data\\kamennazed.pac", "data\\plot_beton_0301_side.pac", "data\\plot_beton_02_side.pac",
                      "data\\betonpoprask_top.pac", "data\\omlacena_bocni_zed.pac", "data\\stodola_f.pac"})
        REQUIRE(vk::PBRMaterial(name, 5) == .90f);
    REQUIRE(vk::PBRMaterial("data\\telegraf_sloup.pac", 0) == .85f);
    for (auto name : {"data\\mutt_kapotag.pac", "data\\sloupek_bily_orezly_.paa", "addon\\sloupek_bily_orezly_.pac",
                      "data\\redbarel.pac", "data\\beton_universal.pac", "data\\newplot.pac",
                      "data\\newplot2.paa", "data\\paleta_front.pac", "data\\domek1_side.pac",
                      "data\\cihldum_back.pac", "addon\\prkna_dark.pac", "data\\prkna_dark.paa",
                      "addon\\strecha_tasky_vln.pac", "data\\strecha_tasky_vln90.paa", ""})
        REQUIRE(vk::PBRMaterial(name, 5) == 0);
}

TEST_CASE("CPU PBR normals match native inverse transpose including scale and orientation", "[Graphics][vulkan-shape]")
{
    Matrix4 modelToWorld = MIdentity;
    modelToWorld(0,0)=0; modelToWorld(0,2)=4;
    modelToWorld(1,1)=3; modelToWorld(2,0)=-2; modelToWorld(2,2)=0;
    modelToWorld.SetPosition(Vector3(10,20,30));
    vk::ShapeLighting native;
    vk::ShapeWorld(native, modelToWorld);
    Vector3 n(1,2,3);
    const auto sw = vk::SoftwarePBRNormal(modelToWorld.InverseGeneral(), n);
    for (int row=0; row<3; ++row)
        REQUIRE(sw[row] == Catch::Approx(-native.normal[row]*n.X()-native.normal[4+row]*n.Y()-native.normal[8+row]*n.Z()));
    REQUIRE(sw.X() == Catch::Approx(-.75f));
    REQUIRE(sw.Y() == Catch::Approx(-2.f/3));
    REQUIRE(sw.Z() == Catch::Approx(.5f));
}

TEST_CASE("CPU clipping preserves PBR normals but rejects mixed lighting modes", "[Graphics][vulkan-shape]")
{
    TLVertexTable table;
    for(int i=0;i<3;++i) table.AddPos();
    table.pbrNormal={{1,0,0,1},{0,1,0,1},{}};
    table.InterpolateHDR(2,0,1,.25f);
    REQUIRE(table.pbrNormal[2] == std::array<float,4>{.75f,.25f,0,1});
    table.pbrNormal[1][3]=2;
    table.InterpolateHDR(2,0,1,.25f);
    REQUIRE(table.pbrNormal[2] == std::array<float,4>{});
    table.ReleaseTables();
    REQUIRE(table.pbrNormal.empty());
}

TEST_CASE("Wet response excludes unclassified and vegetated stock surfaces", "[Graphics][vulkan-shape]")
{
    for (auto surface : {"default", "tn??????", "tt??????", "tv??????", "pl??????", "more_anim*", "podlaha_prkna*", "vehicle"})
        REQUIRE(vk::WetSurface(surface) == std::array<float, 3>{});
    REQUIRE(vk::WetSurface("asfalt*")[1] > vk::WetSurface("pi??????")[1]);
    REQUIRE(vk::WetSurface("sh??????")[0] > 0);
    REQUIRE(vk::WetSurface("cesta*")[0] < .2f);
}

TEST_CASE("Spotlight direct separation preserves the prepared cone and excludes ambient",
          "[Graphics][vulkan-shape][local-shadows]")
{
    LightReflector light(nullptr, Color(.8f, .6f, .4f), HBlack, .2f);
    light.SetPosition(VZero);
    light.Prepare(MIdentity);
    TLMaterial material;
    material.diffuse = Color(.7f, .5f, .3f);
    material.forcedDiffuse = Color(.2f, .1f, .05f);
    material.ambient = Color(.4f, .3f, .2f);
    for (float distance : {1.f, 100.f, 300.f, 2001.f})
        for (float angle : {0.f, .16f, .20f, .25f, 3.f})
            for (float sign : {-1.f, 1.f})
            {
                const Vector3 point(std::sin(angle) * distance, 0, std::cos(angle) * distance);
                const Vector3 normal(0, 0, sign);
                light.SetAmbient(HBlack);
                light.SetMaterial(material);
                const auto total = light.Apply(point, normal), direct = light.ApplyDirect(point, normal);
                REQUIRE(direct.R() == Catch::Approx(total.R()).margin(1e-6f));
                REQUIRE(direct.G() == Catch::Approx(total.G()).margin(1e-6f));
                REQUIRE(direct.B() == Catch::Approx(total.B()).margin(1e-6f));
                light.SetAmbient(Color(.3f, .2f, .1f));
                light.SetMaterial(material);
                REQUIRE(light.ApplyDirect(point, normal).R() == Catch::Approx(direct.R()).margin(1e-6f));
                REQUIRE(light.Apply(point, normal).R() + 1e-6f >= direct.R());
            }
}

TEST_CASE("Local spotlight projection is light anchored and has perspective depth",
          "[Graphics][vulkan-shape][local-shadows]")
{
    auto project = [](const auto& m, std::array<float, 3> p)
    {
        std::array<float, 4> out{};
        for (int r = 0; r < 4; ++r)
            out[r] = m[r] * p[0] + m[4 + r] * p[1] + m[8 + r] * p[2] + m[12 + r];
        for (int r = 0; r < 3; ++r)
            out[r] /= out[3];
        return out;
    };
    const auto m = vk::LocalShadowMatrix({2, 3, 4}, {0, 0, 1}, .3f, 160);
    REQUIRE(project(m, {2, 3, 4.1f})[2] == Catch::Approx(0).margin(.00002));
    REQUIRE(project(m, {2, 3, 164})[2] == Catch::Approx(1));
    REQUIRE(project(m, {2, 3, 14})[0] == Catch::Approx(0));
    REQUIRE(project(m, {2 + 10 * std::tan(.3f), 3, 14})[0] == Catch::Approx(1));
    const auto moved = vk::LocalShadowMatrix({-18, 8, 34}, {0, 0, 1}, .3f, 160);
    const auto a = project(m, {4, 4, 54}), b = project(moved, {-16, 9, 84});
    for (int i = 0; i < 4; ++i)
        REQUIRE(a[i] == Catch::Approx(b[i]));
    const auto vertical = vk::LocalShadowMatrix({0, 0, 0}, {0, 1, 0}, .3f, 160);
    REQUIRE(project(vertical, {0, 10, 0})[0] == Catch::Approx(0));
    REQUIRE(project(vertical, {0, 10, 0})[1] == Catch::Approx(0));
}

TEST_CASE("Clipping preserves the two selected local contributions independently",
          "[Graphics][vulkan-shape][local-shadows]")
{
    TLVertexTable table;
    table.AddPos();
    table.AddPos();
    table.AddPos();
    table.directLocal = {{{2, 4, 6, 0, 0, 0}}, {{10, 12, 14, 4, 8, 12}}};
    table.InterpolateHDR(2, 0, 1, .25f);
    REQUIRE(table.directLocal[2] == std::array<float, 6>{4, 6, 8, 1, 2, 3});
    table.ReleaseTables();
    REQUIRE(table.directLocal.empty());
}

TEST_CASE("RTAO terrain ambient survives clipping and releases with the TL table", "[Graphics][vulkan-shape][rtao]")
{
    TLVertexTable table;
    for (int i = 0; i < 3; ++i) table.AddPos();
    table.aoAmbient = {{{.2f, .4f, .6f, 1}}, {{.6f, .8f, 1.f, 1}}};
    table.terrainMaterial={{{0,1,0,1,1,.8f,.6f,1}},{{0,1,0,1,.6f,1,.8f,1}}};
    table.InterpolateHDR(2, 0, 1, .25f);
    REQUIRE(table.aoAmbient[2][0] == Catch::Approx(.3f));
    REQUIRE(table.aoAmbient[2][1] == Catch::Approx(.5f));
    REQUIRE(table.aoAmbient[2][2] == Catch::Approx(.7f));
    REQUIRE(table.aoAmbient[2][3] == 1);
    REQUIRE(table.terrainMaterial[2][4] == Catch::Approx(.9f));
    REQUIRE(table.terrainMaterial[2][3] == 1);
    table.terrainMaterial[0][3]=0;
    table.InterpolateHDR(2,0,1,.25f);
    REQUIRE(table.terrainMaterial[2][3] == 0);
    table.ReleaseTables();
    REQUIRE(table.aoAmbient.empty());
    REQUIRE(table.terrainMaterial.empty());
}

#if CWR_HAS_VULKAN
TEST_CASE("RTAO instance coordinates and depth reconstruction agree with raster geometry", "[Graphics][vulkan-shape][rtao]")
{
    Matrix4 model(MRotationY, .7f);
    model.SetOrientation(model.Orientation()*1.7f);
    model.SetPosition(Vector3(7108,75,6076));
    Matrix4 camera(MRotationY, -.4f);
    camera.SetPosition(Vector3(7110,74,6060));
    const Vector3 local(.4f,.2f,.8f);
    const auto world = model.FastTransform(local);
    const auto instance = vk::RayInstanceTransform(model);
    for (int i=0;i<3;++i)
        REQUIRE(instance.matrix[i][0]*local.X()+instance.matrix[i][1]*local.Y()+instance.matrix[i][2]*local.Z()+instance.matrix[i][3]
                == Catch::Approx(world[i]).margin(.001f));
    Matrix4 projection(MZero);
    projection(0,0)=1.3f; projection(1,1)=2.1f; projection(2,2)=1.001f;
    projection.SetPosition(Vector3(0,0,-.1001f));
    const auto mvp=vk::ShapeMVP(camera.InverseScaled()*model,projection);
    float clip[4]{};
    for (int i=0;i<4;++i) clip[i]=mvp[i]*local.X()+mvp[4+i]*local.Y()+mvp[8+i]*local.Z()+mvp[12+i];
    const float depth=clip[2]/clip[3];
    const float z=projection.Position().Z()/(depth-projection(2,2));
    const Vector3 reconstructed(clip[0]/clip[3]/projection(0,0)*z, -clip[1]/clip[3]/projection(1,1)*z,z);
    const auto result=camera.FastTransform(reconstructed);
    REQUIRE(result.Distance2(world)<.0001f);
    // Contact rays operate at actual world input dimensions, including odd
    // FSR/scale sizes. Remove jitter for reconstruction, reapply for lookup.
    for (auto size : {std::array<float, 2>{1920, 1080}, {2560, 1440}, {1281, 721}, {1280, 720}})
        for (float offset : {-.375f, 0.f, .375f})
        {
            const float jx = 2 * offset / size[0], jy = -2 * offset / size[1];
            const float u = (clip[0] / clip[3] + jx) * .5f + .5f;
            const float v = (clip[1] / clip[3] + jy) * .5f + .5f;
            const Vector3 point((u * 2 - 1 - jx) / projection(0, 0) * z,
                                -(v * 2 - 1 - jy) / projection(1, 1) * z, z);
            REQUIRE(camera.FastTransform(point).Distance2(world) < .0001f);
            REQUIRE((point.X() * projection(0, 0) / point.Z() + jx) * .5f + .5f == Catch::Approx(u));
            REQUIRE((-point.Y() * projection(1, 1) / point.Z() + jy) * .5f + .5f == Catch::Approx(v));
        }
}
#endif

TEST_CASE("Render identities survive neither copying nor address reuse", "[Graphics][vulkan-shape][temporal]")
{
    render::RenderIdentity original;
    render::RenderIdentity copy(original);
    REQUIRE(copy.Get() != original.Get());
    const auto before = copy.Get();
    copy = original;
    REQUIRE(copy.Get() != original.Get());
    REQUIRE(copy.Get() != before);
    alignas(render::RenderIdentity) unsigned char storage[sizeof(render::RenderIdentity)];
    auto* a = new(storage) render::RenderIdentity;
    const auto id = a->Get();
    a->~RenderIdentity();
    auto* b = new(storage) render::RenderIdentity;
    REQUIRE(b->Get() != id);
    b->~RenderIdentity();
}

TEST_CASE("Temporal software projection agrees with pixel TL without applying FOV twice", "[Graphics][vulkan-shape][temporal]")
{
    Matrix4 p(MZero);
    p(0,0)=800; p(1,1)=-450; p(0,2)=960; p(1,2)=540;
    p(2,2)=1.001f; p.SetPosition(Vector3(0,0,-.1f));
    const auto m=vk::SoftwareProjection(p,1920,1080);
    Vector3 view(.8f,-.3f,4);
    const float x=(m[0]*view.X()+m[8]*view.Z())/view.Z();
    const float y=(m[5]*view.Y()+m[9]*view.Z())/view.Z();
    REQUIRE(x == Catch::Approx(2*(800*.8f/4+960)/1920-1));
    REQUIRE(y == Catch::Approx(2*(-450*-.3f/4+540)/1080-1));
    REQUIRE(m[11] == 1);
    REQUIRE(m[14] == Catch::Approx(-.1f));
}

TEST_CASE("CPU clipping interpolates previous homogeneous positions", "[Graphics][vulkan-shape][temporal]")
{
    TLVertexTable table;
    table.AddPos(); table.AddPos(); table.AddPos();
    table.previousClip = {{{2,4,6,2}},{{10,12,14,4}}};
    table.InterpolatePrevious(2,0,1,.25f);
    REQUIRE(table.previousClip[2] == std::array<float,4>{4,6,8,2.5f});
    // Interpolate clip coordinates before division, not endpoint UV velocities.
    REQUIRE(table.previousClip[2][0]/table.previousClip[2][3] == Catch::Approx(1.6f));
    table.ReleaseTables();
    REQUIRE(table.previousClip.empty());
}

TEST_CASE("HDR software lighting survives clipping and clears on reuse", "[Graphics][vulkan-shape]")
{
    TLVertexTable table;
    table.AddPos(); table.AddPos(); table.AddPos();
    table.hdrColor = {{{4,2,.5f,1}},{{8,4,1,1}}};
    table.directSun = {{{2, 1, .25f, 1}}, {{4, 2, .5f, 1}}};
    table.InterpolateHDR(2,0,1,.25f);
    REQUIRE(table.hdrColor[2] == std::array<float,4>{5,2.5f,.625f,1});
    REQUIRE(table.directSun[2] == std::array<float, 4>{2.5f, 1.25f, .3125f, 1});
    table.ReleaseTables();
    REQUIRE(table.hdrColor.empty());
    REQUIRE(table.directSun.empty());
}

TEST_CASE("Vulkan sky interpolation matches legacy packed RGB555 uploads", "[Graphics][vulkan-shape]")
{
    REQUIRE(vk::InterpolatesRGB555(PacARGB1555));
    REQUIRE(vk::InterpolatesRGB555(PacP8));
    REQUIRE(vk::InterpolatesRGB555(PacDXT1));
    REQUIRE_FALSE(vk::InterpolatesRGB555(PacAI88));
    REQUIRE_FALSE(vk::InterpolatesRGB555(PacARGB8888));
    PacLevelMem mip;
    mip._w = mip._h = 2;
    mip.SetDestFormat(PacARGB1555, 2);
    for (int a : {0, 8, 64, 128, 248, 255})
        for (int b : {0, 32, 192, 255})
            for (float factor : {0.137931f, 0.5f, 0.655172f})
            {
                const uint16_t pa = uint16_t(0x8000 | ((a >> 3) << 10) | ((a >> 3) << 5) | (a >> 3));
                const uint16_t pb = uint16_t(0x8000 | ((b >> 3) << 10) | ((b >> 3) << 5) | (b >> 3));
                std::array<uint16_t, 4> first{pa, pa, pa, pa}, second{pb, pb, pb, pb};
                mip.Interpolate(first.data(), second.data(), mip, factor);
                REQUIRE(vk::InterpolateRGB555(a, b, int(std::floor(factor * 256))) ==
                        ((first[0] & 31) * 255 + 15) / 31);
                REQUIRE((first[0] & 0x8000) != 0);
            }
}

TEST_CASE("Vulkan stock water separates secondary texture source from shader family", "[Graphics][vulkan-shape]")
{
    // LandscapeRender's current WaterFlags and the captured Infantry section.
    const auto water = render::SplitLegacy(SpecularTexture | NoClamp);
    REQUIRE(vk::SupportedShapeSpec(water));
    REQUIRE(vk::ShapeSecondaryMode(water, true) == 1.f);
    REQUIRE(vk::ShapeSecondaryMode(water, false) == 0.f);
    REQUIRE(vk::ShapeSecondaryMode(render::SplitLegacy(DetailTexture), true) == 1.f);
    REQUIRE(vk::ShapeSecondaryMode(render::SplitLegacy(DetailTexture), false) == 0.f);
    REQUIRE(vk::ShapeSecondaryMode(render::SplitLegacy(0), true) == 0.f);
    REQUIRE(vk::ShapeSecondaryMode(render::SplitLegacy(0), false) == 0.f);
    REQUIRE(vk::ShapeSampler(water) == 0); // Both ordinary water textures repeat.
    const auto alpha = vk::ShapeAlpha(water, AlphaStats::Opaque, 1);
    REQUIRE_FALSE(alpha.blend);
    REQUIRE(alpha.cutoff == 0);
    REQUIRE(alpha.depthTest);
    REQUIRE(alpha.depthWrite);
    // IsWater is a distinct shader family; this fix must not change engine flags
    // or silently advertise support for the as-yet unused explicit Water path.
    const auto explicitWater = render::SplitLegacy(IsWater | SpecularTexture | NoClamp);
    REQUIRE(vk::ShapeSecondaryMode(explicitWater, true) == 2.f);
    REQUIRE(vk::ShapeSecondaryMode(explicitWater, false) == 2.f);
    REQUIRE_FALSE(vk::SupportedShapeSpec(explicitWater));
}

TEST_CASE("Vulkan consecutive screen batches preserve fans attributes and 32-bit indices", "[Graphics][vulkan-shape]")
{
    vk::ScreenBatch batch;
    vk::ScreenVertex quad[4]{};
    for (int i = 0; i < 4; ++i)
    {
        quad[i].position[3] = float(i + 1);
        quad[i].uv[0] = float(i) / 4;
        quad[i].color[3] = float(i) / 8;
        quad[i].fog = float(i) / 16;
    }
    batch.Append(quad);
    batch.Append(std::span(quad, 3));
    REQUIRE(batch.indices == std::vector<uint32_t>{0, 1, 2, 0, 2, 3, 4, 5, 6});
    REQUIRE(batch.vertices[5].position[3] == 2);
    REQUIRE(batch.vertices[5].uv[0] == 0.25f);
    REQUIRE(batch.vertices[5].color[3] == 0.125f);
    REQUIRE(batch.vertices[5].fog == 0.0625f);
    REQUIRE_FALSE(batch.NeedsFlush(batch.state, 4));
    REQUIRE(batch.NeedsFlush(batch.state, vk::ScreenBatch::MaxVertices));
    batch.Clear();
    // An oversized individual fan is legal and must not wrap at 16 bits.
    std::vector<vk::ScreenVertex> large(65538);
    batch.Append(large);
    REQUIRE(batch.indices.back() == 65537);
    REQUIRE(batch.NeedsFlush(batch.state, 3));
}

TEST_CASE("Vulkan screen batches split on state changes and hold immutable texture versions", "[Graphics][vulkan-shape]")
{
    vk::ScreenBatch batch;
    auto version = std::make_shared<int>(1);
    std::weak_ptr<int> lifetime = version;
    batch.state.image = std::shared_ptr<vk::TextureImage>(version, reinterpret_cast<vk::TextureImage*>(version.get()));
    version.reset();
    vk::ScreenVertex triangle[3]{};
    batch.Append(triangle);
    REQUIRE_FALSE(lifetime.expired());
    auto next = batch.state;
    REQUIRE_FALSE(batch.NeedsFlush(next, 3));
    next.waterOverlay = true; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state;
    next.image.reset(); REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; ++next.sampler; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; next.cutoff = 0.5f; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; next.blend = !next.blend; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; next.additive = !next.additive; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; next.depthTest = !next.depthTest; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; next.depthWrite = !next.depthWrite; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; ++next.clip.x; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; ++next.clip.y; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; ++next.clip.width; REQUIRE(batch.NeedsFlush(next, 3));
    next = batch.state; ++next.clip.height; REQUIRE(batch.NeedsFlush(next, 3));
    next.image.reset();
    batch.Clear();
    REQUIRE(lifetime.expired());
    REQUIRE(batch.indices.empty());
    REQUIRE(batch.vertices.empty());
}

TEST_CASE("Vulkan software explicit cutouts override decoded alpha but retain fades", "[Graphics][vulkan-shape]")
{
    for (auto texture : {AlphaStats::Opaque, AlphaStats::Cutout, AlphaStats::Blend})
    {
        const auto cutout = vk::SoftwareAlpha(IsTransparent, texture);
        REQUIRE_FALSE(cutout.blend);
        REQUIRE(cutout.cutoff == Catch::Approx(192.f / 255));
        REQUIRE(cutout.depthWrite);
        for (int fade : {IsAlpha, IsAlphaFog, IsLight})
        {
            const auto blended = vk::SoftwareAlpha(IsTransparent | fade | NoZWrite, texture);
            REQUIRE(blended.blend);
            REQUIRE(blended.cutoff == Catch::Approx(1.f / 255));
            REQUIRE_FALSE(blended.depthWrite);
        }
    }
    REQUIRE(vk::SoftwareAlpha(0, AlphaStats::Cutout).cutoff == 0.5f);
    REQUIRE(vk::SoftwareAlpha(0, AlphaStats::Blend).blend);
}

TEST_CASE("Vulkan native alpha flags distinguish blending cutout and fading", "[Graphics][vulkan-shape]")
{
    const auto opaque = vk::ShapeAlpha(render::SplitLegacy(0), AlphaStats::Opaque, 1);
    REQUIRE_FALSE(opaque.blend);
    REQUIRE(opaque.cutoff == 0);
    const auto cutout = vk::ShapeAlpha(render::SplitLegacy(IsTransparent), AlphaStats::Opaque, 1);
    REQUIRE_FALSE(cutout.blend);
    REQUIRE(cutout.cutoff == Catch::Approx(192.f / 255));
    const auto alpha = vk::ShapeAlpha(render::SplitLegacy(IsAlpha), AlphaStats::Opaque, 1);
    REQUIRE(alpha.blend);
    REQUIRE(alpha.depthWrite); // IsAlpha alone does not imply NoZWrite in GL33.
    REQUIRE(alpha.cutoff == Catch::Approx(1.f / 255));
    REQUIRE(vk::ShapeAlpha(render::SplitLegacy(0), AlphaStats::Blend, 1).blend);
    REQUIRE(vk::ShapeAlpha(render::SplitLegacy(0), AlphaStats::Cutout, 1).cutoff == 0.5f);
    const auto fade = vk::ShapeAlpha(render::SplitLegacy(IsTransparent), AlphaStats::Cutout, 0.25f);
    REQUIRE(fade.blend);
    REQUIRE(fade.cutoff == Catch::Approx(1.f / 255));
    const auto readOnly = vk::ShapeAlpha(render::SplitLegacy(NoZWrite), AlphaStats::Opaque, 1);
    REQUIRE_FALSE(readOnly.blend);
    REQUIRE(readOnly.depthTest);
    REQUIRE_FALSE(readOnly.depthWrite);
    const auto overlay = vk::ShapeAlpha(render::SplitLegacy(NoZBuf | IsAlpha), AlphaStats::Opaque, 1);
    REQUIRE_FALSE(overlay.depthTest);
    REQUIRE_FALSE(overlay.depthWrite);
}

TEST_CASE("Vulkan native normals use inverse transpose and materials use the engine sun", "[Graphics][vulkan-shape]")
{
    Matrix4 model(MIdentity);
    model.SetScale(2, 3, 4);
    model.SetOrientation(Matrix3(MRotationY, 0.7f) * model.Orientation());
    model.SetPosition(Vector3(7, 8, 9));
    vk::ShapeLighting lighting;
    vk::ShapeWorld(lighting, model);
    const Vector3 normal(1, 1, 0), tangent(1, -1, 0);
    Vector3 transformed;
    for (int row = 0; row < 3; ++row)
        transformed[row] = lighting.normal[row] * normal.X() + lighting.normal[4 + row] * normal.Y() + lighting.normal[8 + row] * normal.Z();
    REQUIRE(transformed * model.Rotate(tangent) == Catch::Approx(0).margin(1e-5));
    REQUIRE(lighting.world[12] == 7);
    LightSun sun;
    sun.SetDiffuse(Color(0.2f, 0.4f, 0.6f));
    TLMaterial material;
    material.ambient = Color(0.5f, 0.5f, 0.5f);
    material.diffuse = Color(0.5f, 0.25f, 1);
    material.forcedDiffuse = Color(0.1f, 0.2f, 0.3f);
    material.emmisive = Color(0.1f, 0.2f, 0.3f);
    vk::ShapeMaterial(lighting, material, sun, true);
    REQUIRE(lighting.diffuse[1] == Catch::Approx(0.1f));
    REQUIRE(lighting.ambient[2] == Catch::Approx(sun.Ambient().B() * 0.5f + 0.18f));
    REQUIRE(lighting.emissive[0] == Catch::Approx(0.1f));
    REQUIRE(lighting.ambient[3] == 1);
    REQUIRE(lighting.rtSun[0] == 0); // Material setup never enables the opt-in effect.
    REQUIRE(lighting.rtSun[1] == Catch::Approx(0.02f));
    REQUIRE(lighting.rtSun[2] == Catch::Approx(0.08f));
    REQUIRE(lighting.rtSun[3] == Catch::Approx(0.18f));
    // Forced diffuse belongs to sunlight, but ambient/emissive remain separate.
    REQUIRE(lighting.ambient[2] - lighting.rtSun[3] == Catch::Approx(sun.Ambient().B() * 0.5f));
    vk::ShapeMaterial(lighting, material, sun, false);
    REQUIRE(lighting.ambient[3] == 0);
    REQUIRE(lighting.emissive[0] == Catch::Approx(0.1f));
    REQUIRE(offsetof(vk::ShapeLighting, shadowReceiver) == 768);
    REQUIRE(offsetof(vk::ShapeLighting, shadow) == 784);
    REQUIRE(offsetof(vk::ShapeLighting, previousMVP) == 1104);
    REQUIRE(offsetof(vk::ShapeLighting, temporal) == 1168);
    REQUIRE(offsetof(vk::ShapeLighting, outputColor) == 1200);
    REQUIRE(sizeof(vk::ShapeLighting) == 1600);
    REQUIRE(offsetof(vk::ShapeLighting, materialToWorld) == 1504);
    REQUIRE(offsetof(vk::ShapeLighting, materialAmbient) == 1568);
    REQUIRE(offsetof(vk::ShapeLighting, materialSun) == 1584);
    REQUIRE(offsetof(vk::ShapeLighting, aoReceiver) == 1488);
    REQUIRE(lighting.aoReceiver == std::array<float, 4>{});
    REQUIRE(offsetof(vk::ShapeLighting, pbr) == 1456);
    REQUIRE(offsetof(vk::ShapeLighting, pbrDiffuse) == 1472);
    REQUIRE(lighting.pbr == std::array<float, 4>{});
    REQUIRE(offsetof(vk::ShapeLighting, wetSurface) == 1296);
    REQUIRE(lighting.wetSurface == std::array<float, 4>{});
    REQUIRE(offsetof(vk::ShapeLighting, localShadowMatrices) == 1312);
    REQUIRE(offsetof(vk::ShapeLighting, localShadowControl) == 1440);
    REQUIRE(offsetof(vk::ShapeLighting, rainRipples) == 1280);
    REQUIRE(lighting.rainRipples == std::array<float, 4>{});
    REQUIRE(offsetof(vk::ShapeLighting, rtWater) == 1264);
    REQUIRE(lighting.rtWater == std::array<float, 4>{});
    REQUIRE(offsetof(vk::ShapeLighting, rtSun) == 1248);
    REQUIRE(offsetof(vk::ShapeLighting, atmosphereSun) == 1216);
    REQUIRE(offsetof(vk::ShapeLighting, atmosphereColor) == 1232);
}

TEST_CASE("Vulkan software shadow receivers reconstruct the native camera-relative coordinates", "[Graphics][vulkan-shape]")
{
    // Exercise nontrivial camera rotation and two FOVs. The software stream
    // preserves clip W; depth bias must not move its reconstructed receiver.
    Matrix4 camera(MIdentity);
    camera.SetOrientation(Matrix3(MRotationY, 0.73f) * Matrix3(MRotationX, -0.25f));
    Matrix4 view = camera.InverseScaled();
    for (float focal : {0.8f, 2.1f})
    {
        Matrix4 projection(MZero);
        projection.SetScale(focal, focal * 1.777f, 1.001f);
        projection.SetPosition(Vector3(0, 0, -0.1001f));
        const auto mvp = vk::ShapeMVP(view, projection);
        for (Vector3 point : {Vector3(4, 2, 20), Vector3(-7, -3, 80)})
        {
            float clip[4]{};
            for (int row = 0; row < 4; ++row)
                clip[row] = mvp[row] * point.X() + mvp[4 + row] * point.Y() + mvp[8 + row] * point.Z() + mvp[12 + row];
            const Vector3 eye(clip[0] / focal, -clip[1] / (focal * 1.777f), clip[3]);
            const auto reconstructed = camera.Rotate(eye);
            REQUIRE(reconstructed.X() == Catch::Approx(point.X()).margin(0.0001));
            REQUIRE(reconstructed.Y() == Catch::Approx(point.Y()).margin(0.0001));
            REQUIRE(reconstructed.Z() == Catch::Approx(point.Z()).margin(0.0001));
        }
    }
}

TEST_CASE("Vulkan specular constants preserve material power sun color and disable conditions", "[Graphics][vulkan-shape]")
{
    vk::ShapeLighting lighting;
    LightSun sun;
    sun.SetDiffuse(Color(0.2f, 0.4f, 0.6f));
    TLMaterial material;
    material.specular = Color(0.5f, 0.25f, 1);
    material.specularPower = 32;
    vk::ShapeMaterial(lighting, material, sun, true);
    REQUIRE(lighting.specular == std::array<float, 4>{0.1f, 0.1f, 0.6f, 32});
    REQUIRE(lighting.ambient[3] == 1);
    vk::ShapeMaterial(lighting, material, sun, false);
    REQUIRE(lighting.ambient[3] == 0); // Shader gates both diffuse and specular.
    material.specularPower = 0;
    vk::ShapeMaterial(lighting, material, sun, true);
    REQUIRE(lighting.specular[3] == 0);
    material.specularPower = 32;
    sun.SetDiffuse(HBlack);
    vk::ShapeMaterial(lighting, material, sun, true);
    REQUIRE(lighting.specular == std::array<float, 4>{0, 0, 0, 32});
    material.specular = HBlack;
    sun.SetDiffuse(HWhite);
    vk::ShapeMaterial(lighting, material, sun, true);
    REQUIRE(lighting.specular == std::array<float, 4>{0, 0, 0, 32});
    REQUIRE(offsetof(vk::ShapeLighting, specular) == 176);
    REQUIRE(offsetof(vk::ShapeLighting, fogParams) == 192);
    REQUIRE(offsetof(vk::ShapeLighting, localLights) == 256);
}

TEST_CASE("Vulkan screen pipeline keys keep every depth blend combination independent", "[Graphics][vulkan-shape]")
{
    REQUIRE(vk::ScreenPipelineIndex(false, false) == 0);
    REQUIRE(vk::ScreenPipelineIndex(true, false, false) == 6);
    REQUIRE(vk::ScreenPipelineIndex(true, true, false) == 7);
    REQUIRE(vk::ScreenPipelineIndex(false, false, false) == 0);
    REQUIRE(vk::ScreenPipelineIndex(false, true) == 1);
    REQUIRE(vk::ScreenPipelineIndex(false, true, false, true) == 9);
    REQUIRE(vk::ScreenPipelineIndex(true, true, true, true) == 11);
    REQUIRE(vk::ScreenPipelineIndex(true, true, false, true) == 15);
    REQUIRE(vk::ScreenPipelineIndex(true, true, false, false, true) == 23);
    REQUIRE(vk::ScreenPipelineIndex(true, true, false, true, true) == 31);
    REQUIRE(vk::ScreenPipelineIndex(true, false) == 2);
    REQUIRE(vk::ScreenPipelineIndex(true, true) == 3);
}

TEST_CASE("Vulkan fog constants preserve scene range color and disabled state", "[Graphics][vulkan-shape]")
{
    vk::ShapeLighting lighting;
    vk::ShapeFog(lighting, 90, 300, Color(0.2f, 0.4f, 0.6f), true);
    REQUIRE(lighting.fogParams[0] == 90);
    REQUIRE(lighting.fogParams[1] == Catch::Approx(1.f / 210));
    REQUIRE(lighting.fogParams[2] == 1);
    REQUIRE(lighting.fogColor[2] == Catch::Approx(0.6f));
    vk::ShapeFog(lighting, 180, 600, Color(0.3f, 0.5f, 0.7f), false);
    REQUIRE(lighting.fogParams[1] == Catch::Approx(1.f / 420));
    REQUIRE(lighting.fogParams[2] == 0);
    vk::ShapeFog(lighting, 0, 0, HWhite, true);
    REQUIRE(lighting.fogParams[1] == 0);
}

TEST_CASE("Vulkan local lights retain engine attenuation and camera-relative material response", "[Graphics][vulkan-shape]")
{
    LightDescription light;
    light.type = LTSpotLight;
    light.pos = Vector3(1002, 205, 3007);
    light.dir = Vector3(0, 0, 2);
    light.startAtten = 20;
    light.diffuse = Color(1, 0.5f, 0.25f);
    light.ambient = Color(0.2f, 0.3f, 0.4f);
    TLMaterial material;
    material.diffuse = Color(0.5f, 0.5f, 0.5f);
    material.ambient = Color(0.1f, 0.1f, 0.1f);
    vk::ShapeLocalLight packed;
    vk::ShapeLight(packed, light, material, Vector3(1000, 200, 3000), 0.5f);
    REQUIRE(packed.position == std::array<float, 4>{2, 5, 7, 20});
    REQUIRE(packed.direction == std::array<float, 4>{0, 0, 1, 1});
    REQUIRE(packed.diffuse[1] == Catch::Approx(0.125f));
    REQUIRE(packed.ambient[2] == Catch::Approx(0.02f));
    light.type = LTPoint;
    vk::ShapeLight(packed, light, material, VZero, 0);
    REQUIRE(packed.direction[3] == 0);
    REQUIRE(packed.diffuse[0] == 0);
}

TEST_CASE("Vulkan screen packing preserves pixels UV depth reciprocal W and ARGB", "[Graphics][vulkan-shape]")
{
    Vertex2DAbs vertex;
    vertex.x = 600;
    vertex.y = 150;
    vertex.z = 0.75f;
    vertex.w = 0.5f;
    vertex.u = 0.2f;
    vertex.v = 0.8f;
    vertex.color = PackedColor(0x80402010);
    const auto packed = vk::ScreenGeometry(vertex, 800, 600);
    REQUIRE(packed.position[0] == Catch::Approx(1));
    REQUIRE(packed.position[1] == Catch::Approx(-1));
    REQUIRE(packed.position[2] == Catch::Approx(1.5));
    REQUIRE(packed.position[3] == Catch::Approx(2));
    REQUIRE(packed.uv[0] == Catch::Approx(0.2));
    REQUIRE(packed.uv[1] == Catch::Approx(0.8));
    REQUIRE(packed.color[0] == Catch::Approx(64.f / 255));
    REQUIRE(packed.color[1] == Catch::Approx(32.f / 255));
    REQUIRE(packed.color[2] == Catch::Approx(16.f / 255));
    REQUIRE(packed.color[3] == Catch::Approx(128.f / 255));
    REQUIRE(packed.fog == 1); // HUD and ordinary 2D draws never inherit world fog.
    vertex.w = 0;
    REQUIRE_THROWS_AS(vk::ScreenGeometry(vertex, 800, 600), std::invalid_argument);
    vertex.w = -0.0f;
    REQUIRE_THROWS_AS(vk::ScreenGeometry(vertex, 800, 600), std::invalid_argument);
    vertex.w = std::numeric_limits<float>::infinity();
    REQUIRE_THROWS_AS(vk::ScreenGeometry(vertex, 800, 600), std::invalid_argument);
    vertex.w = std::numeric_limits<float>::quiet_NaN();
    REQUIRE_THROWS_AS(vk::ScreenGeometry(vertex, 800, 600), std::invalid_argument);
    vertex.w = 1;
    REQUIRE_THROWS_AS(vk::ScreenGeometry(vertex, 0, 600), std::invalid_argument);
}

TEST_CASE("Software clipped faces upload only referenced vertices without changing triangles", "[Graphics][vulkan-shape]")
{
    std::vector<uint32_t> remap(10000,UINT32_MAX),sources,draw;
    const std::vector<uint32_t> original{8192,9999,17,8192,17,9000};
    vk::CompactSoftwareIndices(original,remap,sources,draw);
    REQUIRE(sources==std::vector<uint32_t>{8192,9999,17,9000});
    REQUIRE(draw==std::vector<uint32_t>{0,1,2,0,2,3});
    for(size_t i=0;i<draw.size();++i) REQUIRE(sources[draw[i]]==original[i]);
    REQUIRE(std::all_of(remap.begin(),remap.end(),[](uint32_t i){return i==UINT32_MAX;}));
    const std::vector<uint32_t> next{17,8192,17};
    vk::CompactSoftwareIndices(next,remap,sources,draw);
    REQUIRE(sources==std::vector<uint32_t>{17,8192});
    REQUIRE(draw==std::vector<uint32_t>{0,1,0});
    const std::vector<uint32_t> invalid{17,10000,8192};
    REQUIRE_THROWS_AS(vk::CompactSoftwareIndices(invalid,remap,sources,draw),std::out_of_range);
    REQUIRE(std::all_of(remap.begin(),remap.end(),[](uint32_t i){return i==UINT32_MAX;}));
}

TEST_CASE("Vulkan screen tracers preserve endpoints behind the eye for homogeneous clipping", "[Graphics][vulkan-shape]")
{
    // Shadow Killer combat crash: a projected tracer crosses the eye plane.
    // Rejecting (or taking abs of) the second reciprocal-W loses valid geometry.
    Vertex2DAbs front, behind;
    front.x = 663.113586f;
    front.y = 564.258362f;
    front.z = 0.979048311f;
    front.w = 0.220856249f;
    behind.x = 614.947754f;
    behind.y = 640.798889f;
    behind.z = 1.04036939f;
    behind.w = -0.419688940f;
    behind.u = 90.4345016f;
    behind.v = 1;
    const auto a = vk::ScreenGeometry(front, 960, 640);
    const auto b = vk::ScreenGeometry(behind, 960, 640);
    REQUIRE(a.position[3] > 0);
    REQUIRE(b.position[3] == Catch::Approx(1 / behind.w));
    REQUIRE(b.position[0] == Catch::Approx((2 * behind.x / 960 - 1) / behind.w));
    REQUIRE(b.position[1] == Catch::Approx((2 * behind.y / 640 - 1) / behind.w));
    REQUIRE(b.position[2] == Catch::Approx(behind.z / behind.w));
    REQUIRE(b.uv[0] == behind.u);
    REQUIRE(b.uv[1] == behind.v);
    // The near-plane intersection has positive W and remains clip-able by GPU.
    const float t = a.position[2] / (a.position[2] - b.position[2]);
    REQUIRE(t > 0);
    REQUIRE(t < 1);
    REQUIRE(a.position[3] + t * (b.position[3] - a.position[3]) > 0);
}

TEST_CASE("Vulkan Shape extraction preserves polygon fans and section ranges", "[Graphics][vulkan-shape]")
{
    Shape shape;
    for (int i = 0; i < 5; ++i)
        shape.AddVertexFast(Vector3(float(i), 2, 3), Vector3(0, 1, 0), 0, 0.25f, 0.75f);
    Poly face;
    face.Init();
    face.SetN(4);
    for (int i = 0; i < 4; ++i)
        face.Set(i, i);
    const auto first = shape.EndFaces();
    shape.AddFace(face);
    const auto second = shape.EndFaces();
    face.SetN(3);
    face.Set(0, 4);
    face.Set(1, 2);
    face.Set(2, 0);
    shape.AddFace(face);
    ShapeSection section;
    section.properties.Init();
    section.material = 0;
    section.beg = first;
    section.end = second;
    shape.AddSection(section);
    section.beg = second;
    section.end = shape.EndFaces();
    shape.AddSection(section);
    auto data = vk::ExtractShapeGeometry(shape);
    REQUIRE(data.indices == std::vector<VertexIndex>{0, 1, 2, 0, 2, 3, 4, 2, 0});
    REQUIRE(data.vertices.size() == 5);
    REQUIRE(data.vertices[4].position[0] == 4);
    REQUIRE(data.vertices[0].normal[1] == -1);
    REQUIRE(data.vertices[0].uv[1] == 0.75f);
    REQUIRE(vk::SelectSections(data.sections, 0, 1).end == 6);
    REQUIRE(vk::SelectSections(data.sections, 1, 2).begin == 6);
    REQUIRE(vk::SelectSections(data.sections, 0, 2).end == 9);
    REQUIRE_THROWS_AS(vk::SelectSections(data.sections, -1, 1), std::out_of_range);
    REQUIRE_THROWS_AS(vk::SelectSections(data.sections, 0, 3), std::out_of_range);
    shape.Face(second).Set(0, 99);
    REQUIRE_THROWS_AS(vk::ExtractShapeGeometry(shape), std::invalid_argument);
}

TEST_CASE("Vulkan Shape extraction rejects empty and unsectioned shapes", "[Graphics][vulkan-shape]")
{
    Shape shape;
    REQUIRE_THROWS_AS(vk::ExtractShapeGeometry(shape), std::invalid_argument);
    for (int i = 0; i < 3; ++i)
        shape.AddVertexFast(Vector3(float(i), 0, 0), VUp, 0, 0, 0);
    Poly face;
    face.Init();
    face.SetN(3);
    face.Set(0, 0);
    face.Set(1, 1);
    face.Set(2, 2);
    shape.AddFace(face);
    REQUIRE_THROWS_AS(vk::ExtractShapeGeometry(shape), std::invalid_argument);
}

TEST_CASE("Vulkan Shape projection preserves transforms and 0..1 depth", "[Graphics][vulkan-shape]")
{
    Matrix4 model(MIdentity);
    model.SetDirectionAndUp(Vector3(1, 0, 0), VUp);
    model.SetPosition(Vector3(2, 3, 7));
    Matrix4 projection(MZero);
    projection(0, 0) = 2;
    projection(1, 1) = 3;
    const float near = 0.5f, far = 100;
    const float q = far / (far - near);
    projection(2, 2) = q;
    projection.SetPosition(Vector3(0, 0, -q * near));
    const auto mvp = vk::ShapeMVP(model, projection);
    const Vector3 point(1, 2, 3);
    const Vector3 view = model.FastTransform(point);
    float clip[4]{};
    for (int row = 0; row < 4; ++row)
        clip[row] = mvp[row] * point.X() + mvp[4 + row] * point.Y() + mvp[8 + row] * point.Z() + mvp[12 + row];
    REQUIRE(clip[0] == Catch::Approx(view.X() * 2));
    REQUIRE(clip[1] == Catch::Approx(-view.Y() * 3));
    REQUIRE(clip[2] == Catch::Approx(q * view.Z() - q * near));
    REQUIRE(clip[3] == Catch::Approx(view.Z()));
    const auto p = vk::ShapeMVP(MIdentity, projection);
    REQUIRE((p[10] * near + p[14]) / near == Catch::Approx(0).margin(1e-6));
    REQUIRE((p[10] * far + p[14]) / far == Catch::Approx(1));
}

TEST_CASE("Vulkan Shape accepts opaque color but rejects unfinished render states and updates",
          "[Graphics][vulkan-shape]")
{
    render::LegacySpec spec;
    REQUIRE(vk::SupportedShapeSpec(spec));
    spec.routing = render::Routing::IsColored;
    spec.material = render::Material::DisableSun;
    REQUIRE(vk::SupportedShapeSpec(spec));
    spec.backend = render::Backend::NoZWrite;
    REQUIRE(vk::SupportedShapeSpec(spec));
    spec.backend = render::Backend::IsTransparent;
    REQUIRE(vk::SupportedShapeSpec(spec));
    spec.backend = render::Backend::ClampU | render::Backend::PointSampling;
    REQUIRE(vk::ShapeSampler(spec) == 5);
    spec.backend = spec.backend | render::Backend::NoClamp;
    REQUIRE(vk::ShapeSampler(spec) == 4);
    spec.backend = render::Backend::None;
    spec.material = render::Material::IsAnimated;
    REQUIRE(vk::SupportedShapeSpec(spec));
    spec.material = render::Material::None;
    spec.routing = render::Routing::OnSurface;
    REQUIRE_FALSE(vk::SupportedShapeSpec(spec));
    spec.backend = render::Backend::IsShadow | render::Backend::IsAlphaFog | render::Backend::NoZWrite;
    REQUIRE(vk::SupportedShapeSpec(spec));
    spec.routing = render::Routing::IsOnSurface;
    REQUIRE(vk::SupportedShapeSpec(spec));
    REQUIRE_FALSE(vk::NeedsShapeSnapshot(false, false));
    REQUIRE(vk::NeedsShapeSnapshot(true, false));
    REQUIRE(vk::NeedsShapeSnapshot(false, true));
    REQUIRE(vk::NeedsShapeSnapshot(true, true));
}
