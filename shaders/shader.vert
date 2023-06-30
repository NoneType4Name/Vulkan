#version 450
#extension GL_EXT_debug_printf : enable

layout( location = 0 ) in vec3 inputPos;
layout( location = 1 ) in vec4 inputColor;

layout( location = 0 ) out vec4 fragColor;

void main()
{
    // debugPrintfEXT( "Color is %f" );
    gl_Position   = vec4( inputPos, 1.0 );
    gl_Position.y = -gl_Position.y;
    fragColor     = inputColor;
}