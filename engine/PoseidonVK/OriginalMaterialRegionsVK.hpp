#pragma once
#include <PoseidonVK/PBRMaterialVK.hpp>
#include <PoseidonVK/EnhancedMaterialVK.hpp>
#include <array>
#include <span>

namespace Poseidon::vk
{
// Additional source evidence for existing triangles, NOT a whole-atlas identity.
// Only triPBR auto consumes these rules. No generated maps or shader masks.
struct OriginalMaterialRegion
{
    unsigned identity, family;
    std::string_view model, texture;
    float roughness, uMin, vMin, uMax, vMax;
    int width, height;
    uint64_t pixels;
    int requiredClamp=0; // Original edge overshoot is only safe with its inspected clamp mode.
    bool reception=true; // Reflection/PBR evidence does not guarantee coherent ambient replacement.
};
inline constexpr OriginalMaterialRegion OriginalMaterialRegions[]{
    {512,6,"data3d\\statek_kulna.p3d","data\\budova2_s.pac", .9f,0,.675f,1,1,256,128,10590596532554886688ull},
    {513,6,"data3d\\statek_kulna.p3d","data\\budova2_s2.pac",.9f,0,.625f,1,1,256,128,11073974456645178822ull},
    {514,6,"data3d\\statek_brana.p3d","data\\budova2_s.pac", .9f,0,.675f,1,1,256,128,10590596532554886688ull},
    // Exact original chapel niche/back/side masonry, not its separate cap or
    // sculptural ornaments. Model proof prevents the photo canvas/background
    // from becoming an unrestricted material source on unrelated geometry.
    {515,6,"data3d\\kaple.p3d","data\\kaple_01_back.pac", .9f,.2f,.01f,.8f,.99f,256,256,5927400689038083574ull},
    {516,6,"data3d\\kaple.p3d","data\\kaple_01_front.pac",.9f,.2f,.01f,.8f,.99f,256,256,8912488960514094014ull},
    {517,6,"data3d\\kaple.p3d","data\\kaple_01_side.pac", .9f,0,.04f,1,1.005f,64,256,13572942148143298778ull},
    // Original barn-family boards and stone/brick, never a whole-atlas identity.
    // Stone fills its entire inspected row, including the original repeated U.
    // Negative wood-edge overshoot additionally requires the original clamp mode.
    {518,4,"o\\hous\\domek_kovarna.p3d","o\\hous\\stodola01.pac",0.85f,-0.003f,-0.003f,0.501f,0.155f,512,512,17838487729387079901ull,49152},
    {519,6,"o\\hous\\domek_sedy.p3d","o\\hous\\stodola01.pac",0.9f,-3.f,0.5f,3.f,1.f,512,512,17838487729387079901ull,0},
    {520,6,"o\\hous\\domek_sedy_bez.p3d","o\\hous\\stodola01.pac",0.9f,-3.f,0.5f,3.f,1.f,512,512,17838487729387079901ull,0},
    {521,6,"o\\hous\\domek_zluty.p3d","o\\hous\\stodola01.pac",0.9f,-3.f,0.5f,3.f,1.f,512,512,17838487729387079901ull,0},
    {522,6,"o\\hous\\domek_zluty_bez.p3d","o\\hous\\stodola01.pac",0.9f,-3.f,0.5f,3.f,1.f,512,512,17838487729387079901ull,0},
    {523,4,"o\\hous\\stodola.p3d","o\\hous\\stodola01.pac",0.85f,0.f,-0.005f,0.501f,0.499f,512,512,17838487729387079901ull,32768},
    {524,6,"o\\hous\\stodola.p3d","o\\hous\\stodola01.pac",0.9f,-3.f,0.5f,3.f,1.f,512,512,17838487729387079901ull,0},
    {525,6,"o\\hous\\stodola.p3d","o\\hous\\stodola01a.pac",0.9f,0.f,0.674f,0.742f,1.f,512,512,18343219702901983844ull,0},
    {526,4,"o\\hous\\stodola2.p3d","o\\hous\\stodola01.pac",0.85f,0.f,-0.005f,0.501f,0.499f,512,512,17838487729387079901ull,32768},
    {527,6,"o\\hous\\stodola2.p3d","o\\hous\\stodola01.pac",0.9f,-3.f,0.5f,3.f,1.f,512,512,17838487729387079901ull,0},
    {528,6,"o\\hous\\stodola2.p3d","o\\hous\\stodola01a.pac",0.9f,0.f,0.674f,0.742f,1.f,512,512,18343219702901983844ull,0},
    // Mixed original board faces retain ambient: a receiver trial produced
    // triangle/step seams against unresolved hardware/wrapped-UV neighbors.
    // Their independently proved PBR and ray-reflection evidence remains valid.
    {529,4,"o\\hous\\stodola3.p3d","o\\hous\\stodola01.pac",0.85f,0.f,-0.005f,0.501f,0.499f,512,512,17838487729387079901ull,32768,false},
    {530,6,"o\\hous\\stodola3.p3d","o\\hous\\stodola01.pac",0.9f,-3.f,0.5f,3.f,1.f,512,512,17838487729387079901ull,0},
    {531,6,"o\\hous\\stodola3.p3d","o\\hous\\stodola01a.pac",0.9f,0.f,0.674f,0.742f,1.f,512,512,18343219702901983844ull,0},
    {532,6,"o\\hous\\zluty_statek.p3d","o\\hous\\stodola01.pac",0.9f,-3.f,0.5f,3.f,1.f,512,512,17838487729387079901ull,0},
    {533,6,"o\\hous\\zluty_statek_in.p3d","o\\hous\\stodola01.pac",0.9f,-3.f,0.5f,3.f,1.f,512,512,17838487729387079901ull,0},
    // Exact original grave-border/headstone back and side stone tiles only.
    // Plaques, inscriptions, soil/grass and photographic canvas stay unknown.
    // All accepted fans preserve complete original faces; original U/V clamp
    // is required. These are not whole-image or all-LOD grave approvals.
    {534,6,"o\\hous\\hrob1.p3d","o\\hous\\nahrobky01.pac",.9f,.503f,.503f,.998f,.998f,512,512,14926218387339521888ull,49152},
    {535,6,"o\\hous\\hrob2.p3d","o\\hous\\nahrobky01.pac",.9f,.503f,.503f,.998f,.998f,512,512,14926218387339521888ull,49152},
    {536,6,"o\\hous\\nahrobek1.p3d","o\\hous\\nahrobky01.pac",.9f,.503f,.503f,.998f,.998f,512,512,14926218387339521888ull,49152},
    {537,6,"o\\hous\\nahrobek2.p3d","o\\hous\\nahrobky01.pac",.9f,.503f,.503f,.998f,.998f,512,512,14926218387339521888ull,49152},
    {538,6,"o\\hous\\nahrobek4.p3d","o\\hous\\nahrobky01.pac",.9f,.503f,.503f,.998f,.998f,512,512,14926218387339521888ull,49152},
    {539,6,"o\\hous\\nahrobek5.p3d","o\\hous\\nahrobky01.pac",.9f,.503f,.503f,.998f,.998f,512,512,14926218387339521888ull,49152}};
struct OriginalRegionGeometry
{
    std::string_view model;
    float resolution;
    int vertices, sections;
    uint64_t fingerprint;
    uint64_t textureBindings=0;
};
inline constexpr OriginalRegionGeometry OriginalRegionGeometries[]{
    {"data3d\\statek_kulna.p3d",.5f,83,6,5710411994966935590ull,9466661084647219132ull},
    {"data3d\\statek_kulna.p3d",1.f,48,6,4372031775053796802ull,1700184925328476168ull},
    {"data3d\\statek_kulna.p3d",3.f,41,5,4806727032941264610ull,1282800507099886727ull},
    {"data3d\\statek_brana.p3d",.5f,198,5,8877832809354361115ull,6883824423279479731ull},
    {"data3d\\statek_brana.p3d",1.f,114,5,1080067108508164804ull,17298411900941076431ull},
    {"data3d\\statek_brana.p3d",1.5f,58,3,8086190560358923383ull,16878692861437147427ull},
    {"data3d\\kaple.p3d",.4f,296,8,12162760974536677195ull,18186419010863367403ull},
    {"data3d\\kaple.p3d",.8f,164,7,6501890606620591884ull,4203960603334158429ull},
    {"data3d\\kaple.p3d",1.6f,118,7,11911249842636768872ull,4203960603334158429ull},
    {"data3d\\kaple.p3d",2.4f,44,4,12542486061275321990ull,12467772214814294321ull},
    {"data3d\\kaple.p3d",4.8f,30,4,6219200114251799554ull,12467772214814294321ull},
    {"o\\hous\\domek_kovarna.p3d",0.5f,2091,8,3173727341161853503ull,3117694670561688717ull},
    {"o\\hous\\domek_kovarna.p3d",1.5f,1179,7,630593750895345530ull,18077243579044112329ull},
    {"o\\hous\\domek_kovarna.p3d",3.f,507,7,17428036638038582214ull,14887194655742379483ull},
    {"o\\hous\\domek_sedy.p3d",0.5f,1880,15,12691296503479137769ull,10714351081534002679ull},
    {"o\\hous\\domek_sedy.p3d",1.5f,1120,15,9468586434256733512ull,3126119381109867655ull},
    {"o\\hous\\domek_sedy.p3d",3.5f,466,13,7972962675013773036ull,7813502529858463645ull},
    {"o\\hous\\domek_sedy.p3d",7.f,216,10,1452695473149682622ull,2061345077978480447ull},
    {"o\\hous\\domek_sedy_bez.p3d",0.5f,998,5,1894312002762255661ull,8075827296508786248ull},
    {"o\\hous\\domek_sedy_bez.p3d",1.5f,546,5,9304956443894179082ull,8075827296508786248ull},
    {"o\\hous\\domek_sedy_bez.p3d",2.25f,198,5,9380546264971115039ull,12093250217865516646ull},
    {"o\\hous\\domek_zluty.p3d",0.5f,2621,18,8582286939701953390ull,12191046242572950343ull},
    {"o\\hous\\domek_zluty.p3d",1.5f,1473,18,4631773550561697489ull,4405137063355517795ull},
    {"o\\hous\\domek_zluty.p3d",2.25f,663,16,13130013541984299542ull,14592640819228009391ull},
    {"o\\hous\\domek_zluty.p3d",3.375f,352,14,1045964688821270480ull,8454130654760369094ull},
    {"o\\hous\\domek_zluty_bez.p3d",0.5f,1488,5,8023935869726233977ull,13912855205116323378ull},
    {"o\\hous\\domek_zluty_bez.p3d",1.5f,740,5,14094888387594786283ull,16191377720879870654ull},
    {"o\\hous\\domek_zluty_bez.p3d",2.25f,500,4,9021668729146944923ull,2197942808308100688ull},
    {"o\\hous\\stodola.p3d",0.5f,983,10,4208822299135802208ull,9836865940232781383ull},
    {"o\\hous\\stodola.p3d",3.5f,582,8,2839320084233116084ull,10776475982071398900ull},
    {"o\\hous\\stodola.p3d",9.f,298,3,13303035623589145014ull,3300911035966363907ull},
    {"o\\hous\\stodola2.p3d",0.5f,1301,13,14800228376042499002ull,11573110536069458975ull},
    {"o\\hous\\stodola2.p3d",1.5f,550,11,14100780980291404840ull,10738673292924993009ull},
    {"o\\hous\\stodola2.p3d",3.f,300,9,1142816792191767574ull,15228613902477028096ull},
    {"o\\hous\\stodola2.p3d",6.f,175,7,18357495451552885005ull,9975022007851211744ull},
    {"o\\hous\\stodola3.p3d",0.5f,1450,3,17477705234234122452ull,12583933808864677075ull},
    {"o\\hous\\stodola3.p3d",1.5f,582,3,9677356035710050879ull,12583933808864677075ull},
    {"o\\hous\\stodola3.p3d",3.5f,328,3,6369575799500251473ull,3300911035966363907ull},
    {"o\\hous\\zluty_statek.p3d",0.5f,2592,6,17116437327258915886ull,5999764286722443489ull},
    {"o\\hous\\zluty_statek.p3d",1.5f,1463,5,5934774355634025011ull,5999764286722443489ull},
    {"o\\hous\\zluty_statek.p3d",3.5f,720,5,5944721980561895669ull,511139636928058893ull},
    {"o\\hous\\zluty_statek.p3d",7.f,342,5,12335834977215397488ull,1466987466348216845ull},
    {"o\\hous\\zluty_statek_in.p3d",0.5f,3869,17,1715232564079771557ull,9265646625243953969ull},
    {"o\\hous\\zluty_statek_in.p3d",1.5f,2256,16,8526395828640554920ull,17349429700551960015ull},
    {"o\\hous\\zluty_statek_in.p3d",3.5f,1163,16,17137202592052439178ull,2579747255288632147ull},
    {"o\\hous\\zluty_statek_in.p3d",7.f,606,13,12894334512326825925ull,5175187356609308597ull},
    {"o\\hous\\zluty_statek_in.p3d",10.5f,328,9,13231501501395638605ull,13513855425676969831ull},
    {"o\\hous\\hrob1.p3d",.5f,56,1,2774244882950967991ull,3899942120700159320ull},
    {"o\\hous\\hrob2.p3d",.5f,62,1,1493411141501128581ull,3899942120700159320ull},
    {"o\\hous\\nahrobek1.p3d",.5f,210,1,11012760105967815471ull,3899942120700159320ull},
    {"o\\hous\\nahrobek2.p3d",.5f,72,1,12145211315437245301ull,3899942120700159320ull},
    {"o\\hous\\nahrobek2.p3d",1.5f,24,1,3951004517269286628ull,3899942120700159320ull},
    {"o\\hous\\nahrobek4.p3d",.5f,56,1,8589466640802520870ull,3899942120700159320ull},
    {"o\\hous\\nahrobek4.p3d",1.5f,24,1,3970343494535995746ull,3899942120700159320ull},
    {"o\\hous\\nahrobek5.p3d",.5f,152,1,5048361025609538717ull,3899942120700159320ull},
    // Exact original float bits 1067966286; rounded tool text 1.3115 is not equal.
    {"o\\hous\\nahrobek5.p3d",1.31150222f,48,1,12748362274169703965ull,3899942120700159320ull}};
inline const OriginalMaterialRegion* OriginalRegion(unsigned identity)
{
    for (const auto& r : OriginalMaterialRegions) if (r.identity==identity) return &r;
    return nullptr;
}
inline unsigned OriginalRegionCandidate(std::string_view model, float lod, int vertices, int sections,
                                        uint64_t fingerprint, std::string_view texture)
{
    // TextureVK supplies its canonical bank key. Fail cheaply for every other
    // source before allocating a model key in the per-frame ray submission path.
    if (!std::any_of(std::begin(OriginalMaterialRegions),std::end(OriginalMaterialRegions),
        [&](const auto& r){return r.texture==texture;})) return 0;
    const auto key=MaterialIdentity(model);
    if (!std::any_of(std::begin(OriginalRegionGeometries),std::end(OriginalRegionGeometries),
        [&](const auto& g) { return g.model==key && g.resolution==lod && g.vertices==vertices &&
                                   g.sections==sections && g.fingerprint==fingerprint; })) return 0;
    for (const auto& r : OriginalMaterialRegions)
        if (r.model==key && r.texture==texture) return r.identity;
    return 0;
}
inline uint64_t OriginalRegionTextureBindings(std::span<const std::string_view> textures)
{
    uint64_t hash=1469598103934665603ull;
    for (const auto name : textures)
    {
        for (unsigned char c : MaterialIdentity(name)) hash=(hash^c)*1099511628211ull;
        hash=(hash^0)*1099511628211ull; // distinguish adjacent resource keys and null entries
    }
    return hash;
}
inline bool OriginalRegionBindingsCompatible(uint64_t geometry, uint64_t textures)
{
    return std::any_of(std::begin(OriginalRegionGeometries),std::end(OriginalRegionGeometries),
        [&](const auto& g){return g.fingerprint==geometry && g.textureBindings && g.textureBindings==textures;});
}
inline bool OriginalRegionUV(unsigned identity, const std::array<std::array<float,2>,3>& triangle)
{
    const auto* region=OriginalRegion(identity);
    if (!region) return false;
    // A convex rectangle contains the ENTIRE original triangle when it contains
    // every corner. No wrapping, clipping a mixed triangle, or edge extrapolation.
    return std::all_of(triangle.begin(),triangle.end(),[&](const auto& uv) {
        return std::isfinite(uv[0]) && std::isfinite(uv[1]) &&
            uv[0]>=region->uMin && uv[0]<=region->uMax && uv[1]>=region->vMin && uv[1]<=region->vMax;
    });
}
inline bool SameOriginalRegionSource(const OriginalMaterialRegion& a, const OriginalMaterialRegion& b)
{
    return a.model==b.model && a.texture==b.texture && a.width==b.width && a.height==b.height && a.pixels==b.pixels;
}
inline bool SameOriginalRegionSource(unsigned a, unsigned b)
{
    const auto* first=OriginalRegion(a); const auto* second=OriginalRegion(b);
    return first && second && SameOriginalRegionSource(*first,*second);
}
inline unsigned SelectOriginalRegion(const OriginalMaterialRegion& anchor,
    const std::array<std::array<float,2>,3>& triangle, int flags,
    std::span<const OriginalMaterialRegion> regions=OriginalMaterialRegions)
{
    unsigned selected=0;
    for (const auto& r : regions)
    {
        if (!SameOriginalRegionSource(anchor,r) || (flags&r.requiredClamp)!=r.requiredClamp) continue;
        if (!std::all_of(triangle.begin(),triangle.end(),[&](const auto& uv) {
            return std::isfinite(uv[0]) && std::isfinite(uv[1]) &&
                uv[0]>=r.uMin && uv[0]<=r.uMax && uv[1]>=r.vMin && uv[1]<=r.vMax;
        })) continue;
        // Overlapping approvals do not choose an arbitrary material by order.
        if (selected) return 0;
        selected=r.identity;
    }
    return selected;
}
inline uint64_t OriginalRegionPixelFingerprint(std::span<const uint8_t> pixels)
{
    uint64_t hash=1469598103934665603ull;
    for (auto byte : pixels) hash=(hash^byte)*1099511628211ull;
    return hash;
}
inline QualifiedMaterial OriginalRegionSource(unsigned identity, std::string_view texture,
                                              int width, int height, uint64_t pixels)
{
    const auto* r=OriginalRegion(identity);
    return r && texture==r->texture && width==r->width && height==r->height && pixels==r->pixels ?
        QualifiedMaterial{r->identity,r->family,r->roughness} : QualifiedMaterial{};
}
} // namespace Poseidon::vk
