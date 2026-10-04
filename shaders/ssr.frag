#version 330

// Screen-space reflections (Phase 1.5 item 9; src/ENGINE/render/Reflections.h).
// For each glossy pixel of the ambient-occlusion prepass: reflect the view
// ray about the surface's normal, march it through the prepass's depth (steps
// that grow with distance, offset per pixel and frame), refine the first hit
// with a binary search, and look the hit point up in LAST frame's anti-aliased
// image (reprojected through last frame's camera). Out: rgb = reflected light,
// a = confidence (0 = no hit; lit shaders keep their sky reflection there).
#define KINJO_LINEAR_OUTPUT

#include "draw.glsl"

struct DrawData
{
	mat4  viewToPrevClip;   // this frame's view space -> last frame's clip space
	vec4  proj;             // projection[0][0], [1][1], [2][0], [2][1] (this frame, as drawn)
	vec4  params;           // max distance, thickness, roughness cutoff, frame index
	vec2  size;             // target size in pixels
	float nearPlane;
};
PER_DRAW(DrawData);

uniform sampler2D viewDepth;    // prepass: linear view depth, 0 = nothing drawn
uniform sampler2D viewNormal;   // prepass: view-space normal * 0.5 + 0.5, a = roughness
uniform sampler2D history;      // last frame's anti-aliased image (linear filtered)

out vec4 result;

const int STEPS = 40;
const int REFINE = 5;

vec3 ViewPosition(vec2 pixel, float d)
{
	vec2 ndc = pixel / draw.size * 2.0 - 1.0;
	return vec3(d * (ndc.x + draw.proj.z) / draw.proj.x, d * (ndc.y + draw.proj.w) / draw.proj.y, -d);
}

// View-space point -> pixel coordinates (x, y) and its view depth (z).
vec3 Project(vec3 q)
{
	float d = -q.z;
	vec2 ndc = vec2((draw.proj.x * q.x) / d - draw.proj.z, (draw.proj.y * q.y) / d - draw.proj.w);
	return vec3((ndc * 0.5 + 0.5) * draw.size, d);
}

float SceneDepth(vec2 pixel)
{
	float d = texelFetch(viewDepth, ivec2(pixel), 0).r;
	return (d > 0.0) ? d : 1.0e7;
}

void main()
{
	ivec2 pix = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(viewDepth, pix, 0).r;
	vec4 nr = texelFetch(viewNormal, pix, 0);
	float rough = nr.a;
	if (depth <= 0.0 || rough >= draw.params.z)
	{
		result = vec4(0.0);
		return;
	}

	vec3 P = ViewPosition(gl_FragCoord.xy, depth);
	vec3 N = normalize(nr.xyz * 2.0 - 1.0);
	vec3 V = normalize(-P);
	vec3 R = reflect(-V, N);

	float maxDistance = draw.params.x;
	// Shorten the ray so it stays in front of the camera.
	if (R.z > 0.0)
		maxDistance = min(maxDistance, (-P.z - draw.nearPlane) / R.z * 0.99);

	vec2 noisePos = gl_FragCoord.xy + 5.588238 * mod(draw.params.w, 64.0);
	float jitter = fract(52.9829189 * fract(dot(noisePos, vec2(0.06711056, 0.00583715))));

	// Start just off the surface, so the ray can't find the surface it leaves.
	P += N * (depth * 0.002 + 0.5);
	vec2 origin = gl_FragCoord.xy;

	float prevT = 0.0;
	float hitT = -1.0;
	for (int i = 1; i <= STEPS; i++)
	{
		float s = (float(i) - jitter) / float(STEPS);
		float t = maxDistance * s * s;   // dense near the surface, sparse far away
		vec3 q = P + R * t;
		vec3 sp = Project(q);
		if (any(lessThan(sp.xy, vec2(0.0))) || any(greaterThanEqual(sp.xy, draw.size)))
			break;
		if (distance(sp.xy, origin) < 1.5)
		{
			prevT = t;   // still on the starting pixel: its depth is the surface itself
			continue;
		}
		float sceneD = SceneDepth(sp.xy);
		float behind = sp.z - sceneD;
		// Behind the depth buffer by more than depth precision, but not by more
		// than a surface could be thick (scaled with the step, which grows with
		// distance).
		float thickness = max(draw.params.y, (t - prevT) * 1.5);
		if (behind > sceneD * 0.001 && behind < thickness)
		{
			// Binary search between the last step in front and this one.
			float a = prevT, b = t;
			for (int k = 0; k < REFINE; k++)
			{
				float m = 0.5 * (a + b);
				vec3 mp = Project(P + R * m);
				if (mp.z > SceneDepth(mp.xy))
					b = m;
				else
					a = m;
			}
			hitT = b;
			break;
		}
		prevT = t;
	}
	if (hitT < 0.0)
	{
		result = vec4(0.0);
		return;
	}

	vec3 hit = P + R * hitT;
	vec4 prevClip = draw.viewToPrevClip * vec4(hit, 1.0);
	vec2 prevUv = prevClip.xy / prevClip.w * 0.5 + 0.5;
	if (prevClip.w <= 0.0 || any(lessThan(prevUv, vec2(0.0))) || any(greaterThan(prevUv, vec2(1.0))))
	{
		result = vec4(0.0);
		return;
	}

	// Confidence: fade near the screen edge, with distance, with roughness,
	// and for rays heading back towards the camera.
	vec2 hitUv = Project(hit).xy / draw.size;
	vec2 edge = min(hitUv, 1.0 - hitUv);
	float confidence = smoothstep(0.0, 0.08, min(edge.x, edge.y));
	confidence *= 1.0 - smoothstep(0.6, 1.0, hitT / max(draw.params.x, 1.0));
	confidence *= 1.0 - smoothstep(draw.params.z * 0.5, draw.params.z, rough);
	confidence *= 1.0 - smoothstep(0.2, 0.8, R.z);

	vec3 color = max(textureLod(history, prevUv, 0.0).rgb, vec3(0.0));
	if (any(isnan(color)) || any(isinf(color)))
		confidence = 0.0;
	result = vec4(color * step(0.0, confidence), confidence);
}
