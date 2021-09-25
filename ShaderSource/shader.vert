#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

const mat4 clipMatrix = mat4(1.0f,  0.0f, 0.0f, 0.0f,
			0.0f, -1.0f, 0.0f, 0.0f,
                        0.0f,  0.0f, 0.5f, 0.0f,
                        0.0f,  0.0f, 0.5f, 1.0f);

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aTextureCoordinate;

layout (std140, set = 0, binding = 0) uniform Camera
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	vec3 position;
} camera;

layout (std140, set = 1, binding = 0) uniform Model
{
	mat4 matrix;
	mat4 normalMatrix;
} model;

layout (location = 0) out vec3 worldFragment;
layout (location = 1) out vec2 textureCoordinate;
layout (location = 2) out mat3 TBN;

void main()
{
	textureCoordinate = aTextureCoordinate;

	vec3 normal = normalize((model.normalMatrix * vec4(aNormal, 0.0)).xyz);
	vec3 tangent = normalize((model.normalMatrix * vec4(aTangent, 0.0)).xyz);
	TBN = mat3(tangent, cross(tangent, normal), normal);

	gl_Position = model.matrix * vec4(aPosition, 1.0);
	worldFragment = gl_Position.xyz;
	gl_Position = clipMatrix * camera.projectionMatrix * camera.viewMatrix * gl_Position;
}