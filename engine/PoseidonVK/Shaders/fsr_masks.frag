#version 450
layout(set=0,binding=0) uniform sampler2D motion;
layout(set=1,binding=0) uniform sampler2D sceneDepth;
layout(push_constant) uniform Inputs { vec4 metrics; vec4 jitterDelta; } inputs;
layout(location=0) out vec2 velocity;
layout(location=1) out float depth;
layout(location=2) out float reactive;
layout(location=3) out float composition;
void main() {
    ivec2 p = ivec2(gl_FragCoord.xy);
    vec4 m = texelFetch(motion, p, 0);
    float validity = m.w;
    // Water/transparency (-1) and unsupported/missing motion (0): discard locks
    // and minimize accumulation. Cutouts (.75) retain some valid correspondence.
    reactive = validity <= 0 ? 1 : validity < .99 ? .5 : 0;
    composition = validity <= 0 ? 1 : validity < .99 ? .75 : 0;
    velocity = validity > 0 ? m.xy - inputs.jitterDelta.xy : vec2(0);
    depth = texelFetch(sceneDepth, p, 0).r;
}
