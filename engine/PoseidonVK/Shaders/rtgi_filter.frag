#version 450
#extension GL_GOOGLE_include_directive : require
#include "rtao_depth.glsl"
#include "rtgi_receiver.glsl"
layout(set=0,binding=4) uniform sampler2D rawGI;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
void main() {
    outColor=vec4(0);
    ivec2 full=clamp(ivec2(texCoord*depthSize()),ivec2(0),depthSize()-1);
    vec4 receiver=giReceiver(full);
    if(receiver.w>=0) return;
    vec3 p=positionAt(full),n=giNormal(full,p);
    if(dot(n,n)<.5) return;
    vec3 sum=vec3(0);float weight=0;
    ivec2 size=textureSize(rawGI,0),center=full/2;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x) {
        ivec2 q=clamp(center+ivec2(x,y),ivec2(0),size-1);
        vec4 sampleGI=texelFetch(rawGI,q,0);
        if(sampleGI.w<=0) continue;
        ivec2 other=min(q*2+1,depthSize()-1);
        vec3 op=positionAt(other),on=giNormal(other,op);
        float plane=abs(dot(op-p,n));
        // Hard plane/normal gates prevent colored interpolation across corners
        // and silhouettes. Multiply by this pixel's reflectance AFTER filtering.
        if(dot(n,on)<.9 || plane>max(.025,p.z*.001) || abs(p.z-op.z)>max(.2,p.z*.02)) continue;
        float w=exp(-float(x*x+y*y)*.7)*exp(-plane/max(.01,p.z*.0005));
        sum+=sampleGI.rgb*w;weight+=w;
    }
    if(weight>.001) outColor.rgb=receiver.rgb*sum/weight;
}
