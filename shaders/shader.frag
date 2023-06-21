#version 450

layout(location=0) in flat uint Index;
layout(location=0) out vec4 Color;

vec3 VertexColor[] = vec3[](
    vec3(1.0,0.0,0.0),
    vec3(0.0,1.0,0.0),
    vec3(0.0,0.0,1.0)
);
void main()
{
    Color = vec4(VertexColor[Index], 1.0);
}
