#version 450
#extension GL_GOOGLE_include_directive : require
#include "rtao_depth.glsl"
#include "rtgi_receiver.glsl"
#include "rtgi_sample.glsl"
#ifdef MULTISAMPLE_DEPTH
layout(set=0,binding=6) uniform sampler2DMS replacedAmbient;
vec4 giAmbient(ivec2 p) { return texelFetch(replacedAmbient,p,0); }
#else
layout(set=0,binding=6) uniform sampler2D replacedAmbient;
vec4 giAmbient(ivec2 p) { return texelFetch(replacedAmbient,p,0); }
#endif
layout(set=0,binding=4) uniform sampler2D rawGI;
layout(set=0,binding=12) uniform sampler2D resolvedGI;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
void main() {
    outColor=vec4(0);
    ivec2 full=clamp(ivec2(texCoord*depthSize()),ivec2(0),depthSize()-1);
    vec4 receiver=giReceiver(full);
    if(receiver.w>=0) return;
    vec3 p=positionAt(full),n=giNormal(full,p);
    if(dot(n,n)<.5) return;
    vec3 worldNormal=normalize(mat3(ao.camera)*n);
    vec3 sum=vec3(0);float weight=0;
    ivec2 size=textureSize(rawGI,0),center=full/2;
    bool temporal=ao.jitter.z>0;
    // Temporal radiance is already spatially filtered at half resolution.
    // Bilateral bilinear upsampling needs only four taps, not another blur.
    vec2 halfPixel=(vec2(full)+.5-1.5)*.5;
    ivec2 base=ivec2(floor(halfPixel));vec2 fraction=fract(halfPixel);
    for(int y=temporal?0:-1;y<=1;++y) for(int x=temporal?0:-1;x<=1;++x) {
        ivec2 q=clamp((temporal?base:center)+ivec2(x,y),ivec2(0),size-1);
        vec4 sampleGI=texelFetch(rawGI,q,0);
        ivec2 other=min(q*2+1,depthSize()-1);
        float w=giPlaneWeight(p,n,worldNormal,other,sampleGI.a);
        if(temporal) {
            w*=(x==0?1-fraction.x:fraction.x)*(y==0?1-fraction.y:fraction.y);
            sampleGI.rgb=texelFetch(resolvedGI,q,0).rgb;
        } else w*=exp(-float(x*x+y*y)*.7);
        sum+=sampleGI.rgb*w;weight+=w;
    }
    vec4 ambient=giAmbient(full);
    if(weight>.001 && ambient.w<0 && rtSameDepth(-ambient.w,-receiver.w)) {
        float fade=1-smoothstep(ao.settings.w*.7,ao.settings.w,length(p));
        // Replace only captured global ambient, after valid spatial reconstruction.
        // The signed HDR delta preserves direct/local/emission and in-scattered fog.
        outColor.rgb=(receiver.rgb*sum/weight-ambient.rgb)*fade;
    }
}
