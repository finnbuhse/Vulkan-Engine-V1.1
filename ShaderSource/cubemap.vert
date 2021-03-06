#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 aPosition;

void main()
{
	gl_Position = vec4(aPosition, 1.0);
}