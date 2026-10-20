// Linear appearance of the stock dome and legacy cloud layers. This backdrop
// is not calibrated lighting radiance: RTGI supplies its irradiance conversion.
// No sun disk: SunSkyColor's broad atmospheric lobe is part of the visible sky;
// the delta direct-sun source is evaluated separately with a shadow ray.
vec3 raySkyDomeLinear(vec3 direction) {
    if(direction.y<=0) return vec3(0);
    float lobe=pow(max(dot(direction,weather.sun.xyz),0),6);
    vec3 color=giSkyGradient(giLighting[2],giLighting[3],direction.y)+giLighting[4]*lobe;
    // Availability is independent of the water/sky-reflection settings.
    if(weather.options.y<.5) return color;
    const float pi=3.14159265359;
    vec2 uv=vec2(atan(direction.z,direction.x)/(2*pi)+.5,asin(clamp(direction.y,0,1))/(pi*.5));
    vec4 texel=textureLod(weatherSky,uv,0);
    if(texel.a>=.99)
        color=texel.rgb*legacyLightToLinear(weather.skyColor.rgb+weather.sunSky.rgb*lobe);
    return color;
}
vec3 raySkyWeatherLinear(vec3 world,vec3 direction,vec3 color) {
    if(direction.y<=0 || weather.options.y<.5) return color;
    // GI origins are real world positions, not the raster skydome's compressed
    // coordinates. A coarse footprint integrates cloud coverage without a new
    // volume march or high-frequency one-sample cloud shimmer.
    for(int i=0;i<3;++i) {
        float t=(weather.layers[i].w-world.y)/direction.y;
        if(t<=0 || t>=19000) continue;
        vec4 cloud=cloudAtLOD(i,world.xz+direction.xz*t,3);
        float opacity=cloud.a*weather.cloudColor.a*clamp(exp(2.9957322736*(4000-t)/19000),0,1);
        color=mix(color,cloud.rgb*giLighting[5],opacity);
    }
    return color;
}
vec3 raySkyLinear(vec3 world,vec3 direction) {
    return raySkyWeatherLinear(world,direction,raySkyDomeLinear(direction));
}
