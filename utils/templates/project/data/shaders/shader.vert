#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec4 vertexColor;
out vec2 TexCoord;
out vec3 FragPos;  // World position for lighting calculations

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform vec2 texFrame;
uniform vec2 texOffset;

void main()
{
	vec4 worldPos = model * vec4(pos.x, pos.y, pos.z, 1.0);
	FragPos = worldPos.xyz;
	gl_Position = projection * view * worldPos;
	vertexColor = vec4(clamp(pos, 0.0f, 1.0f), 1.0f);

	TexCoord = texOffset + (texFrame * tex);
}