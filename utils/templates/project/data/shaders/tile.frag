#version 330

in vec2 TexCoord;

out vec4 color;

uniform sampler2D theTexture;
uniform vec4 spriteColor = vec4(1.0, 1.0, 1.0, 1.0);
uniform float lightRatio = 1.0;

void main()
{
	vec4 texColor = texture(theTexture, TexCoord.xy);
	vec4 finalColor = texColor * spriteColor;
	finalColor.rgb *= lightRatio;
	color = finalColor;
}