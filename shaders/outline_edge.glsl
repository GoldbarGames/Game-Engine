#ifndef KINJO_OUTLINE_EDGE_GLSL
#define KINJO_OUTLINE_EDGE_GLSL
// The toon outline's edge test, shared by the composite (scene3d_edge.frag)
// and, under temporal anti-aliasing, the pass that draws the outline into the
// world before TAA (outline_world.frag). The including shader declares
//     uniform sampler2D depthTex;   // scene depth
//     uniform sampler2D maskTex;    // "is-character" mask: 1.0 = character sprite
// A depth Laplacian, so smooth receding surfaces (a floor) don't count as
// edges, and character sprites never do.
#include "outline.glsl"   // texelSize, near/farPlane, edgeThreshold, thickness, outlineColor

float OutlineLinearDepth(vec2 uv)
{
	float d = texture(depthTex, uv).r * 2.0 - 1.0;   // NDC z
	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - d * (farPlane - nearPlane));
}

bool OutlineIsChar(vec2 uv)
{
	return texture(maskTex, uv).r > 0.5;
}

// Neighbour depth, but character pixels are treated as "same as centre" so a
// character silhouette never counts as a depth edge (no line on the sprite, no
// halo on the geometry/sky behind it). c = centre depth.
float OutlineNeighbourDepth(vec2 uv, float c)
{
	return OutlineIsChar(uv) ? c : OutlineLinearDepth(uv);
}

// 1.0 where the outline is drawn at `uv`, else 0.0.
float OutlineEdge(vec2 uv)
{
	vec2 t = texelSize * max(thickness, 1.0);
	float c = OutlineLinearDepth(uv);
	float l = OutlineNeighbourDepth(uv - vec2(t.x, 0.0), c);
	float r = OutlineNeighbourDepth(uv + vec2(t.x, 0.0), c);
	float u = OutlineNeighbourDepth(uv + vec2(0.0, t.y), c);
	float d = OutlineNeighbourDepth(uv - vec2(0.0, t.y), c);

	// Second difference (Laplacian): ~0 on smooth gradients, spikes at
	// silhouettes / depth jumps. Threshold grows with distance so far surfaces
	// need a bigger discontinuity to count as an edge.
	float lap = abs(l + r - 2.0 * c) + abs(u + d - 2.0 * c);
	float threshold = edgeThreshold * (1.0 + c * 0.03);
#ifdef OUTLINE_SOFT_THRESHOLD
	// Under temporal anti-aliasing (outline_world.frag): a discontinuity near
	// the threshold - a character standing close to a wall - draws a partial
	// line instead of flipping on and off pixel by pixel. With the camera's
	// sub-pixel jitter a hard step there is noise TAA can't settle; strong
	// edges stay solid.
	float edge = smoothstep(threshold * 0.5, threshold * 1.5, lap);
#else
	float edge = step(threshold, lap);
#endif

	// Don't outline the empty far background (nothing was drawn there).
	if (c >= farPlane * 0.98)
		edge = 0.0;

	// Never draw the outline on a character sprite itself.
	if (OutlineIsChar(uv))
		edge = 0.0;
	return edge;
}

// How much distance fog lies over the surface at `uv` (0 = none): the ramp of
// distance_fog.glsl, with the distance to the camera worked out from the depth
// and the pixel's view ray. The outline takes the fog colour by this much.
float OutlineFogAmount(vec2 uv)
{
	if (distFogAmount <= 0.0)
		return 0.0;
	vec2 ndc = uv * 2.0 - 1.0;
	float d = OutlineLinearDepth(uv) * length(vec3(ndc * distFogRange.zw, 1.0));
	return clamp((d - distFogRange.x) / max(distFogRange.y - distFogRange.x, 0.001), 0.0, 1.0) * distFogAmount;
}
#endif
