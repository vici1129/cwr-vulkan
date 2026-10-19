// Cosine-weighted Lambert integration: E/pi = mean(visible sky radiance).
// The receiver uses the same cancellation. Never add a second pi or ambient.
vec3 giCosineDirection(vec3 n,float u,float phi) {
    vec3 tangent=normalize(cross(abs(n.y)<.95?vec3(0,1,0):vec3(1,0,0),n));
    return tangent*(cos(phi)*sqrt(u))+cross(n,tangent)*(sin(phi)*sqrt(u))+n*sqrt(1-u);
}
bool giSkyHemisphere(vec3 direction,vec3 geometricNormal) {
    return direction.y>0 && dot(direction,geometricNormal)>0;
}
vec3 giSkyGradient(vec3 zenith,vec3 horizon,float elevation) {
    float t=sqrt(clamp(elevation,0.f,1.f));
    return horizon*(1-t)+zenith*t;
}
vec3 giLambertOutgoing(vec3 reflectance,vec3 sun,vec3 sky) { return reflectance*(sun+sky); }
bool giSunComponent(float components) { return components==1 || components==3; }
bool giSkyComponent(float components) { return components==2 || components==3; }
float giSkyLuminance(vec3 c) { return dot(c,vec3(.2126,.7152,.0722)); }
// Authored backdrop radiance has no irradiance units. Anchor the unobstructed
// dome's E/pi to the engine's diffuse reference, without changing sky chroma.
// Local clouds and scene occlusion are deliberately NOT in this denominator.
float giSkyCalibration(vec3 reference,vec3 domeEpi) {
    float y=giSkyLuminance(domeEpi);
    return y>1e-6 ? max(giSkyLuminance(reference),0.f)/y : 1.f;
}
