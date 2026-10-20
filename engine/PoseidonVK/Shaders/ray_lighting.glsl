// Engine lighting inputs shared with hit shading. No BRDF or bounce evaluation.
#include "color_space.glsl"
layout(set=0,binding=8,std430) readonly buffer ReflectionLight {
    vec4 ambient, diffuse, zenith, horizon, sunSky, fog, eye, parameters;
    vec4 sceneAmbient, sceneSunlight; // before GetAccomodateEye, fog and night-eye response
} light;
// Display-authored engine coefficients, decoded with the existing headroom
// convention. These are NOT calibrated irradiance or radiance. Keep separate
// from texture albedo, N dot L, visibility and final display transformations.
// Enhanced materials retain the user's ordinary engine brightness/color filter,
// applied to incident light exactly once, identically to enhanced raster.
#ifdef GI_GROUP_LIGHTING
shared vec3 giLighting[6]; // ambient, sunlight, zenith, horizon, sun-sky, cloud
vec3 rayAmbientLinear() { return giLighting[0]; }
vec3 raySunlightLinear() { return giLighting[1]; }
#else
vec3 rayAmbientLinear() { return legacyLightToLinear(light.ambient.rgb); }
vec3 raySunlightLinear() { return legacyLightToLinear(light.diffuse.rgb); }
#endif
