#version 330

// The toon outline drawn INTO the linear HDR world, before temporal
// anti-aliasing (Phase 1.5 item 7; Game::Render). Same edge test as the
// composite (outline_edge.glsl), but alpha-blended over the world here, so TAA
// smooths the outline and it doesn't shimmer with the camera's sub-pixel
// jitter. Draws with a full-screen triangle (resolve.vert).
// Linear-aware (target.glsl): the authored outline colour is converted.

out vec4 color;

uniform sampler2D depthTex;   // scene depth (unit 1)
uniform sampler2D maskTex;    // "is-character" mask (unit 2)

#include "target.glsl"
#define OUTLINE_SOFT_THRESHOLD   // borderline edges draw partially rather than flicker under jitter
#include "outline_edge.glsl"

void main()
{
	vec2 uv = gl_FragCoord.xy * texelSize;
	float edge = OutlineEdge(uv);
	if (edge <= 0.001)
		discard;
	color = vec4(TargetColor(outlineColor), edge);
}
