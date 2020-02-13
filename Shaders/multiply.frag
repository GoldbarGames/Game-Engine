#version 330

in vec2 TexCoord;

out vec4 color;

uniform sampler2D theTexture;
uniform vec4 spriteColor;
uniform vec4 fadeColor;

void main()
{
	color = texture(theTexture, TexCoord) * spriteColor * fadeColor;
}