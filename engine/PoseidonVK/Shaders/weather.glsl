#ifndef WEATHER_SET
#define WEATHER_SET 3
#endif
layout(set=WEATHER_SET,binding=0,std140) uniform WeatherLighting {
    vec4 layers[3];
    vec4 camera, sun, cloudColor, skyColor, sunSky, options;
} weather;
layout(set=WEATHER_SET,binding=1) uniform sampler2D cloudHigh;
layout(set=WEATHER_SET,binding=2) uniform sampler2D cloudMiddle;
layout(set=WEATHER_SET,binding=3) uniform sampler2D cloudLow;
layout(set=WEATHER_SET,binding=4) uniform sampler2D weatherSky;

vec4 cloudAt(int layer,vec2 world) {
    vec2 uv=(world-weather.layers[layer].xy)*weather.layers[layer].z;
    // Never clamp an edge cloud into a stripe outside the finite coverage map.
    float edge=min(min(uv.x,uv.y),min(1-uv.x,1-uv.y));
    if(edge<=0) return vec4(0);
    vec4 c=layer==0?textureLod(cloudHigh,uv,0):layer==1?textureLod(cloudMiddle,uv,0):textureLod(cloudLow,uv,0);
    c.a*=smoothstep(0,.04,edge);
    return c;
}
float cloudSunlight(vec3 world) {
    if(weather.sun.w<=0 || weather.cloudColor.a<=0) return 1;
    float transmission=1;
    for(int i=0;i<3;++i) {
        float t=(weather.layers[i].w-world.y)/max(weather.sun.y,.04);
        if(t<=0) continue;
        float coverage=cloudAt(i,world.xz+weather.sun.xz*t).a*weather.cloudColor.a;
        transmission*=1-coverage*.65;
    }
    return 1-weather.sun.w*(1-transmission);
}
vec3 reflectedWeather(vec3 world,vec3 direction,vec3 fallback) {
    if(weather.options.x<.5) return fallback;
    const float pi=3.14159265359;
    vec2 uv=vec2(atan(direction.z,direction.x)/(2*pi)+.5,asin(clamp(direction.y,0,1))/(pi*.5));
    vec4 texel=textureLod(weatherSky,uv,0);
    if(texel.a<.99) return fallback;
    float lobe=pow(max(dot(direction,weather.sun.xyz),0),6);
    vec3 color=texel.rgb*legacyLightToLinear(weather.skyColor.rgb+weather.sunSky.rgb*lobe);
    // DrawClouds compresses both positions and geometry around the camera by
    // CloudScale. Undo that transform for the water ray's origin only.
    vec3 origin=weather.camera.xyz+(world-weather.camera.xyz)/weather.camera.w;
    for(int i=0;i<3 && direction.y>.001;++i) {
        float t=(weather.layers[i].w-origin.y)/direction.y;
        if(t<=0 || t>=19000) continue;
        vec4 cloud=cloudAt(i,origin.xz+direction.xz*t);
        float opacity=cloud.a*weather.cloudColor.a*clamp(exp(2.9957322736*(4000-t)/19000),0,1);
        color=mix(color,cloud.rgb*legacyLightToLinear(weather.cloudColor.rgb),opacity);
    }
    return color;
}
