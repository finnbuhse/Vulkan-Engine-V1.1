#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 aPosition;

layout (std140, set = 0, binding = 0) uniform Camera
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	vec3 position;
} camera;

layout (location = 0) out vec3 textureVector;

void main()
{
	textureVector = aPosition;
	vec4 position = camera.projectionMatrix * mat4(mat3(camera.viewMatrix)) * vec4(aPosition, 1.0);
	gl_Position = position.xyww;
}