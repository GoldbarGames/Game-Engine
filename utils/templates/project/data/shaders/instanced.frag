#version 330

in vec2 TexCoord;
in vec4 spriteColorOut;

out vec4 color;

uniform sampler2D theTexture;
uniform float lightRatio;

void main()
{
	vec4 newColor = texture(theTexture, TexCoord.xy) * spriteColorOut;
	newColor.r *= lightRatio;
	newColor.g *= lightRatio;
	newColor.b *= lightRatio;
	color = newColor;
}
