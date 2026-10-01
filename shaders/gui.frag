#version 330

in vec2 TexCoord;

out vec4 color;

uniform sampler2D theTexture;
#include "sprite_draw.glsl"

void main()
{
	color = texture(theTexture, TexCoord) * draw.spriteColor;
}
