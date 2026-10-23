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
    {539,6,"o\\hous\\nahrobek5.p3d","o\\hous\\nahrobky01.pac",.9f,.503f,.503f,.998f,.998f,512,512,14926218387339521888ull,49152},
    // Original ruin stone rows only. Broad
    // candidate boxes that split an original polygon fan were not admitted.
    // Upper lintel detail, rubble/ground, cornices, windows, doors and recesses
    // remain untyped; neither image is a whole-source material declaration.
    {540,6,"o\\hous\\ruiny_3_dvere.p3d","o\\hous\\zborenina1.pac",.9f,-2.f,.45f,2.f,1.f,512,512,13902399811565099486ull},
    {541,6,"o\\hous\\ruiny_3_prasklina.p3d","o\\hous\\zborenina1.pac",.9f,-2.f,.45f,2.f,1.f,512,512,13902399811565099486ull},
    {542,6,"o\\hous\\ruiny_3_roh.p3d","o\\hous\\zborenina1.pac",.9f,0.f,0.f,.5f,1.f,512,512,13902399811565099486ull},
    {543,6,"o\\hous\\ruiny_obvod_3.p3d","o\\hous\\zborenina1.pac",.9f,-2.f,.45f,2.f,1.f,512,512,13902399811565099486ull},
    // Original vertical chimney brick/mortar only. Frame/photo-border and
    // black aperture faces remain untyped; every original fan stays intact.
    {544,6,"o\\hous\\domek_rosa.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {545,6,"o\\hous\\dum_patr_nizky_prujezd.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {546,6,"o\\hous\\dum_patrovy01.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {547,6,"o\\hous\\dum_patrovy01c.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {548,6,"o\\hous\\dum_patrovy01d.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {549,6,"o\\hous\\dum_patrovy01prujezd.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {550,6,"o\\hous\\dum_patrovy02.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {551,6,"o\\hous\\dum_patrovy03.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {552,6,"o\\hous\\dum_patrovy04.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {553,6,"o\\hous\\dum_patrovy05.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {554,6,"o\\hous\\dum_patrovy06.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {555,6,"o\\hous\\orlhot.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    {556,6,"o\\hous\\skola.p3d","o\\hous\\orlhot_kom.pac",.9f,.22f,0.f,.79f,1.f,256,128,13663165639993867772ull},
    // Exact original clay-tile regions retain ambient after a CPU receiver
    // trial darkened a roof. PBR/reflection proof is independent of reception.
    {557,3,"data3d\\dum_mesto.p3d","data\\tasky_tmavsi90.pac",.9f,-4.f,-.003f,4.f,2.375f,256,256,13546172877813697002ull,0,false},
    {558,3,"data3d\\dum_mesto_in.p3d","data\\tasky_tmavsi90.pac",.9f,-4.f,-.003f,4.f,2.375f,256,256,13546172877813697002ull,0,false},
    {559,3,"data3d\\mesto_dum.p3d","data\\tasky_tmavsi90.pac",.9f,-4.f,-.003f,4.f,2.375f,256,256,13546172877813697002ull,0,false},
    // Lower original mineral portion only; upper photographed arrow slits and
    // every other model/source retain their independent fallback contracts.
    {560,6,"o\\divk\\zedlv.p3d","o\\divk\\zed_lv03.pac",.9f,0.f,.17f,1.f,1.f,512,512,1822267094740399805ull},
    // Exact original timber underside keeps original ambient: its receiver
    // pilot produced a circular GI lighting patch. Existing PBR/reflection
    // evidence remains separate, without a whole-source receiver exception.
    {561,4,"o\\hous\\posed.p3d","o\\hous\\prkno_str.pac",.85f,-.005f,-.005f,1.005f,1.005f,256,256,11743484817861242001ull,0,false},
    // Lower original fountain mineral row, not the upper spout photograph.
    // Original repeated U and tiny V edge overshoot sample inspected mineral
    // pixels only; geometry/binding/image proof and original gates remain.
    {562,6,"o\\hous\\kasna_new.p3d","o\\hous\\kasna01.pac",.9f,-.2f,.5f,2.f,1.006f,256,256,16872738925428872060ull},
    // Exact original Helfenburk mineral rows/side strips only. Slits, windows,
    // arch recess/background and green edge pixels are not whole-source stone.
    // Repeated U stays within a mineral-only row; original sampler flags remain.
    {563,6,"data3d\\helfenburk.p3d","data\\helfenburk_vez_front.pac",.9f,-10,.56f,10,1.005f,256,256,13782917694651118931ull},
    {564,6,"data3d\\helfenburk_cimburi.p3d","data\\helfenburg_hradby_in.pac",.9f,-10,.22f,10,1.005f,128,128,9827691540806335610ull},
    // Outer battlement row565 withdrawn: a diagonal raster-lighting boundary
    // persisted with GI off and disappeared with PBR off. Do not re-enable it
    // from physical stone/whole-face proof alone or defer reception as a fix.
    {566,6,"data3d\\helfenburk_zed.p3d","data\\helfenburg_hradby_in.pac",.9f,-10,.22f,10,1.005f,128,128,9827691540806335610ull},
    {567,6,"data3d\\helfenburk_brana.p3d","data\\helfenburk_brana_ins.pac",.9f,.005f,.39f,.34f,.98f,256,256,12126241923363600779ull},
    {568,6,"data3d\\helfenburk_brana.p3d","data\\helfenburk_brana_ins.pac",.9f,.64f,.39f,.995f,.98f,256,256,12126241923363600779ull},
    // Only the original dry upper beam row, never the green/dark waterline.
    // Full observed repeated U keeps long original beam fans whole; narrower
    // intermediate bounds cut fans and are not a physical material repair.
    {569,4,"data3d\\molo_drevo.p3d","data\\molo_drev_side.pac",.85f,0,0,43,.25f,64,256,3594089815499870373ull,32768},
    // Complete original bridge curb/parapet concrete faces only. Repeated U
    // stays in the mineral row; paving, vegetation, coated rail and lamp/drain
    // hardware retain their original treatment. Exact source and all four LODs.
    {570,7,"o\\hous\\most_stred30.p3d","o\\misc\\most01.pac",.9f,-1,0,9,.24f,512,512,6332993264091036107ull,32768},
    // Nogova family: intact original plaster/balcony/corridor mineral faces.
    // Gable trials571..577 withdrawn: PBR produced a triangular lighting cut
    // on the original06 gable even with GI off. Do not infer acceptance from
    // mineral pixels or intact fans. Window/door/rust boundaries remain legacy.
    // Corridor edge overshoot requires the original ClampU|ClampV.
    {578,5,"o\\hous\\dum_patrovy04.p3d","o\\hous\\dum_patrovy03.pac",.9f,0.f,0.f,0.6f,0.08f,512,512,11610047035768300935ull,0},
    {579,5,"o\\hous\\dum_patrovy04.p3d","o\\hous\\dum_patrovy03.pac",.9f,0.63f,0.f,0.98f,0.34f,512,512,11610047035768300935ull,0},
    {580,5,"o\\hous\\dum_patrovy05.p3d","o\\hous\\dum_patrovy03.pac",.9f,0.63f,0.f,0.98f,0.34f,512,512,11610047035768300935ull,0},
    {581,5,"o\\hous\\dum_patr_nizky_prujezd.p3d","o\\hous\\dum_patrovy_balkon.pac",.9f,0.f,0.f,0.44f,0.47f,256,256,14158396793892771577ull,0},
    {582,5,"o\\hous\\dum_patrovy01.p3d","o\\hous\\dum_patrovy_balkon.pac",.9f,0.f,0.f,0.44f,0.47f,256,256,14158396793892771577ull,0},
    {583,5,"o\\hous\\dum_patrovy01c.p3d","o\\hous\\dum_patrovy_balkon.pac",.9f,0.f,0.f,0.44f,0.47f,256,256,14158396793892771577ull,0},
    {584,6,"o\\hous\\dum_patrovy01c.p3d","o\\hous\\dum_patrovy_balkon.pac",.9f,0.f,0.53f,1.f,1.f,256,256,14158396793892771577ull,0},
    {585,5,"o\\hous\\dum_patrovy01d.p3d","o\\hous\\dum_patrovy_balkon.pac",.9f,0.f,0.f,0.44f,0.47f,256,256,14158396793892771577ull,0},
    {586,5,"o\\hous\\dum_patrovy01prujezd.p3d","o\\hous\\dum_patrovy_balkon.pac",.9f,0.f,0.f,0.44f,0.47f,256,256,14158396793892771577ull,0},
    {587,5,"o\\hous\\dum_patr_nizky_prujezd.p3d","o\\hous\\interier_chodba01.pac",.9f,-0.001f,-0.001f,0.499f,1.001f,512,512,5968244007576842269ull,49152},
    {588,5,"o\\hous\\dum_patrovy01prujezd.p3d","o\\hous\\interier_chodba01.pac",.9f,-0.001f,-0.001f,0.499f,1.001f,512,512,5968244007576842269ull,49152},
    // Batch 32: broad original sheet faces only. A rough dielectric
    // approximation retains photographed corrugation and original ambient.
    // Supports/hardware and all authored lighting stay outside this region.
    {589,0,"o\\misc\\plot_vlnplech1.p3d","o\\hous\\vlnityplot02.pac",.9f,0.f,.5f,1.f,1.f,512,512,13751937356395318224ull,49152,false},
    {590,0,"o\\misc\\plot_vlnplech2.p3d","o\\hous\\vlnityplot02.pac",.9f,0.f,.5f,1.f,1.f,512,512,13751937356395318224ull,49152,false}};
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
    {"o\\hous\\nahrobek5.p3d",1.31150222f,48,1,12748362274169703965ull,3899942120700159320ull},
    {"o\\hous\\ruiny_3_dvere.p3d",1.5f,258,2,6735771090150322089ull,13338746079624542812ull},
    {"o\\hous\\ruiny_3_dvere.p3d",3.f,172,2,3458622000636597664ull,13338746079624542812ull},
    {"o\\hous\\ruiny_3_dvere.p3d",6.90305948f,48,2,5222537571644633332ull,13338746079624542812ull},
    {"o\\hous\\ruiny_3_prasklina.p3d",1.5f,267,2,9425136041773489085ull,13338746079624542812ull},
    {"o\\hous\\ruiny_3_prasklina.p3d",3.f,188,2,12483104871365995009ull,13338746079624542812ull},
    {"o\\hous\\ruiny_3_prasklina.p3d",6.86704254f,58,2,13671049410095702043ull,13338746079624542812ull},
    {"o\\hous\\ruiny_3_roh.p3d",1.5f,249,2,8091803884691766016ull,13338746079624542812ull},
    {"o\\hous\\ruiny_3_roh.p3d",3.f,168,2,10081780795647295214ull,13338746079624542812ull},
    {"o\\hous\\ruiny_3_roh.p3d",7.31359243f,54,2,6998320021019578047ull,13338746079624542812ull},
    {"o\\hous\\ruiny_obvod_3.p3d",1.5f,1002,2,13917087705965509574ull,13338746079624542812ull},
    {"o\\hous\\ruiny_obvod_3.p3d",3.f,554,2,12306342127119174464ull,13338746079624542812ull},
    {"o\\hous\\ruiny_obvod_3.p3d",7.5f,204,2,4482430926070839521ull,13338746079624542812ull},
    {"o\\hous\\domek_rosa.p3d",0.5f,1341,9,4766940278654667272ull,2621986064651652211ull},
    {"o\\hous\\domek_rosa.p3d",2.5f,542,8,6126301004724235929ull,14766250150565662465ull},
    {"o\\hous\\domek_rosa.p3d",5.0f,302,8,8966809442932655929ull,6086800875379550059ull},
    {"o\\hous\\dum_patr_nizky_prujezd.p3d",0.5f,1984,24,7371139912939269092ull,6893535522452921948ull},
    {"o\\hous\\dum_patr_nizky_prujezd.p3d",1.5f,1040,15,11622020468675999632ull,14349974545310629576ull},
    {"o\\hous\\dum_patr_nizky_prujezd.p3d",3.0f,603,21,14489103156787201498ull,14143602095234176502ull},
    {"o\\hous\\dum_patr_nizky_prujezd.p3d",7.0f,275,9,4816099585769955937ull,16513068050143097683ull},
    {"o\\hous\\dum_patrovy01.p3d",0.5f,1847,26,16208797817269213892ull,10330918882316047275ull},
    {"o\\hous\\dum_patrovy01.p3d",1.5f,965,12,8990096099911990986ull,12810193422293638874ull},
    {"o\\hous\\dum_patrovy01.p3d",3.0f,479,10,2848002429159140261ull,3730141026471807598ull},
    {"o\\hous\\dum_patrovy01.p3d",7.0f,287,9,3724513822221776158ull,12798507154092716156ull},
    {"o\\hous\\dum_patrovy01c.p3d",0.5f,2735,40,16801526132355791922ull,2638788643957498999ull},
    {"o\\hous\\dum_patrovy01c.p3d",2.0f,1338,12,12949641602353578528ull,968778699556710360ull},
    {"o\\hous\\dum_patrovy01c.p3d",5.0f,640,11,15558039239549787068ull,12633046275515444616ull},
    {"o\\hous\\dum_patrovy01c.p3d",8.0f,368,9,6712784521269993101ull,13649160615703152060ull},
    {"o\\hous\\dum_patrovy01d.p3d",0.5f,2691,39,11627128549882487150ull,828272213793428857ull},
    {"o\\hous\\dum_patrovy01d.p3d",2.0f,1436,14,3827001080467410347ull,6007913077051720289ull},
    {"o\\hous\\dum_patrovy01d.p3d",5.0f,686,11,12934676624547972480ull,12633046275515444616ull},
    {"o\\hous\\dum_patrovy01d.p3d",8.0f,360,9,13030586583836083369ull,13649160615703152060ull},
    {"o\\hous\\dum_patrovy01prujezd.p3d",0.5f,2955,37,8843464900919703133ull,17122181645591286898ull},
    {"o\\hous\\dum_patrovy01prujezd.p3d",1.5f,1579,24,13254765128877315394ull,16429359707653218892ull},
    {"o\\hous\\dum_patrovy01prujezd.p3d",3.0f,768,22,1895689731536176506ull,9143672427433939934ull},
    {"o\\hous\\dum_patrovy01prujezd.p3d",7.0f,362,9,5390055627195526710ull,16513068050143097683ull},
    {"o\\hous\\dum_patrovy02.p3d",0.5f,2491,14,15260514001715513966ull,3793760837260550675ull},
    {"o\\hous\\dum_patrovy02.p3d",2.0f,1537,13,17987997740667204002ull,14857009486104362329ull},
    {"o\\hous\\dum_patrovy02.p3d",5.0f,657,9,4551978788867973168ull,10074575501271421040ull},
    {"o\\hous\\dum_patrovy02.p3d",7.5f,359,7,4199737176448309439ull,17654320722081775430ull},
    {"o\\hous\\dum_patrovy03.p3d",0.5f,3193,8,6532476756540685780ull,12849285704697550162ull},
    {"o\\hous\\dum_patrovy03.p3d",2.0f,1544,8,12657109821215851448ull,10236752509587821954ull},
    {"o\\hous\\dum_patrovy03.p3d",5.0f,800,8,16727882173283060645ull,14177229178365320098ull},
    {"o\\hous\\dum_patrovy03.p3d",10.0f,205,5,1719620173868156227ull,14234270085720441824ull},
    {"o\\hous\\dum_patrovy04.p3d",0.5f,894,6,9289433092792172653ull,1897807840339953333ull},
    {"o\\hous\\dum_patrovy04.p3d",2.5f,382,6,160751522682648350ull,1202638268288920601ull},
    {"o\\hous\\dum_patrovy04.p3d",7.0f,142,5,13638695270284637335ull,13533314595768495149ull},
    {"o\\hous\\dum_patrovy05.p3d",0.5f,1649,8,5629608807690780177ull,2835217206283440942ull},
    {"o\\hous\\dum_patrovy05.p3d",2.5f,664,8,2610949292099320097ull,3743025206873511360ull},
    {"o\\hous\\dum_patrovy05.p3d",7.0f,223,4,2811528620017172697ull,14597404181971205185ull},
    {"o\\hous\\dum_patrovy06.p3d",0.5f,1282,6,14960422536987656621ull,7060881783187211768ull},
    {"o\\hous\\dum_patrovy06.p3d",2.5f,588,6,1226269835707237199ull,3995846654520154060ull},
    {"o\\hous\\dum_patrovy06.p3d",3.75f,370,5,10780544951580149457ull,15938069131852038028ull},
    {"o\\hous\\dum_patrovy06.p3d",5.625f,303,5,10292220297655288978ull,4519065071232141056ull},
    {"o\\hous\\orlhot.p3d",0.5f,2229,28,8566978176968122901ull,4278773916612406166ull},
    {"o\\hous\\orlhot.p3d",1.0f,1263,25,4499137448809939619ull,7819644273121627546ull},
    {"o\\hous\\orlhot.p3d",1.5f,588,21,1487192299939258462ull,9571517076849082769ull},
    {"o\\hous\\orlhot.p3d",2.25f,354,19,13422170279055447552ull,16378859050744827187ull},
    {"o\\hous\\skola.p3d",0.75f,5131,8,16890318364205732010ull,7945199211564218868ull},
    {"o\\hous\\skola.p3d",2.5f,2593,8,6843024849697546784ull,3873148727259018644ull},
    {"o\\hous\\skola.p3d",5.0f,1414,6,7487344513501232794ull,640562560758088378ull},
    {"o\\hous\\skola.p3d",7.0f,745,7,55823775213719421ull,9549012262321542300ull},
    {"o\\hous\\skola.p3d",10.5f,453,6,8781888987896163311ull,640562560758088378ull},
    {"data3d\\dum_mesto.p3d",0.800000012f,561,9,13229794698948348814ull,3342267452610380588ull},
    {"data3d\\dum_mesto.p3d",1.5f,306,6,18304640444456031118ull,9157510334747740524ull},
    {"data3d\\dum_mesto.p3d",4.5f,111,5,10151047913987740120ull,2862888864409434420ull},
    {"data3d\\dum_mesto.p3d",8.0f,64,5,8644078405580184596ull,13286391708091769522ull},
    {"data3d\\dum_mesto_in.p3d",0.5f,4568,43,15524842651965961501ull,866191006726804034ull},
    {"data3d\\dum_mesto_in.p3d",1.0f,2902,43,17550286268353246670ull,10132856651103183960ull},
    {"data3d\\dum_mesto_in.p3d",2.0f,1671,39,390129015079528947ull,7458023366274878989ull},
    {"data3d\\dum_mesto_in.p3d",4.0f,968,29,8894436870274565806ull,2001253423388972577ull},
    {"data3d\\dum_mesto_in.p3d",8.0f,475,19,7101397086502021948ull,5814390518578910121ull},
    {"data3d\\dum_mesto_in.p3d",12.0f,115,10,18058121945835722342ull,6691408359009747107ull},
    {"data3d\\mesto_dum.p3d",0.800000012f,561,9,3212206315252044389ull,3342267452610380588ull},
    {"data3d\\mesto_dum.p3d",1.5f,306,6,3368909437175227566ull,9157510334747740524ull},
    {"data3d\\mesto_dum.p3d",4.5f,111,5,2075397282267259389ull,2862888864409434420ull},
    {"data3d\\mesto_dum.p3d",8.0f,64,5,4787542041987995589ull,13286391708091769522ull},
    {"o\\divk\\zedlv.p3d",1.5f,1144,7,11729301607590043927ull,7676748547982011476ull},
    {"o\\divk\\zedlv.p3d",3.0f,356,6,14228621904998315830ull,7676748547982011476ull},
    {"o\\divk\\zedlv.p3d",6.5f,133,6,9347651997511948723ull,7676748547982011476ull},
    {"o\\hous\\posed.p3d",1.0f,947,9,12524194718702672150ull,4442514148780103821ull},
    {"o\\hous\\posed.p3d",3.0f,660,9,6697910606867376652ull,11833992726965263909ull},
    {"o\\hous\\posed.p3d",4.5f,303,8,9212267419171645288ull,5951255797030222080ull},
    {"o\\hous\\posed.p3d",6.75f,219,6,9830010683061641721ull,9513662246265850544ull},
    {"o\\hous\\kasna_new.p3d",0.100000001f,868,8,17211535105529068734ull,2516690770884684202ull},
    {"o\\hous\\kasna_new.p3d",2.f,523,8,18009936640925690221ull,2986319403548584685ull},
    {"o\\hous\\kasna_new.p3d",5.f,291,4,18381355984370083630ull,11309712293679337872ull},
    {"o\\hous\\kasna_new.p3d",6.54187679f,190,5,5126656687611659204ull,5978338520408643504ull},
    {"data3d\\helfenburk.p3d",1.5f,2688,7,10398649646306529083ull,7556951306542028860ull},
    {"data3d\\helfenburk.p3d",3.f,1348,7,34708025095817527ull,7556951306542028860ull},
    {"data3d\\helfenburk.p3d",5.f,576,6,8136165685868148456ull,9358811738638422972ull},
    {"data3d\\helfenburk.p3d",8.5f,273,4,110197564143541049ull,9358811738638422972ull},
    {"data3d\\helfenburk.p3d",12.f,60,3,13851135116701149963ull,16519045380345307852ull},
    {"data3d\\helfenburk_brana.p3d",1.70000005f,756,8,17183233117381342167ull,16304420777501984339ull},
    {"data3d\\helfenburk_brana.p3d",3.5f,551,8,17150133038972266937ull,5694354888778463499ull},
    {"data3d\\helfenburk_brana.p3d",8.5f,199,8,12408982878598658395ull,11501598420521712604ull},
    {"data3d\\helfenburk_brana.p3d",11.25f,106,6,4458443391602502506ull,9964032157763713895ull},
    {"data3d\\helfenburk_cimburi.p3d",1.5f,180,3,7745460651206043115ull,7312600183817937085ull},
    {"data3d\\helfenburk_cimburi.p3d",3.f,155,3,4633544958055012547ull,7312600183817937085ull},
    {"data3d\\helfenburk_cimburi.p3d",7.5f,66,3,10833202311971340885ull,7312600183817937085ull},
    {"data3d\\helfenburk_cimburi.p3d",10.f,34,3,776821966535561065ull,10361178586207389736ull},
    {"data3d\\helfenburk_zed.p3d",2.f,52,3,10458519965023346827ull,5077560204557737039ull},
    {"data3d\\helfenburk_zed.p3d",8.f,28,2,17335393911317756424ull,2913505486968009910ull},
    {"data3d\\molo_drevo.p3d",.699999988f,747,6,10131866362269146586ull,17554237083310623026ull},
    {"data3d\\molo_drevo.p3d",1.45000005f,315,5,9622605280816716790ull,3597680008006599372ull},
    {"data3d\\molo_drevo.p3d",3.5f,108,4,13969350428586801093ull,8142896281327530070ull},
    {"data3d\\molo_drevo.p3d",6.5f,80,4,6691579467177444179ull,8142896281327530070ull},
    {"data3d\\molo_drevo.p3d",9.f,24,3,8603565756850670027ull,15411719490152194493ull},
    {"o\\hous\\most_stred30.p3d",1.f,1268,3,14780229580148744717ull,8554578632992421120ull},
    {"o\\hous\\most_stred30.p3d",2.5f,780,4,10009608823202926598ull,2211745915036769752ull},
    {"o\\hous\\most_stred30.p3d",5.f,500,4,5878835632165113038ull,2211745915036769752ull},
    {"o\\hous\\most_stred30.p3d",9.f,198,4,15025897628212485069ull,13194166653877726212ull},
    {"o\\misc\\plot_vlnplech1.p3d",1.5f,56,2,11321681151004873519ull,7355607920613948024ull},
    {"o\\misc\\plot_vlnplech1.p3d",2.5f,40,3,1920637591834022360ull,12904763434220526056ull},
    {"o\\misc\\plot_vlnplech1.p3d",5.68439007f,8,1,6625424391375498599ull,7355607920613948024ull},
    {"o\\misc\\plot_vlnplech2.p3d",1.5f,56,2,7168004884271722351ull,7355607920613948024ull},
    {"o\\misc\\plot_vlnplech2.p3d",2.5f,40,3,16689575607842308744ull,12904763434220526056ull},
    {"o\\misc\\plot_vlnplech2.p3d",5.68439007f,8,1,6625424391375498599ull,7355607920613948024ull}};
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
