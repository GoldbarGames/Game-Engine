#version 330

in vec2 TexCoord;

out vec4 color;

uniform sampler2D theTexture;
uniform vec4 fadeColor;
uniform float currentTime;

void main()
{
	vec4 pixel =  texture(theTexture, TexCoord) * fadeColor;

	if (pixel.a == 0)
		color = vec4(1, 1, 1, 0.1);
	else
		color = pixel;
}