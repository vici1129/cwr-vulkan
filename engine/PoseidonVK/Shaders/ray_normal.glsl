// Kept independent of ray-query/buffer access so the exact math is unit tested.
bool finiteVector(vec3 v) { return !any(isnan(v)) && !any(isinf(v)); }
bool usableNormal(vec3 v) { return finiteVector(v) && dot(v,v)>1e-12 && dot(v,v)<1e20; }
vec4 rayShadingNormal(vec3 face, vec3 na, vec3 nb, vec3 nc, vec3 weights,
                     mat3 worldToObject, vec3 direction) {
    if(usableNormal(na) && usableNormal(nb) && usableNormal(nc)) {
        vec3 n=transpose(worldToObject)*(na*weights.x+nb*weights.y+nc*weights.z);
        if(usableNormal(n)) {
            n=normalize(n);
            // Source normals and triangle winding can be reversed independently.
            if(dot(n,face)<0) n=-n;
            if(dot(n,face)>.1 && dot(n,direction)<=0) return vec4(n,1);
        }
    }
    return vec4(face,0);
}
