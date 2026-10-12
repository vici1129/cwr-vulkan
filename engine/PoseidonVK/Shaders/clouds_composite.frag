#version 450
layout(set=0,binding=0) uniform sampler2D clouds;
layout(location=0) in vec2 texCoord;
layout(location=0) out vec4 outColor;
#ifdef TEMPORAL
layout(location=1) out vec4 outMotion;
#endif
void main() {
    outColor=texture(clouds,texCoord);
    if(outColor.a<.001) discard;
#ifdef TEMPORAL
    outMotion=vec4(0,0,0,-1);
#endif
}
