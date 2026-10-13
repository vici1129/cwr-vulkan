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
        HitSurface surface=hitSurface(query);
        if(surface.diffuseEligible) atomicAdd(surfaceEligible,1);
        if((surface.available&HitNormal)!=0u) {
            if((surface.available&HitVertexNormal)!=0u) atomicAdd(surfaceVertex,1);
            else atomicAdd(surfaceFace,1);
        }
        if((surface.available&(HitPosition|HitNormal|HitUV|HitAlbedo))!=(HitPosition|HitNormal|HitUV|HitAlbedo))
            atomicAdd(surfaceUnavailable,1);
        // Cross-check transformed barycentric position against the actual ray.
        // A centimetre tolerance accommodates large stock world coordinates.
        vec3 rayPosition=rayQueryGetWorldRayOriginEXT(query)+direction*rayQueryGetIntersectionTEXT(query,true);
        bool invalidNormal=(surface.available&HitNormal)!=0u &&
            (!finiteVector(surface.normal) || abs(dot(surface.normal,surface.normal)-1)>.001 ||
             dot(surface.normal,direction)>.001 || dot(surface.normal,surface.geometricNormal)<.099);
        bool invalidAlbedo=(surface.available&HitAlbedo)!=0u &&
            (!finiteVector(surface.albedo) || any(lessThan(surface.albedo,vec3(0))) ||
             any(greaterThan(surface.albedo,vec3(1.001))));
        if((surface.available&HitPosition)==0u || length(surface.position-rayPosition)>.01 || invalidNormal || invalidAlbedo)
            atomicAdd(surfaceInvalid,1);
        if((surface.available&(HitColor|HitNormal))==(HitColor|HitNormal)) {
            // Preserve the legacy reflection response; future diffuse transport
            // uses surface.normal and surface.albedo with diffuseEligible.
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
            if((surface.available&HitAlphaTest)!=0u) atomicAdd(alphaHits,1);
        }
    }
    color=mix(toLinear(light.fog.rgb),color,water.z);
    float fresnel=.025+.4*pow(1-clamp(-dot(incident,n),0,1),5);
    return vec4(color,fresnel*(1-smoothstep(60,80,distance)));
}
