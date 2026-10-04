#version 330

// Temporal anti-aliasing resolve (Phase 1.5 item 7; src/ENGINE/render/TemporalAA.cpp).
// Each frame is rendered with a different sub-pixel camera offset; this blends
// it into the image accumulated over earlier frames:
//   1. find where this pixel's surface was last frame: reproject the nearest
//      depth of its 3x3 neighbourhood through both frames' camera matrices
//      (the nearest, so a foreground edge drags its own motion along);
//   2. read last frame's image there with a Catmull-Rom filter (5 bilinear
//      taps), which keeps it sharp under repeated resampling;
//   3. clip that history to the colour range this frame's 3x3 neighbourhood
//      allows (variance clipping in YCoCg, Salvi 2016 / Playdead's
//      clip-towards-centre), so what the history can't explain - motion,
//      disocclusion, a light switching - is dropped instead of ghosting;
//   4. blend ~10% of this frame in.
// All in a compressed space (Karis: c / (1 + luma)), so one very bright HDR
// sample can't dominate its neighbours or flicker.
//
// Writes data for the next frame (history) and this frame's image (display);
// KINJO_LINEAR_OUTPUT keeps the linear workflow from adapting it.
#define KINJO_LINEAR_OUTPUT

#include "draw.glsl"

struct DrawData
{
	mat4  reproject;    // this frame's clip space (unjittered) -> last frame's
	vec2  size;         // target size in pixels
	float feedback;     // weight of this frame's sample (0.1 = ~10 frames of history)
	int   useHistory;   // 0: first frame, or after a camera cut
	vec2  jitter;       // this frame's camera offset in pixels: texel q shows the unjittered point q + 0.5 + jitter
};
PER_DRAW(DrawData);

uniform sampler2D currentColor;   // this frame's jittered HDR world (unit 0)
uniform sampler2D historyColor;   // last frame's result, linear filtered (unit 1)
uniform sampler2D sceneDepth;     // this frame's depth (unit 2)
uniform sampler2D sceneMotion;    // objects' motion vectors, b = 1 where written (unit 3; motion.glsl)

layout (location = 0) out vec4 outHistory;
layout (location = 1) out vec4 outDisplay;

