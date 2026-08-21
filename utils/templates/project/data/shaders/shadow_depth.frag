#version 330

// Depth-only, but alpha-cut so transparent sprite pixels don't cast shadow.
in vec2 TexCoord;
uniform sampler2D theTexture;
uniform float alphaCutoff;

void main()
{
	if (texture(theTexture, TexCoord).a < alphaCutoff)
		discard;
	// depth is written automatically
}
