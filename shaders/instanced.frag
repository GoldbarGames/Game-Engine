#version 330

in vec2 TexCoord;
in vec4 spriteColorOut;

out vec4 color;

uniform sampler2D theTexture;
// Per-instance model/frame/tint come from attributes; only lightRatio is per batch.
#include "sprite_draw.glsl"

void main()
{
	vec4 newColor = texture(theTexture, TexCoord.xy) * spriteColorOut;
	newColor.r *= draw.lightRatio;
	newColor.g *= draw.lightRatio;
	newColor.b *= draw.lightRatio;
	color = newColor;
}
