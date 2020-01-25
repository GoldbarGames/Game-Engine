#version 330

in vec2 TexCoord;

out vec4 color;

uniform sampler2D theTexture;
uniform vec4 fadeColor;
uniform float currentTime;

void main()
{
    vec4 pixel = texture(theTexture, TexCoord.xy);
	vec4 newColor = vec4(pixel.r,pixel.g,pixel.b,pixel.a);

	if (fadeColor.r != 1)
		newColor.r = abs(cos(currentTime/1000)) * pixel.r;
	else
		newColor.r = abs(sin(currentTime/1000)) + pixel.r;

	if (fadeColor.g != 1)
		newColor.g = abs(cos(currentTime/1000)) * pixel.g;
	else
		newColor.g = abs(sin(currentTime/1000)) + pixel.g;

	if (fadeColor.b != 1)
		newColor.b = abs(cos(currentTime/1000)) * pixel.b;
	else
		newColor.b = abs(sin(currentTime/1000)) + pixel.b;

	if (fadeColor.a != 1)
		newColor.a = abs(cos(currentTime/1000)) * pixel.a;

	color = newColor; 
}
