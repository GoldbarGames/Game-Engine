#version 330

// Ambient-occlusion denoise (Phase 1.5 item 6; src/ENGINE/render/AmbientOcclusion.cpp).
// gtao.frag rotates its directions through a 4x4 tile of pixels, so averaging
// each pixel's 4x4 block gives every pixel all 16 rotations. Neighbours count
// only if they lie on this pixel's surface - close to its tangent plane, with
// a similar normal - so occlusion doesn't bleed across edges.
//
// Writes data, not colour: KINJO_LINEAR_OUTPUT keeps the linear workflow
// from adapting it.
#define KINJO_LINEAR_OUTPUT

#include "draw.glsl"

struct DrawData
{
	vec4  proj;        // projection[0][0], [1][1], [2][0], [2][1]
	vec2  size;        // target size in pixels
	float tolerance;   // allowed distance from the tangent plane, as a fraction of view depth
};
PER_DRAW(DrawData);

uniform sampler2D aoRaw;
uniform sampler2D aoViewDepth;    // R32F linear view depth; 0 = nothing drawn
uniform sampler2D aoViewNormal;   // view-space normal * 0.5 + 0.5

out vec4 result;

vec3 ViewPosition(vec2 p, float d)
{
	vec2 ndc = p / draw.size * 2.0 - 1.0;
	return vec3(d * (ndc.x + draw.proj.z) / draw.proj.x,
	            d * (ndc.y + draw.proj.w) / draw.proj.y,
	            -d);
}

void main()
{
	ivec2 pix = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(aoViewDepth, pix, 0).r;
	if (depth <= 0.0)
	{
		result = vec4(1.0);
		return;
	}
	vec3 P = ViewPosition(gl_FragCoord.xy, depth);
	vec3 N = normalize(texelFetch(aoViewNormal, pix, 0).xyz * 2.0 - 1.0);
	float maxDistance = draw.tolerance * depth;
	ivec2 last = ivec2(draw.size) - 1;

	float sum = 0.0;
	float weightSum = 0.0;
	float plainSum = 0.0;
	float plainCount = 0.0;
	for (int y = -2; y <= 1; y++)
	{
		for (int x = -2; x <= 1; x++)
		{
			ivec2 q = clamp(pix + ivec2(x, y), ivec2(0), last);
			float d = texelFetch(aoViewDepth, q, 0).r;
			if (d <= 0.0)
				continue;
			float raw = texelFetch(aoRaw, q, 0).r;
			vec3 Q = ViewPosition(vec2(q) + 0.5, d);
			vec3 n = texelFetch(aoViewNormal, q, 0).xyz * 2.0 - 1.0;
			float planeWeight = clamp(1.0 - abs(dot(Q - P, N)) / maxDistance, 0.0, 1.0);
			float normalWeight = pow(clamp(dot(normalize(n), N), 0.0, 1.0), 8.0);
			float w = planeWeight * normalWeight;
			sum += raw * w;
			weightSum += w;
			plainSum += raw;
			plainCount += 1.0;
		}
	}
	// The pixel itself always has weight 1. If hardly any neighbour shares its
	// surface, it is a lone pixel - in practice a seam where two touching
	// boxes' hidden faces showed through edge-on, with a normal at right
	// angles to everything around it - so it takes its block's plain average
	// rather than keeping its own (meaningless) occlusion as a dark dot.
	if (weightSum < 1.5)
		result = vec4(plainSum / plainCount, 0.0, 0.0, 1.0);
	else
		result = vec4(sum / weightSum, 0.0, 0.0, 1.0);
}
