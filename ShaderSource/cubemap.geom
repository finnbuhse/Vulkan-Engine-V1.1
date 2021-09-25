#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
const mat4 projectionMatrix = mat4(1, 0, 0, 0,
			     0, 1, 0, 0,
			     0, 0, -1.0202, -1,
			     0, 0, -0.20202, 0);

const mat4 viewMatrices[6] = mat4[](
	mat4(0, 0, -1, -0,
	     0, -1, 0, -0,
	     -1, -0, -0, 0,
	     0, 0, 0, 1),

	mat4(0, 0, 1, -0,
	     0, -1, 0, -0,
	     1, -0, -0, 0,
	     0, 0, 0, 1),

	mat4(1, 0, 0, -0,
	     0, 0, -1, -0,
	     -0, 1, -0, 0,
	     0, 0, 0, 1),

	mat4(1, 0, 0, -0,
	     0, 0, 1, -0,
	     -0, -1, -0, 0,
	     0, 0, 0, 1),

	mat4(1, 0, -0, -0,
	     0, -1, 0, -0,
	     -0, -0, -1, 0,
	     0, 0, 0, 1),

	mat4(-1, -0, -0, 0,
	     0, -1, 0, -0,
	     -0, -0, 1, 0,
	     0, 0, 0, 1)
);

layout (triangles) in;

layout (triangle_strip, max_vertices = 18) out;
layout (location = 0) out vec3 position;

void main()
{
	for (int i = 0; i < 6; i++)
	{
		gl_Layer = i;
		for (int j = 0; j < 3; j++)
		{
			position = vec3(gl_in[j].gl_Position);
			gl_Position = projectionMatrix * viewMatrices[i] * vec4(position, 1.0);
			EmitVertex();
		}
		EndPrimitive();
	}
}