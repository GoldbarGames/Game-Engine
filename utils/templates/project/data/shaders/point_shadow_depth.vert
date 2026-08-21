#version 330

// Omnidirectional point-light shadow depth pass: one of 6 cube faces per draw.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 TexCoord;
out vec3 WorldPos;

uniform mat4 model;
uniform mat4 viewProj;   // face projection * face view

void main()
{
	vec4 w = model * vec4(pos, 1.0);
	WorldPos = w.xyz;
	gl_Position = viewProj * w;
	TexCoord = tex;
}
