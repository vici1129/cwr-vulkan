#ifndef CWR_MATERIAL_RECORD
#define CWR_MATERIAL_RECORD
// Four spare bits above the unchanged 20-bit oct-normal carry family/CPU route.
// All 24-bit integers round-trip exactly in the existing RGBA32F GI attachment.
float materialReceiverCode(float normalCode,int family) { return normalCode+float(family)*1048576.f; }
int materialReceiverClass(float code) { return int(code)/1048576; }
#ifndef CWR_MATERIAL_CLASS_ONLY
// RTGI alone upgrades the shared non-water record to RGBA32F. Integers below
// 2^24 round-trip exactly. Positive depth remains the unmodified water ABI.
bool enhancedRecord(vec4 s) { return s.w<0 && s.r<=-3; }
vec2 materialOctSign(vec2 v) { return vec2(v.x>=0?1:-1,v.y>=0?1:-1); }
float materialNormalCode(vec3 n) {
    n/=abs(n.x)+abs(n.y)+abs(n.z);
    vec2 oct=n.z>=0?n.xy:(1-abs(n.yx))*materialOctSign(n.xy);
    uvec2 q=uvec2(round(clamp(oct*.5+.5,0,1)*1023));
    return float(q.x|(q.y<<10));
}
vec3 materialDecodeNormal(float code) {
    uint q=uint(code);
    vec2 oct=vec2(q&1023u,(q>>10)&1023u)*(2./1023.)-1;
    vec3 n=vec3(oct,1-abs(oct.x)-abs(oct.y));
    if(n.z<0) n.xy=(1-abs(n.yx))*materialOctSign(n.xy);
    return normalize(n);
}
vec4 materialRecord(vec3 diffuseReflectance,vec3 normal,float fog,float depth,int family) {
    uvec3 c=uvec3(round(clamp(diffuseReflectance,0,.96)*255));
    return vec4(-float(c.r|(c.g<<8)|(c.b<<16))-3,materialReceiverCode(materialNormalCode(normal),family),fog,-depth);
}
vec3 materialReflectance(vec4 s) {
    uint c=uint(-s.r-3);
    return vec3(c&255u,(c>>8)&255u,(c>>16)&255u)/255.;
}
#endif
#endif
