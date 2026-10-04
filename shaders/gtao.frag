#version 330

// Ground-truth ambient occlusion (Phase 1.5 item 6;
// src/ENGINE/render/AmbientOcclusion.cpp). Jimenez, Wu, Pesce, Jarabo,
// "Practical Real-Time Strategies for Accurate Indirect Occlusion" (2016),
// following Intel's XeGTAO reference implementation (MIT licence), as a
// fragment shader because WebGL2 has no compute.
//
// Per pixel: for a few directions across the screen ("slices"), march both
// ways to find the highest horizon the surrounding depth makes, then
// integrate the cosine-weighted visible arc between the two horizons around
// the pixel's normal. Directions and step offsets rotate through a 4x4 tile
// of pixels; ao_denoise.frag averages each 4x4 block back to smooth.
//
// Writes data, not colour: KINJO_LINEAR_OUTPUT keeps the linear workflow
// from adapting it.
#define KINJO_LINEAR_OUTPUT

#include "draw.glsl"

struct DrawData
{
	vec4  proj;            // projection[0][0], [1][1], [2][0], [2][1]
	vec2  size;            // target size in pixels
	float radius;          // world units
	float pixelsPerUnit;   // screen pixels per world unit at view depth 1
	float maxRadiusPx;     // search radius cap (very close surfaces)
	float falloff;         // occluders fade out over this outer fraction of the radius
	float power;           // visibility ^ power
};
PER_DRAW(DrawData);

uniform sampler2D aoViewDepth;    // R32F linear view depth; 0 = nothing drawn
uniform sampler2D aoViewNormal;   // view-space normal * 0.5 + 0.5

out vec4 result;

const float PI = 3.14159265;
const float HALF_PI = 1.57079633;
const int SLICES = 3;
const int STEPS = 6;   // per side

// 4x4 ordered-dither values: each pixel of a 4x4 block gets a different one.
const int BAYER[16] = int[16](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);

// View-space position of the pixel centre `p` at linear depth `d`.
vec3 ViewPosition(vec2 p, float d)
{
	vec2 ndc = p / draw.size * 2.0 - 1.0;
	return vec3(d * (ndc.x + draw.proj.z) / draw.proj.x,
	            d * (ndc.y + draw.proj.w) / draw.proj.y,
	            -d);
}

// Depth at a pixel, clamped to the target; nothing drawn = very far.
float DepthAt(vec2 p)
{
	ivec2 q = clamp(ivec2(p), ivec2(0), ivec2(draw.size) - 1);
	float d = texelFetch(aoViewDepth, q, 0).r;
	return (d > 0.0) ? d : 1.0e7;
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
	vec3 V = normalize(-P);

	// The search radius on screen. Very close surfaces cap it (and shrink the
	// world radius to match); a radius under ~2 pixels can't find anything.
	float radiusPx = draw.radius * draw.pixelsPerUnit / depth;
	float worldRadius = draw.radius * min(1.0, draw.maxRadiusPx / radiusPx);
	radiusPx = min(radiusPx, draw.maxRadiusPx);
	if (radiusPx < 2.0)
	{
		result = vec4(1.0);
		return;
	}

	// Occluders beyond the radius don't count; they fade out over its outer part.
	float falloffRange = draw.falloff * worldRadius;
	float falloffFrom = worldRadius - falloffRange;
	float falloffMul = -1.0 / falloffRange;
	float falloffAdd = falloffFrom / falloffRange + 1.0;

	float sliceNoise = (float(BAYER[(pix.y & 3) * 4 + (pix.x & 3)]) + 0.5) / 16.0;
	float stepNoise = (float(BAYER[((pix.x + 1) & 3) * 4 + ((pix.y + 3) & 3)]) + 0.5) / 16.0;
	float minS = 1.3 / radiusPx;   // the first step skips the pixel itself

	// Screen x/y and view x/y point the same way unless the projection flips one.
	vec2 axisSign = vec2(draw.proj.x < 0.0 ? -1.0 : 1.0, draw.proj.y < 0.0 ? -1.0 : 1.0);

	float visibility = 0.0;
	for (int slice = 0; slice < SLICES; slice++)
	{
		float phi = (float(slice) + sliceNoise) * PI / float(SLICES);
		vec2 omega = vec2(cos(phi), sin(phi));

		// The slice plane (through the view vector), and the normal projected into it.
		vec3 dirVec = vec3(omega * axisSign, 0.0);
		vec3 orthoDir = dirVec - dot(dirVec, V) * V;
		vec3 axis = normalize(cross(orthoDir, V));
		vec3 projN = N - axis * dot(N, axis);
		float projNLen = length(projN);
		float cosN = clamp(dot(projN, V) / max(projNLen, 1.0e-4), 0.0, 1.0);
		float n = sign(dot(orthoDir, projN)) * acos(cosN);

		// Horizons start at the tangent plane, and rise to the highest occluder found.
		float low0 = cos(n + HALF_PI);
		float low1 = cos(n - HALF_PI);
		float horizon0 = low0;
		float horizon1 = low1;
		for (int s = 0; s < STEPS; s++)
		{
			float t = (float(s) + stepNoise) / float(STEPS);
			t = t * t + minS;   // denser near the pixel
			vec2 offset = round(omega * (t * radiusPx));

			vec2 p0 = gl_FragCoord.xy + offset;
			vec2 p1 = gl_FragCoord.xy - offset;
			vec3 d0 = ViewPosition(p0, DepthAt(p0)) - P;
			vec3 d1 = ViewPosition(p1, DepthAt(p1)) - P;
			float l0 = length(d0);
			float l1 = length(d1);
			float w0 = clamp(l0 * falloffMul + falloffAdd, 0.0, 1.0);
			float w1 = clamp(l1 * falloffMul + falloffAdd, 0.0, 1.0);
			horizon0 = max(horizon0, mix(low0, dot(d0, V) / max(l0, 1.0e-4), w0));
			horizon1 = max(horizon1, mix(low1, dot(d1, V) / max(l1, 1.0e-4), w1));
		}

		// XeGTAO's correction for slight over-darkening on steep slopes.
		projNLen = mix(projNLen, 1.0, 0.05);

		// The visible arc between the horizons, cosine-weighted about the normal.
		float h0 = -acos(clamp(horizon1, -1.0, 1.0));
		float h1 = acos(clamp(horizon0, -1.0, 1.0));
		h0 = n + clamp(h0 - n, -HALF_PI, HALF_PI);
		h1 = n + clamp(h1 - n, -HALF_PI, HALF_PI);
		float arc0 = (cosN + 2.0 * h0 * sin(n) - cos(2.0 * h0 - n)) / 4.0;
		float arc1 = (cosN + 2.0 * h1 * sin(n) - cos(2.0 * h1 - n)) / 4.0;
		visibility += projNLen * (arc0 + arc1);
	}
	visibility /= float(SLICES);
	visibility = pow(clamp(visibility, 0.0, 1.0), draw.power);
	result = vec4(max(visibility, 0.03), 0.0, 0.0, 1.0);
}
