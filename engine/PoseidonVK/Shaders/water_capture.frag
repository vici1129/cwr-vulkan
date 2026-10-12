#version 450
layout(set=0,binding=0) uniform sampler2D sceneColor;
#ifdef MULTISAMPLE_DEPTH
layout(set=0,binding=1) uniform sampler2DMS sceneDepth;
#else
layout(set=0,binding=1) uniform sampler2D sceneDepth;
#endif
layout(push_constant) uniform Parameters { mat4 camera; vec4 projection; vec4 jitter; } water;
layout(location=0) out vec4 background;
void main() {
    ivec2 p=ivec2(gl_FragCoord.xy);
    float nearDepth=1,farDepth=0;
#ifdef MULTISAMPLE_DEPTH
    for(int s=0;s<textureSamples(sceneDepth);++s) {
        float d=texelFetch(sceneDepth,p,s).r;
        nearDepth=min(nearDepth,d); farDepth=max(farDepth,d);
    }
#else
    nearDepth=farDepth=texelFetch(sceneDepth,p,0).r;
#endif
    float z=water.projection.w/(nearDepth-water.projection.z);
    float farZ=water.projection.w/(farDepth-water.projection.z);
    // Resolved color at a mixed geometry/sky or large depth edge is not a bottom sample.
    bool valid=farDepth<.999999 && z>0 && z<200 && farZ-z<max(.25,z*.01);
    background=vec4(texelFetch(sceneColor,p,0).rgb,valid?z:0);
}
