struct ShapeLocalLight { vec4 position; vec4 direction; vec4 diffuse; vec4 ambient; };
layout(set = 2, binding = 0, std140) uniform ShapeLighting {
    mat4 world;
    mat3 normalMatrix;
    vec4 sunDirection;
    vec4 ambient;
    vec4 diffuse;
    vec4 emissive;
    vec4 specular; // Sun diffuse * material specular RGB, material power.
    vec4 fogParams;
    vec4 fogColor;
    vec4 eyeCoef;
    vec4 localCount;
    ShapeLocalLight localLights[8];
    vec4 shadowReceiver;
    mat4 cascadeVP[4];
    vec4 cascadeSplits;
    vec4 cascadeControl;
    vec4 shadowForward;
    vec4 shadowStrength;
    mat4 previousMVP;
    vec4 temporal;
    vec4 temporalExtent;
    vec4 outputColor; // world=0/overlay=1, native geometry, display peak/white
    vec4 atmosphereSun; // direction towards sun, daylight/clear-air weight
    vec4 atmosphereColor; // engine sun color, camera altitude density
    vec4 rtSun;
    vec4 rtWater;
    vec4 rainRipples;
    vec4 wetSurface;
    mat4 localShadowVP[2];
    vec4 localShadowControl;
    vec4 pbr;
    vec4 pbrDiffuse;
    vec4 aoReceiver;
    mat4 materialToWorld;
    vec4 materialAmbient, materialSun;
} lighting;
