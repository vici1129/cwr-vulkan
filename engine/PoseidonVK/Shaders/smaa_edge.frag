#version 450
#extension GL_GOOGLE_include_directive : require
#include "color_space.glsl"
// SMAA color edges use perceptual bounded samples, while its neighborhood pass
// continues to filter the original linear HDR color. No extra full-frame copy.
vec4 edgeSample(sampler2D image, vec2 uv) { return vec4(toSRGB(toneMap(texture(image, uv).rgb, 1.0)), 1); }
#define texture edgeSample
#include "smaa_common.glsl"
#undef texture
layout(set=0, binding=0) uniform sampler2D source;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
void main() {
    vec4 offset[3];
    SMAAEdgeDetectionVS(texCoord, offset);
    outColor = vec4(SMAAColorEdgeDetectionPS(texCoord, offset, source), 0, 0);
}
