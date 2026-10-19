#include <catch2/catch_test_macros.hpp>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <stddef.h>
#include <catch2/catch_message.hpp>
#include <cctype>
#include <algorithm>
#include <cmath>

namespace RTCoverage
{
using std::abs;
using std::min;
using std::max;
#include <PoseidonVK/Shaders/rtao_coverage.glsl>
}
namespace SoftShadowMath
{
using std::min;
using std::max;
#include <PoseidonVK/Shaders/soft_shadow_math.glsl>
}
namespace SoftRTShadowMath
{
using std::sqrt;
#include <PoseidonVK/Shaders/soft_rt_shadow_math.glsl>
} // namespace SoftRTShadowMath

namespace RayNormalMath
{
// Tiny test adapter: compile the shipped GLSL math, not a second implementation.
struct vec3
{
    float x, y, z;
    vec3 operator*(float s) const { return {x*s,y*s,z*s}; }
    vec3 operator+(vec3 b) const { return {x+b.x,y+b.y,z+b.z}; }
    vec3 operator-() const { return {-x,-y,-z}; }
};
struct vec4
{
    vec3 xyz;
    float w;
    vec4(vec3 v, float s): xyz(v), w(s) {}
};
struct mat3
{
    vec3 rows[3];
    vec3 operator*(vec3 v) const {
        return {rows[0].x*v.x+rows[0].y*v.y+rows[0].z*v.z,
                rows[1].x*v.x+rows[1].y*v.y+rows[1].z*v.z,
                rows[2].x*v.x+rows[2].y*v.y+rows[2].z*v.z};
    }
};
mat3 transpose(mat3 m) {
    return {{{m.rows[0].x,m.rows[1].x,m.rows[2].x},
             {m.rows[0].y,m.rows[1].y,m.rows[2].y},
             {m.rows[0].z,m.rows[1].z,m.rows[2].z}}};
}
float dot(vec3 a, vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
vec3 normalize(vec3 a) { return a*(1/std::sqrt(dot(a,a))); }
bool any(bool b) { return b; }
bool isnan(vec3 a) { return std::isnan(a.x)||std::isnan(a.y)||std::isnan(a.z); }
bool isinf(vec3 a) { return std::isinf(a.x)||std::isinf(a.y)||std::isinf(a.z); }
#include <PoseidonVK/Shaders/ray_normal.glsl>
}

namespace RTGIHistoryMath
{
using std::abs;
using std::min;
using std::max;
#include <PoseidonVK/Shaders/rtgi_history.glsl>
}

TEST_CASE("RTGI history rejects changed surfaces and bounds accumulation", "[Graphics][Shaders][rtgi-history]")
{
    using namespace RTGIHistoryMath;
    REQUIRE(giHistorySurface(10,10,1));
    REQUIRE(giHistorySurface(10.01f,10,.96f));
    REQUIRE_FALSE(giHistorySurface(10.03f,10,1));
    REQUIRE_FALSE(giHistorySurface(10,10,.94f));
    REQUIRE_FALSE(giHistorySurface(0,0,1));
    REQUIRE(giHistorySurface(44.08f,44,1));
    REQUIRE_FALSE(giHistorySurface(44.1f,44,1));
    float length=1;
    for (int frame=0; frame<32; ++frame)
    {
        length=giHistoryLength(length);
        REQUIRE(length<=4);
        REQUIRE(giHistoryWeight(length)<=.75f);
    }
    REQUIRE(length==4);
    REQUIRE(giHistoryWeight(1)==0);
    // Eight distinct deterministic directions; non-temporal phase zero is unchanged.
    REQUIRE(giSampleShift(0)==0);
    REQUIRE(giSampleRotation(0)==0);
    for (int a=0; a<8; ++a) for (int b=a+1; b<8; ++b)
    {
        const float delta=std::abs(giSampleShift(float(a))-giSampleShift(float(b)));
        REQUIRE(std::abs(delta-std::round(delta))>.01f);
    }
    // Packed 14+5+5 bits are exactly representable in the existing float channel.
    for (unsigned z=1; z<16384; ++z)
    {
        unsigned bits=(z<<10)|1023;
        REQUIRE(unsigned(float(bits))==bits);
        REQUIRE((unsigned(std::abs(-float(bits)))>>10)==z);
    }
}

TEST_CASE("Ray shading normals handle inverse transpose, winding and missing normals",
          "[Graphics][Shaders][ray-surface]")
{
    using namespace RayNormalMath;
    const vec3 face{0,1,0}, ray{0,-1,0}, weights{.2f,.3f,.5f};
    const mat3 identity{{{1,0,0},{0,1,0},{0,0,1}}};
    const mat3 scaled{{{.5f,0,0},{0,1,0},{0,0,2}}}; // model scale 2,1,.5
    const vec3 n{1,1,1};
    const auto result=rayShadingNormal(face,n,n,n,weights,scaled,ray);
    const auto expected=normalize(vec3{.5f,1,2});
    REQUIRE(result.w==1);
    REQUIRE(dot(result.xyz,expected)>.99999f);
    // Rotation with reflection/nonuniform scale: model [0,-1,0; -2,0,0; 0,0,.5].
    const mat3 inverse{{{0,-.5f,0},{-1,0,0},{0,0,2}}};
    const auto transformed=rayShadingNormal({-1,0,0},n,n,n,weights,inverse,{1,0,0});
    REQUIRE(transformed.w==1);
    REQUIRE(dot(transformed.xyz,normalize(vec3{-1,-.5f,2}))>.99999f);
    const auto reversed=rayShadingNormal(face,-n,-n,-n,weights,scaled,ray);
    REQUIRE(dot(reversed.xyz,result.xyz)>.99999f);
    const auto back=rayShadingNormal(-face,n,n,n,weights,scaled,-ray);
    REQUIRE(dot(back.xyz,-result.xyz)>.99999f);
    const auto smooth=rayShadingNormal(face,{1,1,0},{0,1,1},{0,1,0},weights,identity,ray);
    REQUIRE(dot(smooth.xyz,normalize(vec3{.2f,1,.3f}))>.99999f);
    for (vec3 invalid : {vec3{0,0,0}, vec3{NAN,1,0}, vec3{INFINITY,1,0}, vec3{1e30f,1,0}})
    {
        const auto fallback=rayShadingNormal(face,invalid,n,n,weights,identity,ray);
        REQUIRE(fallback.w==0);
        REQUIRE(dot(fallback.xyz,face)==1);
    }
    REQUIRE(rayShadingNormal(face,{1,0,0},{1,0,0},{1,0,0},weights,identity,ray).w==0);
    REQUIRE(rayShadingNormal(face,n,n,n,weights,identity,{1,-.1f,0}).w==0);
}

TEST_CASE("RT sun-disc samples are bounded, balanced and contact hardening", "[Graphics][Shaders][ShadowMath]")
{
    using namespace SoftRTShadowMath;
    REQUIRE(std::abs(rtSunDiscSlope() - std::tan(.4f * 3.14159265359f / 180)) < 1e-8f);
    for (float radial : {.00001f, .125f, .5f, .875f, .99999f})
        for (float rotation : {0.f, .7f, 2.4f, 5.9f})
        {
            float x = 0, y = 0, radiusSquared = 0;
            for (int i = 0; i < 4; ++i)
            {
                const float r = rtSunDiskRadius(i, radial), angle = rtSunDiskAngle(i, rotation);
                REQUIRE(r > 0);
                REQUIRE(r < 1);
                REQUIRE((i < 2 ? r * r < .5f : r * r > .5f));
                x += r * std::cos(angle);
                y += r * std::sin(angle);
                radiusSquared += r * r;
                // Geometry, not screen-space kernel width: displacement vanishes at
                // contact and grows linearly with caster/receiver separation.
                const float nearOffset = .1f * rtSunDiscSlope() * r;
                const float farOffset = 10.f * rtSunDiscSlope() * r;
                REQUIRE(std::abs(farOffset - 100 * nearOffset) < 1e-7f);
            }
            REQUIRE(std::abs(x) < 1e-6f);
            REQUIRE(std::abs(y) < 1e-6f);
            REQUIRE(std::abs(radiusSquared * .25f - .5f) < 1e-6f);
        }
}

TEST_CASE("Directional PCSS radius is contact-hardening and cascade-scaled", "[Graphics][Shaders][ShadowMath]")
{
    using SoftShadowMath::softShadowRadius;
    for (float resolution : {512.f,1024.f,2048.f,4096.f})
    {
        const float texel=1/resolution;
        REQUIRE(softShadowRadius(0,.006f,.1f,texel)==texel);
        REQUIRE(softShadowRadius(-1,.006f,.1f,texel)==texel);
        REQUIRE(softShadowRadius(1e6f,.006f,.1f,texel)==8*texel);
        REQUIRE(softShadowRadius(20,.006f,.1f,texel)>=softShadowRadius(5,.006f,.1f,texel));
    }
    // Away from the floor/cap, changing cascade width changes UV radius, not metres.
    REQUIRE(std::abs(softShadowRadius(10,.006f,.02f,1.f/4096)/.02f-.06f)<1e-6f);
    REQUIRE(std::abs(softShadowRadius(10,.006f,.03f,1.f/4096)/.03f-.06f)<1e-6f);
}

// I-28 / B-025 — shader source related-computation co-location.
//
// B-025 was a vertex-shader regression where computing the lighting
// term required `worldNormal` and `viewPos` named locals; a "clean
// up unused intermediates" edit could delete one and the dependent
// arithmetic far below would silently compile against shader globals
// or break in subtle ways.
//
// glslang gives us the strongest enforcement available without
// custom codegen: every shipped GLSL block must parse successfully
// against GL 3.30 core profile rules.  A deletion of `vec3 worldNormal`
// (or `vec4 viewPos`, or any other named local the downstream code
// references) trips the parser with "undeclared identifier" at the
// reference site.  The test compiles each `static const char s_*GLSL[]`
// block in EngineGL33_Shaders.cpp and asserts info-log emptiness.
//
// Limitation: we don't run the linker (no `glslang::TProgram`) —
// individual stages compile, cross-stage `in`/`out` matching is
// not verified here.  That's I-NEW territory if it ever becomes a
// failure mode.

namespace
{
std::string ReadTextFile(const std::filesystem::path& p)
{
    std::ifstream f(p);
    if (!f.is_open())
        return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

struct ShaderBlock
{
    std::string symbol; // e.g. "s_vsTransformGLSL"
    std::string source; // GLSL body between R"( and )"
};

// Extract every `static const char s_*GLSL[] = R"(...)";` block from the
// shaders source file.  The raw-string delimiter is empty (`R"(`) so we
// can match it without needing to know which delimiter token was used.
std::vector<ShaderBlock> ExtractShaderBlocks(const std::string& src)
{
    std::vector<ShaderBlock> blocks;
    static const std::regex header(R"(static\s+const\s+char\s+(s_\w+GLSL)\[\]\s*=\s*R\"\()");
    auto begin = std::sregex_iterator(src.begin(), src.end(), header);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it)
    {
        const std::smatch& m = *it;
        const std::string sym = m[1].str();
        const size_t start = static_cast<size_t>(m.position(0)) + m.length(0);
        const size_t close = src.find(")\"", start);
        if (close == std::string::npos)
            continue;
        blocks.push_back({sym, src.substr(start, close - start)});
    }
    return blocks;
}

EShLanguage StageFromSymbol(const std::string& sym)
{
    // "s_vs..." prefix → vertex stage; everything else (PS, FS) → fragment.
    if (sym.rfind("s_vs", 0) == 0)
        return EShLangVertex;
    return EShLangFragment;
}

struct CompileOutcome
{
    bool success;
    std::string info;
};

CompileOutcome CompileGLSL(const std::string& src, EShLanguage stage)
{
    glslang::TShader shader(stage);
    const char* strings[1] = {src.c_str()};
    shader.setStrings(strings, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientOpenGL, 330);
    shader.setEnvClient(glslang::EShClientOpenGL, glslang::EShTargetOpenGL_450);
    shader.setEnvTarget(glslang::EShTargetNone, glslang::EShTargetSpv_1_0);
    const TBuiltInResource* resources = GetDefaultResources();
    const EShMessages rules = EShMsgDefault;
    const bool ok = shader.parse(resources, 330, /*forwardCompatible*/ false, rules);
    CompileOutcome out;
    out.success = ok;
    out.info = shader.getInfoLog();
    return out;
}

struct GlslangInit
{
    GlslangInit() { glslang::InitializeProcess(); }
    ~GlslangInit() { glslang::FinalizeProcess(); }
};
} // namespace

TEST_CASE("Vulkan AA native-resolution footprint excludes neighboring depth", "[Graphics][Shaders][vulkan-aa]")
{
    const auto path = std::filesystem::path(TESTS_ROOT_DIR).parent_path() /
                      "engine" / "PoseidonVK" / "Shaders" / "aa_composite.frag";
    const auto source = ReadTextFile(path);
    // Pin the shipped expression as well as checking its float32 arithmetic:
    // width * rounded(1/width) can be below 1 (e.g. width 642). Any positive
    // neighboring coverage participates fully in the conservative depth min.
    REQUIRE(std::regex_search(source, std::regex(
        R"(vec2\s+ratio\s*=\s*aa\.metrics\.zw\s*/\s*aa\.outputMetrics\.zw\s*;)")));
    for (int extent : {642, 647, 1080, 1920})
    {
        const float sourceExtent = float(extent), outputExtent = float(extent);
        const float ratio = sourceExtent / outputExtent;
        REQUIRE(ratio == 1.f);
        for (int pixel : {0, 1, 100, extent - 1})
        {
            CAPTURE(extent, pixel);
            const float lo = float(pixel) * ratio, hi = lo + ratio;
            REQUIRE(lo == float(pixel));
            REQUIRE(hi == float(pixel + 1));
        }
    }
}

TEST_CASE("Volumetric clouds match the RTAO world pass without writing its ambient slot", "[Graphics][Shaders][rtao]")
{
    const auto root = std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonVK";
    const auto clouds = ReadTextFile(root / "VulkanContext_Clouds.cpp");
    const auto world = ReadTextFile(root / "VulkanContext_AA.cpp");
    REQUIRE(clouds.find("outputs[]{output, {}, {}, {}, {}}") != std::string::npos);
    REQUIRE(clouds.find("blend.attachmentCount = i ? (_rtaoTargets ? 5 : _rtWaterTargets ? 4 : _rtShadowTargets ? 3 : temporal ? 2 : 1) : 1;") != std::string::npos);
    REQUIRE(world.find("subpass.colorAttachmentCount = _rtaoTargets ? 5 : _rtWaterTargets ? 4 : 3;") != std::string::npos);
    REQUIRE(clouds.find("outputs[4].colorWriteMask") == std::string::npos);
}

TEST_CASE("Legacy RTAO ambient cannot overlap separately subtracted sunlight", "[Graphics][Shaders][rtao]")
{
    const auto root = std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonVK" / "Shaders";
    const auto shape = ReadTextFile(root / "shape.frag");
    REQUIRE(shape.find("vec3 aoBase=captureSun ? max(litColor-directSun.rgb,vec3(0)) : litColor;") != std::string::npos);
    REQUIRE(shape.find("legacyLightToLinear(draw.color.rgb*aoBase)-") != std::string::npos);
    REQUIRE(shape.find("max(aoBase-aoAmbient.rgb,vec3(0))") != std::string::npos);
    REQUIRE(shape.find("vec3 aoReference=lighting.pbr.x<=0.0 && captureSun ? (separate ? base : contactBase) : outColor.rgb;") != std::string::npos);
    REQUIRE(shape.find("nightResponse(max(aoReference-ambientContribution,vec3(0)))") != std::string::npos);
    REQUIRE(shape.find("max(ambientResponse-ambientRemoved,vec3(0))*visibility") != std::string::npos);
    REQUIRE(shape.find("captureSun = lighting.rtSun.x") < shape.find("vec3 aoBase="));

    // Pin the shipped conversion used by this scalar, achromatic counterexample.
    const auto conversion = ReadTextFile(root / "color_space.glsl");
    REQUIRE(conversion.find("pow((c + .055) / 1.055, vec3(2.4))") != std::string::npos);
    REQUIRE(conversion.find("toLinear(min(c, vec3(1))) + max(c - 1., vec3(0))") != std::string::npos);
    const auto linear = [](double c) {
        const double bounded = std::clamp(c, 0., 1.);
        return (bounded <= .04045 ? bounded / 12.92 : std::pow((bounded + .055) / 1.055, 2.4)) + std::max(c - 1., 0.);
    };
    const double oldAmbient = linear(1.) - linear(.8);
    REQUIRE(linear(.2) - .5 * oldAmbient < 0.); // the original black-shadow failure
    for (double local : {0., .1, 1.2})
    for (double sun : {0., .2, .8})
    for (double csm : {0., .4, 1.})
    for (double rt : {0., .5, 1.})
    for (double ao : {0., .5, .55})
    for (double scale : {.3, 1.}) // texture/detail/wetness/fog factors
    {
        const double base = linear(.2 + local);
        const double ambient = base - linear(local);
        const double direct = linear(.2 + local + sun) - base;
        const double result = (base + direct * csm - direct * (csm - std::min(csm, rt)) - ambient * ao) * scale;
        const double expected = (linear(local) + ambient * (1. - ao) + direct * std::min(csm, rt)) * scale;
        REQUIRE(std::abs(result - expected) < 1e-12);
        REQUIRE(result >= -1e-12);
    }
}

TEST_CASE("Night-eye keeps raster ambient and ordinary AO instead of GI replacement", "[Graphics][Shaders][rtgi-order]")
{
    const auto root = std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonVK";
    const auto mesh = ReadTextFile(root / "VulkanContext_Mesh.cpp");
    const auto gate = mesh.find("if (receiver.eyeCoef[3] < 1.f)");
    REQUIRE(gate != std::string::npos);
    REQUIRE(std::regex_search(mesh.substr(gate), std::regex(
        R"(^if \(receiver.eyeCoef\[3\] < 1.f\)\s*receiver.rtWater\[3\] = 0;)")));
    // Disable the tag before GI can force ambient capture independently of RTAO.
    REQUIRE(gate < mesh.find("if (!RTAOEnabled() || !_worldActive"));
    REQUIRE(gate < mesh.find("if (RTGIEnabled() && receiver.rtWater[3]>1.5f"));
    const auto shape = ReadTextFile(root / "Shaders/shape.frag");
    REQUIRE(shape.find("if(lighting.rtWater.w>1.5 && outAmbient.a>0) outAmbient.a=-outAmbient.a;") != std::string::npos);
    REQUIRE(std::regex_search(shape, std::regex(
        R"(if\(lighting.rtWater.w>1.5\)\s*outWater=materialRecord)")));
    // Without either negative-depth tag, RTAO remains eligible and SSAO does not
    // skip this draw. GI reconstruction rejects its empty material record.
    const auto ssao = ReadTextFile(root / "Shaders/ssao.frag");
    REQUIRE(ssao.find("texelFetch(giAmbient,p,0).w<0") != std::string::npos);
    REQUIRE(ssao.find("texelFetch(giAmbient,p,i).w>=0) return false;") != std::string::npos);
    const auto receiver = ReadTextFile(root / "Shaders/rtgi_receiver.glsl");
    REQUIRE(receiver.find("if(!enhancedRecord(s)) return vec4(0);") != std::string::npos);
    REQUIRE(receiver.find("if(!enhancedRecord(s) || d<=0") != std::string::npos);
}

TEST_CASE("RTGI keeps SSAO after shadow subtraction and before bounce addition", "[Graphics][Shaders][rtgi-order]")
{
    const auto root = std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonVK";
    const auto aa = ReadTextFile(root / "VulkanContext_AA.cpp");
    const auto rt = ReadTextFile(root / "VulkanContext_RTAO.cpp");
    // Pin the actual call sites, not only a second implementation of the math.
    const auto finish = aa.find("void VulkanContext::FinishAAWorld(");
    REQUIRE(finish != std::string::npos);
    const auto world = aa.substr(finish);
    REQUIRE(world.find("bool ssaoHandled = false;") != std::string::npos);
    REQUIRE(world.find("DrawRTAO(projection, ssaoHandled);") < world.find("DrawSSAO(projection);"));
    REQUIRE(world.find("if (!RTAOEnabled() && !ssaoHandled)") != std::string::npos);
    REQUIRE(rt.find("for (int effect : {1, 2, 0, 3})") != std::string::npos);
    REQUIRE(std::regex_search(rt, std::regex(
        R"(if\s*\(effect == 3 && !RTAOEnabled\(\)\)\s*\{[\s\S]*?DrawSSAO\(projection\);\s*ssaoHandled = true;\s*\})")));
    REQUIRE(rt.find("bool& ssaoHandled") != std::string::npos); // survives later exceptions

    const float color = 1.f, shadow = .6f;
    for (float ao : {0.f, .5f, 1.f})
    {
        const float off = (color - shadow) * ao;
        for (float bounce : {0.f, .1f})
        {
            const float fixed = (color - shadow) * ao + bounce;
            REQUIRE(std::abs((fixed - off) - bounce) < 1e-6f);
        }
        const float oldZeroBounce = color * ao - shadow;
        REQUIRE(std::abs((oldZeroBounce - off) + shadow * (1.f - ao)) < 1e-6f);
    }
}

TEST_CASE("RTGI filters incident radiance before receiver reflectance with bounded history", "[Graphics][Shaders][rtgi-history]")
{
    const auto root = std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonVK";
    const auto filter = ReadTextFile(root / "Shaders/rtgi_filter.frag");
    const auto resolve = ReadTextFile(root / "Shaders/rtgi_resolve.comp");
    const auto trace = ReadTextFile(root / "Shaders/rtgi.comp");
    const auto host = ReadTextFile(root / "VulkanContext_RTAO.cpp");
    REQUIRE(filter.find("receiver.rgb*sum/weight") != std::string::npos);
    REQUIRE(filter.find("positionAt(other)") == std::string::npos);
    REQUIRE(filter.find("giNormal(other") == std::string::npos);
    REQUIRE(resolve.find("previousRawGI") != std::string::npos);
    REQUIRE(resolve.find("clamp(old,low,high)") != std::string::npos);
    REQUIRE(resolve.find("supported && !unsafe") != std::string::npos);
    REQUIRE(resolve.find("giReceiver") == std::string::npos);
    REQUIRE(trace.find("if(ao.jitter.z>0)") != std::string::npos);
    REQUIRE(trace.find("if(hit.kind==3u) result.a=-result.a;") < trace.find("if(!hit.diffuseEligible)"));
    REQUIRE(host.find("RTGIEnabled() && IsTemporal(_aaMode)") != std::string::npos);
    REQUIRE(host.find("_taaFrame != rt.giLastFrame + 1") != std::string::npos);
    REQUIRE(host.find("if (!_taaValid || lightingChanged") != std::string::npos);
}

TEST_CASE("Post-resolve RT corrections reject mixed MSAA coverage", "[Graphics][Shaders][vulkan-aa]")
{
    using namespace RTCoverage;
    const auto root = std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonVK" / "Shaders";
    const auto sun = ReadTextFile(root / "sunlight_depth.glsl");
    REQUIRE(ReadTextFile(root / "rtao.comp").find("#include \"sunlight_depth.glsl\"") != std::string::npos);
    REQUIRE(ReadTextFile(root / "contact_shadows.frag").find("#include \"sunlight_depth.glsl\"") != std::string::npos);
    const auto depth = ReadTextFile(root / "rtao_depth.glsl");
    const auto filter = ReadTextFile(root / "rtao_filter.frag");
    REQUIRE(sun.find("!rtSameDepth(viewZ(d),z)") != std::string::npos);
    REQUIRE(sun.find("!rtSameLight(direct[channel],other[channel])") != std::string::npos);
    REQUIRE(depth.find("if(s.w<=0 || s.x<-1) return vec4(0);") != std::string::npos);
    REQUIRE(filter.find("vec4 w=waterAt(p);") != std::string::npos);
    REQUIRE(filter.find("if(w.w<=0) {outColor=vec4(0);return;}") != std::string::npos);
    REQUIRE(filter.find("vec4 other=waterAt(q);") != std::string::npos);
    REQUIRE(filter.find("if(other.w<=0) continue;") != std::string::npos);
    REQUIRE(rtSameDepth(10, 10.01f)); // ordinary small slope
    REQUIRE_FALSE(rtSameDepth(10, 11));
    REQUIRE_FALSE(rtSameDepth(0, 0));
    REQUIRE(rtSameLight(.8f, .801f));
    REQUIRE_FALSE(rtSameLight(.8f, 0));
    REQUIRE_FALSE(rtSameLight(.8f, .4f)); // material/CSM discontinuity
    for (int samples : {2, 4, 8})
    {
        CAPTURE(samples);
        for (int coverage = 1; coverage < samples; ++coverage)
        {
            bool coherent = true, fullWater = true;
            for (int sample = 0; sample < samples; ++sample)
            {
                coherent &= rtSameDepth(10, sample < coverage ? 10.f : 20.f);
                fullWater &= sample < coverage;
            }
            REQUIRE_FALSE(coherent);
            REQUIRE_FALSE(fullWater);
            const float resolved = .5f, correction = .8f;
            const float oldSun = resolved - correction;
            const float coveredSun = resolved - correction * float(coverage) / samples;
            REQUIRE(oldSun < coveredSun);
            REQUIRE(resolved - (coherent ? correction : 0.f) == resolved);
            // Reject at composition even when valid filtered neighbors exist.
            const float filteredAlpha = .4f, filteredColor = .6f;
            const float alpha = fullWater ? filteredAlpha : 0.f;
            REQUIRE(filteredColor * alpha + resolved * (1-alpha) == resolved);
        }
    }
}

TEST_CASE("I-28: every shipped GL33 shader compiles cleanly under glslang", "[Graphics][Shaders][I-28]")
{
    GlslangInit init;

    const std::filesystem::path shadersCpp =
        std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonGL33" / "EngineGL33_Shaders.cpp";
    const std::string body = ReadTextFile(shadersCpp);
    REQUIRE_FALSE(body.empty());

    const auto blocks = ExtractShaderBlocks(body);
    REQUIRE(blocks.size() >= 4); // at least VSTransform + PSNormal + VSScreen + PSFlat

    for (const auto& b : blocks)
    {
        CAPTURE(b.symbol);
        const auto outcome = CompileGLSL(b.source, StageFromSymbol(b.symbol));
        CAPTURE(outcome.info);
        REQUIRE(outcome.success);
    }
}

TEST_CASE("I-28: VSTransform exposes worldNormal and viewPos as named locals", "[Graphics][Shaders][I-28]")
{
    // Direct pin: the LIT vertex shader's lighting math depends on
    // `worldNormal` and `viewPos` being declared and computed.  A
    // delete-the-intermediate regression trips the glslang compile
    // above; this test adds a structural sanity check so the names
    // can't drift without an explicit rename touching this list.
    const std::filesystem::path shadersCpp =
        std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonGL33" / "EngineGL33_Shaders.cpp";
    const std::string body = ReadTextFile(shadersCpp);
    REQUIRE(body.find("vec3 worldNormal") != std::string::npos);
    REQUIRE(body.find("vec4 viewPos") != std::string::npos);
}

namespace
{

// Count occurrences of `needle` in `haystack` as a whole identifier
// (boundaries: must not be preceded/followed by alphanumeric or '_').
// This avoids matching `worldNormal` inside `someOtherworldNormal`.
int CountIdentifier(const std::string& haystack, const std::string& needle)
{
    int count = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos)
    {
        const bool startOk =
            pos == 0 || !(std::isalnum(static_cast<unsigned char>(haystack[pos - 1])) || haystack[pos - 1] == '_');
        const size_t endPos = pos + needle.size();
        const bool endOk = endPos >= haystack.size() ||
                           !(std::isalnum(static_cast<unsigned char>(haystack[endPos])) || haystack[endPos] == '_');
        if (startOk && endOk)
            ++count;
        pos += needle.size();
    }
    return count;
}

// Find the GLSL block whose `static const char s_<symbol>GLSL[]` symbol
// matches `symbol`.  Returns the body or empty string if not found.
std::string FindShaderBlock(const std::string& src, const std::string& symbol)
{
    const std::string header = "static const char " + symbol + "[] = R\"(";
    const size_t start = src.find(header);
    if (start == std::string::npos)
        return {};
    const size_t bodyStart = start + header.size();
    const size_t bodyEnd = src.find(")\"", bodyStart);
    if (bodyEnd == std::string::npos)
        return {};
    return src.substr(bodyStart, bodyEnd - bodyStart);
}

} // namespace

