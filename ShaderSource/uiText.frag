#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 textureCoordinate;

layout (push_constant) uniform PushConstant
{
    layout (offset = 16) vec3 textColour;
} pc;

layout (set = 0, binding = 0) uniform sampler2D glyph;

layout (location = 0) out vec4 colour;

void main()
{    
    colour = vec4(pc.textColour, texture(glyph, textureCoordinate).r);
}