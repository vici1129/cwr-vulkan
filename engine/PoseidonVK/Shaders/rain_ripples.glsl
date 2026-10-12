// Stateless impacts anchored in world metres. Each cell has its own period and
// phase; each new impact picks a new location. The 3x3 neighborhood includes the
// entire compact support, so cell boundaries never clip a ring.
uvec3 rainHash(uvec3 p) {
    p = p * 1664525u + 1013904223u;
    p.x += p.y*p.z; p.y += p.z*p.x; p.z += p.x*p.y;
    p ^= p >> 16u;
    p.x += p.y*p.z; p.y += p.z*p.x; p.z += p.x*p.y;
    return p;
}
vec3 rainRandom(ivec2 cell, uint event) {
    return vec3(rainHash(uvec3(cell, event)) >> 8u) * (1.0 / 16777216.0);
}
vec2 rainSlope(vec3 relativeWorld) {
    vec2 p = relativeWorld.xz + lighting.rainRipples.zw;
    // Derivatives precede nonuniform exits, including underwater/distance culls.
    float footprint = max(length(dFdx(p)), length(dFdy(p)));
    float fade = (1-smoothstep(12,55,length(relativeWorld))) *
                 (1-smoothstep(.015,.075,footprint));
    if (fade <= 0 || relativeWorld.y >= 0) return vec2(0);
    const float cellSize = .8;
    ivec2 cell = ivec2(floor(p/cellSize));
    vec2 slope = vec2(0);
    float rain = lighting.rainRipples.x;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x) {
        ivec2 c = cell + ivec2(x,y);
        vec3 seed = rainRandom(c, 0u);
        float clock = lighting.rainRipples.y / (1.1+seed.x*1.3) + seed.y;
        float event = floor(clock);
        float age = fract(clock)*(1.1+seed.x*1.3)/.8;
        vec3 impact = rainRandom(c, uint(int(event))+1u);
        // Smooth probability threshold avoids popping during weather transitions.
        float weight = smoothstep(impact.z,impact.z+.12,rain);
        if(age>=1 || weight<=0) continue;
        vec2 delta = p-(vec2(c)+impact.xy)*cellSize;
        float r = length(delta);
        float q = (r-(.025+.34*age))/.085;
        if(abs(q)>=1) continue;
        float envelope = (1-q*q)*(1-q*q) * smoothstep(0,.12,age)*(1-age);
        // Signed radial slope gives both faces of a spreading ring, no point glint.
        slope += delta/max(r,.001) * sin(q*6.2831853) * envelope * weight;
    }
    return slope * (.08+.16*rain) * rain * fade;
}