TEST_CASE("I-28 (Option E): every named intermediate is declared AND used in the same shader block",
          "[Graphics][Shaders][I-28]")
{
    // The B-025 failure shape: a vertex shader computes
    // `worldNormal = normalize(mat3(world) * normal)` as an
    // intermediate, then the lighting math several lines below
    // references it.  A "clean up unused intermediates" edit could
    // delete the declaration; glslang catches that at compile time
    // because the reference is now undefined — already covered by
    // the compile-everything test above.
    //
    // What that compile test does NOT catch: the *reverse* — a
    // declaration left in place after all uses got deleted.  A dead
    // intermediate keeps the shader compiling but signals that the
    // computation it represents has drifted away from its callers.
    // For named outputs the engine's runtime depends on (the lit-
    // family computes worldNormal / viewPos in VSTransform and the
    // lighting code further down reads them), require BOTH:
    //
    //   (1) declaration site exists in this shader block,
    //   (2) name is referenced at least twice elsewhere in the
    //       same block (one decl + one+ uses; we require 3 total
    //       occurrences to insist the intermediate has at least
    //       one consumer).
    //
    // Splitting these intermediates out of VSTransform into a
    // separate block (without porting the lighting math too) trips
    // this test before any visual regression.
    const std::filesystem::path shadersCpp =
        std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonGL33" / "EngineGL33_Shaders.cpp";
    const std::string src = ReadTextFile(shadersCpp);
    REQUIRE_FALSE(src.empty());

    struct Requirement
    {
        std::string shaderSymbol;
        std::string identifier;
        int minOccurrences;
    };

    const Requirement requirements[] = {
        // VSTransform: B-025's failure class.  worldNormal: declared
        // once + 2 lighting uses (NdotL, NdotH).  viewPos: declared
        // once + 1 use (gl_Position = proj * viewPos).  Pin minimums
        // a hair below current counts to absorb non-semantic edits.
        {"s_vsTransformGLSL", "worldNormal", 3},
        {"s_vsTransformGLSL", "viewPos", 2},
    };

    for (const auto& r : requirements)
    {
        CAPTURE(r.shaderSymbol, r.identifier);
        const std::string block = FindShaderBlock(src, r.shaderSymbol);
        REQUIRE_FALSE(block.empty());
        const int occurrences = CountIdentifier(block, r.identifier);
        CAPTURE(occurrences);
        REQUIRE(occurrences >= r.minOccurrences);
    }
}
