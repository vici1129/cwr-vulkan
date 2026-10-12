#version 450
#extension GL_GOOGLE_include_directive : require
#include "shape_uniforms.glsl"
#include "color_space.glsl"
#include "weather.glsl"
layout(set=0,binding=0) uniform sampler2D scattering;
#ifdef MULTISAMPLE_DEPTH
layout(set=1,binding=0) uniform sampler2DMS sceneDepth;
ivec2 depthSize() { return textureSize(sceneDepth); }
float depthAt(ivec2 p) {
    float d=1;
    for(int i=0;i<textureSamples(sceneDepth);++i) d=min(d,texelFetch(sceneDepth,p,i).r);
    return d;
}
#else
layout(set=1,binding=0) uniform sampler2D sceneDepth;
ivec2 depthSize() { return textureSize(sceneDepth,0); }
float depthAt(ivec2 p) { return texelFetch(sceneDepth,p,0).r; }
#endif
layout(set=3,binding=5) uniform sampler2DArray shadowMap;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
layout(push_constant) uniform Volume { vec4 projection; vec4 settings; } volume;
// settings: composite, jitter x/y in NDC, maximum march distance.
float linearDepth(float d) { return min(volume.projection.w/(d-volume.projection.z),65000.0); }
// Return unknown as zero contribution. Never invent sunlight outside CSM coverage.
float cascadeVisibility(vec3 p,int c) {
    vec4 q=lighting.cascadeVP[c]*vec4(p,1);
    vec3 sc=q.xyz/q.w;
    vec2 uv=sc.xy*vec2(.5,-.5)+.5;
    float texel=lighting.shadowStrength.y;
    if(any(lessThan(uv,vec2(texel))) || any(greaterThan(uv,vec2(1-texel))) || sc.z<=0 || sc.z>=1) return -1;
    // Air samples have no surface acne: avoid the large receiver/slope bias
    // that would let sunlight pass through thin walls and tree trunks.
    float sum=0;
    for(int y=0;y<2;++y) for(int x=0;x<2;++x)
        sum+=step(sc.z-0.000001,texture(shadowMap,vec3(uv+(vec2(x,y)-.5)*texel,c)).r);
    return sum*.25;
}
float visibility(vec3 p) {
    int count=int(lighting.cascadeControl.x), omni=int(lighting.cascadeControl.w);
    float z=dot(p,lighting.shadowForward.xyz), r=length(p);
    for(int c=0;c<count;++c) {
        float metric=c<omni?r:z;
        if(metric>lighting.cascadeSplits[c]) continue;
        float v=cascadeVisibility(p,c);
        if(v<0) continue;
        float previous=c>0?lighting.cascadeSplits[c-1]:0;
        float blend=smoothstep(mix(previous,lighting.cascadeSplits[c],.85),lighting.cascadeSplits[c],metric);
        float next=c+1<count?cascadeVisibility(p,c+1):0;
        return mix(v,max(next,0),blend);
    }
    return 0;
}
void main() {
    ivec2 size=depthSize();
    ivec2 pixel=clamp(ivec2(texCoord*size),ivec2(0),size-1);
    float z=linearDepth(depthAt(pixel));
    if(volume.settings.x>.5) {
        // Joint bilateral upsampling. Reject background taps at silhouettes;
        // lack of a matching half-res sample gives zero, never a bright halo.
        ivec2 halfSize=textureSize(scattering,0);
        vec2 p=texCoord*halfSize-.5;
        ivec2 base=ivec2(floor(p));
        vec2 f=fract(p);
        vec3 sum=vec3(0); float total=0;
        for(int y=0;y<2;++y) for(int x=0;x<2;++x) {
            vec4 s=texelFetch(scattering,clamp(base+ivec2(x,y),ivec2(0),halfSize-1),0);
            float dz=abs(s.a-z), tolerance=max(.3,z*.015);
            float w=(x==0?1-f.x:f.x)*(y==0?1-f.y:f.y);
            w*=exp(-dz/tolerance)*step(dz,3*tolerance);
            sum+=s.rgb*w; total+=w;
        }
        outColor=vec4(total>0.001?sum/total:vec3(0),0);
        return;
    }
    // Nearest surface of this 2x2 footprint bounds the ray, including MSAA
    // samples. Retain that sample's actual UV for reconstruction.
    ivec2 origin=ivec2(gl_FragCoord.xy)*2, chosen=origin;
    float nearest=1;
    for(int y=0;y<2;++y) for(int x=0;x<2;++x) {
        ivec2 p=clamp(origin+ivec2(x,y),ivec2(0),size-1);
        float d=depthAt(p);
        if(d<=nearest) { nearest=d; chosen=p; }
    }
    z=linearDepth(nearest);
    vec2 ndc=(vec2(chosen)+.5)/size*2-1-volume.settings.yz;
    vec3 view=vec3(ndc*vec2(1,-1)/volume.projection.xy,1);
    vec3 ray=normalize(mat3(lighting.world)*view);
    float distance=z*length(view);
    float end=min(distance,volume.settings.w);
    float start=0;
    // The legacy clear-distance offset is an artistic visibility control, not
    // an empty sphere of air around the viewer. Derive a homogeneous scattering
    // density from its fog interval, without changing that visibility curve.
    float sigma=1.0/max(lighting.fogParams.y-lighting.fogParams.x,1.0);
    float sum=0;
    // Spatially fixed interleaved gradient noise prevents coherent step bands;
    // existing TAA/FSR consumes this completed HDR color, no separate history.
    float noise=fract(52.9829189*fract(dot(gl_FragCoord.xy,vec2(.06711056,.00583715))));
    for(int i=0;i<48 && end>start;++i) {
        float a=mix(start,end,float(i)/48), b=mix(start,end,float(i+1)/48);
        float t=mix(a,b,noise);
        // Analytic Beer-Lambert segment weight, single scattering only. No
        // additional extinction is applied to the already fogged scene color.
        sum+=(exp(-sigma*a)-exp(-sigma*b))*visibility(ray*t);
    }
    float mu=dot(ray,lighting.atmosphereSun.xyz), g=.55;
    float phase=(1-g*g)/pow(max(1+g*g-2*g*mu,.01),1.5);
    vec3 sun=legacyLightToLinear(lighting.atmosphereColor.rgb);
    vec3 fog=toLinear(lighting.fogColor.rgb);
    // Integrate the same foreground air against sky and geometry: suppressing
    // sky contributions would cut each shaft off at a foliage silhouette.
    // Bound directional energy against the existing atmospheric brightness,
    // scaling all channels together to retain the engine's sunlight hue.
    // Clouds vary over kilometres: one midpoint lookup per short fog ray is
    // sufficient, sharing the surface coverage without 48 repeated lookups.
    vec3 radiance=sun*(.18*phase*sum*lighting.atmosphereSun.w)*cloudSunlight(weather.camera.xyz+ray*end*.5);
    float budget=max(dot(fog,vec3(.2126,.7152,.0722)),.02)*.3;
    radiance*=min(1.0,budget/max(dot(radiance,vec3(.2126,.7152,.0722)),.00001));
    outColor=vec4(radiance,z);
}
