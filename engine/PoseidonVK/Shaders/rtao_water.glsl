// Closest-hit water reflection shading on the shared AO/sunlight TLAS.
#include "ray_surface.glsl"
#define WEATHER_SET 1
#define CLOUD_VOLUME
#include "weather.glsl"
#include "ray_lighting.glsl"
vec3 eyeResponse(vec3 c) {
    vec3 p=toSRGB(c); float l=max(dot(p,light.eye.rgb),0);
    return toLinear(mix(vec3(l),p,clamp(l+light.eye.a,0,1)));
}
vec4 traceWater(ivec2 pixel) {
    vec4 water=waterAt(pixel);
    if(water.w<=0 || water.z<=.001) return vec4(0);
    vec2 ndc=(vec2(pixel)+.5)/vec2(depthSize())*2-1-ao.jitter.xy;
    vec3 view=vec3(ndc*vec2(1,-1)/ao.projection.xy*water.w,water.w);
    float distance=length(view);
    if(distance>=80) return vec4(0);
    vec3 world=(ao.camera*vec4(view,1)).xyz;
    vec3 incident=normalize(mat3(ao.camera)*view);
    vec3 n=normalize(vec3(water.x,sqrt(max(.01,1-dot(water.xy,water.xy))),water.y));
    if(dot(incident,n)>=0) return vec4(0); // underwater/back face retains ordinary water
    vec3 direction=reflect(incident,n);
    rayQueryEXT query;
    rayQueryInitializeEXT(query,scene,0,255,world+n*.04,.02,direction,70.0);
    while(rayQueryProceedEXT(query)) {
        if(rayQueryGetIntersectionTypeEXT(query,false)==gl_RayQueryCandidateIntersectionTriangleEXT && opaqueHit(query))
            rayQueryConfirmIntersectionEXT(query);
    }
    atomicAdd(groupRays,1);
    // Existing weather sky texture endpoints and the engine's sun-sky lighting lobe.
    vec3 color=eyeResponse(reflectedWeather(world,direction,toLinear(mix(light.horizon.rgb,light.zenith.rgb,sqrt(max(direction.y,0)))+
        light.sunSky.rgb*pow(max(dot(direction,ao.sun.xyz),0),6))));
    if(rayQueryGetIntersectionTypeEXT(query,true)==gl_RayQueryCommittedIntersectionTriangleEXT) {
        atomicAdd(groupHits,1);
        uint custom=rayQueryGetIntersectionInstanceCustomIndexEXT(query,true);
        if((custom>>20)==3) atomicAdd(dynamicHits,1);
        ReflectionSurface surface;
        if(hitDiagnostics) {
            HitSurface detailed=hitSurface(query);
            if(detailed.diffuseEligible) atomicAdd(surfaceEligible,1);
            if((detailed.available&HitNormal)!=0u) {
                if((detailed.available&HitVertexNormal)!=0u) atomicAdd(surfaceVertex,1);
                else atomicAdd(surfaceFace,1);
            }
            if((detailed.available&(HitPosition|HitNormal|HitUV|HitAlbedo))!=(HitPosition|HitNormal|HitUV|HitAlbedo))
                atomicAdd(surfaceUnavailable,1);
            // Cross-check transformed barycentric position against the actual ray.
            // A centimetre tolerance accommodates large stock world coordinates.
            vec3 rayPosition=rayQueryGetWorldRayOriginEXT(query)+direction*rayQueryGetIntersectionTEXT(query,true);
            bool invalidNormal=(detailed.available&HitNormal)!=0u &&
                (!finiteVector(detailed.normal) || abs(dot(detailed.normal,detailed.normal)-1)>.001 ||
                 dot(detailed.normal,direction)>.001 || dot(detailed.normal,detailed.geometricNormal)<.099);
            bool invalidAlbedo=(detailed.available&HitAlbedo)!=0u &&
                (!finiteVector(detailed.albedo) || any(lessThan(detailed.albedo,vec3(0))) ||
                 any(greaterThan(detailed.albedo,vec3(1.001))));
            if((detailed.available&HitPosition)==0u || length(detailed.position-rayPosition)>.01 || invalidNormal || invalidAlbedo)
                atomicAdd(surfaceInvalid,1);
            surface.geometricNormal=detailed.geometricNormal;
            surface.textureColorLinear=detailed.textureColorLinear;
            surface.available=(detailed.available&(HitColor|HitNormal))==(HitColor|HitNormal);
            surface.alphaTest=(detailed.available&HitAlphaTest)!=0u;
        } else {
            surface=reflectionSurface(query);
        }
        if(surface.available) {
            // Preserve the legacy reflection response; future diffuse transport
            // uses hitSurface().normal and .albedo with diffuseEligible.
            vec3 normal=surface.geometricNormal;
            color=surface.textureColorLinear*legacyLightToLinear(light.ambient.rgb+light.diffuse.rgb*max(dot(normal,ao.sun.xyz),0));
            if(weather.sun.w>0) {
                vec3 ambient=surface.textureColorLinear*legacyLightToLinear(light.ambient.rgb);
                color=ambient+max(color-ambient,vec3(0))*cloudSunlight(world+direction*rayQueryGetIntersectionTEXT(query,true));
            }
            float fog=clamp((rayQueryGetIntersectionTEXT(query,true)-light.parameters.x)*light.parameters.y,0,1);
            color=mix(eyeResponse(color),toLinear(light.fog.rgb),fog);
            atomicAdd(shadedHits,1);
            if((custom>>20)==3) atomicAdd(dynamicShaded,1);
            if((custom>>20)==2) atomicAdd(terrainHits,1);
            if(surface.alphaTest) atomicAdd(alphaHits,1);
        }
    }
    color=mix(toLinear(light.fog.rgb),color,water.z);
    float fresnel=.025+.4*pow(1-clamp(-dot(incident,n),0,1),5);
    return vec4(color,fresnel*(1-smoothstep(60,80,distance)));
}
