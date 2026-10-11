// Shared post-resolve receiver policy for hardware and screen-space sunlight.
#include "rtao_coverage.glsl"
#ifdef MULTISAMPLE_DEPTH
layout(set=0,binding=6) uniform sampler2DMS directLight;
vec4 directAt(ivec2 p) {
    int selected=0;
    float closest=texelFetch(sceneDepth,p,0).r;
    for(int i=1;i<textureSamples(sceneDepth);++i) {
        float d=texelFetch(sceneDepth,p,i).r;
        if(d<closest) { closest=d; selected=i; }
    }
    vec4 direct=texelFetch(directLight,p,selected);
    float z=viewZ(closest);
    for(int i=0;i<textureSamples(sceneDepth);++i) {
        float d=texelFetch(sceneDepth,p,i).r;
        if(d<=0 || d>=.999999 || !rtSameDepth(viewZ(d),z)) return vec4(0);
        vec4 other=texelFetch(directLight,p,i);
        for(int channel=0;channel<4;++channel)
            if(!rtSameLight(direct[channel],other[channel])) return vec4(0);
    }
    return direct;
}
#else
layout(set=0,binding=6) uniform sampler2D directLight;
vec4 directAt(ivec2 p) { return texelFetch(directLight,p,0); }
#endif
