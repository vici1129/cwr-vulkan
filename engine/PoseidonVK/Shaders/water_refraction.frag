#version 450
#ifdef MULTISAMPLE_DEPTH
layout(set=0,binding=2) uniform sampler2DMS waterSurface;
vec4 surface(ivec2 p) {
    vec4 w=vec4(0); float closest=1e30, rain=0;
    for(int s=0;s<textureSamples(waterSurface);++s) {
        vec4 v=texelFetch(waterSurface,p,s);
        if(v.w<=0) return vec4(0); // preserve foreground at every MSAA sample
        rain=min(rain,v.x);
        if(v.w<closest) { w=v; closest=v.w; }
    }
    if(rain<-1) w.x=rain; // optics reject rain at any covered sample, just as before
    return w;
}
#else
layout(set=0,binding=2) uniform sampler2D waterSurface;
vec4 surface(ivec2 p) { return texelFetch(waterSurface,p,0); }
#endif
layout(set=0,binding=3) uniform sampler2D background;
layout(push_constant) uniform Parameters {
    mat4 camera; vec4 projection; vec4 jitter;
    vec4 effects; // refraction, shoreline foam
    vec4 originTime; // camera world XYZ, simulation seconds
} water;
layout(location=0) out vec4 transmitted;
vec3 rayAt(vec2 p,vec2 size) {
    vec2 ndc=(p+.5)/size*2-1-water.jitter.xy;
    return vec3(ndc*vec2(1,-1)/water.projection.xy,1);
}
bool bottom(ivec2 p,vec2 size,out vec4 color,out float thickness,bool allowRain) {
    color=texelFetch(background,p,0);
    vec4 w=surface(p);
    vec3 ray=rayAt(vec2(p),size);
    vec3 delta=mat3(water.camera)*ray*(color.a-w.a);
    // Both depths are FP16 view metres. Accept their bounded rounding error at
    // contact instead of leaving an opaque dark ribbon around the shoreline.
    float tolerance=max(.01,w.a*.001);
    thickness=length(ray)*max(color.a-w.a,0);
    return (allowRain || w.x>=-1) && w.a>0 && color.a>0 && color.a>=w.a-tolerance &&
           delta.y<=tolerance*length(ray) && thickness<35;
}
// Integer cell hashes avoid the large-world precision instability of sin hashes.
float foamHash(ivec2 p) {
    uvec2 q=uvec2(p)*uvec2(1597334677u,3812015801u);
    uint h=(q.x^q.y)*2246822519u;
    h=(h^(h>>13u))*3266489917u;
    return float(h^(h>>16u))*(1.0/4294967296.0);
}
float foamNoise(vec2 p) {
    ivec2 cell=ivec2(floor(p)); vec2 f=fract(p); f=f*f*(3-2*f);
    return mix(mix(foamHash(cell),foamHash(cell+ivec2(1,0)),f.x),
               mix(foamHash(cell+ivec2(0,1)),foamHash(cell+1),f.x),f.y);
}
float shoreFoam(ivec2 p,vec2 size,vec4 w,vec4 base,vec3 worldRay,float footprint) {
    vec3 waterPosition=worldRay*w.a;
    vec3 ground=worldRay*base.a;
    float depth=waterPosition.y-ground.y;
    // Require a real bottom, above-water camera and sub-metre vertical depth.
    // In particular, a shallow *flat* bay is not a coastline.
    if(depth<0 || depth>.65 || length(waterPosition)>65 || footprint>.3) return 0;
    float coast=0;
    const vec2 directions[8]=vec2[8](vec2(1,0),vec2(-1,0),vec2(0,1),vec2(0,-1),
        vec2(.7071,.7071),vec2(-.7071,.7071),vec2(.7071,-.7071),vec2(-.7071,-.7071));
    for(int i=0;i<8;++i) {
        // Project a bounded WORLD-space footprint onto the existing terrain capture.
        // Probe dry terrain as well as water; a missing water-mask neighbor is not a shore.
        vec3 target=waterPosition+vec3(directions[i].x,0,directions[i].y)*2.2;
        vec3 view=transpose(mat3(water.camera))*target;
        if(view.z<=0) continue;
        vec2 uv=(view.xy/view.z*water.projection.xy*vec2(1,-1)+water.jitter.xy)*.5+.5;
        ivec2 q=ivec2(floor(uv*size));
        if(any(lessThan(q,ivec2(0))) || any(greaterThanEqual(q,ivec2(size)))) continue;
        float z=texelFetch(background,q,0).a;
        if(z<=0) continue;
        vec3 land=mat3(water.camera)*rayAt(vec2(q),size)*z;
        float error=length(land.xz-target.xz);
        float span=length(land.xz-ground.xz);
        float rise=land.y-ground.y;
        float slope=rise/max(span,.01);
        float above=land.y-waterPosition.y;
        // Reject foreground, large depth jumps, cliffs and unreliable grazing probes.
        // The capture precedes objects, so vehicle/boat bottoms cannot seed this mask.
        if(error>1.0 || span<.15 || span>3.3 || above<0 || rise>1.5) continue;
        float shoreDistance=depth/max(slope,.001);
        float candidate=(1-smoothstep(.9,1.9,shoreDistance))*smoothstep(.012,.06,slope)*
                        (1-smoothstep(.65,1.1,slope))*(1-smoothstep(.5,1.0,error));
        candidate*=smoothstep(0,.06,above);
        coast=max(coast,candidate);
    }
    if(coast<=0) return 0;
    vec2 world=waterPosition.xz+water.originTime.xz;
    float time=water.originTime.w;
    // Same slow wave clock as stock TGWater, with bounded drift and no independent sea simulation.
    float wave=sin(time*.5);
    vec2 drift=vec2(.11,.055)*wave+w.xy*.08;
    float broad=foamNoise((world+drift)*.9);
    float dissolve=foamNoise(world*.47+vec2(time*.025,-time*.018));
    float fine=foamNoise((world+drift)*3.7);
    float detail=1-smoothstep(.045,.16,footprint);
    float broken=smoothstep(.46,.69,broad*.72+dissolve*.28);
    broken*=mix(.65,smoothstep(.25,.72,fine),detail);
    // Soft changing width, never a solid depth contour or bright white outline.
    float band=1-smoothstep(.13+wave*.018,.48+broad*.12,depth);
    return .48*coast*band*broken*w.z*(1-smoothstep(30,65,length(waterPosition)))*
           (1-smoothstep(.12,.3,footprint));
}
vec4 refraction(ivec2 p,ivec2 size,vec4 w,vec4 base,float thickness,vec3 worldRay,float distance) {
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
        valid=bottom(b,vec2(size),c[0],t[0],false) && bottom(b+ivec2(1,0),vec2(size),c[1],t[1],false) &&
              bottom(b+ivec2(0,1),vec2(size),c[2],t[2],false) && bottom(b+1,vec2(size),c[3],t[3],false);
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
    return vec4(color*absorption,weight);
}
void main() {
    transmitted=vec4(0);
    ivec2 size=textureSize(background,0),p=ivec2(gl_FragCoord.xy);
    vec4 w=surface(p),base; float thickness;
    vec3 ray=rayAt(vec2(p),vec2(size));
    vec3 worldRay=mat3(water.camera)*ray;
    // Derivatives must execute before pixel-dependent exits (also at terrain/foreground edges).
    vec2 worldSurface=worldRay.xz*w.a;
    float footprint=max(length(dFdx(worldSurface)),length(dFdy(worldSurface)));
    if(w.a<=0 || w.z<=.001 || !bottom(p,vec2(size),base,thickness,water.effects.y>.5)) return;
    if(worldRay.y>=-.015) return;
    float distance=length(ray)*w.a;
    if(distance>=80) return;
    if(water.effects.x>.5 && w.x>=-1) transmitted=refraction(p,size,w,base,thickness,worldRay,distance);
    if(water.effects.y>.5) {
        if(w.x<-1) { w.z*=1-clamp(-w.x-2,0,1); w.x=0; }
        float alpha=shoreFoam(p,vec2(size),w,base,worldRay,footprint);
        if(alpha<=0) return;
        // Terrain capture is already lit, night-adapted and fogged. A subdued neutral
        // scattering tint follows that illumination; there is no emissive white floor.
        float light=dot(base.rgb,vec3(.2126,.7152,.0722));
        vec3 foam=mix(base.rgb,light*vec3(1.02,1.04,1.04),.8)*1.8;
        float combined=alpha+transmitted.a*(1-alpha);
        transmitted=vec4((foam*alpha+transmitted.rgb*transmitted.a*(1-alpha))/combined,combined);
    }
}
