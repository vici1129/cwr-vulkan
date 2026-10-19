// Reuse non-water surface records. Positive W remains exclusively water;
// negative W identifies diffuse reflectance and its original raster depth.
#include "rtao_coverage.glsl"
#include "material_record.glsl"
#ifdef MULTISAMPLE_DEPTH
layout(set=0,binding=7) uniform sampler2DMS receiverSurface;
vec4 giSurface(ivec2 p) { return texelFetch(receiverSurface,p,0); }
vec4 giReceiver(ivec2 p) {
    vec4 s=texelFetch(receiverSurface,p,0);
    if(!enhancedRecord(s)) return vec4(0);
    for(int i=0;i<textureSamples(sceneDepth);++i) {
        vec4 other=texelFetch(receiverSurface,p,i);
        float d=texelFetch(sceneDepth,p,i).r;
        if(d<=0 || d>=.999999 || !enhancedRecord(other) ||
           abs(s.r-other.r)>.5 || dot(materialDecodeNormal(s.g),materialDecodeNormal(other.g))<.995 ||
           !rtSameDepth(-other.w,-s.w) || !rtSameDepth(viewZ(d),-s.w)) return vec4(0);
    }
    return vec4(materialReflectance(s)*s.b,s.w);
}
#else
layout(set=0,binding=7) uniform sampler2D receiverSurface;
vec4 giSurface(ivec2 p) { return texelFetch(receiverSurface,p,0); }
vec4 giReceiver(ivec2 p) {
    vec4 s=texelFetch(receiverSurface,p,0);
    float d=depthAt(p);
    if(!enhancedRecord(s) || d<=0 || d>=.999999 ||
       !rtSameDepth(viewZ(d),-s.w)) return vec4(0);
    return vec4(materialReflectance(s)*s.b,s.w);
}
#endif
vec3 giNormal(ivec2 pixel,vec3 p) {
    vec3 l=positionAt(pixel-ivec2(1,0)),r=positionAt(pixel+ivec2(1,0));
    vec3 t=positionAt(pixel-ivec2(0,1)),b=positionAt(pixel+ivec2(0,1));
    vec3 dx=abs(p.z-l.z)<abs(r.z-p.z)?p-l:r-p;
    vec3 dy=abs(p.z-t.z)<abs(b.z-p.z)?p-t:b-p;
    // Reject discontinuities and degenerate depth rather than invent a plane.
    if(max(length(dx),length(dy))>max(.25,p.z*.04)) return vec3(0);
    vec3 n=cross(dx,dy);
    if(dot(n,n)<1e-12) return vec3(0);
    n=normalize(n);
    return dot(n,p)>0?-n:n;
}
