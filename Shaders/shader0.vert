#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec4 vertexColor;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform vec2 texFrame;
uniform vec2 texOffset;

void main()
{
	gl_Position = projection * view * model * vec4(pos.x, pos.y, pos.z, 1.0);
	vertexColor = vec4(clamp(pos, 0.0f, 1.0f), 1.0f);
	
	vec2 frame = vec2(0.166f, 1.0f);
	vec2 offset = vec2(0.5f, 0.0f);
	TexCoord = texOffset + (texFrame * tex);
}