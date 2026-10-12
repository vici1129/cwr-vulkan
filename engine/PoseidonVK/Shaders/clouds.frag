#version 450
#extension GL_GOOGLE_include_directive : require
#include "color_space.glsl"
#define WEATHER_SET 1
#define CLOUD_VOLUME
#include "weather.glsl"
layout(push_constant) uniform CloudView { mat4 camera; vec4 projection; vec4 jitter; } view;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
void main() {
    vec2 ndc=texCoord*2-1-view.jitter.xy;
    vec3 ray=normalize(mat3(view.camera)*vec3(ndc*vec2(1,-1)/view.projection.xy,1));
    // Camera-relative CloudScale scales positions and thickness uniformly, so
    // undoing it leaves the camera origin and ray direction unchanged.
    float jitter=fract(52.9829189*fract(dot(gl_FragCoord.xy,vec2(.06711056,.00583715))));
    outColor=marchClouds(weather.camera.xyz,ray,32,jitter);
}
