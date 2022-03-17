#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTextureCoordinate;

layout (push_constant) uniform PushConstant
{
    layout (offset = 0) mat4 transform;
} pc;

layout (location = 0) out vec2 textureCoordinate;

void main()
{
    gl_Position = pc.transform * vec4(aPosition, 1.0);
    textureCoordinate = aTextureCoordinate;
}