#version 450
#extension GL_GOOGLE_include_directive : require
#include "color_space.glsl"
layout(set=0,binding=0) uniform sampler2D reconstructed;
layout(set=3,binding=0) uniform sampler2D bloom;
layout(set=1,binding=0) uniform sampler2D sceneDepth;
layout(push_constant) uniform Composite { vec4 render; vec4 display; vec4 colorControl; } aa;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
void main() {
    outColor = texelFetch(reconstructed, ivec2(gl_FragCoord.xy), 0);
    if (aa.colorControl.z > 0.) outColor.rgb += texture(bloom, texCoord).rgb * aa.colorControl.z;
    outColor.rgb = toneMap(outColor.rgb, aa.colorControl.x);
    // Native overlay depth testing still uses the world's original nonlinear depth.
    gl_FragDepth = texture(sceneDepth, texCoord + aa.render.xy).r;
}
