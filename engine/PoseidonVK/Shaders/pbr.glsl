#include "material_diffuse.glsl"
// Material BRDF: isotropic GGX, separable exact Smith G1,
// Schlick Fresnel and Lambert diffuse weighted by (1-F). No environment term.
// Roughness is perceptual; alpha = roughness^2. Inputs/outputs are linear HDR.
const float pbrPi = 3.141592653589793;
vec3 pbrNormalize(vec3 v) { return v * inversesqrt(max(dot(v,v), 1e-12)); }
float pbrSmith(float NoX, float alpha2) {
    return 2.0*NoX / max(NoX+sqrt(alpha2+(1.0-alpha2)*NoX*NoX), 1e-6);
}
vec3 pbrMaterialBRDF(vec3 albedo, vec3 n, vec3 v, vec3 l, float roughness, float f0, float metallic) {
    float NoL=max(dot(n,l),0.0), NoV=max(dot(n,v),0.0);
    if(NoL<=0.0 || NoV<=0.0) return vec3(0);
    vec3 h=pbrNormalize(l+v);
    float NoH=max(dot(n,h),0.0), VoH=clamp(dot(v,h),0.0,1.0);
    float alpha=roughness*roughness, a2=alpha*alpha;
    float denominator=NoH*NoH*(a2-1.0)+1.0;
    float D=a2/(pbrPi*denominator*denominator);
    float G=pbrSmith(NoL,a2)*pbrSmith(NoV,a2);
    vec3 F0=mix(vec3(f0),albedo,metallic);
    vec3 F=F0+(1.0-F0)*pow(1.0-VoH,5.0);
    return ((1.0-F)*albedo*(1-metallic)/pbrPi + D*G*F/max(4.0*NoL*NoV,1e-6))*NoL;
}
vec3 pbrBRDF(vec3 albedo, vec3 n, vec3 v, vec3 l, float roughness, float f0) {
    return pbrMaterialBRDF(albedo,n,v,l,roughness,f0,0);
}
vec3 pbrAmbient(vec3 texel,vec3 tint) {
    if(lighting.pbr.w>.5)
        return materialDiffuse(texel*toLinear(tint),lighting.pbr.w,lighting.pbr.y,lighting.pbr.z)*
            legacyLightToLinear(lighting.materialAmbient.rgb);
    return texel*legacyLightToLinear(tint)*
        legacyLightToLinear(max(lighting.ambient.rgb-lighting.rtSun.yzw,vec3(0)))*(1-lighting.pbr.y);
}
void pbrLighting(vec3 texel, vec3 tint, vec3 position, vec3 normal, vec2 localVisibility,
                 float roughness, out vec3 base, out vec3 sun) {
    vec3 n=pbrNormalize(normal), v=pbrNormalize(-position);
    vec3 albedo=materialBaseColor(texel*toLinear(tint),lighting.pbr.w);
    float f0=lighting.pbr.y, metallic=lighting.pbr.z;
    // Ambient and emissive keep their authored meaning. Forced diffuse belongs
    // to sunlight, so it is removed from ambient and shadowed with direct sun.
    base=pbrAmbient(texel,tint)+texel*legacyLightToLinear(tint)*legacyLightToLinear(lighting.emissive.rgb);
    // Legacy unit diffuse is a Lambert response, not radiometric irradiance.
    // Multiplying incident light by pi preserves that reference scale while
    // retaining a normalized BRDF. Angular response now operates in linear light.
    // Stock TLMaterial diffuse also contains the engine's over-white object
    // brightness gain (e.g. 1.6). It is a light multiplier, not texture albedo.
    sun=pbrPi*legacyLightToLinear(lighting.pbr.w>.5?lighting.materialSun.rgb:lighting.diffuse.rgb)*
        pbrMaterialBRDF(albedo,n,v,pbrNormalize(-lighting.sunDirection.xyz),roughness,f0,metallic);
    if(lighting.pbr.w<.5) sun+=texel*legacyLightToLinear(tint*lighting.rtSun.yzw)*(1.0-f0);
    for(int i=0;i<int(lighting.localCount.x);++i) {
        ShapeLocalLight light=lighting.localLights[i];
        vec3 toLight=light.position.xyz-position;
        float size2=dot(toLight,toLight), start2=light.position.w*light.position.w;
        if(size2>=start2*100.0) continue;
        float cone=1.0;
        if(light.direction.w>.5) {
            float inside=-dot(toLight,light.direction.xyz);
            if(inside<=0.0) continue;
            float cos2=inside*inside/max(size2,1e-8);
            if(cos2<.95677279) continue;
            cone=clamp((cos2-.95677279)/(.98063081-.95677279),0.0,1.0);
        }
        float attenuation=min(1.0,start2/max(size2,1e-8));
        // Preserve existing engine ambient/falloff/cone semantics; direct
        // diffuse AND specular use the matching local shadow map once.
        vec3 localColor=lighting.pbr.w>.5?materialBaseColor(texel,lighting.pbr.w):texel;
        base+=localColor*legacyLightToLinear(tint*light.ambient.rgb)*(1.0-f0)*(1-metallic)*attenuation*
            (dot(n,toLight)>0.0 ? cone : 1.0);
        float shadow=light.diffuse.w==1.0 ? localVisibility.x :
                     light.diffuse.w==2.0 ? localVisibility.y : 1.0;
        base+=pbrPi*legacyLightToLinear(light.diffuse.rgb)*
            pbrMaterialBRDF(albedo,n,v,pbrNormalize(toLight),roughness,f0,metallic)*attenuation*cone*shadow;
    }
}
