#version 450

layout(location=0) out flat uint Index;
vec2 position[] = vec2[](
    vec2(0.0, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5)
);
void main()
{
    gl_Position = vec4(position[gl_VertexIndex], 0.0, 1.0);
    Index = gl_VertexIndex;
}