#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec4 newColor;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform vec4 multiplyColor;

void main()
{
	gl_Position = projection * view * model * vec4(pos.x, pos.y, pos.z, 1.0);
	newColor = multiplyColor;
	
	TexCoord = tex;
}