float Luma(vec3 c)
{
	return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// Karis' reversible tonemap.
vec3 Compress(vec3 c)
{
	return c / (1.0 + Luma(c));
}

vec3 Expand(vec3 c)
{
	return c / max(1.0 - Luma(c), 1.0e-4);
}

vec3 RgbToYCoCg(vec3 c)
{
	return vec3(dot(c, vec3(0.25, 0.5, 0.25)), dot(c, vec3(0.5, 0.0, -0.5)), dot(c, vec3(-0.25, 0.5, -0.25)));
}

vec3 YCoCgToRgb(vec3 c)
{
	return vec3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

vec4 FetchCurrent(ivec2 p)
{
	vec4 c = texelFetch(currentColor, clamp(p, ivec2(0), ivec2(draw.size) - 1), 0);
	if (any(isnan(c)) || any(isinf(c)))
		c = vec4(0.0);
	return vec4(RgbToYCoCg(Compress(max(c.rgb, vec3(0.0)))), c.a);
}

// Catmull-Rom sampling of the history in 5 bilinear taps (the four corner
// taps of the 4x4 kernel are dropped; their weights are tiny).
vec3 SampleHistory(vec2 uv)
{
	vec2 samplePos = uv * draw.size;
	vec2 texPos1 = floor(samplePos - 0.5) + 0.5;
	vec2 f = samplePos - texPos1;
	vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
	vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
	vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
	vec2 w3 = f * f * (-0.5 + 0.5 * f);
	vec2 w12 = w1 + w2;
	vec2 texPos0 = (texPos1 - 1.0) / draw.size;
	vec2 texPos3 = (texPos1 + 2.0) / draw.size;
	vec2 texPos12 = (texPos1 + w2 / w12) / draw.size;

	vec3 sum = textureLod(historyColor, vec2(texPos12.x, texPos0.y), 0.0).rgb * (w12.x * w0.y)
	         + textureLod(historyColor, vec2(texPos0.x, texPos12.y), 0.0).rgb * (w0.x * w12.y)
	         + textureLod(historyColor, vec2(texPos12.x, texPos12.y), 0.0).rgb * (w12.x * w12.y)
	         + textureLod(historyColor, vec2(texPos3.x, texPos12.y), 0.0).rgb * (w3.x * w12.y)
	         + textureLod(historyColor, vec2(texPos12.x, texPos3.y), 0.0).rgb * (w12.x * w3.y);
	float weight = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
	return max(sum / weight, vec3(0.0));   // the kernel's negative lobes can undershoot
}

// Pull `history` toward the box's centre until it lies inside the box.
vec3 ClipToBox(vec3 history, vec3 boxMin, vec3 boxMax)
{
	vec3 centre = 0.5 * (boxMax + boxMin);
	vec3 extent = 0.5 * (boxMax - boxMin) + 1.0e-5;
	vec3 offset = history - centre;
	vec3 units = abs(offset / extent);
	float m = max(units.x, max(units.y, units.z));
	return (m > 1.0) ? centre + offset / m : history;
}

void main()
{
	ivec2 pix = ivec2(gl_FragCoord.xy);
	ivec2 last = ivec2(draw.size) - 1;

	// This frame's 3x3 neighbourhood: colour moments and range, the nearest
	// depth, and this frame's sample reconstructed at the UNJITTERED pixel
	// centre - each neighbour weighted by its distance from it (a Gaussian fit
	// of Blackman-Harris, Karis 2014). Taking the jittered centre pixel as is
	// would feed the blend a value that swings with the jitter phase, which
	// shows as shimmer on sharp edges even with a still camera.
	vec4 centre = vec4(0.0);
	vec4 filtered = vec4(0.0);
	float filterWeight = 0.0;
	vec3 m1 = vec3(0.0);
	vec3 m2 = vec3(0.0);
	vec3 lo = vec3(1.0e9);
	vec3 hi = vec3(-1.0e9);
	float nearest = 2.0;
	ivec2 nearestPix = pix;
	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			ivec2 q = pix + ivec2(x, y);
			vec4 c = FetchCurrent(q);
			if (x == 0 && y == 0)
				centre = c;
			vec2 offset = vec2(x, y) + draw.jitter;
			float w = exp(-2.29 * dot(offset, offset));
			filtered += c * w;
			filterWeight += w;
			m1 += c.rgb;
			m2 += c.rgb * c.rgb;
			lo = min(lo, c.rgb);
			hi = max(hi, c.rgb);
			float d = texelFetch(sceneDepth, clamp(q, ivec2(0), last), 0).r;
			if (d < nearest)
			{
				nearest = d;
				nearestPix = clamp(q, ivec2(0), last);
			}
		}
	}
	m1 /= 9.0;
	vec3 sigma = sqrt(max(m2 / 9.0 - m1 * m1, vec3(0.0)));
	vec3 boxMin = max(lo, m1 - sigma);
	vec3 boxMax = min(hi, m1 + sigma);
	filtered /= filterWeight;

	vec3 result = filtered.rgb;
	if (draw.useHistory != 0)
	{
		// Where the nearest surface was last frame (its motion, applied here):
		// the object's own motion vector where it wrote one (it moved, or the
		// camera follows it), else the camera's motion through the depth.
		vec2 velocity;
		bool valid = true;
		vec4 objectMotion = texelFetch(sceneMotion, nearestPix, 0);
		if (objectMotion.b > 0.5)
		{
			velocity = objectMotion.rg;
		}
		else
		{
			vec2 uvNearest = (vec2(nearestPix) + 0.5) / draw.size;
			vec4 prevClip = draw.reproject * vec4(uvNearest * 2.0 - 1.0, nearest * 2.0 - 1.0, 1.0);
			velocity = uvNearest - (prevClip.xy / prevClip.w) * 0.5 - 0.5;
			valid = prevClip.w > 0.0;
		}
		vec2 prevUv = gl_FragCoord.xy / draw.size - velocity;

		if (valid && all(greaterThanEqual(prevUv, vec2(0.0))) && all(lessThanEqual(prevUv, vec2(1.0))))
		{
			vec3 history = RgbToYCoCg(Compress(SampleHistory(prevUv)));
			history = ClipToBox(history, boxMin, boxMax);
			// Moving: lean a little more on this frame (the history is being
			// resampled every frame, which softens it).
			float motionPx = length(velocity * draw.size);
			float weight = draw.feedback + 0.15 * clamp(motionPx / 32.0, 0.0, 1.0);
			result = mix(history, filtered.rgb, weight);
		}
	}

	vec3 rgb = Expand(max(YCoCgToRgb(result), vec3(0.0)));
	outHistory = vec4(rgb, 1.0);
	outDisplay = vec4(rgb, filtered.a);
}
