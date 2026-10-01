#version 330

out vec4 color;
#include "sprite_draw.glsl"

void main()
{
	color = draw.spriteColor;
}
