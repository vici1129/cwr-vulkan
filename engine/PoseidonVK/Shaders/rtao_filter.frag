#version 450
#extension GL_GOOGLE_include_directive : require
#include "rtao_depth.glsl"
layout(set=0,binding=4) uniform sampler2D rawAO;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
void main() {
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
}
