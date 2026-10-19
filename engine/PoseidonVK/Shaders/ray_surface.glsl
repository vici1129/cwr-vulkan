// Surface reconstruction only: no illumination, fog, eye response or tone mapping.
#include "color_space.glsl"
#include "ray_normal.glsl"
#include "material_normal.glsl"
#include "material_diffuse.glsl"
// High sampler bits are metadata; low three bits retain the existing sampler ABI.
const uint RayDiffuse = 8u, RayVertexNormals = 16u, RayTerrainUV = 32u;
const uint RayNormalMap = 64u, RayRoughnessMap = 128u;
const uint HitPosition = 1u, HitNormal = 2u, HitUV = 4u, HitColor = 8u,
    HitAlbedo = 16u, HitVertexNormal = 32u, HitAlphaTest = 64u, HitApproximateUV = 128u;
struct HitSurface {
    vec3 position, geometricNormal, normal;
    vec2 uv;
    vec3 textureColorLinear, albedo;
    uint available; // Hit* bits classify missing data, cutouts and approximate terrain UVs.
    uint kind; // Existing scene class: 0 static, 1 extended alpha, 2 terrain, 3 dynamic.
    bool diffuseEligible;
    uint materialIdentity;
    float roughness, metallic;
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

vec3 hitWeights(rayQueryEXT query) {
    vec2 bary=rayQueryGetIntersectionBarycentricsEXT(query,true);
    return vec3(1-bary.x-bary.y,bary);
}
void hitTriangle(rayQueryEXT query,Material m,uint first,out vec3 a,out vec3 b,out vec3 c) {
    mat4x3 model=rayQueryGetIntersectionObjectToWorldEXT(query,true);
    a=model*vec4(vertexPosition(m,first),1);
    b=model*vec4(vertexPosition(m,first+1),1);
    c=model*vec4(vertexPosition(m,first+2),1);
}
vec3 hitFaceNormal(vec3 a,vec3 b,vec3 c,vec3 direction) {
    vec3 face=cross(b-a,c-a);
    if(!usableNormal(face)) return vec3(0);
    face=normalize(face);
    return dot(face,direction)>0 ? -face : face;
}
vec2 hitUV(Material m,uint first,vec3 weights) {
    return vertexUV(m,first)*weights.x+vertexUV(m,first+1)*weights.y+vertexUV(m,first+2)*weights.z;
}
bool hasHitColor(Material m) {
    return m.color!=uint64_t(0) && m.colorWidth>0u && m.colorHeight>0u;
}
bool usableUV(vec2 uv) { return !any(isnan(uv)) && !any(isinf(uv)); }

// Only the inputs used by legacy water. No hit position, vertex normals,
// inverse transpose, diffuse classification or validation instrumentation.
struct ReflectionSurface {
    vec3 geometricNormal, textureColorLinear;
    bool available, alphaTest;
};
ReflectionSurface reflectionSurface(rayQueryEXT query) {
    ReflectionSurface s;
    s.geometricNormal=vec3(0); s.textureColorLinear=vec3(0);
    s.available=false; s.alphaTest=false;
    // Caller has established a committed triangle, just as the water path did
    // before the foundation. Keep missing-material and degenerate-data fallback.
    uint custom=rayQueryGetIntersectionInstanceCustomIndexEXT(query,true);
    uint64_t address=materials[custom&65535u];
    if(address==uint64_t(0)) return s;
    Material m=Materials(address).m[rayQueryGetIntersectionGeometryIndexEXT(query,true)];
    if(m.indices==uint64_t(0) || m.vertices==uint64_t(0) || !hasHitColor(m)) return s;
    uint first=m.first+rayQueryGetIntersectionPrimitiveIndexEXT(query,true)*3;
    vec3 a,b,c;
    hitTriangle(query,m,first,a,b,c);
    s.geometricNormal=hitFaceNormal(a,b,c,rayQueryGetWorldRayDirectionEXT(query));
    vec2 uv=hitUV(m,first,hitWeights(query));
    if(dot(s.geometricNormal,s.geometricNormal)==0 || !usableUV(uv)) return s;
    s.textureColorLinear=sampleColor(m,uv);
    s.available=true; s.alphaTest=m.alpha!=uint64_t(0);
    return s;
}

// Committed triangles only. Never dereference material addresses for a miss.
// Normals face the incident ray (the scene is deliberately two-sided). The
// geometric normal remains separate for ray offsets and legacy water lighting.
HitSurface hitSurface(rayQueryEXT query) {
    HitSurface s;
    s.position=vec3(0); s.geometricNormal=vec3(0); s.normal=vec3(0);
    s.uv=vec2(0); s.textureColorLinear=vec3(0); s.albedo=vec3(0);
    s.available=0u; s.kind=0u; s.diffuseEligible=false;
    s.materialIdentity=0u; s.roughness=1; s.metallic=0;
    if(rayQueryGetIntersectionTypeEXT(query,true)!=gl_RayQueryCommittedIntersectionTriangleEXT) return s;
    uint custom=rayQueryGetIntersectionInstanceCustomIndexEXT(query,true);
    s.kind=custom>>20;
    uint64_t address=materials[custom&65535u];
    if(address==uint64_t(0)) return s;
    Material m=Materials(address).m[rayQueryGetIntersectionGeometryIndexEXT(query,true)];
    if(m.indices==uint64_t(0) || m.vertices==uint64_t(0)) return s;
    uint first=m.first+rayQueryGetIntersectionPrimitiveIndexEXT(query,true)*3;
    vec3 weights=hitWeights(query);
    vec3 a,b,c;
    hitTriangle(query,m,first,a,b,c);
    s.position=a*weights.x+b*weights.y+c*weights.z;
    if(finiteVector(s.position)) s.available|=HitPosition;
    vec3 face=hitFaceNormal(a,b,c,rayQueryGetWorldRayDirectionEXT(query));
    if(dot(face,face)>0) {
        s.geometricNormal=face; s.normal=face; s.available|=HitNormal;
        // Only use validated original shape normals or the original terrain
        // stencil. Missing/unusable data retains the actual triangle normal.
        if((m.samplerFlags&RayVertexNormals)!=0u) {
            vec3 na=vertexNormal(m,first), nb=vertexNormal(m,first+1), nc=vertexNormal(m,first+2);
            vec4 normal=rayShadingNormal(face,na,nb,nc,weights,
                mat3(rayQueryGetIntersectionWorldToObjectEXT(query,true)),rayQueryGetWorldRayDirectionEXT(query));
            s.normal=normal.xyz;
            if(normal.w>0) s.available|=HitVertexNormal;
        }
    }
    s.uv=hitUV(m,first,weights);
    if(usableUV(s.uv)) s.available|=HitUV;
    if(m.alpha!=uint64_t(0)) s.available|=HitAlphaTest;
    if((m.samplerFlags&RayTerrainUV)!=0u) s.available|=HitApproximateUV;
    if((s.available&HitUV)!=0u && hasHitColor(m)) {
        s.textureColorLinear=sampleColor(m,s.uv);
        s.available|=HitColor;
        if((m.samplerFlags&RayDiffuse)!=0u) {
            s.albedo=s.textureColorLinear; s.available|=HitAlbedo;
        }
    }
    if(m.enhanced.w>.5 &&
       (s.available&(HitUV|HitVertexNormal|HitAlbedo))==(HitUV|HitVertexNormal|HitAlbedo)) {
        s.materialIdentity=uint(m.enhanced.w); s.roughness=m.enhanced.x; s.metallic=m.enhanced.y;
        s.albedo=materialDiffuse(s.textureColorLinear,m.enhanced.w,m.enhanced.z,s.metallic);
        if(m.normalRoughness!=uint64_t(0) && m.normalWidth>0 && m.normalHeight>0) {
        // Same linear data sampling and tangent convention as raster. Preserve
        // the original geometric normal for ray offsets and hemisphere tests.
        Material map=m;
        map.color=m.normalRoughness;map.colorWidth=m.normalWidth;map.colorHeight=m.normalHeight;
        vec2 pixel=s.uv*vec2(map.colorWidth,map.colorHeight)-.5;
        ivec2 base=ivec2(floor(pixel));vec2 f=fract(pixel);
        vec4 data=mix(mix(colorTexel(map,base),colorTexel(map,base+ivec2(1,0)),f.x),
            mix(colorTexel(map,base+ivec2(0,1)),colorTexel(map,base+ivec2(1,1)),f.x),f.y);
        if((m.samplerFlags&RayNormalMap)!=0u) {
        mat3 worldToObject=mat3(rayQueryGetIntersectionWorldToObjectEXT(query,true));
        mat3 objectToWorld=mat3(rayQueryGetIntersectionObjectToWorldEXT(query,true));
        vec3 objectNormal=normalize(transpose(objectToWorld)*s.normal);
        vec3 mapped=materialNormal(objectNormal,vertexPosition(m,first+1)-vertexPosition(m,first),
            vertexPosition(m,first+2)-vertexPosition(m,first),vertexUV(m,first+1)-vertexUV(m,first),
            vertexUV(m,first+2)-vertexUV(m,first),normalize(data.xyz*2-1));
        s.normal=normalize(transpose(worldToObject)*mapped);
        }
        if((m.samplerFlags&RayRoughnessMap)!=0u) s.roughness=clamp(data.a*m.enhanced.x,.045,1);
        }
    }
    // Cutouts retain real traversal/color data, but no inferred leaf transmission
    // or two-sided diffuse bounce. The GI caller preserves legacy ambient for
    // unsupported endpoints; water keeps its original reflection contract.
    s.diffuseEligible=(s.available&(HitPosition|HitNormal|HitUV|HitAlbedo|HitAlphaTest))==
        (HitPosition|HitNormal|HitUV|HitAlbedo);
    return s;
}
