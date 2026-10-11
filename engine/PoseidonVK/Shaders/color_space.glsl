#ifndef CWR_COLOR_SPACE_GLSL
#define CWR_COLOR_SPACE_GLSL
// Legacy assets and lighting coefficients are authored in display-referred RGB.
// Asset decoding uses sRGB; over-range legacy lighting uses linear headroom.
vec3 toLinear(vec3 c) {
    c = max(c, vec3(0));
    return mix(c / 12.92, pow((c + .055) / 1.055, vec3(2.4)), greaterThan(c, vec3(.04045)));
}
vec3 toSRGB(vec3 c) {
    c = max(c, vec3(0));
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0/2.4)) - .055, greaterThan(c, vec3(.0031308)));
}
vec3 legacyLightToLinear(vec3 c) {
    // Values above legacy white are light energy, not sRGB code values. Raising
    // them to 2.4 would invent intensity and wash ordinary diffuse materials out.
    return toLinear(min(c, vec3(1))) + max(c - 1., vec3(0));
}
// Fixed unit exposure, identity shadows/midtones, C1 continuous neutral shoulder.
// Scale RGB together to preserve hue. Peak is in reference-white units.
vec3 toneMap(vec3 c, float peak) {
    c = max(c, vec3(0));
    float v = max(c.r, max(c.g, c.b));
    float start = .65 * peak;
    if (v <= start) return c;
    float range = peak - start;
    float mapped = start + range * (v - start) / (v - start + range);
    return c * (mapped / v);
}
vec3 toPQ(vec3 nits) {
    vec3 p = pow(clamp(nits / 10000., 0., 1.), vec3(2610. / 16384.));
    return pow((3424. / 4096. + (2413. / 128.) * p) /
               (1. + (2392. / 128.) * p), vec3(2523. / 32.));
}
vec3 rec709To2020(vec3 c) {
    return mat3(.627404, .069097, .0163916,
                .329283, .919540, .0880132,
                .0433136, .0113612, .895595) * c;
}
#endif
