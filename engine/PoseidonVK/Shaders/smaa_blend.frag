#version 450
#extension GL_GOOGLE_include_directive : require
#include "smaa_common.glsl"
#include "underwater.glsl"
layout(set=0, binding=0) uniform sampler2D source;
layout(set=1, binding=0) uniform sampler2D weights;
layout(set=2, binding=0) uniform sampler2D sceneDepth;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
void main() {
    vec4 offset;
    SMAANeighborhoodBlendingVS(texCoord, offset);
    outColor = SMAANeighborhoodBlendingPS(texCoord, offset, source, weights);
    gl_FragDepth = texture(sceneDepth,texCoord).r;
    if (aa.colorControl.y > .5) outColor.rgb = toneMap(underwaterColor(outColor.rgb,gl_FragDepth,texCoord), aa.colorControl.x);
}
