#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.1415926535

layout (location = 0) in vec3 position;

layout (set = 0, binding = 0) uniform samplerCube environmentMap;

layout (location = 0) out vec3 colour;

void main()
{		
    	vec3 normal = normalize(position);
  
    	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, normal);
	up = cross(normal, right);

	colour = vec3(0, 0, 0);
	float sampleDelta = 0.025;
	uint nSamples = 0;
	for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
    		for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
    		{
        		// Spherical to cartesian (tangent space)
        		vec3 tangentTextureVector = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

        		// Tangent space to world space
        		vec3 textureVector = tangentTextureVector.x * right + tangentTextureVector.y * up + tangentTextureVector.z * normal; 
        		
			colour += texture(environmentMap, textureVector).rgb * cos(theta) * sin(theta);
        		nSamples++;
    		}
	}
	colour = PI * colour * (1.0 / float(nSamples));
}
