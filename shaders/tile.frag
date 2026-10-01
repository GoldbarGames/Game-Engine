#version 330

in vec2 TexCoord;

out vec4 color;

uniform sampler2D theTexture;
#include "sprite_draw.glsl"

void main()
{
	vec4 texColor = texture(theTexture, TexCoord.xy);
	vec4 finalColor = texColor * draw.spriteColor;
	finalColor.rgb *= draw.lightRatio;
	color = finalColor;
}
