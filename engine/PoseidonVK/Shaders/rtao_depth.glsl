#ifdef MULTISAMPLE_DEPTH
layout(set=0,binding=0) uniform sampler2DMS sceneDepth;
ivec2 depthSize() { return textureSize(sceneDepth); }
float depthAt(ivec2 p) {
    p=clamp(p,ivec2(0),depthSize()-1);
    float d=1;
    for(int i=0;i<textureSamples(sceneDepth);++i) d=min(d,texelFetch(sceneDepth,p,i).r);
    return d;
}
#else
layout(set=0,binding=0) uniform sampler2D sceneDepth;
ivec2 depthSize() { return textureSize(sceneDepth,0); }
float depthAt(ivec2 p) { return texelFetch(sceneDepth,clamp(p,ivec2(0),depthSize()-1),0).r; }
#endif
layout(push_constant) uniform Parameters {
    mat4 camera;
    vec4 projection;
    vec4 settings; // radius, bias, strength, fade distance
    vec4 jitter;
    vec4 sun;
} ao;
float viewZ(float d) { return ao.projection.w/(d-ao.projection.z); }
vec3 positionAt(ivec2 p) {
    vec2 ndc=(vec2(p)+.5)/vec2(depthSize())*2-1-ao.jitter.xy;
    float z=viewZ(depthAt(p));
    return vec3(ndc*vec2(1,-1)/ao.projection.xy*z,z);
}
