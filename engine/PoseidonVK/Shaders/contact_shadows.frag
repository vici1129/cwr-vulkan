#version 450
#extension GL_GOOGLE_include_directive : require
#include "rtao_depth.glsl"
#include "sunlight_depth.glsl"
#ifdef CONTACT_RT
layout(set=0,binding=4) uniform sampler2D rtReduction;
#endif
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;

float edgeFade(vec2 uv) {
    vec2 d=min(uv,1-uv);
    return smoothstep(0.0,.035,min(d.x,d.y));
}
float contactOcclusion(ivec2 pixel, vec3 p) {
    float distance=length(p);
    if(p.z<=0 || distance>=40) return 0;
    vec3 l=positionAt(pixel-ivec2(1,0)), r=positionAt(pixel+ivec2(1,0));
    vec3 t=positionAt(pixel-ivec2(0,1)), b=positionAt(pixel+ivec2(0,1));
    vec3 dx=abs(p.z-l.z)<abs(r.z-p.z)?p-l:r-p;
    vec3 dy=abs(p.z-t.z)<abs(b.z-p.z)?p-t:b-p;
    vec3 n=cross(dx,dy);
    if(dot(n,n)<1e-12) return 0;
    n=normalize(n);
    if(dot(n,p)>0) n=-n;
    vec3 direction=normalize(transpose(mat3(ao.camera))*ao.sun.xyz);
    // A depth-derived back face or grazing receiver is too ambiguous to shade.
    float facing=smoothstep(.02,.15,dot(n,direction));
    if(facing<=0) return 0;
    vec3 origin=p+n*.02+direction*.01;
    float occlusion=0;
    for(int i=1;i<=16;++i) {
        // Uniform short steps keep narrow poles/fences from producing widely
        // separated copies. Fixed samples avoid time-varying noise/history.
        float travel=1.2*(float(i)-.5)/16;
        vec3 q=origin+direction*travel;
        if(q.z<=.01) break;
        vec2 uv=(q.xy*vec2(1,-1)*ao.projection.xy/q.z+ao.jitter.xy)*.5+.5;
        if(any(lessThanEqual(uv,vec2(0))) || any(greaterThanEqual(uv,vec2(1)))) break;
        ivec2 samplePixel=ivec2(uv*depthSize());
        if(all(equal(samplePixel,pixel))) continue;
        float d=depthAt(samplePixel);
        if(d<=0 || d>=.999999) continue;
        float sampleZ=viewZ(d);
        float delta=q.z-sampleZ;
        // A bounded slab, not an infinitely thick silhouette extrusion.
        float thickness=.16+min(.04,p.z*.002);
        float hit=smoothstep(.005,.015,delta)*(1-smoothstep(thickness*.6,thickness,delta));
        vec2 sampleNdc=(vec2(samplePixel)+.5)/vec2(depthSize())*2-1-ao.jitter.xy;
        vec3 surface=vec3(sampleNdc*vec2(1,-1)/ao.projection.xy*sampleZ,sampleZ);
        // A contact is near the receiver plane. Raised hulls and higher limbs
        // seen only as a thin depth slab otherwise leave hollow shadow bands.
        float height=dot(surface-p,n);
        hit*=smoothstep(.005,.025,height)*(1-smoothstep(.2,.5,height));
        hit*=1-smoothstep(0.0,1.2,travel);
        occlusion=max(occlusion,hit*edgeFade(uv));
    }
    vec2 uv=(vec2(pixel)+.5)/depthSize();
    return occlusion*facing*edgeFade(uv)*(1-smoothstep(25.0,40.0,distance))*ao.sun.w;
}
void main() {
    ivec2 pixel=clamp(ivec2(texCoord*depthSize()),ivec2(0),depthSize()-1);
    vec3 reduction=vec3(0);
#ifdef CONTACT_RT
    reduction=texelFetch(rtReduction,pixel,0).rgb;
#endif
    vec4 direct=directAt(pixel);
    float d=depthAt(pixel);
    if(d>0 && d<.999999 && max(max(direct.r,direct.g),direct.b)>.00001) {
        float visibility=1-contactOcclusion(pixel,positionAt(pixel));
        vec3 contact=direct.rgb*max(0,direct.a-min(direct.a,visibility));
        // Equivalent to min(CSM,RT,contact). Never multiply shadow darkness.
        reduction=max(reduction,contact);
    }
    outColor=vec4(reduction,0);
}
