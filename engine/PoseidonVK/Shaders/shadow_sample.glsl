layout(set=3, binding=0) uniform sampler2DArray shadowMap;
#include "soft_shadow_math.glsl"
// Fixed light-space patterns: no screen noise, frame rotation or extra history.
const vec2 softDisk[8]=vec2[8](
    vec2(.25,.25),vec2(-.25,.25),vec2(.25,-.25),vec2(-.25,-.25),
    vec2(.85,0),vec2(-.85,0),vec2(0,.85),vec2(0,-.85));

float softSunlight(int c, vec2 uv, float depth, float bias, vec3 worldDx, vec3 worldDy) {
    mat4 m=lighting.cascadeVP[c];
    vec3 x=vec3(m[0][0],m[1][0],m[2][0]);
    vec3 y=vec3(m[0][1],m[1][1],m[2][1]);
    vec3 z=vec3(m[0][2],m[1][2],m[2][2]);
    vec2 uvPerMetre=vec2(length(x),length(y))*.5;
    float depthPerMetre=max(length(z),1e-8);
    float texel=lighting.shadowStrength.y;
    // Receiver-plane depth correction prevents wider kernels shadowing their
    // own sloping receiver. Bound it at grazing/discontinuous derivatives.
    vec3 sx=vec3(dot(x,worldDx)*.5,-dot(y,worldDx)*.5,dot(z,worldDx));
    vec3 sy=vec3(dot(x,worldDy)*.5,-dot(y,worldDy)*.5,dot(z,worldDy));
    float det=sx.x*sy.y-sx.y*sy.x;
    vec2 gradient=abs(det)>1e-12 ? vec2(sy.y*sx.z-sx.y*sy.z,sx.x*sy.z-sy.x*sx.z)/det : vec2(0);
    float correctionLimit=max(bias*2,depthPerMetre*.05);
    // Target 60 cm of search, clamped to 1-8 texels regardless of depth padding.
    vec2 search=min(max(uvPerMetre*.6,vec2(texel)),vec2(texel*8));
    float separation=0, blockers=0, contactLit=0;
    for(int i=0;i<13;++i) {
        vec2 offset=i<9 ? vec2(i%3-1,i/3-1)*texel : softDisk[i-5]*search;
        offset=clamp(uv+offset,vec2(texel),vec2(1-texel))-uv;
        float receiver=depth-bias+clamp(dot(gradient,offset),-correctionLimit,correctionLimit);
        float blocker=textureLod(shadowMap,vec3(uv+offset,float(c)),0).r;
        if(blocker<receiver) {separation+=receiver-blocker; blockers+=1;}
        if(i<9) contactLit+=blocker<depth-bias ? 0.0 : 1.0;
    }
    if(blockers==0) return 1;
    separation/=blockers*depthPerMetre;
    vec2 radius=vec2(softShadowRadius(separation,lighting.shadowStrength.z,uvPerMetre.x,texel),
                     softShadowRadius(separation,lighting.shadowStrength.z,uvPerMetre.y,texel));
    if(max(radius.x,radius.y)<=texel) return contactLit/9;
    float widen=smoothstep(1.0,2.0,max(radius.x,radius.y)/texel);
    // Do not sample the white border as light: narrow the kernel at map edges.
    radius=min(radius,max(min(uv,1-uv)-texel,vec2(0)));
    float lit=0;
    for(int i=0;i<8;++i) {
        vec2 offset=softDisk[i]*radius;
        vec2 sampleUV=clamp(uv+offset,vec2(texel),vec2(1-texel));
        vec2 pixel=sampleUV/texel-.5;
        vec2 f=fract(pixel);
        vec2 centerOffset=(floor(pixel)+.5)*texel-uv;
        vec4 receiver=vec4(depth-bias)+clamp(vec4(
            dot(gradient,centerOffset+vec2(0,texel)),dot(gradient,centerOffset+vec2(texel)),
            dot(gradient,centerOffset+vec2(texel,0)),dot(gradient,centerOffset)),
            vec4(-correctionLimit),vec4(correctionLimit));
        vec4 taps=step(receiver,textureGather(shadowMap,vec3(sampleUV,float(c)),0));
        lit+=mix(mix(taps.w,taps.z,f.x),mix(taps.x,taps.y,f.x),f.y);
    }
    return mix(contactLit/9,lit/8,widen);
}
// GL33's tier selection, coverage fallthrough, 3x3 PCF and transition bands.
float sunlightVisibility(vec3 relativeWorld, float fog) {
    // Evaluate derivatives before divergent cascade selection / coverage tests.
    vec3 worldDx=vec3(0),worldDy=vec3(0);
    if(lighting.shadowStrength.z>0) {worldDx=dFdx(relativeWorld); worldDy=dFdy(relativeWorld);}
    if (lighting.shadowReceiver.x < 0.5 || lighting.cascadeControl.x < 0.5) return 1.0;
    int count = int(lighting.cascadeControl.x);
    int omni = int(lighting.cascadeControl.w);
    float eyeDepth = dot(relativeWorld, lighting.shadowForward.xyz);
    float distance3D = length(relativeWorld);
    int first = count;
    for (int i=0; i<4; ++i) {
        if (i >= count) break;
        if ((i < omni ? distance3D : eyeDepth) <= lighting.cascadeSplits[i]) { first=i; break; }
    }
    if (first == count) return 1.0;
    float previous = first > 0 ? lighting.cascadeSplits[first-1] : 0.0;
    float band = (lighting.cascadeSplits[first] - previous) * 0.15;
    float metric = first < omni ? distance3D : eyeDepth;
    float blend = first+1 < count ? clamp((metric - lighting.cascadeSplits[first] + band) / max(band, 0.001), 0.0, 1.0) : 0.0;
    float litSum=0.0, weightSum=0.0;
    for (int p=0; p<4; ++p) {
        int c=first+p;
        if (c >= count) break;
        float weight = p==0 ? 1.0-blend : (weightSum<=0.0 ? 1.0 : (p==1 ? blend : 0.0));
        if (weight<=0.0) continue;
        vec4 projected=lighting.cascadeVP[c] * vec4(relativeWorld, 1.0);
        vec3 sc=projected.xyz / projected.w;
        vec2 uv=sc.xy * vec2(0.5, -0.5) + 0.5;
        if (all(greaterThan(uv,vec2(0.0))) && all(lessThan(uv,vec2(1.0))) && sc.z>0.0 && sc.z<1.0) {
            float bias=lighting.cascadeControl.z * float((c+1)*(c+1));
            float lit=0.0;
            if(lighting.shadowStrength.z>0)
                lit=9.0*softSunlight(c,uv,sc.z,bias,worldDx,worldDy);
            else
                for (int y=-1; y<=1; ++y) for (int x=-1; x<=1; ++x)
                    lit += sc.z-bias > texture(shadowMap,vec3(uv+vec2(x,y)*lighting.shadowStrength.y,float(c))).r ? 0.0 : 1.0;
            litSum += weight * lit / 9.0;
            weightSum += weight;
        }
    }
    if (weightSum<=0.0) return 1.0;
    float fade=clamp((lighting.cascadeSplits[count-1]-eyeDepth)/max(lighting.cascadeControl.y,0.001),0.0,1.0);
    return mix(1.0,lighting.shadowStrength.x,(1.0-litSum/weightSum)*fade*clamp(fog,0.0,1.0));
}
