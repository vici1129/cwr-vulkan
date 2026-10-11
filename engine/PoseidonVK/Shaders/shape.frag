#version 450
#extension GL_GOOGLE_include_directive : require
#include "shape_uniforms.glsl"
#include "shadow_sample.glsl"
#include "color_space.glsl"
layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in float fogVisibility;
layout(location = 3) in vec3 specularColor;
layout(location = 4) in vec3 shadowWorld;
layout(location = 5) in vec4 previousClip;
layout(location = 6) in vec4 directSun;
#ifdef RT_SUN
layout(location = 2) out vec4 outSun;
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
    vec4 texel = WORLD_TEXTURE(diffuseTexture, texCoord);
    outColor = vec4(texel.rgb * legacyLightToLinear(draw.color.rgb * vertexColor.rgb),
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
    vec3 base = outColor.rgb;
    float csm = sunlightVisibility(shadowWorld, fogVisibility);
    bool separate = lighting.rtSun.x > .5 && directSun.a > .99 && draw.detailEnabled < 1.5;
    if (separate) {
        // Derive the sunlight-only difference in linear HDR, not by multiplying
        // ambient/emissive/local lighting or already-composited fog.
        base = texel.rgb * legacyLightToLinear(draw.color.rgb * max(vertexColor.rgb-directSun.rgb,vec3(0)));
        if(draw.detailEnabled>.5) base *= legacyLightToLinear(vec3(WORLD_TEXTURE(detailTexture,texCoord*32).a*2));
        csm = sunlightVisibility(shadowWorld, 1.0);
        outColor.rgb = base + max(outColor.rgb-base,vec3(0))*csm;
    } else outColor.rgb *= csm;
    // Keep the night-eye response in its calibrated perceptual domain.
    outColor.rgb = nightResponse(outColor.rgb);
    float visibility = clamp(fogVisibility, 0.0, 1.0);
#ifdef RT_SUN
    // Alpha is existing CSM visibility. Later use min(CSM,RT), never CSM*RT.
    outSun = separate ? vec4(max(outColor.rgb-nightResponse(base),vec3(0))*visibility/max(csm,.0001),csm) : vec4(0);
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
    // World tone mapping follows reconstruction; cockpit geometry uses the same
    // curve. Authored 2D UI remains bounded at reference white.
    if (lighting.outputColor.x > .5)
        outColor.rgb = lighting.outputColor.y > .5 ? toneMap(outColor.rgb, lighting.outputColor.z) :
                       min(outColor.rgb, vec3(1));
#ifdef TEMPORAL
    // UV displacement current -> previous (including projection jitter), expected
    // previous depth, validity. Unsupported/translucent fragments overwrite validity.
    bool reactive = lighting.temporal.w > 1.5 || (lighting.temporal.w > .5 && outColor.a < .98);
    motion = vec4(0,0,0,reactive ? -1 : 0);
    if (previousClip.w > 0.0001 && !reactive) {
        vec3 oldNdc = previousClip.xyz / previousClip.w;
        motion = vec4(oldNdc.xy * 0.5 + 0.5 - gl_FragCoord.xy * lighting.temporalExtent.xy,
                      previousClip.w, draw.alphaCutoff > 0.0 ? 0.75 : 1.0);
    }
#endif
}
