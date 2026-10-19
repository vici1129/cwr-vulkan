#ifndef CWR_MATERIAL_NORMAL
#define CWR_MATERIAL_NORMAL
// dP/dUV in OBJECT space. Preserve the UV determinant's sign (mirrored charts)
// and orthonormalize before transforming the final normal with inverse transpose.
// Raster derivatives and ray-triangle edges use this exact same convention:
// normal RGB maps +X/+Y toward increasing U/V; +Z is the original outward normal.
vec3 materialNormal(vec3 n, vec3 e1, vec3 e2, vec2 uv1, vec2 uv2, vec3 map) {
    n=normalize(n);
    float det=uv1.x*uv2.y-uv1.y*uv2.x;
    if(abs(det)<1e-10) return n;
    vec3 t=(e1*uv2.y-e2*uv1.y)/det;
    vec3 b=(e2*uv1.x-e1*uv2.x)/det;
    t-=n*dot(n,t);
    if(dot(t,t)<1e-12 || dot(b,b)<1e-12) return n;
    t=normalize(t);
    float handedness=dot(cross(n,t),b)<0?-1:1;
    return normalize(t*map.x+cross(n,t)*handedness*map.y+n*map.z);
}
#endif
