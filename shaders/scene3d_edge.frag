#version 330

// Post-process toon outline: composite the 3D scene colour and overlay a solid
// outline wherever the scene DEPTH has a discontinuity (object silhouettes,
// where things meet, creases against the background). Works for ALL geometry
// regardless of mesh topology - unlike an inverted hull. Uses a depth Laplacian
// so smooth receding surfaces (a floor) don't false-trigger.
// (Under temporal anti-aliasing the outline goes into the world before TAA
// instead - outline_world.frag - and this composite isn't used.)

in vec2 TexCoord;
out vec4 color;

uniform sampler2D theTexture;   // 3D scene colour (unit 0)
uniform sampler2D depthTex;     // scene depth (unit 1)
uniform sampler2D maskTex;      // "is-character" mask (unit 2): 1.0 = character sprite

#include "sprite_draw.glsl"     // draw.spriteColor: framebuffer sprite tint (usually white)
#include "outline_edge.glsl"    // OutlineEdge (and outline.glsl's settings)

void main()
{
	vec4 base = texture(theTexture, TexCoord) * draw.spriteColor;
	float edge = OutlineEdge(TexCoord);
	color = vec4(mix(base.rgb, outlineColor, edge), base.a);
}
