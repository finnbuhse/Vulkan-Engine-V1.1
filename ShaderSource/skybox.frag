#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define GAMMA 2.2
#define EXPOSURE 1.0

layout (location = 0) in vec3 textureVector;

layout (set = 0, binding = 1) uniform samplerCube skybox;

layout (location = 0) out vec3 colour;

void main()
{
	colour = texture(skybox, textureVector).rgb;
    	colour = 1.0 - exp(-colour * EXPOSURE); // Exposure tone mapping
    	colour = pow(colour, vec3(1.0 / GAMMA)); // Gamma correction
}