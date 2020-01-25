#version 330

in vec4 newColor;
in vec2 TexCoord;

out vec4 color;

uniform sampler2D theTexture;

void main()
{
    vec4 pixel = texture(theTexture, TexCoord.xy);
	color = newColor * pixel;
}
