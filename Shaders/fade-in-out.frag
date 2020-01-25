#version 330

in vec2 TexCoord;

out vec4 color;

uniform sampler2D theTexture;
uniform vec4 fadeColor;

void main()
{
    vec4 pixel = texture(theTexture, TexCoord.xy);
	color = pixel * fadeColor;
}
