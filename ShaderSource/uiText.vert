#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

const mat4 clipMatrix = mat4(1.0f,  0.0f, 0.0f, 0.0f,
			0.0f, -1.0f, 0.0f, 0.0f,
                        0.0f,  0.0f, 0.5f, 0.0f,
                        0.0f,  0.0f, 0.5f, 1.0f);

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTextureCoordinate;

layout (push_constant) uniform PushConstant
{
    layout (offset = 0) vec2 position;
    layout (offset = 8) vec2 size;
} pc;

layout (location = 0) out vec2 textureCoordinate;

void main()
{
    gl_Position = vec4(pc.position + aPosition.xy * pc.size, 0.0, 1.0);
    textureCoordinate = aTextureCoordinate;
}