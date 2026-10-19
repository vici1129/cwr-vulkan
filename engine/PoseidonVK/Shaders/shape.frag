#version 450
#extension GL_GOOGLE_include_directive : require
#include "shape_uniforms.glsl"
#include "shadow_sample.glsl"
#include "color_space.glsl"
#include "pbr.glsl"
#include "weather.glsl"
#include "rain_ripples.glsl"
#include "material_normal.glsl"
#include "material_record.glsl"
layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in float fogVisibility;
layout(location = 3) in vec3 specularColor;
layout(location = 4) in vec3 shadowWorld;
layout(location = 5) in vec4 previousClip;
layout(location = 6) in vec4 directSun;
layout(location = 7) in vec3 localDirect0;
layout(location = 8) in vec3 localDirect1;
layout(location = 9) in vec3 surfaceNormal;
layout(location = 10) in vec4 aoAmbient;
#ifdef RT_AO
layout(location = 4) out vec4 outAmbient;
#endif
layout(set = 3, binding = 6) uniform sampler2DArray localDepth0;
layout(set = 3, binding = 7) uniform sampler2DArray localDepth1;

float localVisibility(int slot, sampler2DArray map) {
    vec4 clip = lighting.localShadowVP[slot] * vec4(shadowWorld, 1);
    // Evaluate derivatives before divergent frustum exits. The guarded divisor
    // keeps rejected receivers finite without changing any valid projection.
    vec3 p = clip.xyz / max(clip.w, .1);
    vec2 uv = vec2(p.x, -p.y) * .5 + .5;
    vec3 dx=dFdx(vec3(uv,p.z)), dy=dFdy(vec3(uv,p.z));
    if (clip.w <= .1) return 1.0;
    if (p.z <= 0 || p.z >= 1 || any(lessThan(uv,vec2(0))) || any(greaterThan(uv,vec2(1)))) return 1.0;
    // Receiver-plane PCF correction keeps sloping terrain attached without a
    // large constant perspective-depth bias. Bound corrections at silhouettes.
    float det=dx.x*dy.y-dx.y*dy.x;
    vec2 gradient=abs(det)>1e-12 ? vec2(dy.y*dx.z-dx.y*dy.z,dx.x*dy.z-dy.x*dx.z)/det : vec2(0);
    float texel=1.0/lighting.localShadowControl.y;
    float bias=max(2e-7, .006/(clip.w*clip.w));
    float visibility=0;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x) {
        vec2 offset=vec2(x,y)*texel;
        float correction=clamp(dot(gradient,offset),-bias*4,bias*4);
        // Single-level depth maps need no implicit derivatives after the exits.
        visibility += p.z+correction-bias <= textureLod(map,vec3(uv+offset,0),0.0).r ? 1.0 : 0.0;
    }
    float range=lighting.localShadowControl[2+slot];
    return mix(visibility/9.0,1.0,smoothstep(range*.8,range,clip.w));
}
#ifdef RT_SUN
layout(location = 2) out vec4 outSun;
#endif
#ifdef RT_WATER
layout(location = 3) out vec4 outWater;
#endif
#ifdef TEMPORAL
layout(location = 1) out vec4 motion;
#define WORLD_TEXTURE(s, uv) texture(s, uv, lighting.temporalExtent.z)
#else
#define WORLD_TEXTURE(s, uv) texture(s, uv)
#endif
layout(set = 0, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 1, binding = 0) uniform sampler2D detailTexture;
layout(push_constant) uniform ShapeDraw { mat4 mvp; vec4 color; float alphaCutoff; float reserved; float detailEnabled; vec4 lightDirection; } draw;
vec3 nightResponse(vec3 color) {
    vec3 perceptual=toSRGB(color);
    float luminance=max(dot(perceptual,lighting.eyeCoef.rgb),0.0);
    return toLinear(mix(vec3(luminance),perceptual,clamp(luminance+lighting.eyeCoef.a,0.0,1.0)));
}
void main() {
    bool giReceiverReady=false;
    vec3 ambientContribution=vec3(0);
    vec3 waterNormal=vec3(0,1,0), originalWaterNormal=waterNormal;
    if(lighting.rtWater.x>.5) {
        vec3 geometric=normalize(cross(dFdx(shadowWorld),dFdy(shadowWorld)));
        if(geometric.y<0) geometric=-geometric;
        if(lighting.rtWater.x>1.5) {
            // Stock specular_dx is scalar intensity/alpha detail, not XYZ normals.
            // Reflect about the actual animated water geometry. Its detail alpha
            // still modulates raster color below; existing rain slopes follow.
            originalWaterNormal=geometric;
        } else {
            vec3 bump=WORLD_TEXTURE(detailTexture,texCoord+lighting.rtWater.yz).xyz*2-1;
            originalWaterNormal=normalize(geometric+vec3(bump.y,0,bump.x)*.06);
        }
        waterNormal=originalWaterNormal;
        if(lighting.rainRipples.x>0) {
            vec2 slope=rainSlope(shadowWorld);
            if(dot(slope,slope)>0)
                waterNormal=normalize(originalWaterNormal+vec3(slope.x,0,slope.y));
        }
    }
    vec4 texel = WORLD_TEXTURE(diffuseTexture, texCoord);
    vec3 materialN=surfaceNormal;
    float materialRoughness=lighting.pbr.x;
    if(lighting.pbr.w>.5 && (lighting.materialAmbient.w>.5 || lighting.materialSun.w>.5)) {
        vec4 map=WORLD_TEXTURE(detailTexture,texCoord);
        if(lighting.materialAmbient.w>.5) {
        mat3 model=mat3(lighting.materialToWorld), worldToModel=inverse(model);
        vec3 objectNormal=normalize(transpose(model)*surfaceNormal);
        vec3 mapped=materialNormal(objectNormal,worldToModel*dFdx(shadowWorld),worldToModel*dFdy(shadowWorld),
            dFdx(texCoord),dFdy(texCoord),normalize(map.xyz*2-1));
        materialN=normalize(transpose(worldToModel)*mapped);
        }
        if(lighting.materialSun.w>.5) materialRoughness=clamp(map.a*materialRoughness,.045,1);
    }
    vec3 base, contactBase;
    bool captureSun, separate;
    float csm;
    if (lighting.pbr.x > 0.0) {
        int mask=int(lighting.localShadowControl.x);
        vec2 localShadow=vec2(1);
        if((mask & 1)!=0) localShadow.x=localVisibility(0,localDepth0);
        if((mask & 2)!=0) localShadow.y=localVisibility(1,localDepth1);
        vec3 sun;
        pbrLighting(texel.rgb,draw.color.rgb,shadowWorld,materialN,localShadow,materialRoughness,base,sun);
        captureSun=lighting.rtSun.x>.5;
        // PBR always separates sunlight, including CSM-only and contact-only.
        separate=true;
        csm=sunlightVisibility(shadowWorld,1.0);
        sun*=cloudSunlight(shadowWorld+weather.camera.xyz);
        outColor=vec4(base+sun*csm,texel.a*draw.color.a);
        contactBase=base;
#ifdef RT_AO
        ambientContribution=pbrAmbient(texel.rgb,draw.color.rgb);
#endif
    } else {
    vec3 litColor = vertexColor.rgb;
    int localMask = int(lighting.localShadowControl.x);
    if ((localMask & 1) != 0) litColor -= localDirect0 * (1.0-localVisibility(0,localDepth0));
    if ((localMask & 2) != 0) litColor -= localDirect1 * (1.0-localVisibility(1,localDepth1));
    if (localMask != 0) litColor = max(litColor,vec3(0));
    outColor = vec4(texel.rgb * legacyLightToLinear(draw.color.rgb * litColor),
                    texel.a * draw.color.a * vertexColor.a);
    // Match GL33 TGDetail's 32x UV and PSDetail's alpha modulation.
    if (draw.detailEnabled > 1.5) {
        // Explicit Water shader family, not SpecularTexture alone.
        vec3 bumpNormal = -(WORLD_TEXTURE(detailTexture, texCoord).xyz * 2.0 - 1.0);
        outColor.rgb += toLinear(vec3(max(dot(draw.lightDirection.xyz, bumpNormal), 0.0)));
    } else if (draw.detailEnabled > 0.5) outColor.rgb *= legacyLightToLinear(vec3(WORLD_TEXTURE(detailTexture, texCoord * 32.0).a * 2.0));
    // PSNormal/PSDetail add specular after diffuse/detail, before night-eye and fog.
    if (draw.detailEnabled < 1.5) outColor.rgb += legacyLightToLinear(specularColor);
    if (draw.reserved > 0.5) {
        // Only opaque/cutout world draws use coverage. True transparency retains
        // ordinary blending. Center the coverage ramp on the original cutoff.
        outColor.a = draw.alphaCutoff > 0.0 ?
            clamp((outColor.a-draw.alphaCutoff)/max(fwidth(outColor.a),1.0/255.0)+0.5,0.0,1.0) : 1.0;
    } else if (outColor.a < draw.alphaCutoff) discard;
    float wetDiffuse = 1.0;
    vec3 wetHighlight = vec3(0);
    if (lighting.wetSurface.x > 0) {
        vec3 normal = normalize(cross(dFdx(shadowWorld),dFdy(shadowWorld)));
        if (normal.y < 0) normal = -normal;
        float exposure = smoothstep(.15,.8,normal.y) * smoothstep(0.0,.35,shadowWorld.y-lighting.wetSurface.w);
        wetDiffuse -= lighting.wetSurface.x * exposure;
        vec3 view = normalize(-shadowWorld);
        vec3 halfVector = normalize(view-lighting.sunDirection.xyz);
        float lobe = pow(max(dot(normal,halfVector),0.0),lighting.wetSurface.z);
        float fresnel = .04+.16*pow(1-clamp(dot(normal,view),0.0,1.0),5);
        wetHighlight = legacyLightToLinear(max(directSun.rgb,vec3(0))) *
            (lighting.wetSurface.y*exposure*lobe*(1+fresnel*4));
        outColor.rgb = outColor.rgb*wetDiffuse+wetHighlight;
    }
    base = outColor.rgb;
    captureSun = lighting.rtSun.x > .5 && directSun.a > .99 && draw.detailEnabled < 1.5;
#ifdef RT_AO
    // Extract AO from the same sun-independent base used by shadow subtraction.
    // A difference around sun-lit legacy RGB overlaps the nonlinear sun term.
    vec3 aoBase=captureSun ? max(litColor-directSun.rgb,vec3(0)) : litColor;
    ambientContribution=texel.rgb*max(legacyLightToLinear(draw.color.rgb*aoBase)-
        legacyLightToLinear(draw.color.rgb*max(aoBase-aoAmbient.rgb,vec3(0))),vec3(0));
    if(draw.detailEnabled>.5 && draw.detailEnabled<1.5)
        ambientContribution*=legacyLightToLinear(vec3(WORLD_TEXTURE(detailTexture,texCoord*32).a*2));
#endif
    separate = captureSun && lighting.rtSun.x < 1.5;
    csm = sunlightVisibility(shadowWorld, separate ? 1.0 : fogVisibility);
    if (captureSun) {
        // Derive the sunlight-only difference in linear HDR, not by multiplying
        // ambient/emissive/local lighting or already-composited fog.
        base = texel.rgb * legacyLightToLinear(draw.color.rgb * max(litColor-directSun.rgb,vec3(0)));
        if(draw.detailEnabled>.5) base *= legacyLightToLinear(vec3(WORLD_TEXTURE(detailTexture,texCoord*32).a*2));
        base *= wetDiffuse;
    }
    if (separate) {
        outColor.rgb = base + max(outColor.rgb-base,vec3(0))*csm*cloudSunlight(shadowWorld+weather.camera.xyz);
    } else {
        outColor.rgb *= csm;
        // Blended road edges do not enter the separate sunlight MRT. Their new
        // sheen still follows cloud coverage and the existing CSM visibility.
        if (lighting.wetSurface.x > 0)
            outColor.rgb -= wetHighlight*csm*(1-cloudSunlight(shadowWorld+weather.camera.xyz));
    }
    // Contact-only capture must not change legacy CSM shading where no ray
    // hits. Preserve its original scene result, including ambient and fog.
    contactBase = base*csm;
#ifdef RT_AO
    ambientContribution*=wetDiffuse*(separate?1:csm);
#endif
    if(lighting.rtWater.x>.5 && lighting.rainRipples.x>0) {
        // Broad, bounded response on the existing lit water (also without RT).
        // Use only the change in normal: no new light, gloss lobe or Fresnel term.
        vec3 view=normalize(-shadowWorld);
        float response=dot(waterNormal-originalWaterNormal,view);
        float modulation=1+clamp(response*3.0,-.22,.22);
        outColor.rgb *= modulation;
        // Keep any separately captured sunlight a difference of equally shaded
        // colors; the normal response must not relabel ambient as direct sun.
        base *= modulation;
        contactBase *= modulation;
    }
    }
    // Keep the night-eye response in its calibrated perceptual domain.
#ifdef RT_AO
    // Night-eye is nonlinear too: keep the AO difference on that same base.
    // PBR and draws without sunlight capture retain their original reference.
    vec3 aoReference=lighting.pbr.x<=0.0 && captureSun ? (separate ? base : contactBase) : outColor.rgb;
    vec3 ambientRemoved=nightResponse(max(aoReference-ambientContribution,vec3(0)));
    vec3 ambientResponse=nightResponse(aoReference);
#endif
    outColor.rgb = nightResponse(outColor.rgb);
    float visibility = clamp(fogVisibility, 0.0, 1.0);
#ifdef RT_AO
    // Subtract only the captured ambient difference. Direct/local/emissive light
    // and in-scattered fog are never multiplied by the AO visibility.
    outAmbient=lighting.aoReceiver.x>.5 && aoAmbient.a>.99 ?
        vec4(max(ambientResponse-ambientRemoved,vec3(0))*visibility,1.0/gl_FragCoord.w):vec4(0);
    if(lighting.pbr.w>.5 && lighting.aoReceiver.x>.5 && aoAmbient.a>.99)
        outAmbient=vec4(max(nightResponse(base)-nightResponse(max(base-ambientContribution,vec3(0))),vec3(0))*visibility,
            1.0/gl_FragCoord.w);
    // A negative depth reserves this independently captured ambient difference
    // for RTGI replacement. RTAO rejects it; direct/local/emission remain intact.
    giReceiverReady=lighting.rtWater.w>1.5 && outAmbient.a>0 &&
        lighting.pbr.x>0 && lighting.pbr.w>.5 && dot(materialN,materialN)>.5 &&
        length(shadowWorld)<lighting.aoReceiver.y;
    if(giReceiverReady) outAmbient.a=-outAmbient.a;
#endif
#ifdef RT_SUN
    // Alpha is existing CSM visibility. Later use min(CSM,RT), never CSM*RT.
    outSun = captureSun ? vec4(max(outColor.rgb-nightResponse(separate ? base : contactBase),vec3(0))*visibility/max(csm,.0001),csm) : vec4(0);
#endif
    vec3 fogColor = toLinear(lighting.fogColor.rgb);
    if (lighting.atmosphereSun.w > 0.0 && visibility < 1.0) {
        // Tint the in-scattered light of the EXISTING fog. Preserve its original
        // transmittance, mission ranges and full-fog horizon endpoint.
        vec3 ray = shadowWorld / max(length(shadowWorld), 0.001);
        float mu = dot(ray, lighting.atmosphereSun.xyz);
        float forward = pow(max(mu, 0.0), 8.0);
        float luma = dot(fogColor, vec3(.2126,.7152,.0722));
        vec3 sun = toLinear(lighting.atmosphereColor.rgb);
        sun /= max(dot(sun, vec3(.2126,.7152,.0722)), .01);
        vec3 scatter = mix(fogColor * vec3(.78,.94,1.16), luma * sun * 1.3, forward * .65);
        float density = lighting.atmosphereColor.w * exp(-max(shadowWorld.y,0.0) / 2400.0);
        float weight = lighting.atmosphereSun.w * density * smoothstep(0.0,.35,visibility);
        fogColor = mix(fogColor, scatter, weight);
    }
    outColor.rgb = mix(fogColor, outColor.rgb, visibility);
#ifdef RT_WATER
    outWater=vec4(0);
    // Negative view depth tags opaque diffuse GI receivers. RGB is bounded
    // base-texture reflectance times fog transmittance, never lit scene color.
    // Detail/baked lighting and material tint are intentionally not inferred.
    if(giReceiverReady)
        outWater=materialRecord(materialDiffuse(texel.rgb*toLinear(draw.color.rgb),lighting.pbr.w,
            lighting.pbr.y,lighting.pbr.z),materialN,visibility,1.0/gl_FragCoord.w,int(lighting.pbrDiffuse.w));
    // Foam-only rain marker: MIN writes only R; depth, fog and the other normal
    // component survive. Refraction and RT still reject these rain-covered pixels.
    if(draw.reserved<-.5) outWater.x=-2-outColor.a;
    if(lighting.rtWater.x>.5) {
        outWater=vec4(waterNormal.x,waterNormal.z,visibility,1.0/gl_FragCoord.w);
    }
#endif
    // World tone mapping follows reconstruction; cockpit geometry uses the same
    // curve. Authored 2D UI remains bounded at reference white.
    if (lighting.outputColor.x > .5)
        outColor.rgb = lighting.outputColor.y > .5 ? toneMap(outColor.rgb, lighting.outputColor.z) :
                       min(outColor.rgb, vec3(1));
#ifdef TEMPORAL
    // UV displacement current -> previous (including projection jitter), expected
    // previous depth, validity. Unsupported/translucent fragments overwrite validity.
    bool reactive = lighting.rtWater.x>.5 || lighting.temporal.w > 1.5 || (lighting.temporal.w > .5 && outColor.a < .98);
    motion = vec4(0,0,0,reactive ? -1 : 0);
    if (previousClip.w > 0.0001 && !reactive) {
        vec3 oldNdc = previousClip.xyz / previousClip.w;
        motion = vec4(oldNdc.xy * 0.5 + 0.5 - gl_FragCoord.xy * lighting.temporalExtent.xy,
                      previousClip.w, draw.alphaCutoff > 0.0 ? 0.75 : 1.0);
    }
#endif
}
