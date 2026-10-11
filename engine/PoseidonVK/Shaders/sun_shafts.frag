#version 450
#extension GL_GOOGLE_include_directive : require
#include "color_space.glsl"
layout(set=0,binding=0) uniform sampler2D source;
layout(set=1,binding=0) uniform sampler2D shafts;
#ifdef MULTISAMPLE_DEPTH
layout(set=2,binding=0) uniform sampler2DMS sceneDepth;
#else
layout(set=2,binding=0) uniform sampler2D sceneDepth;
#endif
layout(push_constant) uniform Shaft {
    vec4 sun; // unjittered UV, intensity, pass
    vec4 color;
    vec4 metrics; // quarter texel size, aspect, unused
    vec4 depth; // jitter UV, world dimensions
} shaft;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;

float sky(ivec2 p) {
    p = clamp(p, ivec2(0), ivec2(shaft.depth.zw)-1);
#ifdef MULTISAMPLE_DEPTH
    float visible = 0;
    for (int s=0; s<textureSamples(sceneDepth); ++s)
        visible += step(1.0, texelFetch(sceneDepth,p,s).r);
    return visible / float(textureSamples(sceneDepth));
#else
    return step(1.0, texelFetch(sceneDepth,p,0).r);
#endif
}
float footprintSky(vec2 uv) {
    vec2 p = (uv + shaft.depth.xy) * shaft.depth.zw - .5;
    ivec2 base = ivec2(floor(p));
    vec2 f = fract(p);
    return mix(mix(sky(base),sky(base+ivec2(1,0)),f.x),
               mix(sky(base+ivec2(0,1)),sky(base+ivec2(1,1)),f.x),f.y);
}
void main() {
    vec2 radius = (texCoord - shaft.sun.xy) * vec2(shaft.metrics.z,1);
    if (shaft.sun.w < .5) {
        // Area samples prevent thin leaves from turning into a binary flashing
        // quarter-resolution mask. Color never seeds this mask (white walls
        // and specular objects cannot masquerade as the sun).
        float mask = 0;
        for (int y=0; y<4; ++y) for (int x=0; x<4; ++x)
            mask += footprintSky(texCoord + (vec2(x,y)-1.5) * shaft.metrics.xy / 4.0);
        outColor = vec4(mask / 16.0 * exp(-dot(radius,radius) / .032));
    } else if (shaft.sun.w < 1.5) {
        // Filter the actual sun aperture, not CPU tree bounding volumes. Fully
        // covered discs (roofs/walls/interiors) contribute no scattering.
        float aperture = 0;
        for (int y=-1; y<=1; ++y) for (int x=-1; x<=1; ++x)
            aperture += texture(source,shaft.sun.xy + vec2(x,y) * vec2(.004/shaft.metrics.z,.004)).r;
        aperture = smoothstep(.03,.65,aperture/9.0);
        vec2 delta = (shaft.sun.xy-texCoord) * (.96/48.0);
        vec2 uv = texCoord + delta * .5;
        float sum = 0;
        for (int i=0; i<48; ++i) {
            float t = (float(i)+.5)/48.0;
            sum += texture(source,uv).r * (1.0-.65*t);
            uv += delta;
        }
        outColor = vec4(sum / 32.4 * exp(-dot(radius,radius)*2.0) * aperture);
    } else {
        vec4 scene = texture(source,texCoord);
        // FSR removes projection jitter, native TAA retains its current lattice.
        // The radial field is unjittered; align both sampling and receiver depth
        // with the actual reconstructed color, without moving geometry edges.
        vec2 uv = texCoord - shaft.depth.xy * shaft.color.w;
        // Smooth spatial reconstruction, no temporal image or stochastic taps.
        vec2 d = shaft.metrics.xy * .6;
        float light = texture(shafts,uv).r * .4;
        light += (texture(shafts,uv+vec2(d.x,0)).r + texture(shafts,uv-vec2(d.x,0)).r +
                  texture(shafts,uv+vec2(0,d.y)).r + texture(shafts,uv-vec2(0,d.y)).r) * .15;
        // Conservative receiver gate: beams exist in visible sky openings,
        // never painted over world walls, ground, people or cockpit interiors.
        float receiver = footprintSky(uv);
        scene.rgb += toLinear(shaft.color.rgb) * (light * receiver * shaft.sun.z);
        outColor = scene;
    }
}
