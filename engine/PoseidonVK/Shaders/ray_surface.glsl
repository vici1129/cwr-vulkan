// Surface reconstruction only: no illumination, fog, eye response or tone mapping.
#include "color_space.glsl"
#include "ray_normal.glsl"
// High sampler bits are metadata; low three bits retain the existing sampler ABI.
const uint RayDiffuse = 8u, RayVertexNormals = 16u, RayTerrainUV = 32u;
const uint HitPosition = 1u, HitNormal = 2u, HitUV = 4u, HitColor = 8u,
    HitAlbedo = 16u, HitVertexNormal = 32u, HitAlphaTest = 64u, HitApproximateUV = 128u;
struct HitSurface {
    vec3 position, geometricNormal, normal;
    vec2 uv;
    vec3 textureColorLinear, albedo;
    uint available; // Hit* bits classify missing data, cutouts and approximate terrain UVs.
    uint kind; // Existing scene class: 0 static, 1 extended alpha, 2 terrain, 3 dynamic.
    bool diffuseEligible;
};
vec3 vertexPosition(Material m,uint index) {
    uint offset=vertexIndex(m,index)*8;
    return vec3(Floats(m.vertices).data[offset],Floats(m.vertices).data[offset+1],Floats(m.vertices).data[offset+2]);
}
vec4 colorTexel(Material m,ivec2 p) {
    ivec2 size=ivec2(m.colorWidth,m.colorHeight);
    p.x=(m.samplerFlags&1)!=0?clamp(p.x,0,size.x-1):((p.x%size.x)+size.x)%size.x;
    p.y=(m.samplerFlags&2)!=0?clamp(p.y,0,size.y-1):((p.y%size.y)+size.y)%size.y;
    return unpackUnorm4x8(Words(m.color).data[p.y*size.x+p.x]);
}
vec3 sampleColor(Material m,vec2 uv) {
    vec2 p=uv*vec2(m.colorWidth,m.colorHeight)-.5;
    if((m.samplerFlags&4)!=0) return toLinear(colorTexel(m,ivec2(floor(p+.5))).rgb);
    ivec2 b=ivec2(floor(p)); vec2 f=fract(p);
    return mix(mix(toLinear(colorTexel(m,b).rgb),toLinear(colorTexel(m,b+ivec2(1,0)).rgb),f.x),
        mix(toLinear(colorTexel(m,b+ivec2(0,1)).rgb),toLinear(colorTexel(m,b+ivec2(1,1)).rgb),f.x),f.y);
}

vec3 vertexNormal(Material m,uint index) {
    uint offset=vertexIndex(m,index)*8+3;
    return vec3(Floats(m.vertices).data[offset],Floats(m.vertices).data[offset+1],Floats(m.vertices).data[offset+2]);
}

// Committed triangles only. Never dereference material addresses for a miss.
// Normals face the incident ray (the scene is deliberately two-sided). The
// geometric normal remains separate for ray offsets and legacy water lighting.
HitSurface hitSurface(rayQueryEXT query) {
    HitSurface s;
    s.position=vec3(0); s.geometricNormal=vec3(0); s.normal=vec3(0);
    s.uv=vec2(0); s.textureColorLinear=vec3(0); s.albedo=vec3(0);
    s.available=0u; s.kind=0u; s.diffuseEligible=false;
    if(rayQueryGetIntersectionTypeEXT(query,true)!=gl_RayQueryCommittedIntersectionTriangleEXT) return s;
    uint custom=rayQueryGetIntersectionInstanceCustomIndexEXT(query,true);
    s.kind=custom>>20;
    uint64_t address=materials[custom&65535u];
    if(address==uint64_t(0)) return s;
    Material m=Materials(address).m[rayQueryGetIntersectionGeometryIndexEXT(query,true)];
    if(m.indices==uint64_t(0) || m.vertices==uint64_t(0)) return s;
    uint first=m.first+rayQueryGetIntersectionPrimitiveIndexEXT(query,true)*3;
    vec2 bary=rayQueryGetIntersectionBarycentricsEXT(query,true);
    vec3 weights=vec3(1-bary.x-bary.y,bary);
    mat4x3 model=rayQueryGetIntersectionObjectToWorldEXT(query,true);
    vec3 a=model*vec4(vertexPosition(m,first),1);
    vec3 b=model*vec4(vertexPosition(m,first+1),1);
    vec3 c=model*vec4(vertexPosition(m,first+2),1);
    s.position=a*weights.x+b*weights.y+c*weights.z;
    if(finiteVector(s.position)) s.available|=HitPosition;
    vec3 face=cross(b-a,c-a);
    if(usableNormal(face)) {
        face=normalize(face);
        if(dot(face,rayQueryGetWorldRayDirectionEXT(query))>0) face=-face;
        s.geometricNormal=face; s.normal=face; s.available|=HitNormal;
        // Terrain's (0,1,0) values are placeholders, not sampled terrain normals.
        if((m.samplerFlags&RayVertexNormals)!=0u) {
            vec3 na=vertexNormal(m,first), nb=vertexNormal(m,first+1), nc=vertexNormal(m,first+2);
            vec4 normal=rayShadingNormal(face,na,nb,nc,weights,
                mat3(rayQueryGetIntersectionWorldToObjectEXT(query,true)),rayQueryGetWorldRayDirectionEXT(query));
            s.normal=normal.xyz;
            if(normal.w>0) s.available|=HitVertexNormal;
        }
    }
    s.uv=vertexUV(m,first)*weights.x+vertexUV(m,first+1)*weights.y+vertexUV(m,first+2)*weights.z;
    if(!any(isnan(s.uv)) && !any(isinf(s.uv))) s.available|=HitUV;
    if(m.alpha!=uint64_t(0)) s.available|=HitAlphaTest;
    if((m.samplerFlags&RayTerrainUV)!=0u) s.available|=HitApproximateUV;
    if((s.available&HitUV)!=0u && m.color!=uint64_t(0) && m.colorWidth>0u && m.colorHeight>0u) {
        s.textureColorLinear=sampleColor(m,s.uv);
        s.available|=HitColor;
        if((m.samplerFlags&RayDiffuse)!=0u) {
            s.albedo=s.textureColorLinear; s.available|=HitAlbedo;
        }
    }
    // Cutouts retain real traversal/color data, but no inferred leaf transmission
    // or two-sided diffuse bounce. Unsupported/missing materials contribute zero.
    s.diffuseEligible=(s.available&(HitPosition|HitNormal|HitUV|HitAlbedo|HitAlphaTest))==
        (HitPosition|HitNormal|HitUV|HitAlbedo);
    return s;
}
