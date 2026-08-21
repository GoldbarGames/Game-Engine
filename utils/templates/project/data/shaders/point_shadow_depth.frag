#version 330

// Store LINEAR distance from the light (0..1 over farPlane) so the main shader
// can compare it against the fragment's own distance to the light.
in vec2 TexCoord;
in vec3 WorldPos;

uniform sampler2D theTexture;
uniform float alphaCutoff;
uniform vec3 lightPos;
uniform float farPlane;

void main()
{
	if (texture(theTexture, TexCoord).a < alphaCutoff)
		discard;
	gl_FragDepth = length(WorldPos - lightPos) / farPlane;
}
