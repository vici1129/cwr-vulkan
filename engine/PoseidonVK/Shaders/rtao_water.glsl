// Closest-hit water reflection shading on the shared AO/sunlight TLAS.
#include "color_space.glsl"
#define WEATHER_SET 1
#define CLOUD_VOLUME
#include "weather.glsl"
layout(set=0,binding=8,std430) readonly buffer ReflectionLight {
    vec4 ambient, diffuse, zenith, horizon, sunSky, fog, eye, parameters;
} light;
uint vertexIndex(Material m,uint index) {
    uint word=Words(m.indices).data[m.index16!=0?index/2:index];
    return m.index16!=0?(word>>((index&1)*16))&65535:word;
}
vec3 vertexPosition(Material m,uint index) {
    uint offset=vertexIndex(m,index)*8;
    return vec3(Floats(m.vertices).data[offset],Floats(m.vertices).data[offset+1],Floats(m.vertices).data[offset+2]);
}
vec4 colorTexel(Material m,ivec2 p) {
    ivec2 size=ivec2(m.colorWidth,m.colorHeight);
    p.x=(m.samplerFlags&1)!=0?clamp(p.x,0,size.x-1):((p.x%size.x)+size.x)%size.x;
    p.y=(m.samplerFlags&2)!=0?clamp(p.y,0,size.y-1):((p.y%size.y)+size.y)%size.y;
    return unpackUnorm4x8(Words(m.color).data[p.y*size.x+p.x]);
}
vec3 sampleColor(Material m,vec2 uv) {
    vec2 p=uv*vec2(m.colorWidth,m.colorHeight)-.5;
    if((m.samplerFlags&4)!=0) return toLinear(colorTexel(m,ivec2(floor(p+.5))).rgb);
    ivec2 b=ivec2(floor(p)); vec2 f=fract(p);
    return mix(mix(toLinear(colorTexel(m,b).rgb),toLinear(colorTexel(m,b+ivec2(1,0)).rgb),f.x),
        mix(toLinear(colorTexel(m,b+ivec2(0,1)).rgb),toLinear(colorTexel(m,b+ivec2(1,1)).rgb),f.x),f.y);
}
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
        Material m=Materials(materials[custom&65535u]).m[rayQueryGetIntersectionGeometryIndexEXT(query,true)];
        if(m.color!=uint64_t(0) && m.colorWidth>0 && m.colorHeight>0) {
            uint first=m.first+rayQueryGetIntersectionPrimitiveIndexEXT(query,true)*3;
            vec2 bary=rayQueryGetIntersectionBarycentricsEXT(query,true);
            vec2 uv=vertexUV(m,first)*(1-bary.x-bary.y)+vertexUV(m,first+1)*bary.x+vertexUV(m,first+2)*bary.y;
            mat4x3 model=rayQueryGetIntersectionObjectToWorldEXT(query,true);
            vec3 a=model*vec4(vertexPosition(m,first),1), b=model*vec4(vertexPosition(m,first+1),1), c=model*vec4(vertexPosition(m,first+2),1);
            vec3 normal=normalize(cross(b-a,c-a));
            if(dot(normal,direction)>0) normal=-normal;
            color=sampleColor(m,uv)*legacyLightToLinear(light.ambient.rgb+light.diffuse.rgb*max(dot(normal,ao.sun.xyz),0));
            if(weather.sun.w>0) {
                vec3 ambient=sampleColor(m,uv)*legacyLightToLinear(light.ambient.rgb);
                color=ambient+max(color-ambient,vec3(0))*cloudSunlight(world+direction*rayQueryGetIntersectionTEXT(query,true));
            }
            float fog=clamp((rayQueryGetIntersectionTEXT(query,true)-light.parameters.x)*light.parameters.y,0,1);
            color=mix(eyeResponse(color),toLinear(light.fog.rgb),fog);
            atomicAdd(shadedHits,1);
            if((custom>>20)==3) atomicAdd(dynamicShaded,1);
            if((custom>>20)==2) atomicAdd(terrainHits,1);
            if(m.alpha!=uint64_t(0)) atomicAdd(alphaHits,1);
        }
    }
    color=mix(toLinear(light.fog.rgb),color,water.z);
    float fresnel=.025+.4*pow(1-clamp(-dot(incident,n),0,1),5);
    return vec4(color,fresnel*(1-smoothstep(60,80,distance)));
}
