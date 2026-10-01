#version 330

// Full-screen composite of the 3D framebuffer (matches the gui.vert interface
// the framebuffer sprite uses) - the fragment shader adds depth-edge outlines.

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 TexCoord;

#include "camera.glsl"
#include "sprite_draw.glsl"

void main()
{
	gl_Position = projection * view * draw.model * vec4(pos, 1.0);
	TexCoord = draw.texOffset + (draw.texFrame * tex);
}
