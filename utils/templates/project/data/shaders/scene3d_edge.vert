#version 330

// Full-screen composite of the 3D framebuffer (matches the gui.vert interface
// the framebuffer sprite uses) - the fragment shader adds depth-edge outlines.

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform vec2 texFrame;
uniform vec2 texOffset;

void main()
{
	gl_Position = projection * view * model * vec4(pos, 1.0);
	TexCoord = texOffset + (texFrame * tex);
}
