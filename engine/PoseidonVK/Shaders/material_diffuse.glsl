#ifndef CWR_MATERIAL_DIFFUSE
#define CWR_MATERIAL_DIFFUSE
// Stock RGB includes painted weathering and baked shading. It is an appearance
// input, not measured albedo. Never brighten/de-light it. Preserve chroma and
// bound its largest linear component to .5 for these rough stock dielectrics.
// This deliberately conservative proxy is shared by raster and GI endpoints.
vec3 materialBaseColor(vec3 artistic, float identity) {
    vec3 c=clamp(artistic,0,1);
    if(identity>.5) c*=min(1.f,.5f/max(max(c.x,c.y),max(c.z,.5f)));
    return c;
}
vec3 materialDiffuse(vec3 artistic,float identity,float f0,float metallic) {
    return materialBaseColor(artistic,identity)*(1-f0)*(1-metallic);
}
vec3 terrainDiffuse(vec3 textureColor,vec3 tintLinear,vec3 detailLinear,float wet) {
    return materialDiffuse(textureColor*tintLinear*detailLinear,1000,.04,0)*wet;
}
#endif
