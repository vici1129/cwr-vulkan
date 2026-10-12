#version 450
#ifdef MULTISAMPLE_DEPTH
layout(set=0,binding=2) uniform sampler2DMS waterSurface;
vec4 surface(ivec2 p) {
    vec4 w=vec4(0); float closest=1e30;
    for(int s=0;s<textureSamples(waterSurface);++s) {
        vec4 v=texelFetch(waterSurface,p,s);
        if(v.w<=0) return vec4(0); // preserve foreground at every MSAA sample
        if(v.w<closest) { w=v; closest=v.w; }
    }
    return w;
}
#else
layout(set=0,binding=2) uniform sampler2D waterSurface;
vec4 surface(ivec2 p) { return texelFetch(waterSurface,p,0); }
#endif
layout(set=0,binding=3) uniform sampler2D background;
layout(push_constant) uniform Parameters { mat4 camera; vec4 projection; vec4 jitter; } water;
layout(location=0) out vec4 transmitted;
vec3 rayAt(vec2 p,vec2 size) {
    vec2 ndc=(p+.5)/size*2-1-water.jitter.xy;
    return vec3(ndc*vec2(1,-1)/water.projection.xy,1);
}
bool bottom(ivec2 p,vec2 size,out vec4 color,out float thickness) {
    color=texelFetch(background,p,0);
    vec4 w=surface(p);
    vec3 ray=rayAt(vec2(p),size);
    vec3 delta=mat3(water.camera)*ray*(color.a-w.a);
    // Both depths are FP16 view metres. Accept their bounded rounding error at
    // contact instead of leaving an opaque dark ribbon around the shoreline.
    float tolerance=max(.01,w.a*.001);
    thickness=length(ray)*max(color.a-w.a,0);
    return w.a>0 && color.a>0 && color.a>=w.a-tolerance &&
           delta.y<=tolerance*length(ray) && thickness<35;
}
void main() {
    transmitted=vec4(0);
    ivec2 size=textureSize(background,0),p=ivec2(gl_FragCoord.xy);
    vec4 w=surface(p),base; float thickness;
    if(w.a<=0 || w.z<=.001 || !bottom(p,vec2(size),base,thickness)) return;
    vec3 ray=rayAt(vec2(p),vec2(size));
    vec3 worldRay=mat3(water.camera)*ray;
    if(worldRay.y>=-.015) return; // above-water viewing only
    float distance=length(ray)*w.a;
    if(distance>=80) return;
    vec3 n=normalize(vec3(w.x,sqrt(max(.01,1-dot(w.xy,w.xy))),w.y));
    float incidence=clamp(-dot(normalize(worldRay),n),0,1);
    float fresnel=.025+.4*pow(1-incidence,5); // same restrained Fresnel as RT water
    // Normal perturbation only, not the planar normal: a few output pixels at most.
    vec3 viewBump=transpose(mat3(water.camera))*vec3(w.x,0,w.y);
    vec2 offset=vec2(viewBump.x,-viewBump.y)*18*smoothstep(0,.6,thickness);
    offset*=min(vec2(size).y/1080,1.5);
    vec2 q=vec2(p)+clamp(offset,vec2(-2.5),vec2(2.5));
    ivec2 b=ivec2(floor(q)); vec2 f=fract(q);
    vec4 c[4]; float t[4]; bool valid=all(greaterThanEqual(b,ivec2(0))) && all(lessThan(b+1,size));
    if(valid) {
        valid=bottom(b,vec2(size),c[0],t[0]) && bottom(b+ivec2(1,0),vec2(size),c[1],t[1]) &&
              bottom(b+ivec2(0,1),vec2(size),c[2],t[2]) && bottom(b+1,vec2(size),c[3],t[3]);
        for(int i=0;i<4 && valid;++i)
            valid=abs(t[i]-thickness)<max(.35,thickness*.25);
    }
    // Reject the entire displaced bilinear footprint at shorelines/foreground/depth jumps.
    vec3 color=base.rgb;
    if(valid) color=mix(mix(c[0].rgb,c[1].rgb,f.x),mix(c[2].rgb,c[3].rgb,f.x),f.y);
    vec3 absorption=exp(-thickness*vec3(.16,.065,.035));
    float weight=.72*(1-fresnel)*exp(-thickness*.14);
    weight*=w.z*(1-smoothstep(55,80,distance));
    // Existing fog and weather-lit water remain the scattering color via destination blending.
    // Both source colors already include scene fog; do not apply that fog a second time.
    transmitted=vec4(color*absorption,weight);
}
