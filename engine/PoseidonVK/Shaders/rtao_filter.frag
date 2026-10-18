#version 450
#extension GL_GOOGLE_include_directive : require
#include "rtao_depth.glsl"
layout(set=0,binding=4) uniform sampler2D rawAO;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
void main() {
#ifdef WATER_REFLECTION
    ivec2 p=ivec2(gl_FragCoord.xy); vec4 w=waterAt(p);
    if(w.w<=0) {outColor=vec4(0);return;}
    vec4 sum=vec4(0);float weight=0;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x) {
        ivec2 q=clamp(p+ivec2(x,y),ivec2(0),depthSize()-1);vec4 other=waterAt(q);
        if(other.w<=0) continue;
        float v=exp(-abs(w.w-other.w)/max(.05,w.w*.002))*exp(-float(x*x+y*y));
        vec4 reflected=texelFetch(rawAO,q,0);
        // Filter radiance with its coverage. Zero-weight misses/faded receivers
        // must not introduce black RGB and darken a partially covered reflection.
        sum+=vec4(reflected.rgb*reflected.a,reflected.a)*v;weight+=v;
    }
    outColor=vec4(sum.rgb/max(sum.a,.0001),sum.a/max(weight,.0001));
#elif defined(SUN_SHADOW)
    // Full-resolution deterministic visibility needs no stochastic denoiser.
    outColor=vec4(texelFetch(rawAO,ivec2(gl_FragCoord.xy),0).rgb,0);
#else
    ivec2 full=clamp(ivec2(texCoord*depthSize()),ivec2(0),depthSize()-1);
    float d=depthAt(full);
    outColor=vec4(1);
    if(d<=0 || d>=.999999) return;
    float z=viewZ(d);
    ivec2 center=full/2, size=textureSize(rawAO,0);
    float sum=0,weight=0;
    for(int y=-2;y<=2;++y) for(int x=-2;x<=2;++x) {
        ivec2 p=clamp(center+ivec2(x,y),ivec2(0),size-1);
        vec2 sampleAO=texelFetch(rawAO,p,0).rg;
        float w=exp(-float(x*x+y*y)*.35)*exp(-abs(z-sampleAO.g)/max(.04,z*.005));
        sum+=sampleAO.r*w; weight+=w;
    }
    outColor=vec4(vec3(weight>.001?sum/weight:1),1);
#endif
}
