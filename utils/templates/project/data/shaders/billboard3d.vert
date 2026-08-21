#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 TexCoord;
out vec3 FragPos;

uniform mat4 model;
// Shared camera matrices (std140 UBO, binding 0); see scene3d.vert.
layout(std140) uniform Camera
{
	mat4 view;
	mat4 projection;
};

void main()
{
	vec4 worldPos = model * vec4(pos, 1.0);
	FragPos = worldPos.xyz;
	gl_Position = projection * view * worldPos;
	TexCoord = tex;
}
