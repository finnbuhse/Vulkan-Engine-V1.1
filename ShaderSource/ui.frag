#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 textureCoordinate;

layout (push_constant) uniform PushConstant
{ 
    layout (offset = 64) vec3 colour;
    layout (offset = 76) uint text;
} pc;

layout (set = 0, binding = 0) uniform sampler2D image;

layout (location = 0) out vec4 colour;

void main()
{
    vec4 fragment = texture(image, textureCoordinate);
    colour = pc.text == 1 ? vec4(pc.colour, fragment.r) : vec4(fragment.rgb * pc.colour, fragment.a);
}