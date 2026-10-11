#version 450
layout(set=0,binding=0) uniform sampler2D source;
layout(set=1,binding=0) uniform sampler2D detail;
// xy: source texel size, z: threshold, w: 0 extract, 1 downsample, 2 upsample.
layout(push_constant) uniform Bloom { vec4 control; } bloom;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
vec3 sampleLight(vec2 offset) {
    vec3 c = max(texture(source, texCoord + offset * bloom.control.xy).rgb, vec3(0));
    if (bloom.control.w < .5) {
        // Peak-channel threshold retains saturated fire/red lamps without
        // changing hue. The soft knee begins near reference white (.9375).
        float brightness = max(c.r, max(c.g, c.b));
        float knee = bloom.control.z * .25;
        float soft = clamp(brightness - bloom.control.z + knee, 0., 2. * knee);
        soft = soft * soft / (4. * knee);
        c *= max(brightness - bloom.control.z, soft) / max(brightness, 1e-5);
    }
    return c;
}
void main() {
    vec3 c;
    if (bloom.control.w < 1.5) {
        // Normalized 13-tap low-pass. Threshold per tap keeps small bright
        // sources alive; no firefly clamp or LDR conversion discards energy.
        c = sampleLight(vec2(0)) * .125;
        c += (sampleLight(vec2(-2,-2)) + sampleLight(vec2(2,-2)) +
              sampleLight(vec2(-2,2)) + sampleLight(vec2(2,2))) * .03125;
        c += (sampleLight(vec2(-2,0)) + sampleLight(vec2(2,0)) +
              sampleLight(vec2(0,-2)) + sampleLight(vec2(0,2))) * .0625;
        c += (sampleLight(vec2(-1,-1)) + sampleLight(vec2(1,-1)) +
              sampleLight(vec2(-1,1)) + sampleLight(vec2(1,1))) * .125;
    } else {
        // 3x3 tent reconstruction; normalized octave mixing prevents the
        // number of levels from increasing uniform-field brightness.
        c = sampleLight(vec2(0)) * .25;
        c += (sampleLight(vec2(-1,0)) + sampleLight(vec2(1,0)) +
              sampleLight(vec2(0,-1)) + sampleLight(vec2(0,1))) * .125;
        c += (sampleLight(vec2(-1,-1)) + sampleLight(vec2(1,-1)) +
              sampleLight(vec2(-1,1)) + sampleLight(vec2(1,1))) * .0625;
        c = mix(texture(detail, texCoord).rgb, c, .65);
    }
    outColor = vec4(c, 1);
}
