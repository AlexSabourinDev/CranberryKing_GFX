#version 450
#pragma shader_stage(vertex)

#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex
{
    vec4 gl_Position;
};

layout (location = 0) in vec2 in_Position;

void main()
{
	gl_Position = vec4(in_Position, 0.0, 1.0);
}