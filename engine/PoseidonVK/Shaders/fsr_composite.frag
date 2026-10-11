#version 450
layout(set=0,binding=0) uniform sampler2D reconstructed;
layout(set=1,binding=0) uniform sampler2D sceneDepth;
layout(push_constant) uniform Composite { vec4 render; vec4 display; } aa;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
void main() {
    outColor = texelFetch(reconstructed, ivec2(gl_FragCoord.xy), 0);
    // Native overlay depth testing still uses the world's original nonlinear depth.
    gl_FragDepth = texture(sceneDepth, texCoord + aa.render.xy).r;
}
