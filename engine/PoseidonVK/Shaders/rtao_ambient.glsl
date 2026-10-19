// Full-resolution radiance and matching raster depth, captured only for RTAO.
// Conservatively reject mixed MSAA coverage and stale records under effects.
#include "rtao_coverage.glsl"
#ifdef MULTISAMPLE_DEPTH
layout(set=0,binding=6) uniform sampler2DMS ambientLight;
vec4 ambientAt(ivec2 p) {
    vec4 a=texelFetch(ambientLight,p,0);
    if(a.w<=0) return vec4(0);
    for(int i=0;i<textureSamples(sceneDepth);++i) {
        vec4 b=texelFetch(ambientLight,p,i);
        float d=texelFetch(sceneDepth,p,i).r;
        if(d<=0 || d>=.999999 || !rtSameDepth(a.w,b.w) || !rtSameDepth(a.w,viewZ(d))) return vec4(0);
        for(int c=0;c<3;++c) if(!rtSameLight(a[c],b[c])) return vec4(0);
    }
    return a;
}
#else
layout(set=0,binding=6) uniform sampler2D ambientLight;
vec4 ambientAt(ivec2 p) {
    vec4 a=texelFetch(ambientLight,p,0);
    float d=depthAt(p);
    if(a.w<=0 || d<=0 || d>=.999999 || !rtSameDepth(a.w,viewZ(d))) return vec4(0);
    return a;
}
#endif
