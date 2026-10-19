// Shared ray-scene ABI and cutout traversal. ShapeVertex is eight float words.
struct Material {
    uint64_t indices, vertices, alpha;
    uint first, index16, width, height, samplerFlags;
    float cutoff;
    uint64_t color;
    uint colorWidth, colorHeight;
    uint64_t normalRoughness;
    uint normalWidth, normalHeight;
    vec4 enhanced;
};
layout(buffer_reference,std430,buffer_reference_align=8) readonly buffer Materials { Material m[]; };
layout(buffer_reference,std430,buffer_reference_align=4) readonly buffer Words { uint data[]; };
layout(buffer_reference,std430,buffer_reference_align=4) readonly buffer Floats { float data[]; };
layout(set=0,binding=5,std430) readonly buffer Instances { uint64_t materials[]; };
uint vertexIndex(Material m,uint index) {
    uint word=Words(m.indices).data[m.index16!=0?index/2:index];
    return m.index16!=0?(word>>((index&1)*16))&65535:word;
}
vec2 vertexUV(Material m,uint index) {
    uint vertex=vertexIndex(m,index);
    return vec2(Floats(m.vertices).data[vertex*8+6],Floats(m.vertices).data[vertex*8+7]);
}
float alphaTexel(Material m,ivec2 p) {
    ivec2 size=ivec2(m.width,m.height);
    p.x=(m.samplerFlags&1)!=0?clamp(p.x,0,size.x-1):((p.x%size.x)+size.x)%size.x;
    p.y=(m.samplerFlags&2)!=0?clamp(p.y,0,size.y-1):((p.y%size.y)+size.y)%size.y;
    return float(Words(m.alpha).data[p.y*size.x+p.x]>>24)/255;
}
bool opaqueHit(rayQueryEXT query) {
    Material m=Materials(materials[rayQueryGetIntersectionInstanceCustomIndexEXT(query,false)&65535u]).m[rayQueryGetIntersectionGeometryIndexEXT(query,false)];
    if(m.alpha==uint64_t(0)) return true;
    uint first=m.first+rayQueryGetIntersectionPrimitiveIndexEXT(query,false)*3;
    vec2 bary=rayQueryGetIntersectionBarycentricsEXT(query,false);
    vec2 uv=vertexUV(m,first)*(1-bary.x-bary.y)+vertexUV(m,first+1)*bary.x+vertexUV(m,first+2)*bary.y;
    vec2 p=uv*vec2(m.width,m.height)-.5;
    if((m.samplerFlags&4)!=0) return alphaTexel(m,ivec2(floor(p+.5)))>=m.cutoff;
    ivec2 base=ivec2(floor(p)); vec2 f=fract(p);
    float a=mix(mix(alphaTexel(m,base),alphaTexel(m,base+ivec2(1,0)),f.x),
                mix(alphaTexel(m,base+ivec2(0,1)),alphaTexel(m,base+ivec2(1,1)),f.x),f.y);
    return a>=m.cutoff;
}
