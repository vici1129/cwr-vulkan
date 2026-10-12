// Density is an extrusion of the original cloud-type coverage, not a weather
// generator. Noise is stationary in each layer's seed coordinates and advects
// with Weather::CloudsPosition, including pause and accelerated simulation.
float cloudHash(vec3 p) {
    p=fract(p*.1031);
    p+=dot(p,p.yzx+33.33);
    return fract((p.x+p.y)*p.z);
}
float cloudNoise(vec3 p) {
    vec3 i=floor(p), f=fract(p); f=f*f*(3-2*f);
    return mix(mix(mix(cloudHash(i),cloudHash(i+vec3(1,0,0)),f.x),
                   mix(cloudHash(i+vec3(0,1,0)),cloudHash(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(cloudHash(i+vec3(0,0,1)),cloudHash(i+vec3(1,0,1)),f.x),
                   mix(cloudHash(i+vec3(0,1,1)),cloudHash(i+vec3(1)),f.x),f.y),f.z);
}
float cloudThickness(int layer) { return 1200-100*float(layer); }
float cloudDensity(int layer,vec3 p,float footprint,out vec3 tint) {
    float thickness=cloudThickness(layer);
    float h=(p.y-weather.layers[layer].w)/thickness+.5;
    if(h<=0 || h>=1) { tint=vec3(1); return 0; }
    float texelMetres=1/(weather.layers[layer].z*float(textureSize(cloudHigh,0).x));
    vec4 coverage=cloudAtLOD(layer,p.xz,max(0,log2(max(footprint/texelMetres,1))));
    tint=mix(vec3(1),coverage.rgb,.65);
    if(coverage.a<.015 || weather.cloudColor.a<=0) return 0;
    vec3 seed=vec3(p.x-weather.drift[layer].x,p.y,p.z-weather.drift[layer].y);
    float billow=cloudNoise(seed*.0028+float(layer)*17);
    float detail=cloudNoise(seed*.007+vec3(9,3,1));
    billow=mix(billow,.5,smoothstep(120,500,footprint));
    detail=mix(detail,.5,smoothstep(50,180,footprint));
    float bottom=.04+.24*(1-billow);
    float top=.45+.42*billow+.12*coverage.a;
    float profile=smoothstep(bottom,bottom+.16,h)*(1-smoothstep(top-.20,top,h));
    // Erode margins; retain broad stock coverage and its cloud-shadow footprint.
    float body=smoothstep(0,.30,coverage.a*.8-.16-(1-billow)*.30-(1-detail)*.10);
    return body*profile*(.45+.55*detail)*weather.cloudColor.a;
}
vec2 cloudInterval(int layer,vec3 origin,vec3 ray) {
    float halfHeight=cloudThickness(layer)*.5;
    float bottom=weather.layers[layer].w-halfHeight, top=bottom+2*halfHeight;
    if(abs(ray.y)<.0001)
        return origin.y>bottom && origin.y<top ? vec2(0,19000) : vec2(19000,0);
    vec2 t=(vec2(bottom,top)-origin.y)/ray.y;
    return vec2(max(0,min(t.x,t.y)),min(19000,max(t.x,t.y)));
}
vec4 marchClouds(vec3 origin,vec3 ray,int steps,float jitter) {
    if(weather.cloudColor.a<=.001) return vec4(0);
    vec2 intervals[3]; int order[3];
    for(int i=0;i<3;++i) { intervals[i]=cloudInterval(i,origin,ray); order[i]=i; }
    // Supports cameras below, inside or above any of the disjoint layers.
    for(int i=0;i<2;++i) for(int j=i+1;j<3;++j)
        if(intervals[order[j]].x<intervals[order[i]].x) { int k=order[i];order[i]=order[j];order[j]=k; }
    vec3 color=vec3(0); float transmission=1;
    // Stock clouds pack their accommodated lighting into UNORM vertex colors.
    // Preserve that energy ceiling before scattering; unbounded accommodation
    // would flatten all internal shading against the HDR shoulder.
    vec3 base=legacyLightToLinear(clamp(weather.cloudColor.rgb,0,1));
    vec3 total=max(weather.volumeAmbient.rgb+weather.volumeSun.rgb,vec3(.001));
    vec3 directFraction=clamp(weather.volumeSun.rgb/total,0,1);
    float phase=.8+.35*pow(max(dot(ray,weather.sun.xyz),0),8);
    for(int k=0;k<3 && transmission>.015;++k) {
        int layer=order[k]; vec2 segment=intervals[layer];
        if(segment.y<=segment.x) continue;
        float dt=(segment.y-segment.x)/float(steps);
        float extinction=6.0/cloudThickness(layer);
        for(int s=0;s<32 && s<steps && transmission>.015;++s) {
            // Fixed spatial stratification breaks coherent horizontal sample
            // planes. No changing random seed or dedicated temporal history.
            float t=segment.x+(float(s)+jitter)*dt;
            vec3 p=origin+ray*t, tint;
            float density=cloudDensity(layer,p,dt,tint);
            if(density<.002) continue;
            vec3 unused;
            float optical=0;
            if(weather.volumeSun.w>.001 && weather.sun.y>0) {
                float stride=cloudThickness(layer)*.32;
                optical=(cloudDensity(layer,p+weather.sun.xyz*stride,dt,unused)+
                         cloudDensity(layer,p+weather.sun.xyz*stride*2.5,dt,unused))*stride*extinction;
            }
            float sunVisibility=exp(-optical);
            float h=clamp((p.y-weather.layers[layer].w)/cloudThickness(layer)+.5,0,1);
            vec3 light=base*(.20+.22*h+(.40+.35*directFraction)*sunVisibility*phase);
            // Original alpha-fog range: unscaled cloud ray distances, matching
            // TL DoCloudLighting. Smooth terminal fade removes the finite end.
            float fade=clamp(exp(2.9957322736*(4000-t)/19000),0,1)*(1-smoothstep(16000,19000,t));
            float alpha=(1-exp(-density*extinction*dt))*fade;
            color+=transmission*alpha*tint*light;
            transmission*=1-alpha;
        }
    }
    return vec4(color,1-transmission);
}
