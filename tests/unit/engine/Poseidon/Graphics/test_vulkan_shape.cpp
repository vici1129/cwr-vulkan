#include <catch2/catch_test_macros.hpp>
#include <BuildConfig.h>
#if CWR_HAS_VULKAN
#include <PoseidonVK/RayTransformVK.hpp>
#endif
#include <PoseidonVK/ShapeGeometryVK.hpp>
#include <PoseidonVK/ShapeTransformVK.hpp>
#include <PoseidonVK/ScreenGeometryVK.hpp>
#include <PoseidonVK/ScreenPipelineVK.hpp>
#include <PoseidonVK/ShapeLightingVK.hpp>
#include <PoseidonVK/TextureInterpolationVK.hpp>
#include <catch2/catch_approx.hpp>
#include <limits>
#include <PoseidonVK/LocalShadowVK.hpp>
#include <PoseidonVK/WetSurfaceVK.hpp>
#include <PoseidonVK/PBRMaterialVK.hpp>
#include <PoseidonVK/RaySurfaceVK.hpp>
#include <Poseidon/Graphics/Core/TLVertex.hpp>

using namespace Poseidon;

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
    table.InterpolateHDR(2, 0, 1, .25f);
    REQUIRE(table.aoAmbient[2][0] == Catch::Approx(.3f));
    REQUIRE(table.aoAmbient[2][1] == Catch::Approx(.5f));
    REQUIRE(table.aoAmbient[2][2] == Catch::Approx(.7f));
    REQUIRE(table.aoAmbient[2][3] == 1);
    table.ReleaseTables();
    REQUIRE(table.aoAmbient.empty());
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
    REQUIRE(sizeof(vk::ShapeLighting) == 1504);
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
