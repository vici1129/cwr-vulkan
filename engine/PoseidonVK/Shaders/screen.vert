#version 450
#extension GL_GOOGLE_include_directive : require
#include "shape_uniforms.glsl"
layout(location = 0) in vec4 clipPosition;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;
layout(location = 3) in float fog;
layout(location = 4) in vec4 oldClip;
layout(location = 5) in vec4 sunColor;
layout(location = 6) in vec3 localColor0;
layout(location = 7) in vec3 localColor1;
layout(location = 8) in vec4 ambientColor;
layout(location = 9) in vec3 pbrNormal;
layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec4 vertexColor;
layout(location = 2) out float fogVisibility;
layout(location = 3) out vec3 specularColor;
layout(location = 4) out vec3 shadowWorld;
layout(location = 5) out vec4 previousClip;
layout(location = 6) out vec4 directSun;
layout(location = 7) out vec3 localDirect0;
layout(location = 8) out vec3 localDirect1;
layout(location = 9) out vec3 surfaceNormal;
layout(location = 10) out vec4 aoAmbient;
void main() {
    gl_Position = clipPosition;
    surfaceNormal = pbrNormal;
    aoAmbient = ambientColor;
    gl_Position.xy += lighting.temporal.xy * gl_Position.w;
    previousClip = oldClip;
    directSun = sunColor;
    localDirect0 = localColor0;
    localDirect1 = localColor1;
    texCoord = uv;
    vertexColor = color;
    fogVisibility = fog;
    specularColor = vec3(0.0);
    shadowWorld = vec3(0.0);
    if (lighting.shadowReceiver.x > 0.5 || lighting.atmosphereSun.w > 0.0 || lighting.rtSun.x > .5 || lighting.rtWater.x>.5 || lighting.localShadowControl.x>0 || lighting.wetSurface.x>0 || lighting.pbr.x>0) {
        vec3 view = vec3(clipPosition.x / lighting.shadowReceiver.y,
                         -clipPosition.y / lighting.shadowReceiver.z, clipPosition.w);
        shadowWorld = (lighting.world * vec4(view, 0.0)).xyz;
    }
}
