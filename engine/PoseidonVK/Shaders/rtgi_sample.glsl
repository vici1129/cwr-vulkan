// The existing RGBA32F ray result carries radiance plus compact surface data.
// 14 depth bits (1/256 m, <64 m) + two 5-bit octahedral normal coordinates
// fit exactly in float's 24 integer bits. The sign marks dynamic-hit history
// as unsafe; zero remains an ineligible sample. No extra geometry target.
float giSampleDepth(float data) { return float(uint(abs(data))>>10)/256.; }
vec2 giSign(vec2 v) { return vec2(v.x>=0?1:-1,v.y>=0?1:-1); }
float giSampleData(float depth,vec3 worldNormal) {
    vec3 n=worldNormal/(abs(worldNormal.x)+abs(worldNormal.y)+abs(worldNormal.z));
    vec2 oct=n.z>=0?n.xy:(1-abs(n.yx))*giSign(n.xy);
    uvec2 code=uvec2(floor(clamp(oct*.5+.5,0,1)*31+.5));
    uint z=uint(clamp(floor(depth*256+.5),1,16383));
    return float((z<<10)|(code.y<<5)|code.x);
}
vec3 giSampleNormal(float data) {
    uint bits=uint(abs(data));
    vec2 oct=vec2(bits&31u,(bits>>5)&31u)*(2./31.)-1;
    vec3 n=vec3(oct,1-abs(oct.x)-abs(oct.y));
    if(n.z<0) n.xy=(1-abs(n.yx))*giSign(n.xy);
    return normalize(n);
}
vec3 giPosition(ivec2 full,float z) {
    vec2 ndc=(vec2(full)+.5)/vec2(depthSize())*2-1-ao.jitter.xy;
    return vec3(ndc*vec2(1,-1)/ao.projection.xy*z,z);
}
float giPlaneWeight(vec3 p,vec3 n,vec3 worldNormal,ivec2 other,float data) {
    if(data==0) return 0;
    vec3 op=giPosition(other,giSampleDepth(data));
    float plane=abs(dot(op-p,n));
    if(dot(worldNormal,giSampleNormal(data))<.9 ||
       plane>max(.025,p.z*.001) || abs(p.z-op.z)>max(.2,p.z*.02)) return 0;
    return exp(-plane/max(.01,p.z*.0005));
}
