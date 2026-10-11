#version 450
#extension GL_GOOGLE_include_directive : require
#include "color_space.glsl"
layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D frame;
layout(push_constant) uniform Gamma { float invGamma; float hdr; float whiteNits; float srgbAttachment; } gamma;
void main() {
    vec3 c = texture(frame, texCoord).rgb;
    vec3 encoded = toSRGB(c);
    if (gamma.invGamma != 1.0)
        encoded = mix(pow(clamp(encoded, 0., 1.), vec3(gamma.invGamma)), encoded, greaterThan(encoded, vec3(1)));
    if (gamma.hdr > .5)
        encoded = toPQ(rec709To2020(toLinear(encoded)) * gamma.whiteNits);
    else if (gamma.srgbAttachment > .5)
        encoded = toLinear(encoded); // Attachment performs the sole sRGB encode.
    outColor = vec4(encoded, 1.0);
}
